#include <pebble.h>
#include "heart_rate.h"
#include "app_state.h"
#include "ui.h"

static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventHeartRateUpdate) {
    HealthValue value = health_service_peek_current_value(HealthMetricHeartRateBPM);
    if (value > 0) {
      g_state.current_bpm = (int)value;
      g_state.bpm_available = true;
    } else {
      g_state.bpm_available = false;
    }
    ui_update_bpm();
  }
}

void heart_rate_init(void) {
  g_state.bpm_available = false;
  g_state.current_bpm = 0;
  
  if (health_service_events_subscribe(health_handler, NULL)) {
    // Force initial read
    health_handler(HealthEventHeartRateUpdate, NULL);
  }
}

void heart_rate_deinit(void) {
  health_service_events_unsubscribe();
}

void heart_rate_update_second(void) {
  // Push the current value into history
  if (g_state.bpm_history_count < 180) {
    g_state.bpm_history_count++;
  }
  
  // Shift history
  for (int i = g_state.bpm_history_count - 1; i > 0; i--) {
    g_state.bpm_history[i] = g_state.bpm_history[i - 1];
    g_state.bpm_valid[i] = g_state.bpm_valid[i - 1];
  }
  
  // Add new sample at index 0 (newest sample)
  if (g_state.bpm_available && g_state.current_bpm > 0) {
    g_state.bpm_history[0] = g_state.current_bpm;
    g_state.bpm_valid[0] = true;
  } else {
    g_state.bpm_valid[0] = false;
  }
  
  ui_update_graph();
}
