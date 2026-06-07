#pragma once

#include <pebble.h>

// glossary: hr_sample_buffer, target_cycles
// HR Sample Buffer capacity; also equals Target Cycles (one HR Sample per Cycle).
#define HR_SAMPLE_BUFFER 20

// glossary: hr_sample, hr_sample_buffer
// Sentinel stored in hr_samples[] for a Cycle that produced no valid HR
// reading (MINIMAL mode, or sensor returned out-of-range). Valid HR is
// always > 30 BPM so 0 cannot collide with a real sample. Lets the HR Graph
// render a visible gap rather than collapsing missing cycles out of the plot.
#define HR_SAMPLE_NONE 0

// glossary: vibe_mode, vibe_every_second, vibe_phase_only, vibe_off
typedef enum {
  VIBE_EVERY_SECOND,
  VIBE_PHASE_ONLY,
  VIBE_OFF
} VibrationMode;

// glossary: display_mode, display_default, display_backlight, display_minimal,
//           display_battery_saver
// DISPLAY_DEFAULT — backlight off, HR sampling on, full layout.
// DISPLAY_BACKLIGHT — Backlight Always-On enabled, HR sampling on, full layout.
// DISPLAY_MINIMAL — backlight off, HR sampling off; Current HR Display + HR Graph
//                   hidden so Breathing Circle expands into freed vertical space.
// DISPLAY_BATTERY_SAVER — backlight off, HR sampling off; tick handler runs at
//                   MINUTE_UNIT so the screen only redraws once per minute. A
//                   1 Hz AppTimer carries vibes + cycle bookkeeping. Filler
//                   Circle pinned at max radius; Session Elapsed / Cycle
//                   Counter / Current HR blanked; Phase Dot Indicator hidden.
typedef enum {
  DISPLAY_DEFAULT,
  DISPLAY_BACKLIGHT,
  DISPLAY_MINIMAL,
  DISPLAY_BATTERY_SAVER
} DisplayMode;

// glossary: phase, inhale_phase, hold_full_phase, exhale_phase, hold_empty_phase
typedef enum {
  PHASE_INHALE,
  PHASE_HOLD_FULL,
  PHASE_EXHALE,
  PHASE_HOLD_EMPTY
} BreathingPhase;

// glossary: breathing_technique, classic_technique, beginner_technique,
//           extended_technique, meditation_technique, advanced_technique,
//           extended_exhale_technique, technique_478
// Ordered to match the cycling order driven by Middle Button Long.
// Persisted to Technique Persist Key (key 2).
typedef enum {
  TECHNIQUE_CLASSIC,
  TECHNIQUE_BEGINNER,
  TECHNIQUE_EXTENDED,
  TECHNIQUE_MEDITATION,
  TECHNIQUE_ADVANCED,
  TECHNIQUE_EXTENDED_EXHALE,
  TECHNIQUE_478,
  TECHNIQUE_COUNT
} BreathingTechnique;

// glossary: app_state
typedef struct {
  bool paused;                      // glossary: pause_state
  DisplayMode display_mode;         // glossary: display_mode, backlight_always_on

  VibrationMode vibration_mode;     // glossary: vibe_mode

  BreathingTechnique breathing_technique;  // glossary: breathing_technique  (persist key 2)

  uint32_t session_elapsed_ms;      // glossary: session_elapsed
  uint32_t cycle_elapsed_sec;       // glossary: cycle_elapsed_sec  (0 .. Cycle Length − 1; owned by Tick Handler)
  uint32_t anim_sub_ms;             // glossary: anim_sub_ms        (0–999, owned by Animation Timer)

  uint32_t completed_cycles;        // glossary: completed_cycles
  int16_t  hr_samples[HR_SAMPLE_BUFFER];  // glossary: hr_sample_buffer
  uint8_t  hr_write_idx;            // glossary: hr_write_index
  uint8_t  hr_sample_count;
  int16_t  current_hr;              // glossary: current_hr  (latest valid BPM, decoupled from buffer state)

  time_t current_time;              // glossary: clock

  BreathingPhase current_phase;     // glossary: phase
} AppState;

extern AppState g_state;
