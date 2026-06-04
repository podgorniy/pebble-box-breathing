#include <pebble.h>
#include "ui.h"
#include "app_state.h"
#include "breathing.h"
#include "layout.h"

static Window *s_window;
static Layout *s_root_layout;
static Layer *s_top_container;
static TextLayer *s_clock_layer;
static TextLayer *s_session_layer;
static Layer *s_breathing_layer;
static Layer *s_status_layer;

static void breathing_update_proc(Layer *layer, GContext *ctx) {
  GRect layer_bounds = layer_get_bounds(layer);
  int bar_h = 30;
  int bar_w = layer_bounds.size.w - 20;
  int bar_x = 10;
  int bar_y = (layer_bounds.size.h - bar_h) / 2;
  
  GRect bounds = GRect(bar_x, bar_y, bar_w, bar_h);
  
  // Draw outline
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_rect(ctx, bounds);
  
  // Fill inside
  float fill_pct = breathing_get_fill();
  int fill_w = (int)((bar_w - 2) * fill_pct);
  if (fill_w < 0) fill_w = 0;
  if (fill_w > bar_w - 2) fill_w = bar_w - 2;
  
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorCyan, GColorWhite));
  graphics_fill_rect(ctx, GRect(bar_x + 1, bar_y + 1, fill_w, bar_h - 2), 0, GCornerNone);
  
  // Determine Text and Color
  const char *text = "";
  GColor text_color = GColorWhite;
  switch (g_state.current_phase) {
    case PHASE_INHALE: 
      text = "In";
      break;
    case PHASE_HOLD_FULL: 
      text = "Hold";
      text_color = GColorBlack; // Dark color when full
      break;
    case PHASE_EXHALE: 
      text = "Out";
      break;
    case PHASE_HOLD_EMPTY: 
      text = "Hold";
      text_color = GColorWhite; // Light color when empty
      break;
  }

  // Draw Text with drop shadow (shadow only for white text for contrast, or always draw a subtle shadow)
  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  GRect text_bounds = GRect(bar_x, bar_y + (bar_h - 24) / 2 - 4, bar_w, 24);
  
  if (text_color == GColorWhite) {
    // Shadow
    graphics_context_set_text_color(ctx, GColorBlack);
    GRect shadow_bounds = GRect(bar_x + 1, bar_y + (bar_h - 24) / 2 - 3, bar_w, 24);
    graphics_draw_text(ctx, text, font, shadow_bounds, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }
  
  // Main Text
  graphics_context_set_text_color(ctx, text_color);
  graphics_draw_text(ctx, text, font, text_bounds, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void status_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int h = bounds.size.h;
  int w = bounds.size.w;
  
  int center_y = h / 2;
  int right_x = w - 15;
  int left_x = 15;
  
  int vibe_y = center_y - 5;
  int light_y = center_y + 10;
  
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);
  
  // Draw Vibe Mode (Right side, upper)
  if (g_state.vibration_mode == VIBE_EVERY_SECOND) {
    graphics_fill_circle(ctx, GPoint(right_x, vibe_y), 4);
  } else if (g_state.vibration_mode == VIBE_PHASE_ONLY) {
    graphics_draw_circle(ctx, GPoint(right_x, vibe_y), 4);
  } else {
    // Vibe Off (Crossed out circle)
    graphics_draw_circle(ctx, GPoint(right_x, vibe_y), 4);
    graphics_draw_line(ctx, GPoint(right_x - 4, vibe_y - 4), GPoint(right_x + 4, vibe_y + 4));
  }

  // Draw Brightness Mode (Right side, lower)
  if (g_state.backlight_always_on) {
    graphics_fill_circle(ctx, GPoint(right_x, light_y), 3);
    graphics_draw_line(ctx, GPoint(right_x - 5, light_y), GPoint(right_x + 5, light_y));
    graphics_draw_line(ctx, GPoint(right_x, light_y - 5), GPoint(right_x, light_y + 5));
  } else {
    graphics_draw_circle(ctx, GPoint(right_x, light_y), 3);
  }

  // Draw Pause State (Left side)
  if (g_state.paused) {
    graphics_fill_rect(ctx, GRect(left_x - 4, center_y - 6, 3, 12), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(left_x + 1, center_y - 6, 3, 12), 0, GCornerNone);
  }
}

static void top_container_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  layer_set_frame(text_layer_get_layer(s_clock_layer), GRect(5, 5, bounds.size.w / 2 - 5, bounds.size.h - 5));
  layer_set_frame(text_layer_get_layer(s_session_layer), GRect(bounds.size.w / 2, 5, bounds.size.w / 2 - 5, bounds.size.h - 5));
}

void ui_init(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  
  // Use Layout
  s_root_layout = layout_create(LayoutOrientationVertical, 3);
  
  // Slot 0: Top Container (Clock and Session) - 25% weight
  s_top_container = layer_create(GRectZero);
  layer_set_update_proc(s_top_container, top_container_update_proc);
  
  s_clock_layer = text_layer_create(GRectZero);
  text_layer_set_text_color(s_clock_layer, GColorWhite);
  text_layer_set_background_color(s_clock_layer, GColorClear);
  text_layer_set_font(s_clock_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_clock_layer, GTextAlignmentLeft);
  layer_add_child(s_top_container, text_layer_get_layer(s_clock_layer));
  
  s_session_layer = text_layer_create(GRectZero);
  text_layer_set_text_color(s_session_layer, GColorWhite);
  text_layer_set_background_color(s_session_layer, GColorClear);
  text_layer_set_font(s_session_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_session_layer, GTextAlignmentRight);
  layer_add_child(s_top_container, text_layer_get_layer(s_session_layer));

  layout_add_layer_with_params(s_root_layout, s_top_container, 0, 25);

  // Slot 1: Breathing Bar - 50% weight
  s_breathing_layer = layer_create(GRectZero);
  layer_set_update_proc(s_breathing_layer, breathing_update_proc);
  layout_add_layer_with_params(s_root_layout, s_breathing_layer, 1, 50);

  // Slot 2: Status Layer - 25% weight
  s_status_layer = layer_create(GRectZero);
  layer_set_update_proc(s_status_layer, status_update_proc);
  layout_add_layer_with_params(s_root_layout, s_status_layer, 2, 25);

  layout_add_to_window(s_root_layout, s_window);

  window_stack_push(s_window, true);
  ui_update_all();
}

void ui_deinit(void) {
  layout_destroy(s_root_layout);
  text_layer_destroy(s_clock_layer);
  text_layer_destroy(s_session_layer);
  layer_destroy(s_top_container);
  layer_destroy(s_breathing_layer);
  layer_destroy(s_status_layer);
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
  layer_mark_dirty(s_status_layer);
}

void ui_update_breathing(void) {
  layer_mark_dirty(s_breathing_layer);
}

void ui_update_all(void) {
  ui_update_clock();
  ui_update_status();
  ui_update_breathing();
}