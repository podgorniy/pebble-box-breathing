#include <pebble.h>
#include "graph.h"
#include "app_state.h"

void graph_draw(GContext *ctx, GRect bounds) {
  // Clear background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  int width = bounds.size.w;
  int height = bounds.size.h;

  graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorGreen, GColorWhite));

  int prev_x = -1;
  int prev_y = -1;

  // History index 0 is newest (right side), history index 180 is oldest (left side)
  // X=width-1 is newest, X=0 is oldest
  
  for (int i = 0; i < g_state.bpm_history_count && i < width; i++) {
    int x = width - 1 - i;
    
    if (g_state.bpm_valid[i]) {
      int bpm = g_state.bpm_history[i];
      if (bpm < 40) bpm = 40;
      if (bpm > 160) bpm = 160;
      
      // Map 40-160 to height-1 to 0
      // 160 -> y=0
      // 40 -> y=height-1
      int y = height - 1 - ((bpm - 40) * (height - 1) / 120);
      
      if (prev_x != -1 && prev_y != -1) {
        graphics_draw_line(ctx, GPoint(x, y), GPoint(prev_x, prev_y));
      } else {
        graphics_draw_pixel(ctx, GPoint(x, y));
      }
      prev_x = x;
      prev_y = y;
    } else {
      prev_x = -1;
      prev_y = -1;
    }
  }
}
