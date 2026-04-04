#include "home_window.h"
#include "../colors.h"
#include "../data_models.h"
#include "calendar_window.h"
#include "driver_standings_window.h"
#include "team_standings_window.h"
#include <pebble.h>

// Current season (will be set to current year)
int g_current_season = 2025;

#ifdef PBL_ROUND
  #define H_INSET 16
  #define HDR_INSET 22
#else
  #define H_INSET 4
  #define HDR_INSET 4
#endif

static Window *s_window;
static MenuLayer *s_menu_layer;
static char s_subtitle_text[16];

// Menu items
#define MENU_ITEM_CALENDAR 0
#define MENU_ITEM_DRIVER_STANDINGS 1
#define MENU_ITEM_TEAM_STANDINGS 2
#define NUM_MENU_ITEMS 3

// Menu layer callbacks
static uint16_t get_num_rows_callback(MenuLayer *menu_layer,
                                      uint16_t section_index, void *context) {
  return NUM_MENU_ITEMS;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer,
                              MenuIndex *cell_index, void *context) {
  const char *title = NULL;

  switch (cell_index->row) {
  case MENU_ITEM_CALENDAR:
    title = "Calendar";
    break;
  case MENU_ITEM_DRIVER_STANDINGS:
    title = "Driver Standings";
    break;
  case MENU_ITEM_TEAM_STANDINGS:
    title = "Team Standings";
    break;
  }

  GRect bounds = layer_get_bounds(cell_layer);
  bool selected = menu_layer_is_index_selected(s_menu_layer, cell_index);

  if (selected) {
    graphics_context_set_fill_color(ctx, HIGHLIGHT_BG);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  }

  graphics_context_set_text_color(ctx, selected ? TEXT_COLOR_SELECTED : TEXT_COLOR_UNSELECTED);
  GRect text_rect = GRect(H_INSET, 2, bounds.size.w - 2 * H_INSET, bounds.size.h - 4);
  graphics_draw_text(ctx, title,
                    fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                    text_rect,
                    GTextOverflowModeTrailingEllipsis,
                    GTextAlignmentLeft,
                    NULL);
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer,
                                        MenuIndex *cell_index, void *context) {
  return 28;
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index,
                            void *context) {
  switch (cell_index->row) {
  case MENU_ITEM_CALENDAR:
    APP_LOG(APP_LOG_LEVEL_INFO, "Calendar selected");
    calendar_window_push();
    break;
  case MENU_ITEM_DRIVER_STANDINGS:
    APP_LOG(APP_LOG_LEVEL_INFO, "Driver Standings selected");
    driver_standings_window_push();
    break;
  case MENU_ITEM_TEAM_STANDINGS:
    APP_LOG(APP_LOG_LEVEL_INFO, "Team Standings selected");
    team_standings_window_push();
    break;
  }
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer,
                                 uint16_t section_index, void *context) {
  GRect bounds = layer_get_bounds(cell_layer);
  graphics_context_set_text_color(ctx, GColorBlack);

  GRect title_rect = GRect(HDR_INSET, 0, bounds.size.w - 2 * HDR_INSET, 20);
  graphics_draw_text(ctx, "Flashback",
                    fonts_get_system_font(FONT_KEY_GOTHIC_14),
                    title_rect,
                    GTextOverflowModeTrailingEllipsis,
                    GTextAlignmentCenter,
                    NULL);

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_line(ctx, GPoint(HDR_INSET, 20), GPoint(bounds.size.w - HDR_INSET, 20));

  GRect subtitle_left = GRect(HDR_INSET, 22, 80, 14);
  graphics_draw_text(ctx, "Season",
                    fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                    subtitle_left,
                    GTextOverflowModeTrailingEllipsis,
                    GTextAlignmentLeft,
                    NULL);

  GRect subtitle_right = GRect(bounds.size.w - 80 - HDR_INSET, 22, 76, 14);
  graphics_draw_text(ctx, s_subtitle_text,
                    fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                    subtitle_right,
                    GTextOverflowModeTrailingEllipsis,
                    GTextAlignmentRight,
                    NULL);
}

static int16_t get_header_height_callback(struct MenuLayer *menu_layer,
                                          uint16_t section_index,
                                          void *context) {
  return 42;
}

static uint16_t get_num_sections_callback(struct MenuLayer *menu_layer,
                                          void *context) {
  return 1;
}

// Window lifecycle
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create menu layer (full screen, no status bar)
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_click_config_onto_window(s_menu_layer, window);

#ifdef PBL_ROUND
  menu_layer_set_center_focused(s_menu_layer, true);
#endif

  menu_layer_set_highlight_colors(s_menu_layer, HIGHLIGHT_BG, TEXT_COLOR_SELECTED);

  // Set callbacks
  menu_layer_set_callbacks(s_menu_layer, NULL,
                           (MenuLayerCallbacks){
                               .get_num_rows = get_num_rows_callback,
                               .draw_row = draw_row_callback,
                               .get_cell_height = get_cell_height_callback,
                               .select_click = select_callback,
                               .draw_header = draw_header_callback,
                               .get_header_height = get_header_height_callback,
                               .get_num_sections = get_num_sections_callback,
                           });

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

  snprintf(s_subtitle_text, sizeof(s_subtitle_text), "%d", g_current_season);
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
}

void home_window_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
                                             .load = window_load,
                                             .unload = window_unload,
                                         });
  }

  window_stack_push(s_window, true);
}

void home_window_destroy(void) {
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }
}
