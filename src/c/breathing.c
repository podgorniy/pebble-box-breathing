#include <pebble.h>
#include "breathing.h"
#include "app_state.h"
#include "vibration.h"

#define MIN_FILL 0.18f
#define MAX_FILL 1.00f

static float smoothstep(float p) {
  return p * p * (3.0f - 2.0f * p);
}

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
  float eased = smoothstep(p);
  float fill = MIN_FILL;

  switch (g_state.current_phase) {
    case PHASE_INHALE:
      fill = MIN_FILL + eased * (MAX_FILL - MIN_FILL);
      break;
    case PHASE_HOLD_FULL:
      fill = MAX_FILL;
      break;
    case PHASE_EXHALE:
      fill = MAX_FILL - eased * (MAX_FILL - MIN_FILL);
      break;
    case PHASE_HOLD_EMPTY:
      fill = MIN_FILL;
      break;
  }

  return fill;
}
