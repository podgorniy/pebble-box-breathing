#include <pebble.h>
#include "app_state.h"
#include "ui.h"
#include "breathing.h"
#include "vibration.h"

AppState g_state;
static AppTimer *s_animation_timer;

static void reset_session() {
  g_state.session_elapsed_ms = 0;
  g_state.cycle_elapsed_ms = 0;
  g_state.current_phase = PHASE_INHALE;
}

static void anim_timer_callback(void *data) {
  if (!g_state.paused) {
    breathing_update(100);
    ui_update_breathing();
  }
  s_animation_timer = app_timer_register(100, anim_timer_callback, NULL);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  g_state.current_time = time(NULL);
  
  if (!g_state.paused) {
    if (units_changed & SECOND_UNIT) {
      g_state.session_elapsed_ms += 1000;
    }
  }
  ui_update_clock();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  g_state.paused = !g_state.paused;
  ui_update_status();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  g_state.vibration_mode = (g_state.vibration_mode + 1) % 3;
  persist_write_int(0, g_state.vibration_mode);
  light_enable(g_state.vibration_mode == VIBE_OFF);
  ui_update_status();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  reset_session();
  ui_update_all();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void init() {
  g_state.paused = false;
  g_state.vibration_mode = persist_exists(0) ? persist_read_int(0) : VIBE_EVERY_SECOND;
  g_state.current_time = time(NULL);
  reset_session();
  
  light_enable(g_state.vibration_mode == VIBE_OFF);
  
  ui_init();
  window_set_click_config_provider(ui_get_window(), click_config_provider);
  
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  s_animation_timer = app_timer_register(100, anim_timer_callback, NULL);
}

static void deinit() {
  ui_deinit();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
