#include <pebble.h>
#include "breathing.h"
#include "app_state.h"

// glossary: phase
// Map total elapsed ms within a Cycle (0–15999) to its Phase.
static void update_phase(uint32_t ms) {
  if (ms < 4000) {
    g_state.current_phase = PHASE_INHALE;        // glossary: inhale_phase
  } else if (ms < 8000) {
    g_state.current_phase = PHASE_HOLD_FULL;     // glossary: hold_full_phase
  } else if (ms < 12000) {
    g_state.current_phase = PHASE_EXHALE;        // glossary: exhale_phase
  } else {
    g_state.current_phase = PHASE_HOLD_EMPTY;    // glossary: hold_empty_phase
  }
}

// glossary: animation_timer, anim_sub_ms, phase_fill
// Called every 100 ms by the Animation Timer. Advances Anim Sub Ms only —
// never fires Vibes. Cycle Elapsed Sec is owned by the Tick Handler.
void breathing_update(uint32_t delta_ms) {
  g_state.anim_sub_ms += delta_ms;
  if (g_state.anim_sub_ms > 999) g_state.anim_sub_ms = 999;
  update_phase(g_state.cycle_elapsed_sec * 1000 + g_state.anim_sub_ms);
}

// glossary: tick_handler, cycle_elapsed_sec, cycle_wrap
// Called every second by the Tick Handler. Advances Cycle Elapsed Sec (mod 16)
// and resets Anim Sub Ms. Returns true on Cycle Wrap (end of Hold Empty Phase →
// start of next Inhale Phase) — the caller uses this to capture an HR Sample,
// increment Completed Cycles, and possibly fire Session-Complete Vibe.
bool breathing_tick(void) {
  g_state.anim_sub_ms = 0;
  g_state.cycle_elapsed_sec = (g_state.cycle_elapsed_sec + 1) % 16;
  update_phase(g_state.cycle_elapsed_sec * 1000);
  return g_state.cycle_elapsed_sec == 0;
}

// glossary: phase_fill
// Compute Phase Fill (0.0–1.0) used to interpolate Filler Circle radius
// between Exhaled Reference Outline and Inhaled Reference Outline.
float breathing_get_fill(void) {
  uint32_t ms = g_state.cycle_elapsed_sec * 1000 + g_state.anim_sub_ms;
  uint32_t phase_elapsed_ms = ms % 4000;
  float p = phase_elapsed_ms / 4000.0f;
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
