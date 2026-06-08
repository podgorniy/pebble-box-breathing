#include <pebble.h>
#include "ui.h"
#include "app_state.h"
#include "breathing.h"
#include "layout.h"
#include "techniques.h"

static Window *s_window;
static Layout *s_root_layout;
static Layer *s_top_container;
static TextLayer *s_clock_layer;
static TextLayer *s_session_layer;
static TextLayer *s_cycle_layer;
static TextLayer *s_hr_layer;
static Layer *s_technique_layer;   // glossary: technique_display, phase_dot_indicator
static Layer *s_breathing_layer;
static Layer *s_status_layer;
static Layer *s_graph_layer;

// ---------------------------------------------------------------------------
// Breathing Circle
// glossary: breathing_circle, filler_circle,
//           exhaled_reference_outline, inhaled_reference_outline, phase_fill
// ---------------------------------------------------------------------------

static void breathing_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  // glossary: breathing_circle
  // Center Breathing Circle on the watchface (window center), not on the
  // breathing Layout Slot — slot 1 sits below the window's vertical midpoint
  // because slot 0 (top container) is 31%, so without this correction the
  // circle hangs noticeably low (especially in DISPLAY_MINIMAL where slot 1
  // is 69%). cx/cy are in layer-local coordinates.
  GRect frame = layer_get_frame(layer);
  GRect win_bounds = layer_get_bounds(window_get_root_layer(s_window));
  int cx = win_bounds.size.w / 2 - frame.origin.x;
  int cy = win_bounds.size.h / 2 - frame.origin.y;
  // glossary: display_minimal, display_battery_saver
  // In MINIMAL and BATTERY_SAVER the circle grows and slot 1 extends to the
  // window bottom; shift the center down so the larger disc settles inside
  // the freed lower half instead of crowding the top container.
  if (g_state.display_mode == DISPLAY_MINIMAL ||
      g_state.display_mode == DISPLAY_BATTERY_SAVER) {
    cy += 16;
  }

  // Max radius starts as the smaller of the four distances from (cx,cy) to a
  // layer edge — the slot-fit maximum — then expanded so the Inhaled
  // Reference Outline bleeds past slot 1 into the surrounding window. This
  // overflow is only visible because ui_init sets layer_set_clips(false) on
  // the breathing layer; otherwise the GContext would clip at the layer rect.
  int top_room   = cy;
  int bot_room   = bounds.size.h - cy;
  int left_room  = cx;
  int right_room = bounds.size.w - cx;
  int max_r = top_room;
  if (bot_room   < max_r) max_r = bot_room;
  if (left_room  < max_r) max_r = left_room;
  if (right_room < max_r) max_r = right_room;
  // glossary: display_minimal
  // top_room (= 31% slot 0) is the binding constraint in both modes, so the
  // slot-1 weight change alone doesn't grow the circle. In MINIMAL the HR
  // Graph is hidden, so we can bleed further down/out without colliding.
  if (g_state.display_mode == DISPLAY_MINIMAL) {
    max_r = (max_r * 7) / 5;  // +40%
  } else {
    max_r = (max_r * 7) / 5;  // +40%
  }

  int min_r = 19;         // glossary: exhaled_reference_outline  (min radius)
  if (max_r < min_r) max_r = min_r;

  float fill = breathing_get_fill();                       // glossary: phase_fill
  
  const TechniquePreset *t = technique_current();
  uint8_t phase_dur_sec = technique_phase_duration_sec(t, g_state.current_phase);
  if (phase_dur_sec == 0) phase_dur_sec = 1;

  // glossary: display_battery_saver
  // Pin Filler Circle at its smallest state (radius = min_r, the Exhale
  // endpoint) so the screen never needs to redraw between Phases.
  if (g_state.display_mode == DISPLAY_BATTERY_SAVER) {
    fill = 0.0f;
  }

  int radius = min_r + (int)((max_r - min_r) * fill + 0.5f);
  GPoint center = GPoint(cx, cy);

  // glossary: filler_circle  (solid cyan; white fallback on mono)
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorCyan, GColorWhite));
  graphics_fill_circle(ctx, center, radius);

  // Draw the inner small unanimated circle sectors
  GColor color_cyan = COLOR_FALLBACK(GColorCyan, GColorWhite);
  GColor color_blue = COLOR_FALLBACK(GColorOxfordBlue, GColorBlack);

  GColor inner_bg = color_blue;
  GColor inner_fg = color_cyan;
  float inner_fill = 0.0f;

  if (g_state.display_mode != DISPLAY_BATTERY_SAVER) {
    uint32_t phase_start_ms = technique_phase_start_sec(t, g_state.current_phase) * 1000;
    uint32_t phase_dur_ms = phase_dur_sec * 1000;
    uint32_t ms = g_state.cycle_elapsed_sec * 1000 + g_state.anim_sub_ms;
    uint32_t phase_elapsed_ms = (ms >= phase_start_ms) ? (ms - phase_start_ms) : 0;
    if (phase_elapsed_ms > phase_dur_ms) phase_elapsed_ms = phase_dur_ms;
    inner_fill = phase_elapsed_ms / (float)phase_dur_ms;
  }

  if (g_state.current_phase == PHASE_INHALE) {
    inner_bg = color_blue;
    inner_fg = color_cyan;
  } else if (g_state.current_phase == PHASE_EXHALE) {
    inner_bg = color_blue;
    inner_fg = color_cyan;
  } else if (g_state.current_phase == PHASE_HOLD_FULL) {
    inner_bg = color_cyan;
    inner_fg = color_blue;
  } else {
    inner_bg = color_cyan;
    inner_fg = color_blue;
  }

  graphics_context_set_fill_color(ctx, inner_bg);
  graphics_fill_circle(ctx, center, min_r);

  graphics_context_set_fill_color(ctx, inner_fg);
  GRect inner_rect = GRect(cx - min_r, cy - min_r, min_r * 2, min_r * 2);

  int32_t angle_end = (int32_t)(TRIG_MAX_ANGLE * inner_fill);
  if (angle_end > 0) {
    graphics_fill_radial(ctx, inner_rect, GOvalScaleModeFitCircle, min_r, 0, angle_end);
  }

  // glossary: exhaled_reference_outline, inhaled_reference_outline,
  //           display_battery_saver
  // Thin Cobalt-Blue outlines that mark the radius bounds of Filler Circle.
  // In BATTERY_SAVER only the outer (Inhaled Reference) outline is drawn —
  // it frames the static Exhale-state disc; the inner outline would be
  // covered by the filler anyway.
  graphics_context_set_stroke_color(ctx, GColorCobaltBlue);
  graphics_context_set_stroke_width(ctx, 1);
  if (g_state.display_mode != DISPLAY_BATTERY_SAVER) {
    graphics_draw_circle(ctx, center, min_r);
  }
  graphics_draw_circle(ctx, center, max_r);
  // Restore stroke for phase label outline
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 2);

  // Phase label - hidden for now
  // const char *text = "";
  // switch (g_state.current_phase) {
  //   case PHASE_INHALE:      text = "In";   break;
  //   case PHASE_HOLD_FULL:   text = "Hold"; break;
  //   case PHASE_EXHALE:      text = "Out";  break;
  //   case PHASE_HOLD_EMPTY:  text = "Hold"; break;
  // }
  //
  // GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  // int text_h = 29;
  // GRect tb = GRect(cx - radius, cy - text_h / 2 - 2, radius * 2, text_h + 5);
  //
  // Main text in Celeste (color) or White (mono) - no outline, 1px larger
  // graphics_context_set_text_color(ctx, COLOR_FALLBACK(GColorCeleste, GColorWhite));
  // graphics_draw_text(ctx, text, font, tb,
  //                    GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

