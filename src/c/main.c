#include <pebble.h>
#include "app_state.h"
#include "ui.h"
#include "breathing.h"
#include "vibration.h"
#include "techniques.h"

// glossary: app_state
AppState g_state;
static AppTimer *s_animation_timer;
// glossary: display_battery_saver
// 1 Hz AppTimer that carries per-second work (vibes, cycle progression,
// Session-Complete Vibe) only while in DISPLAY_BATTERY_SAVER, where the
// tick handler is subscribed to MINUTE_UNIT and so cannot drive per-second
// behavior itself.
static AppTimer *s_second_timer;

// glossary: cycle_elapsed_sec, anim_sub_ms, phase, inhale_phase
// Rewinds Cycle progression to the start of an Inhale Phase. Shared by Reset
// (Top Button long) and Breathing Technique cycling (Middle Button long).
static void reset_cycle_position(void) {
  g_state.cycle_elapsed_sec = 0;
  g_state.anim_sub_ms = 0;
  g_state.current_phase = PHASE_INHALE;
}

// glossary: reset, session, completed_cycles, hr_sample_buffer
static void reset_session() {
  reset_cycle_position();
  g_state.session_elapsed_ms = 0;
  g_state.completed_cycles = 0;
  g_state.hr_write_idx = 0;
  g_state.hr_sample_count = 0;
  g_state.current_hr = 0;
}

// glossary: animation_timer, filler_circle, phase_fill, phase_dot_indicator,
//           display_battery_saver
// 100 ms Animation Timer callback. Skips work while in Pause State, but
// otherwise re-arms itself. Only advances Anim Sub Ms + redraws Filler
// Circle — no Vibes. When the current Phase changes, also dirties the
// Technique Display so the Phase Dot Indicator tracks the breath in real
// time without redrawing the whole top container 10 Hz.
// In DISPLAY_BATTERY_SAVER the animation is suppressed entirely: the
// callback neither updates nor re-registers, so the timer chain dies and
// is restarted by apply_display_mode_runtime() on exit from the mode.
static void anim_timer_callback(void *data) {
  if (g_state.display_mode == DISPLAY_BATTERY_SAVER) {
    s_animation_timer = NULL;
    return;
  }
  if (!g_state.paused) {
    BreathingPhase prev_phase = g_state.current_phase;
    breathing_update(100);
    ui_update_breathing();
    if (g_state.current_phase != prev_phase) {
      ui_update_technique();
    }
  }
  s_animation_timer = app_timer_register(100, anim_timer_callback, NULL);
}

// glossary: tick_handler, display_battery_saver
// Per-second mutation + vibe firing, factored out so it can run from either
// the SECOND_UNIT tick handler (normal modes) or the 1 Hz AppTimer
// (DISPLAY_BATTERY_SAVER, where the tick handler is on MINUTE_UNIT).
// MUST NOT call ui_update_* — callers decide whether to redraw.
// Returns true on Cycle Wrap so the SECOND_UNIT caller can dirty the right
// layers; the AppTimer caller ignores the return value (no redraws in
// battery saver beyond the per-minute Clock refresh).
static bool do_second_work(void) {
  if (g_state.paused) return false;

  g_state.session_elapsed_ms += 1000;              // glossary: session_elapsed

  bool wrapped = breathing_tick();                 // glossary: cycle_wrap
  vibration_trigger_for_second(g_state.cycle_elapsed_sec);  // glossary: vibe_mode_dispatch

  if (wrapped) {
    // glossary: hr_sample, hr_sample_buffer, hr_write_index, display_minimal,
    //           display_battery_saver
    // Every Cycle Wrap consumes one slot. In DISPLAY_MINIMAL /
    // DISPLAY_BATTERY_SAVER (sensor idled) or when the sensor returned an
    // out-of-range value, the slot is filled with HR_SAMPLE_NONE so the HR
    // Graph can render a visible gap for that Cycle instead of collapsing
    // missing Cycles out of the plot. current_hr is only updated on a
    // valid reading — it sticks at the last valid BPM until the next one
    // arrives.
    bool hr_off = (g_state.display_mode == DISPLAY_MINIMAL ||
                   g_state.display_mode == DISPLAY_BATTERY_SAVER);
    HealthValue hr = hr_off
                       ? 0
                       : health_service_peek_current_value(HealthMetricHeartRateBPM);
    if (hr > 30 && hr < 250) {
      g_state.hr_samples[g_state.hr_write_idx] = (int16_t)hr;
      g_state.current_hr = (int16_t)hr;
    } else {
      g_state.hr_samples[g_state.hr_write_idx] = HR_SAMPLE_NONE;
    }
    g_state.hr_write_idx = (g_state.hr_write_idx + 1) % HR_SAMPLE_BUFFER;
    if (g_state.hr_sample_count < HR_SAMPLE_BUFFER) g_state.hr_sample_count++;

    // glossary: completed_cycles, target_cycles, session_complete_vibe
    g_state.completed_cycles++;
    if (g_state.completed_cycles % 20 == 0) {
      vibration_trigger_session_complete();
    }
  }

  return wrapped;
}

