#pragma once

#include <pebble.h>

void breathing_update(uint32_t delta_ms);
bool breathing_tick(void);
float breathing_get_fill(void);
