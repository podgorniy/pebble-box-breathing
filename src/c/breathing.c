#include <pebble.h>
#include "breathing.h"
#include "app_state.h"
#include "techniques.h"

// glossary: phase, phase_duration_table
// Resolve the active Phase from total elapsed ms inside the current Cycle by
// consulting the Phase Duration Table of the active Breathing Technique.
static void update_phase(uint32_t ms) {
  // glossary: breathing_technique
  const TechniquePreset *t = technique_current();
  g_state.current_phase = technique_phase_at_sec(t, ms / 1000);  // glossary: inhale_phase, hold_full_phase, exhale_phase, hold_empty_phase
}

// glossary: animation_timer, anim_sub_ms, phase_fill
// Called every 100 ms by the Animation Timer. Advances Anim Sub Ms only —
// never fires Vibes. Cycle Elapsed Sec is owned by the Tick Handler.
void breathing_update(uint32_t delta_ms) {
  g_state.anim_sub_ms += delta_ms;
  if (g_state.anim_sub_ms > 999) g_state.anim_sub_ms = 999;
  update_phase(g_state.cycle_elapsed_sec * 1000 + g_state.anim_sub_ms);
}

// glossary: tick_handler, cycle_elapsed_sec, cycle_wrap, cycle_length
// Called every second by the Tick Handler. Advances Cycle Elapsed Sec mod
// Cycle Length of the active Breathing Technique and resets Anim Sub Ms.
// Returns true on Cycle Wrap (end of last Phase → start of next Inhale Phase) —
// the caller uses this to capture an HR Sample, increment Completed Cycles,
// and possibly fire Session-Complete Vibe.
bool breathing_tick(void) {
  // glossary: breathing_technique, phase_duration_table
  uint8_t cycle_len = technique_cycle_length_sec(technique_current());
  g_state.anim_sub_ms = 0;
  g_state.cycle_elapsed_sec = (g_state.cycle_elapsed_sec + 1) % cycle_len;
  update_phase(g_state.cycle_elapsed_sec * 1000);
  return g_state.cycle_elapsed_sec == 0;
}

// glossary: phase_fill, phase_duration_table
// Compute Phase Fill (0.0–1.0) used to interpolate Filler Circle radius
// between Exhaled Reference Outline and Inhaled Reference Outline. Uses the
// current Phase Duration from the active Breathing Technique so the animation
// stretches/shrinks to match the technique.
float breathing_get_fill(void) {
  const TechniquePreset *t = technique_current();
  uint32_t phase_start_ms = technique_phase_start_sec(t, g_state.current_phase) * 1000;
  uint32_t phase_dur_ms   = technique_phase_duration_sec(t, g_state.current_phase) * 1000;
  if (phase_dur_ms == 0) phase_dur_ms = 1;  // avoid div-by-zero on stale state

  uint32_t ms = g_state.cycle_elapsed_sec * 1000 + g_state.anim_sub_ms;
  // Guard against the (transient) case where ms briefly precedes phase_start_ms
  // right after a technique switch or Reset.
  uint32_t phase_elapsed_ms = (ms >= phase_start_ms) ? (ms - phase_start_ms) : 0;
  if (phase_elapsed_ms > phase_dur_ms) phase_elapsed_ms = phase_dur_ms;

  float p = phase_elapsed_ms / (float)phase_dur_ms;
  float fill = 0.0f;

  switch (g_state.current_phase) {
    case PHASE_INHALE:                            // glossary: inhale_phase
      fill = p;
      break;
    case PHASE_HOLD_FULL:                         // glossary: hold_full_phase
      fill = 1.0f;
      break;
    case PHASE_EXHALE:                            // glossary: exhale_phase
      fill = 1.0f - p;
      break;
    case PHASE_HOLD_EMPTY:                        // glossary: hold_empty_phase
      fill = 0.0f;
      break;
  }

  return fill;
}
