#include <pebble.h>
#include "ui.h"
#include "app_state.h"
#include "breathing.h"
#include "graph.h"

static Window *s_window;
static TextLayer *s_clock_layer;
static TextLayer *s_session_layer;
static TextLayer *s_bpm_layer;
static TextLayer *s_status_layer;
static Layer *s_breathing_layer;
static Layer *s_graph_layer;

static void breathing_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int w = bounds.size.w;
  int h = bounds.size.h;
  
  // Draw outline
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_rect(ctx, bounds);
  
  // Draw segment dividers
  int segment_w = w / 4;
  for (int i = 1; i < 4; i++) {
    graphics_draw_line(ctx, GPoint(i * segment_w, 0), GPoint(i * segment_w, h));
  }
  
  // Fill inside
  float fill_pct = breathing_get_fill();
  int fill_w = (int)((w - 2) * fill_pct);
  if (fill_w < 0) fill_w = 0;
  if (fill_w > w - 2) fill_w = w - 2;
  
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorCyan, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, 1, fill_w, h - 2), 0, GCornerNone);
  
  // Draw marker
  int marker_x = 0;
  switch (g_state.current_phase) {
    case PHASE_INHALE: marker_x = segment_w / 2; break;
    case PHASE_HOLD_FULL: marker_x = segment_w + segment_w / 2; break;
    case PHASE_EXHALE: marker_x = 2 * segment_w + segment_w / 2; break;
    case PHASE_HOLD_EMPTY: marker_x = 3 * segment_w + segment_w / 2; break;
  }
  
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorYellow, GColorWhite));
  graphics_fill_circle(ctx, GPoint(marker_x, -5), 3); // draw slightly above
}

static void graph_update_proc(Layer *layer, GContext *ctx) {
  graph_draw(ctx, layer_get_bounds(layer));
}

void ui_init(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create top row layers
  s_clock_layer = text_layer_create(GRect(5, 5, bounds.size.w / 2 - 5, 24));
  text_layer_set_text_color(s_clock_layer, GColorWhite);
  text_layer_set_background_color(s_clock_layer, GColorClear);
  text_layer_set_font(s_clock_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_clock_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_clock_layer));
  
  s_session_layer = text_layer_create(GRect(bounds.size.w / 2, 5, bounds.size.w / 2 - 5, 24));
  text_layer_set_text_color(s_session_layer, GColorWhite);
  text_layer_set_background_color(s_session_layer, GColorClear);
  text_layer_set_font(s_session_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_session_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(s_session_layer));

  // Create second row layers
  s_bpm_layer = text_layer_create(GRect(5, 30, bounds.size.w / 2 - 5, 24));
  text_layer_set_text_color(s_bpm_layer, GColorWhite);
  text_layer_set_background_color(s_bpm_layer, GColorClear);
  text_layer_set_font(s_bpm_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_bpm_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_bpm_layer));
  
  s_status_layer = text_layer_create(GRect(bounds.size.w / 2, 30, bounds.size.w / 2 - 5, 24));
  text_layer_set_text_color(s_status_layer, GColorWhite);
  text_layer_set_background_color(s_status_layer, GColorClear);
  text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_status_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(s_status_layer));

  // Middle breathing bar
  s_breathing_layer = layer_create(GRect(10, 75, bounds.size.w - 20, 30));
  layer_set_update_proc(s_breathing_layer, breathing_update_proc);
  layer_add_child(window_layer, s_breathing_layer);

  // Bottom graph layer
  s_graph_layer = layer_create(GRect(0, 115, bounds.size.w, bounds.size.h - 115));
  layer_set_update_proc(s_graph_layer, graph_update_proc);
  layer_add_child(window_layer, s_graph_layer);

  window_stack_push(s_window, true);
  ui_update_all();
}

void ui_deinit(void) {
  text_layer_destroy(s_clock_layer);
  text_layer_destroy(s_session_layer);
  text_layer_destroy(s_bpm_layer);
  text_layer_destroy(s_status_layer);
  layer_destroy(s_breathing_layer);
  layer_destroy(s_graph_layer);
  window_destroy(s_window);
}

Window* ui_get_window(void) {
  return s_window;
}

void ui_update_clock(void) {
  static char clock_buf[8];
  static char session_buf[16];
  
  struct tm *tick_time = localtime(&g_state.current_time);
  strftime(clock_buf, sizeof(clock_buf), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_clock_layer, clock_buf);
  
  int session_s = (g_state.session_elapsed_ms / 1000) % 60;
  int session_m = (g_state.session_elapsed_ms / 1000) / 60;
  snprintf(session_buf, sizeof(session_buf), "%02d:%02d", session_m, session_s);
  text_layer_set_text(s_session_layer, session_buf);
}

void ui_update_status(void) {
  static char status_buf[16];
  
  const char* vibe_str = "";
  if (g_state.vibration_mode == VIBE_EVERY_SECOND) vibe_str = "\xe2\x80\xa2"; // bullet •
  else if (g_state.vibration_mode == VIBE_PHASE_ONLY) vibe_str = "\xe2\x97\x8f"; // black circle ●
  else vibe_str = "\xe2\x97\x8b"; // white circle ○
  
  const char* pause_str = g_state.paused ? "\xe2\x85\xa1" : ""; // roman numeral II Ⅱ
  
  snprintf(status_buf, sizeof(status_buf), "%s%s", pause_str, vibe_str);
  text_layer_set_text(s_status_layer, status_buf);
}

void ui_update_bpm(void) {
  static char bpm_buf[16];
  if (g_state.bpm_available) {
    snprintf(bpm_buf, sizeof(bpm_buf), "%d BPM", g_state.current_bpm);
  } else {
    snprintf(bpm_buf, sizeof(bpm_buf), "-- BPM");
  }
  text_layer_set_text(s_bpm_layer, bpm_buf);
}

void ui_update_breathing(void) {
  layer_mark_dirty(s_breathing_layer);
}

void ui_update_graph(void) {
  layer_mark_dirty(s_graph_layer);
}

void ui_update_all(void) {
  ui_update_clock();
  ui_update_status();
  ui_update_bpm();
  ui_update_breathing();
  ui_update_graph();
}
