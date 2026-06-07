#pragma once

#include <pebble.h>
#include "app_state.h"

// glossary: technique_preset, phase_duration_table, cycle_length
// One row of the Phase Duration Table. `phase_count` is 3 (4-7-8) or 4 (everything
// else). `phase_sec[i]` holds the Phase Duration of phase i in seconds; for
// 3-phase techniques `phase_sec[3] == 0` and Hold Empty Phase is skipped at
// Cycle Wrap.
typedef struct {
  const char *display;      // "4-4-4-4" / "4-7-8"
  uint8_t     phase_count;  // 3 or 4
  uint8_t     phase_sec[4]; // durations per Phase (Inhale, Hold Full, Exhale, Hold Empty)
} TechniquePreset;

// glossary: phase_duration_table, breathing_technique
const TechniquePreset *technique_get(BreathingTechnique t);
const TechniquePreset *technique_current(void);

// glossary: cycle_length
uint8_t technique_cycle_length_sec(const TechniquePreset *p);

// glossary: phase, phase_duration_table
BreathingPhase technique_phase_at_sec(const TechniquePreset *p, uint32_t sec_in_cycle);
uint8_t        technique_phase_index(const TechniquePreset *p, BreathingPhase phase);
uint8_t        technique_phase_duration_sec(const TechniquePreset *p, BreathingPhase phase);
uint32_t       technique_phase_start_sec(const TechniquePreset *p, BreathingPhase phase);

// glossary: phase_boundary, phase_duration_table
bool technique_is_phase_boundary(const TechniquePreset *p, uint32_t sec_in_cycle);
