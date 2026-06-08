#include "flashback_screen.h"
#include "../colors.h"
#include "../ui_constants.h"
#include <pebble.h>

static Layer *s_header_bg_layer;

static void draw_header_bg_callback(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, HEADER_COLOR);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

MenuLayer *flashback_screen_create_menu_layer(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

#ifdef PBL_ROUND
  window_set_background_color(window, HEADER_COLOR);
  if (!s_header_bg_layer) {
    s_header_bg_layer = layer_create(GRect(0, 0, bounds.size.w,
                                           MENU_HEADER_HEIGHT + 24));
    layer_set_update_proc(s_header_bg_layer, draw_header_bg_callback);
    layer_add_child(window_layer, s_header_bg_layer);
  }
#endif

  MenuLayer *menu_layer = menu_layer_create(bounds);
  menu_layer_set_click_config_onto_window(menu_layer, window);

#ifdef PBL_ROUND
  menu_layer_set_center_focused(menu_layer, true);
#endif

  menu_layer_set_highlight_colors(menu_layer, HIGHLIGHT_BG, TEXT_COLOR_SELECTED);
  layer_add_child(window_layer, menu_layer_get_layer(menu_layer));

  return menu_layer;
}

void flashback_screen_destroy_header_background(void) {
  if (s_header_bg_layer) {
    layer_destroy(s_header_bg_layer);
    s_header_bg_layer = NULL;
  }
}

static void draw_flashback_logo(GContext *ctx, GPoint center) {
  graphics_context_set_stroke_color(ctx, HEADER_CONTENT_COLOR);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_circle(ctx, center, 8);

  graphics_draw_line(ctx,
                     GPoint(center.x - 1, center.y - 3),
                     GPoint(center.x - 4, center.y));
  graphics_draw_line(ctx,
                     GPoint(center.x - 4, center.y),
                     GPoint(center.x - 1, center.y + 3));

  graphics_draw_line(ctx,
                     GPoint(center.x + 3, center.y - 3),
                     GPoint(center.x, center.y));
  graphics_draw_line(ctx,
                     GPoint(center.x, center.y),
                     GPoint(center.x + 3, center.y + 3));

  graphics_context_set_stroke_width(ctx, 1);
}

void flashback_screen_draw_header(GContext *ctx, const Layer *cell_layer,
                                  const char *subtitle_left,
                                  const char *subtitle_right) {
  GRect bounds = layer_get_bounds(cell_layer);
  graphics_context_set_fill_color(ctx, HEADER_COLOR);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_text_color(ctx, HEADER_CONTENT_COLOR);

  const int logo_center_y = MENU_HEADER_TITLE_Y + 10;
  const int logo_center_x = HDR_INSET + 8;
  draw_flashback_logo(ctx, GPoint(logo_center_x, logo_center_y));

  const int title_x = HDR_INSET + MENU_HEADER_LOGO_SIZE + MENU_HEADER_LOGO_GAP;
  const int title_width = bounds.size.w - title_x - HDR_INSET - MENU_HEADER_RIGHT_WIDTH;
  GRect title_rect = GRect(title_x, MENU_HEADER_TITLE_Y, title_width > 0 ? title_width : 0, 20);
  graphics_draw_text(ctx, APP_TITLE,
                    MENU_HEADER_TITLE_FONT,
                    title_rect,
                    GTextOverflowModeTrailingEllipsis,
                    GTextAlignmentLeft,
                    NULL);

  if (subtitle_right) {
    GRect right_rect = GRect(bounds.size.w - HDR_INSET - MENU_HEADER_RIGHT_WIDTH,
                             MENU_HEADER_TITLE_Y,
                             MENU_HEADER_RIGHT_WIDTH, 20);
    graphics_draw_text(ctx, subtitle_right,
                      MENU_HEADER_TITLE_FONT,
                      right_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentRight,
                      NULL);
  }

  if (subtitle_right) {
    GRect left_rect = GRect(HDR_INSET, MENU_HEADER_SUBTITLE_Y,
                            bounds.size.w - 2 * HDR_INSET, 24);
    graphics_draw_text(ctx, subtitle_left,
                      MENU_ROW_FONT,
                      left_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);
  } else {
    GRect full_rect = GRect(HDR_INSET, MENU_HEADER_SUBTITLE_Y,
                            bounds.size.w - 2 * HDR_INSET, 24);
    graphics_draw_text(ctx, subtitle_left,
                      MENU_ROW_FONT,
                      full_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);
  }
}

int16_t flashback_screen_cell_height_callback(struct MenuLayer *menu_layer,
                                              MenuIndex *cell_index,
                                              void *context) {
  return MENU_CELL_HEIGHT;
}

int16_t flashback_screen_header_height_callback(struct MenuLayer *menu_layer,
                                                uint16_t section_index,
                                                void *context) {
  return MENU_HEADER_HEIGHT;
}

uint16_t flashback_screen_num_sections_callback(struct MenuLayer *menu_layer,
                                                void *context) {
  return 1;
}
