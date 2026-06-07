#include <pebble.h>
#include "vibration.h"
#include "app_state.h"
#include "techniques.h"

// glossary: short_vibe
static const uint32_t s_short_pulse_segments[] = { 50 };
static const VibePattern s_short_pulse = {
  .durations = s_short_pulse_segments,
  .num_segments = ARRAY_LENGTH(s_short_pulse_segments),
};

// glossary: long_vibe
static const uint32_t s_long_pulse_segments[] = { 250 };
static const VibePattern s_long_pulse = {
  .durations = s_long_pulse_segments,
  .num_segments = ARRAY_LENGTH(s_long_pulse_segments),
};

// glossary: session_complete_vibe
static const uint32_t s_session_complete_segments[] = { 400, 200, 400, 200, 400 };
static const VibePattern s_session_complete = {
  .durations = s_session_complete_segments,
  .num_segments = ARRAY_LENGTH(s_session_complete_segments),
};

// glossary: session_complete_vibe, target_cycles
// Fired by Tick Handler when Completed Cycles reaches a multiple of Target Cycles.
// Intentionally NOT gated by Vibe Mode — fires even in Vibe Off.
void vibration_trigger_session_complete(void) {
  vibes_enqueue_custom_pattern(s_session_complete);
}

// glossary: vibe_mode_dispatch, vibe_mode, phase_boundary, short_vibe, long_vibe
// Called once per second from Tick Handler with current Cycle Elapsed Sec.
// Decides which Vibe (if any) to fire based on current Vibe Mode and whether
// this second is a Phase Boundary under the active Breathing Technique
// (consults the Phase Duration Table).
void vibration_trigger_for_second(uint32_t cycle_sec) {
  if (g_state.vibration_mode == VIBE_OFF) {       // glossary: vibe_off
    return;
  }

  // glossary: phase_boundary, phase_duration_table, breathing_technique
  bool is_phase_boundary = technique_is_phase_boundary(technique_current(), cycle_sec);

  if (g_state.vibration_mode == VIBE_PHASE_ONLY) {  // glossary: vibe_phase_only
    if (is_phase_boundary) {
      vibes_enqueue_custom_pattern(s_short_pulse);
    }
  } else if (g_state.vibration_mode == VIBE_EVERY_SECOND) {  // glossary: vibe_every_second
    if (is_phase_boundary) {
      vibes_enqueue_custom_pattern(s_long_pulse);
    } else {
      vibes_enqueue_custom_pattern(s_short_pulse);
    }
  }
}
