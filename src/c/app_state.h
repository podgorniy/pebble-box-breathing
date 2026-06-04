#pragma once

#include <pebble.h>

typedef enum {
  VIBE_EVERY_SECOND,
  VIBE_PHASE_ONLY,
  VIBE_OFF
} VibrationMode;

typedef enum {
  PHASE_INHALE,
  PHASE_HOLD_FULL,
  PHASE_EXHALE,
  PHASE_HOLD_EMPTY
} BreathingPhase;

typedef struct {
  bool paused;
  bool backlight_always_on;

  VibrationMode vibration_mode;

  uint32_t session_elapsed_ms;
  uint32_t cycle_elapsed_ms; // 0 to 15999 ms

  time_t current_time;
  
  BreathingPhase current_phase;
} AppState;

extern AppState g_state;
