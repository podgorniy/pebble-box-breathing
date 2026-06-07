#include "techniques.h"

// glossary: phase_duration_table, breathing_technique
// Single source of truth for every Breathing Technique: its Technique Display
// string, phase count, and per-Phase durations. Adding a new preset is a
// one-row change here plus extending the BreathingTechnique enum.
static const TechniquePreset g_techniques[TECHNIQUE_COUNT] = {
  // glossary: classic_technique
  [TECHNIQUE_CLASSIC]         = { "4-4-4-4", 4, { 4, 4, 4, 4 } },
  // glossary: beginner_technique
  [TECHNIQUE_BEGINNER]        = { "3-3-3-3", 4, { 3, 3, 3, 3 } },
  // glossary: extended_technique
  [TECHNIQUE_EXTENDED]        = { "5-5-5-5", 4, { 5, 5, 5, 5 } },
  // glossary: meditation_technique
  [TECHNIQUE_MEDITATION]      = { "6-6-6-6", 4, { 6, 6, 6, 6 } },
  // glossary: advanced_technique
  [TECHNIQUE_ADVANCED]        = { "8-8-8-8", 4, { 8, 8, 8, 8 } },
  // glossary: extended_exhale_technique
  [TECHNIQUE_EXTENDED_EXHALE] = { "4-4-6-4", 4, { 4, 4, 6, 4 } },
  // glossary: technique_478
  [TECHNIQUE_478]             = { "4-7-8",   3, { 4, 7, 8, 0 } },
};

const TechniquePreset *technique_get(BreathingTechnique t) {
  if (t >= TECHNIQUE_COUNT) t = TECHNIQUE_CLASSIC;
  return &g_techniques[t];
}

const TechniquePreset *technique_current(void) {
  return technique_get(g_state.breathing_technique);
}

uint8_t technique_cycle_length_sec(const TechniquePreset *p) {
  uint8_t total = 0;
  for (uint8_t i = 0; i < p->phase_count; i++) total += p->phase_sec[i];
  return total;
}

// Phase enum positions are 0=INHALE, 1=HOLD_FULL, 2=EXHALE, 3=HOLD_EMPTY.
// For 3-phase techniques HOLD_EMPTY is never returned (the Cycle wraps at the
// end of EXHALE).
BreathingPhase technique_phase_at_sec(const TechniquePreset *p, uint32_t sec_in_cycle) {
  uint32_t cumulative = 0;
  for (uint8_t i = 0; i < p->phase_count; i++) {
    cumulative += p->phase_sec[i];
    if (sec_in_cycle < cumulative) return (BreathingPhase)i;
  }
  return (BreathingPhase)(p->phase_count - 1);  // clamp on overshoot
}

uint8_t technique_phase_index(const TechniquePreset *p, BreathingPhase phase) {
  (void)p;
  return (uint8_t)phase;
}

uint8_t technique_phase_duration_sec(const TechniquePreset *p, BreathingPhase phase) {
  uint8_t i = (uint8_t)phase;
  if (i >= p->phase_count) return 0;
  return p->phase_sec[i];
}

uint32_t technique_phase_start_sec(const TechniquePreset *p, BreathingPhase phase) {
  uint32_t start = 0;
  uint8_t target = (uint8_t)phase;
  if (target > p->phase_count) target = p->phase_count;
  for (uint8_t i = 0; i < target; i++) start += p->phase_sec[i];
  return start;
}

// glossary: phase_boundary
// True iff sec_in_cycle lands exactly on any cumulative Phase Duration offset
// of the active Breathing Technique (including 0, which is Cycle Wrap).
bool technique_is_phase_boundary(const TechniquePreset *p, uint32_t sec_in_cycle) {
  if (sec_in_cycle == 0) return true;
  uint32_t cumulative = 0;
  for (uint8_t i = 0; i < p->phase_count - 1; i++) {
    cumulative += p->phase_sec[i];
    if (sec_in_cycle == cumulative) return true;
  }
  return false;
}
