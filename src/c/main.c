#include <pebble.h>
#include "app_state.h"
#include "ui.h"
#include "breathing.h"
#include "vibration.h"

// glossary: app_state
AppState g_state;
static AppTimer *s_animation_timer;

// glossary: reset, session, completed_cycles, hr_sample_buffer
static void reset_session() {
  g_state.session_elapsed_ms = 0;
  g_state.cycle_elapsed_sec = 0;
  g_state.anim_sub_ms = 0;
  g_state.current_phase = PHASE_INHALE;
  g_state.completed_cycles = 0;
  g_state.hr_write_idx = 0;
  g_state.hr_sample_count = 0;
}

// glossary: animation_timer, filler_circle, phase_fill
// 100 ms Animation Timer callback. Skips work while in Pause State, but always
// re-arms itself. Only advances Anim Sub Ms + redraws Filler Circle — no Vibes.
static void anim_timer_callback(void *data) {
  if (!g_state.paused) {
    breathing_update(100);
    ui_update_breathing();
  }
  s_animation_timer = app_timer_register(100, anim_timer_callback, NULL);
}

// glossary: tick_handler
// 1 Hz Tick Handler. Sole driver of Cycle progression, Session Elapsed, and
// Vibes. On Cycle Wrap also captures an HR Sample, increments Completed Cycles,
// and fires Session-Complete Vibe at multiples of Target Cycles.
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  g_state.current_time = time(NULL);

  if (!g_state.paused && (units_changed & SECOND_UNIT)) {
    g_state.session_elapsed_ms += 1000;            // glossary: session_elapsed

    bool wrapped = breathing_tick();               // glossary: cycle_wrap
    vibration_trigger_for_second(g_state.cycle_elapsed_sec);  // glossary: vibe_mode_dispatch

    if (wrapped) {
      // glossary: hr_sample, hr_sample_buffer, hr_write_index
      HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
      if (hr > 30 && hr < 250) {
        g_state.hr_samples[g_state.hr_write_idx] = (int16_t)hr;
        g_state.hr_write_idx = (g_state.hr_write_idx + 1) % HR_SAMPLE_BUFFER;
        if (g_state.hr_sample_count < HR_SAMPLE_BUFFER) g_state.hr_sample_count++;
      }

      // glossary: completed_cycles, target_cycles, session_complete_vibe
      g_state.completed_cycles++;
      if (g_state.completed_cycles % 20 == 0) {
        vibration_trigger_session_complete();
      }
      ui_update_graph();
      ui_update_status();
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

// glossary: hr_sample, initial_hr_sample
// Captures one synchronous HR Sample at launch / on Reset, so Current HR
// Display and HR Graph are non-blank from the start when the sensor has data.
static void sample_initial_hr(void) {
  HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
  if (hr > 30 && hr < 250) {
    g_state.hr_samples[0] = (int16_t)hr;
    g_state.hr_write_idx = 1;
    g_state.hr_sample_count = 1;
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

// glossary: bottom_button, backlight_always_on, backlight_persist_key
// Bottom Button → toggle Backlight Always-On, persist, apply.
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  g_state.backlight_always_on = !g_state.backlight_always_on;
  persist_write_bool(1, g_state.backlight_always_on);
  light_enable(g_state.backlight_always_on);
  ui_update_status();
}

// glossary: top_button, middle_button, bottom_button
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 500, up_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

// glossary: vibe_mode_persist_key, backlight_persist_key, initial_hr_sample,
//           tick_handler, animation_timer
static void init() {
  g_state.paused = false;
  g_state.vibration_mode = persist_exists(0) ? persist_read_int(0) : VIBE_EVERY_SECOND;
  g_state.backlight_always_on = persist_exists(1) ? persist_read_bool(1) : false;
  g_state.current_time = time(NULL);
  reset_session();

  light_enable(g_state.backlight_always_on);
  (void)health_service_set_heart_rate_sample_period(16);
  sample_initial_hr();

  ui_init();
  window_set_click_config_provider(ui_get_window(), click_config_provider);

  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  s_animation_timer = app_timer_register(100, anim_timer_callback, NULL);
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
