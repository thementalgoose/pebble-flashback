#include "flashback_screen.h"
#include "../colors.h"
#include "../ui_constants.h"
#include <pebble.h>

MenuLayer *flashback_screen_create_menu_layer(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  MenuLayer *menu_layer = menu_layer_create(bounds);
  menu_layer_set_click_config_onto_window(menu_layer, window);

#ifdef PBL_ROUND
  menu_layer_set_center_focused(menu_layer, true);
#endif

  menu_layer_set_highlight_colors(menu_layer, HIGHLIGHT_BG, TEXT_COLOR_SELECTED);
  layer_add_child(window_layer, menu_layer_get_layer(menu_layer));

  return menu_layer;
}

void flashback_screen_draw_header(GContext *ctx, const Layer *cell_layer,
                                  const char *subtitle_left,
                                  const char *subtitle_right) {
  GRect bounds = layer_get_bounds(cell_layer);
  graphics_context_set_text_color(ctx, GColorBlack);

  GRect title_rect = GRect(HDR_INSET, 0, bounds.size.w - 2 * HDR_INSET, MENU_HEADER_DIVIDER_Y);
  graphics_draw_text(ctx, APP_TITLE,
                    MENU_HEADER_TITLE_FONT,
                    title_rect,
                    GTextOverflowModeTrailingEllipsis,
                    GTextAlignmentCenter,
                    NULL);

  graphics_context_set_stroke_color(ctx, DIVIDER_COLOR);
  graphics_draw_line(ctx,
                    GPoint(HDR_INSET, MENU_HEADER_DIVIDER_Y),
                    GPoint(bounds.size.w - HDR_INSET, MENU_HEADER_DIVIDER_Y));

  if (subtitle_right) {
    GRect left_rect = GRect(HDR_INSET, MENU_HEADER_SUBTITLE_Y, 80, 14);
    graphics_draw_text(ctx, subtitle_left,
                      MENU_HEADER_SUBTITLE_FONT,
                      left_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    GRect right_rect = GRect(bounds.size.w - 80 - HDR_INSET, MENU_HEADER_SUBTITLE_Y, 76, 14);
    graphics_draw_text(ctx, subtitle_right,
                      MENU_HEADER_SUBTITLE_FONT,
                      right_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentRight,
                      NULL);
  } else {
    GRect full_rect = GRect(HDR_INSET, MENU_HEADER_SUBTITLE_Y,
                            bounds.size.w - 2 * HDR_INSET, 14);
    graphics_draw_text(ctx, subtitle_left,
                      MENU_HEADER_SUBTITLE_FONT,
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
