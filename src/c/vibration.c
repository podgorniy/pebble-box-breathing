#include <pebble.h>
#include "vibration.h"
#include "app_state.h"

static const uint32_t s_short_pulse_segments[] = { 50 };
static const VibePattern s_short_pulse = {
  .durations = s_short_pulse_segments,
  .num_segments = ARRAY_LENGTH(s_short_pulse_segments),
};

static const uint32_t s_long_pulse_segments[] = { 250 };
static const VibePattern s_long_pulse = {
  .durations = s_long_pulse_segments,
  .num_segments = ARRAY_LENGTH(s_long_pulse_segments),
};

void vibration_trigger_for_second(uint32_t cycle_sec) {
  if (g_state.vibration_mode == VIBE_OFF) {
    return;
  }

  bool is_phase_boundary = (cycle_sec % 4 == 0);

  if (g_state.vibration_mode == VIBE_PHASE_ONLY) {
    if (is_phase_boundary) {
      vibes_enqueue_custom_pattern(s_short_pulse);
    }
  } else if (g_state.vibration_mode == VIBE_EVERY_SECOND) {
    if (is_phase_boundary) {
      vibes_enqueue_custom_pattern(s_long_pulse);
    } else {
      vibes_enqueue_custom_pattern(s_short_pulse);
    }
  }
}
