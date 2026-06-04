#include <pebble.h>
#include "breathing.h"
#include "app_state.h"
#include "vibration.h"

void breathing_update(uint32_t delta_ms) {
  uint32_t old_sec = g_state.cycle_elapsed_ms / 1000;
  g_state.cycle_elapsed_ms = (g_state.cycle_elapsed_ms + delta_ms) % 16000;
  uint32_t new_sec = g_state.cycle_elapsed_ms / 1000;

  if (old_sec != new_sec) {
    vibration_trigger_for_second(new_sec);
  }

  if (g_state.cycle_elapsed_ms < 4000) {
    g_state.current_phase = PHASE_INHALE;
  } else if (g_state.cycle_elapsed_ms < 8000) {
    g_state.current_phase = PHASE_HOLD_FULL;
  } else if (g_state.cycle_elapsed_ms < 12000) {
    g_state.current_phase = PHASE_EXHALE;
  } else {
    g_state.current_phase = PHASE_HOLD_EMPTY;
  }
}

float breathing_get_fill() {
  uint32_t phase_elapsed_ms = g_state.cycle_elapsed_ms % 4000;
  float p = phase_elapsed_ms / 4000.0f;
  float fill = 0.0f;

  switch (g_state.current_phase) {
    case PHASE_INHALE:
      fill = p;
      break;
    case PHASE_HOLD_FULL:
      fill = 1.0f;
      break;
    case PHASE_EXHALE:
      fill = 1.0f - p;
      break;
    case PHASE_HOLD_EMPTY:
      fill = 0.0f;
      break;
  }

  return fill;
}
