#pragma once

#include <pebble.h>

void ui_init(void);
void ui_deinit(void);

Window* ui_get_window(void);

void ui_update_clock(void);
void ui_update_status(void);
void ui_update_breathing(void);
void ui_update_graph(void);
void ui_update_all(void);

// glossary: display_mode, display_minimal, layout_slot
// Reflows Layout Slots based on g_state.display_mode: in DISPLAY_MINIMAL the
// HR Graph slot collapses to 0% and the Breathing Circle slot expands to fill.
void ui_apply_display_mode(void);