// ---------------------------------------------------------------------------
// Legacy status layer renderer — unused; Status Strip lives in top container.
// Kept for reference; bound to Layout Slot 3 with weight 0.
// glossary: status_strip  (see top_container_update_proc for the live one)
// ---------------------------------------------------------------------------

static void status_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int w = bounds.size.w;
  int center_y = bounds.size.h / 2;

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);

  // Layout all three indicators in a horizontal row centered in the layer
  int center_x = w / 2;
  int spacing = 24;

  // Pause icon on left
  int pause_x = center_x - spacing;
  if (g_state.paused) {
    graphics_fill_rect(ctx, GRect(pause_x - 4, center_y - 6, 3, 12), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(pause_x + 1, center_y - 6, 3, 12), 0, GCornerNone);
  }

  // Vibe indicator in center (heartbeat indicator)
  int vibe_x = center_x;
  graphics_context_set_stroke_color(ctx, GColorFolly);
  graphics_context_set_fill_color(ctx, GColorFolly);
  if (g_state.vibration_mode == VIBE_EVERY_SECOND) {
    graphics_fill_circle(ctx, GPoint(vibe_x, center_y), 4);
  } else if (g_state.vibration_mode == VIBE_PHASE_ONLY) {
    graphics_draw_circle(ctx, GPoint(vibe_x, center_y), 4);
  } else {
    graphics_draw_circle(ctx, GPoint(vibe_x, center_y), 4);
    graphics_draw_line(ctx, GPoint(vibe_x - 4, center_y - 4),
                            GPoint(vibe_x + 4, center_y + 4));
  }
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);

  // Light indicator on right
  int light_x = center_x + spacing;
  if (g_state.display_mode == DISPLAY_BACKLIGHT) {
    graphics_fill_circle(ctx, GPoint(light_x, center_y), 3);
    graphics_draw_line(ctx, GPoint(light_x - 5, center_y),
                            GPoint(light_x + 5, center_y));
    graphics_draw_line(ctx, GPoint(light_x, center_y - 5),
                            GPoint(light_x, center_y + 5));
  } else if (g_state.display_mode == DISPLAY_MINIMAL) {
    graphics_fill_circle(ctx, GPoint(light_x, center_y), 3);
  } else {
    graphics_draw_circle(ctx, GPoint(light_x, center_y), 3);
  }
}