// glossary: display_battery_saver
// 1 Hz callback registered only while in DISPLAY_BATTERY_SAVER. Re-arms
// itself for as long as the mode is still battery saver; otherwise dies
// quietly without running do_second_work() — the SECOND_UNIT tick handler
// is now driving per-second work and we must not double-tick if the user
// just left battery saver between fires.
static void second_timer_callback(void *data) {
  if (g_state.display_mode != DISPLAY_BATTERY_SAVER) {
    s_second_timer = NULL;
    return;
  }
  (void)do_second_work();
  s_second_timer = app_timer_register(1000, second_timer_callback, NULL);
}

// glossary: tick_handler
// In normal modes: subscribed to SECOND_UNIT — sole driver of Cycle
// progression, Session Elapsed, and Vibes. On Cycle Wrap also marks the
// HR Graph + Status Strip for redraw.
// In DISPLAY_BATTERY_SAVER: subscribed to MINUTE_UNIT — only refreshes the
// Clock once per minute. The per-second work is carried by
// second_timer_callback instead.
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  g_state.current_time = time(NULL);

  if (units_changed & SECOND_UNIT) {
    BreathingPhase prev_phase = g_state.current_phase;
    bool wrapped = do_second_work();
    if (!g_state.paused) {
      if (g_state.current_phase != prev_phase) {
        // glossary: phase_dot_indicator
        ui_update_technique();
      }
      if (wrapped) {
        ui_update_graph();
        ui_update_status();
      }
    }
  }

  ui_update_clock();                                // glossary: clock, session_elapsed_display
}

// glossary: top_button, pause_state
// Top Button short press → toggle Pause State.
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  g_state.paused = !g_state.paused;
  ui_update_status();
}

// glossary: hr_sample, initial_hr_sample, display_minimal, display_battery_saver
// Captures one synchronous HR Sample at launch / on Reset, so Current HR
// Display and HR Graph are non-blank from the start when the sensor has data.
// No-op in DISPLAY_MINIMAL or DISPLAY_BATTERY_SAVER — sensor is idled and
// the HR UI is hidden anyway.
// Only called from init / Reset (where buffer is being freshly initialized);
// the buffer-preserving Display Mode switch path does its own peek inline.
static void sample_initial_hr(void) {
  if (g_state.display_mode == DISPLAY_MINIMAL ||
      g_state.display_mode == DISPLAY_BATTERY_SAVER) return;
  HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
  if (hr > 30 && hr < 250) {
    g_state.hr_samples[0] = (int16_t)hr;
    g_state.hr_write_idx = 1;
    g_state.hr_sample_count = 1;
    g_state.current_hr = (int16_t)hr;
  }
}

// glossary: display_mode, display_battery_saver, backlight_always_on,
//           animation_timer, tick_handler
// Applies all Display-Mode-dependent runtime side effects: backlight, HR
// sample period, tick subscription, and Animation Timer / second AppTimer
// lifecycle. Idempotent — safe to call at launch (with the persisted
// mode) and on every Display Mode switch.
// `prev` is the previous Display Mode; pass the new mode (== g_state.
// display_mode) for "fresh entry" at launch.
static void apply_display_mode_runtime(DisplayMode prev) {
  DisplayMode mode = g_state.display_mode;

  light_enable(mode == DISPLAY_BACKLIGHT);

  bool hr_off = (mode == DISPLAY_MINIMAL || mode == DISPLAY_BATTERY_SAVER);
  (void)health_service_set_heart_rate_sample_period(hr_off ? 0 : 16);

  // Leaving MINIMAL or BATTERY_SAVER for a mode that re-enables HR: peek
  // the sensor once to refresh Current HR Display without touching the HR
  // Sample Buffer. Buffer is preserved across Display Mode switches so the
  // user keeps their prior HR Graph history (with a visible gap for the
  // Cycles spent with HR off). The next Cycle Wrap pushes a fresh sample.
  bool prev_hr_off = (prev == DISPLAY_MINIMAL || prev == DISPLAY_BATTERY_SAVER);
  if (prev_hr_off && !hr_off) {
    HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
    if (hr > 30 && hr < 250) g_state.current_hr = (int16_t)hr;
  }

  // glossary: display_battery_saver, tick_handler
  // Tick cadence: SECOND_UNIT in normal modes, MINUTE_UNIT in battery saver.
  // tick_timer_service_subscribe() replaces any prior subscription.
  if (mode == DISPLAY_BATTERY_SAVER) {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    // Start the 1 Hz AppTimer if we're entering battery saver. The
    // Animation Timer self-suppresses on its next fire (sees the new mode
    // and stops re-registering).
    if (s_second_timer == NULL) {
      s_second_timer = app_timer_register(1000, second_timer_callback, NULL);
    }
  } else {
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
    // Restart the Animation Timer if we just left battery saver and the
    // chain died on its own. The second-timer chain dies similarly on its
    // own once it sees the mode has flipped.
    if (s_animation_timer == NULL) {
      s_animation_timer = app_timer_register(100, anim_timer_callback, NULL);
    }
  }
}

