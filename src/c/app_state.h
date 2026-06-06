#pragma once

#include <pebble.h>

// glossary: hr_sample_buffer, target_cycles
// HR Sample Buffer capacity; also equals Target Cycles (one HR Sample per Cycle).
#define HR_SAMPLE_BUFFER 20

// glossary: vibe_mode, vibe_every_second, vibe_phase_only, vibe_off
typedef enum {
  VIBE_EVERY_SECOND,
  VIBE_PHASE_ONLY,
  VIBE_OFF
} VibrationMode;

// glossary: display_mode, display_default, display_backlight, display_minimal
// DISPLAY_DEFAULT — backlight off, HR sampling on, full layout.
// DISPLAY_BACKLIGHT — Backlight Always-On enabled, HR sampling on, full layout.
// DISPLAY_MINIMAL — backlight off, HR sampling off; Current HR Display + HR Graph
//                   hidden so Breathing Circle expands into freed vertical space.
typedef enum {
  DISPLAY_DEFAULT,
  DISPLAY_BACKLIGHT,
  DISPLAY_MINIMAL
} DisplayMode;

// glossary: phase, inhale_phase, hold_full_phase, exhale_phase, hold_empty_phase
typedef enum {
  PHASE_INHALE,
  PHASE_HOLD_FULL,
  PHASE_EXHALE,
  PHASE_HOLD_EMPTY
} BreathingPhase;

// glossary: app_state
typedef struct {
  bool paused;                      // glossary: pause_state
  DisplayMode display_mode;         // glossary: display_mode, backlight_always_on

  VibrationMode vibration_mode;     // glossary: vibe_mode

  uint32_t session_elapsed_ms;      // glossary: session_elapsed
  uint32_t cycle_elapsed_sec;       // glossary: cycle_elapsed_sec  (0–15, owned by Tick Handler)
  uint32_t anim_sub_ms;             // glossary: anim_sub_ms        (0–999, owned by Animation Timer)

  uint32_t completed_cycles;        // glossary: completed_cycles
  int16_t  hr_samples[HR_SAMPLE_BUFFER];  // glossary: hr_sample_buffer
  uint8_t  hr_write_idx;            // glossary: hr_write_index
  uint8_t  hr_sample_count;

  time_t current_time;              // glossary: clock

  BreathingPhase current_phase;     // glossary: phase
} AppState;

extern AppState g_state;