// ---------------------------------------------------------------------------
// Technique Display (left half row 2) + Phase Dot Indicator
// glossary: technique_display, phase_dot_indicator, breathing_technique,
//           phase_duration_table
// Lives as a child of the top container so it inherits Slot 0 positioning.
// Renders the active Breathing Technique's display string (e.g. "4-4-4-4" or
// "4-7-8") digit-segment by digit-segment so per-segment x-centres are known
// and the Phase Dot Indicator can be placed under the active Phase's digit.
// ---------------------------------------------------------------------------

static void technique_layer_update_proc(Layer *layer, GContext *ctx) {
  const TechniquePreset *t = technique_current();
  GRect bounds = layer_get_bounds(layer);
  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  GColor text_color = COLOR_FALLBACK(GColorVividCerulean, GColorWhite);

  int text_h = 18;
  int box_h  = text_h + 7;  // extra room so Phase Dot Indicator clears the digit baseline
  int centres[4] = {0};
  int seg_count = 0;
  int running_x = 0;
  char seg_buf[4];

  // glossary: technique_display
  // Walk t->display; each digit-run = one Phase, separated by '-'. Measure each
  // glyph run via graphics_text_layout_get_content_size so the Phase Dot
  // Indicator sits exactly under the digit even when widths differ ("4-7-8").
  const char *p = t->display;
  graphics_context_set_text_color(ctx, text_color);
  while (*p && seg_count < 4) {
    int n = 0;
    while (*p && *p != '-' && n < (int)sizeof(seg_buf) - 1) {
      seg_buf[n++] = *p++;
    }
    seg_buf[n] = '\0';

    if (n > 0) {
      GSize sz = graphics_text_layout_get_content_size(
        seg_buf, font, GRect(0, 0, 200, box_h),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      graphics_draw_text(ctx, seg_buf, font,
        GRect(running_x, 0, sz.w + 2, box_h),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      centres[seg_count++] = running_x + sz.w / 2;
      running_x += sz.w;
    }

    if (*p == '-') {
      GSize sz = graphics_text_layout_get_content_size(
        "-", font, GRect(0, 0, 200, box_h),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      graphics_draw_text(ctx, "-", font,
        GRect(running_x, 0, sz.w + 2, box_h),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      running_x += sz.w;
      p++;
    }
  }

  // glossary: phase_dot_indicator, display_battery_saver
  // Phase Dot Indicator is suppressed in DISPLAY_BATTERY_SAVER so the top
  // container has nothing that needs to redraw between Phase Boundaries.
  uint8_t idx = technique_phase_index(t, g_state.current_phase);
  if (idx < seg_count && g_state.display_mode != DISPLAY_BATTERY_SAVER) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    int dot_y = bounds.size.h - 2;
    if (dot_y < text_h + 5) dot_y = text_h + 5;
    graphics_fill_circle(ctx, GPoint(centres[idx], dot_y), 2);
  }
}

// ---------------------------------------------------------------------------
// Top container — Slot 0
// Hosts Clock, Session Elapsed Display, Cycle Counter Display, Current HR
// Display, and Status Strip (Pause / Vibe / Backlight Indicators).
// glossary: clock, session_elapsed_display, cycle_counter_display,
//           current_hr_display, status_strip, pause_indicator,
//           vibe_indicator, backlight_indicator
// ---------------------------------------------------------------------------

static void top_container_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int side = PBL_IF_ROUND_ELSE(32, 5);
  int top  = PBL_IF_ROUND_ELSE(8, 5);
  int half_w = bounds.size.w / 2;
  int right_w = half_w - side;
  int row1_h = 24;  // session elapsed (GOTHIC_24_BOLD)
  int row2_h = 18;  // cycle counter (GOTHIC_18_BOLD)
  int row3_h = 18;  // HR (GOTHIC_18_BOLD)
  
  int icon_spacing = 11;
  // Align icons vertically with text - center of line height
  int icon_center_y = top + row1_h / 2 + 4;

  // glossary: clock  (left half of top row)
  layer_set_frame(text_layer_get_layer(s_clock_layer),
    GRect(side, top, half_w - side, row1_h));

  // glossary: session_elapsed_display  (right-half top row, cyan MM:SS)
  layer_set_frame(text_layer_get_layer(s_session_layer),
    GRect(half_w, top, right_w, row1_h));

  // glossary: status_strip
  // Status Strip: centered horizontally; auto-recenters when Pause Indicator
  // appears/disappears so the visible icons always sit symmetrically.
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);

  int icon_center_x = bounds.size.w / 2;

  int num_icons = g_state.paused ? 3 : 2;
  int block_width = (num_icons - 1) * icon_spacing;
  int start_x = icon_center_x - block_width / 2;

  int icon_x = start_x;

  // glossary: pause_indicator
  if (g_state.paused) {
    graphics_fill_rect(ctx, GRect(icon_x - 4, icon_center_y - 6, 3, 12), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(icon_x + 1, icon_center_y - 6, 3, 12), 0, GCornerNone);
    icon_x += icon_spacing;
  }

  // glossary: vibe_indicator, vibe_mode
  // Filled = Vibe Every Second; outlined = Vibe Phase Only; crossed = Vibe Off.
  if (g_state.vibration_mode == VIBE_EVERY_SECOND) {
    graphics_fill_circle(ctx, GPoint(icon_x, icon_center_y), 4);
  } else if (g_state.vibration_mode == VIBE_PHASE_ONLY) {
    graphics_draw_circle(ctx, GPoint(icon_x, icon_center_y), 4);
  } else {
    graphics_draw_circle(ctx, GPoint(icon_x, icon_center_y), 4);
    graphics_draw_line(ctx, GPoint(icon_x - 4, icon_center_y - 4), GPoint(icon_x + 4, icon_center_y + 4));
  }
  icon_x += icon_spacing;

  // glossary: backlight_indicator, display_mode, display_default,
  //           display_backlight, display_minimal, display_battery_saver
  // Outline circle = DEFAULT (backlight off, HR on);
  // 4-point star = BACKLIGHT (backlight on, HR on);
  // filled dot = MINIMAL (backlight off, HR off, big-circle layout);
  // tiny battery glyph = BATTERY_SAVER (minute-tick + frozen UI).
  if (g_state.display_mode == DISPLAY_BACKLIGHT) {
    graphics_fill_circle(ctx, GPoint(icon_x, icon_center_y), 3);
    graphics_draw_line(ctx, GPoint(icon_x - 5, icon_center_y), GPoint(icon_x + 5, icon_center_y));
    graphics_draw_line(ctx, GPoint(icon_x, icon_center_y - 5), GPoint(icon_x, icon_center_y + 5));
  } else if (g_state.display_mode == DISPLAY_MINIMAL) {
    graphics_fill_circle(ctx, GPoint(icon_x, icon_center_y), 3);
  } else if (g_state.display_mode == DISPLAY_BATTERY_SAVER) {
    // 9x5 outline rect centered on (icon_x, icon_center_y) + 2x3 nub on right.
    graphics_draw_rect(ctx, GRect(icon_x - 4, icon_center_y - 2, 9, 5));
    graphics_fill_rect(ctx, GRect(icon_x + 5, icon_center_y - 1, 2, 3), 0, GCornerNone);
  } else {
    graphics_draw_circle(ctx, GPoint(icon_x, icon_center_y), 3);
  }

  // glossary: cycle_counter_display
  layer_set_frame(text_layer_get_layer(s_cycle_layer),
    GRect(half_w, top + row1_h + 2, right_w, row2_h));

  // glossary: current_hr_display
  layer_set_frame(text_layer_get_layer(s_hr_layer),
    GRect(half_w, top + row1_h + 2 + row2_h + 2, right_w, row3_h));

  // glossary: technique_display, phase_dot_indicator
  // Left half row 2 — directly under Clock. Layer height is row2_h + dot
  // padding so the Phase Dot Indicator sits a few pixels below the digit
  // baseline with breathing room.
  layer_set_frame(s_technique_layer,
    GRect(side, top + row1_h + 2, half_w - side, row2_h + 7));
}

// ---------------------------------------------------------------------------
// HR Graph — Slot 2
// glossary: hr_graph, hr_sample_buffer, hr_min_label, hr_max_label
// ---------------------------------------------------------------------------

static void hr_graph_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // glossary: display_minimal, display_battery_saver
  // HR Graph hidden entirely in MINIMAL and BATTERY_SAVER (sensor idled
  // in both; layout collapses Slot 2 to weight 0).
  if (g_state.display_mode == DISPLAY_MINIMAL ||
      g_state.display_mode == DISPLAY_BATTERY_SAVER) return;
  if (g_state.hr_sample_count == 0) return;

  int label_w = 28;
  int side = PBL_IF_ROUND_ELSE(32, 5);
  int label_x = side;  // Align with clock
  int px      = side + label_w + 2;  // Chart starts after labels
  int pw      = bounds.size.w - px - side;
  // Align with clock's top padding, plus 2px margin to avoid overwriting breathing circle outline
  int py = PBL_IF_ROUND_ELSE(8, 5) + 2;
  int ph = bounds.size.h - 6 - PBL_IF_ROUND_ELSE(8, 5) - 2;
  if (pw < 4 || ph < 4) return;

  // Min/max over valid samples only. Sentinel slots (Cycles without a
  // measurement, e.g. spent in MINIMAL) are skipped so they don't pull
  // the y-range or render as bogus labels.
  int16_t mn = 250, mx = 30;
  int valid_count = 0;
  for (int i = 0; i < g_state.hr_sample_count; i++) {
    uint8_t idx = (uint8_t)((g_state.hr_write_idx + HR_SAMPLE_BUFFER
                              - g_state.hr_sample_count + i) % HR_SAMPLE_BUFFER);
    int16_t s = g_state.hr_samples[idx];
    if (s == HR_SAMPLE_NONE) continue;
    if (s < mn) mn = s;
    if (s > mx) mx = s;
    valid_count++;
  }
  if (valid_count == 0) return;
  int16_t range = mx - mn;
  if (range < 5) range = 5;

  // glossary: hr_max_label, hr_min_label
  char max_buf[4], min_buf[4];
  snprintf(max_buf, sizeof(max_buf), "%d", (int)mx);
  snprintf(min_buf, sizeof(min_buf), "%d", (int)mn);
  GFont label_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  
  // Get font height for precise vertical positioning
  GSize text_size = graphics_text_layout_get_content_size("999", label_font,
    GRect(0, 0, label_w, ph), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
  int font_h = text_size.h;

  // Outline for HR labels (black, visible on all platforms)
  graphics_context_set_text_color(ctx, GColorBlack);
  for (int dx = -1; dx <= 1; dx++) {
    for (int dy = -1; dy <= 1; dy++) {
      if (dx == 0 && dy == 0) continue;
      graphics_draw_text(ctx, max_buf, label_font,
        GRect(label_x + dx, py + dy, label_w, font_h),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      graphics_draw_text(ctx, min_buf, label_font,
        GRect(label_x + dx, py + ph - font_h + dy, label_w, font_h),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    }
  }

  // Main text in Folly
  graphics_context_set_text_color(ctx, COLOR_FALLBACK(GColorFolly, GColorWhite));
  graphics_draw_text(ctx, max_buf, label_font,
    GRect(label_x, py, label_w, font_h),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, min_buf, label_font,
    GRect(label_x, py + ph - font_h, label_w, font_h),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // glossary: hr_graph  (Shocking Pink line + 2 px dots per HR Sample)
  graphics_context_set_stroke_color(ctx, GColorShockingPink);
  graphics_context_set_fill_color(ctx, GColorShockingPink);
  graphics_context_set_stroke_width(ctx, 2);

  GPoint prev = GPoint(0, 0);
  bool has_prev = false;

  for (int i = 0; i < g_state.hr_sample_count; i++) {
    uint8_t idx = (uint8_t)((g_state.hr_write_idx + HR_SAMPLE_BUFFER
                              - g_state.hr_sample_count + i) % HR_SAMPLE_BUFFER);
    int16_t hr = g_state.hr_samples[idx];

    // Sentinel slot — Cycle had no valid HR. Leave a visible gap: skip the
    // dot and break the connecting line on either side.
    if (hr == HR_SAMPLE_NONE) {
      has_prev = false;
      continue;
    }

    int x = px + (i * pw) / (HR_SAMPLE_BUFFER - 1);
    int y = py + ph - ((hr - mn) * ph / range);
    if (y < py)      y = py;
    if (y > py + ph) y = py + ph;

    GPoint pt = GPoint(x, y);
    if (has_prev) graphics_draw_line(ctx, prev, pt);
    graphics_fill_circle(ctx, pt, 2);
    prev = pt;
    has_prev = true;
  }
}

// ---------------------------------------------------------------------------
// Init / deinit
// ---------------------------------------------------------------------------

// glossary: layout_slot
// 4 vertical Layout Slots, weights 31 / 41 / 28 / 0:
//   Slot 0 — top container (Clock, Session Elapsed Display, Cycle Counter
//            Display, Current HR Display, Status Strip)
//   Slot 1 — Breathing Circle
//   Slot 2 — HR Graph
//   Slot 3 — reserved (weight 0)
void ui_init(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);

  s_root_layout = layout_create(LayoutOrientationVertical, 4);

  // Slot 0: top container — 31%
  s_top_container = layer_create(GRectZero);
  layer_set_update_proc(s_top_container, top_container_update_proc);
  // Allow Phase Dot Indicator to render past Slot 0's bottom edge — Slot 0's
  // 31% height is just shy of fitting (Clock + Technique Display + dot
  // padding). The dot's overflow lands in the empty top of Slot 1, above the
  // Breathing Circle.
  layer_set_clips(s_top_container, false);

  s_clock_layer = text_layer_create(GRectZero);
  text_layer_set_text_color(s_clock_layer, GColorWhite);
  text_layer_set_background_color(s_clock_layer, GColorClear);
  text_layer_set_font(s_clock_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_clock_layer, GTextAlignmentLeft);
  layer_add_child(s_top_container, text_layer_get_layer(s_clock_layer));

  s_session_layer = text_layer_create(GRectZero);
  text_layer_set_text_color(s_session_layer, GColorCyan);
  text_layer_set_background_color(s_session_layer, GColorClear);
  text_layer_set_font(s_session_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_session_layer, GTextAlignmentRight);
  layer_add_child(s_top_container, text_layer_get_layer(s_session_layer));

  s_cycle_layer = text_layer_create(GRectZero);
  text_layer_set_text_color(s_cycle_layer, COLOR_FALLBACK(GColorVividCerulean, GColorWhite));
  text_layer_set_background_color(s_cycle_layer, GColorClear);
  text_layer_set_font(s_cycle_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_cycle_layer, GTextAlignmentRight);
  layer_add_child(s_top_container, text_layer_get_layer(s_cycle_layer));

  s_hr_layer = text_layer_create(GRectZero);
  text_layer_set_text_color(s_hr_layer, COLOR_FALLBACK(GColorFolly, GColorWhite));
  text_layer_set_background_color(s_hr_layer, GColorClear);
  text_layer_set_font(s_hr_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_hr_layer, GTextAlignmentRight);
  layer_add_child(s_top_container, text_layer_get_layer(s_hr_layer));

  // glossary: technique_display, phase_dot_indicator
  s_technique_layer = layer_create(GRectZero);
  layer_set_update_proc(s_technique_layer, technique_layer_update_proc);
  // Dot sits 1 px past the layer's logical bottom; disable clipping so it
  // renders fully (parent clip is also off — see s_top_container above).
  layer_set_clips(s_technique_layer, false);
  layer_add_child(s_top_container, s_technique_layer);

  layout_add_layer_with_params(s_root_layout, s_top_container, 0, 31);

  // Slot 1: Breathing Circle — 41%
  // glossary: breathing_circle
  s_breathing_layer = layer_create(GRectZero);
  layer_set_update_proc(s_breathing_layer, breathing_update_proc);
  // Allow Breathing Circle to render past slot 1 — the +20% Inhaled Reference
  // Outline radius in breathing_update_proc would otherwise be clipped.
  layer_set_clips(s_breathing_layer, false);
  layout_add_layer_with_params(s_root_layout, s_breathing_layer, 1, 41);

  // Slot 2: HR Graph — 28%
  // glossary: hr_graph
  s_graph_layer = layer_create(GRectZero);
  layer_set_update_proc(s_graph_layer, hr_graph_update_proc);
  layout_add_layer_with_params(s_root_layout, s_graph_layer, 2, 28);

  // Slot 3: reserved (Status Strip lives in top container, not here) — 0%
  s_status_layer = layer_create(GRectZero);
  layout_add_layer_with_params(s_root_layout, s_status_layer, 3, 0);

  layout_add_to_window(s_root_layout, s_window);

  window_stack_push(s_window, true);
  ui_update_all();
}

void ui_deinit(void) {
  layout_destroy(s_root_layout);
  text_layer_destroy(s_clock_layer);
  text_layer_destroy(s_session_layer);
  text_layer_destroy(s_cycle_layer);
  text_layer_destroy(s_hr_layer);
  layer_destroy(s_technique_layer);
  layer_destroy(s_top_container);
  layer_destroy(s_breathing_layer);
  layer_destroy(s_status_layer);
  layer_destroy(s_graph_layer);
  window_destroy(s_window);
}

Window* ui_get_window(void) {
  return s_window;
}

// ---------------------------------------------------------------------------
// Update helpers
// ---------------------------------------------------------------------------

// glossary: clock, session_elapsed_display, cycle_counter, cycle_counter_display,
//           current_hr, current_hr_display
// Refreshes Clock + Session Elapsed Display + Cycle Counter Display + Current
// HR Display text on every Tick Handler invocation.
void ui_update_clock(void) {
  static char clock_buf[8];
  static char session_buf[16];
  static char cycle_buf[8];
  static char hr_buf[8];

  // glossary: clock
  struct tm *tick_time = localtime(&g_state.current_time);
  strftime(clock_buf, sizeof(clock_buf),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_clock_layer, clock_buf);

  // glossary: session_elapsed, session_elapsed_display, display_battery_saver
  // Session Elapsed Display is blank in BATTERY_SAVER.
  if (g_state.display_mode == DISPLAY_BATTERY_SAVER) {
    session_buf[0] = '\0';
  } else {
    int session_s = (g_state.session_elapsed_ms / 1000) % 60;
    int session_m = (g_state.session_elapsed_ms / 1000) / 60;
    snprintf(session_buf, sizeof(session_buf), "%02d:%02d", session_m, session_s);
  }
  text_layer_set_text(s_session_layer, session_buf);

  // glossary: cycle_counter, target_cycles, display_battery_saver
  // Counter keeps incrementing past Target Cycles (no modulo wrap), so the
  // displayed value matches Completed Cycles for the whole Session.
  // Blank in BATTERY_SAVER.
  if (g_state.display_mode == DISPLAY_BATTERY_SAVER) {
    cycle_buf[0] = '\0';
  } else {
    snprintf(cycle_buf, sizeof(cycle_buf), "%u/20",
             (unsigned)g_state.completed_cycles);
  }
  text_layer_set_text(s_cycle_layer, cycle_buf);

  // glossary: current_hr, current_hr_display, display_minimal,
  //           display_battery_saver
  // Current HR Display is blank in MINIMAL and BATTERY_SAVER (HR sensor
  // idled in both). Outside those modes, read g_state.current_hr directly
  // — it's the latest valid BPM regardless of whether the most recent
  // buffer slot is a sentinel from a skipped Cycle.
  if (g_state.display_mode == DISPLAY_MINIMAL ||
      g_state.display_mode == DISPLAY_BATTERY_SAVER) {
    hr_buf[0] = '\0';
  } else if (g_state.current_hr > 0) {
    snprintf(hr_buf, sizeof(hr_buf), "%d", (int)g_state.current_hr);
  } else {
    snprintf(hr_buf, sizeof(hr_buf), "--");
  }
  text_layer_set_text(s_hr_layer, hr_buf);
}

void ui_update_status(void) {
  layer_mark_dirty(s_top_container);
}

void ui_update_breathing(void) {
  layer_mark_dirty(s_breathing_layer);
}

void ui_update_graph(void) {
  layer_mark_dirty(s_graph_layer);
}

// glossary: technique_display, phase_dot_indicator
// Marks just the Technique Display layer dirty. Called whenever the current
// Phase changes (from the Animation Timer hook in main.c) or the Breathing
// Technique itself changes (from Middle Button long press).
void ui_update_technique(void) {
  layer_mark_dirty(s_technique_layer);
}

void ui_update_all(void) {
  ui_update_clock();
  ui_update_status();
  ui_update_breathing();
  ui_update_graph();
  ui_update_technique();
}

// glossary: display_mode, display_minimal, display_battery_saver, layout_slot,
//           breathing_circle, hr_graph
// In MINIMAL and BATTERY_SAVER, collapse Slot 2 (HR Graph) to 0% and grow
// Slot 1 (Breathing Circle) to absorb the freed 28% — making the Filler
// Circle visibly larger. Order matters: shrink first then grow so the
// layout module's weight-sum guard (sums all current weights, must stay
// ≤ 100) never trips.
void ui_apply_display_mode(void) {
  if (g_state.display_mode == DISPLAY_MINIMAL ||
      g_state.display_mode == DISPLAY_BATTERY_SAVER) {
    layout_add_layer_with_params(s_root_layout, s_graph_layer, 2, 0);
    layout_add_layer_with_params(s_root_layout, s_breathing_layer, 1, 69);
  } else {
    layout_add_layer_with_params(s_root_layout, s_breathing_layer, 1, 41);
    layout_add_layer_with_params(s_root_layout, s_graph_layer, 2, 28);
  }
}