// glossary: top_button, reset
// Top Button long press → Reset the Session and re-capture initial HR Sample.
static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  reset_session();
  sample_initial_hr();
  ui_update_all();
}

// glossary: middle_button, vibe_mode, vibe_mode_persist_key
// Middle Button → advance Vibe Mode (Every Second → Phase Only → Off → …)
// and persist the new value.
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  g_state.vibration_mode = (g_state.vibration_mode + 1) % 3;
  persist_write_int(0, g_state.vibration_mode);
  ui_update_status();
}

// glossary: middle_button, breathing_technique, technique_persist_key,
//           phase_duration_table, classic_technique
// Middle Button long press → advance Breathing Technique through the Phase
// Duration Table and persist. Resets Cycle position only — Session Elapsed,
// Completed Cycles, and the HR Sample Buffer are intentionally kept so the
// user can compare techniques inside the same session.
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  g_state.breathing_technique = (g_state.breathing_technique + 1) % TECHNIQUE_COUNT;
  persist_write_int(2, g_state.breathing_technique);
  reset_cycle_position();
  ui_update_all();
}

// glossary: bottom_button, display_mode, backlight_always_on,
//           backlight_persist_key, display_battery_saver
// Bottom Button → advance Display Mode
// (Default → Backlight → Minimal → Battery Saver → …), persist, apply
// runtime side effects + layout.
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  DisplayMode prev_mode = g_state.display_mode;
  g_state.display_mode = (g_state.display_mode + 1) % 4;
  persist_write_int(1, g_state.display_mode);

  apply_display_mode_runtime(prev_mode);

  ui_apply_display_mode();
  ui_update_all();
}

// glossary: top_button, middle_button, middle_button_long, bottom_button
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 500, up_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 500, select_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

// glossary: vibe_mode_persist_key, backlight_persist_key, technique_persist_key,
//           initial_hr_sample, tick_handler, animation_timer
static void init() {
  g_state.paused = false;
  g_state.vibration_mode = persist_exists(0) ? persist_read_int(0) : VIBE_EVERY_SECOND;
  // Display Mode persists as int. Old builds wrote a bool here; persist_read_int
  // returns 0 for that, which maps to DISPLAY_DEFAULT — acceptable migration.
  g_state.display_mode = persist_exists(1) ? persist_read_int(1) : DISPLAY_DEFAULT;
  if (g_state.display_mode > DISPLAY_BATTERY_SAVER) g_state.display_mode = DISPLAY_DEFAULT;
  // glossary: breathing_technique, classic_technique
  // Absent on builds before techniques shipped → fall back to Classic.
  g_state.breathing_technique = persist_exists(2) ? persist_read_int(2) : TECHNIQUE_CLASSIC;
  if (g_state.breathing_technique >= TECHNIQUE_COUNT) g_state.breathing_technique = TECHNIQUE_CLASSIC;
  g_state.current_time = time(NULL);
  reset_session();

  // Fresh launch — pass the current mode as `prev` so the helper does not
  // fire the "leaving MINIMAL/BATTERY_SAVER" HR peek; sample_initial_hr()
  // handles the launch HR seeding.
  apply_display_mode_runtime(g_state.display_mode);
  sample_initial_hr();

  ui_init();
  ui_apply_display_mode();
  window_set_click_config_provider(ui_get_window(), click_config_provider);
}

static void deinit() {
  (void)health_service_set_heart_rate_sample_period(0);
  ui_deinit();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
