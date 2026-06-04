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
