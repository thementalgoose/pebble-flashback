#include "home_window.h"
#include "flashback_screen.h"
#include "../colors.h"
#include "../ui_constants.h"
#include "../data_models.h"
#include "calendar_window.h"
#include "driver_standings_window.h"
#include "team_standings_window.h"
#include <pebble.h>

// Current season (will be set to current year)
int g_current_season = 2025;

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
                    MENU_ROW_FONT,
                    text_rect,
                    GTextOverflowModeTrailingEllipsis,
                    GTextAlignmentLeft,
                    NULL);
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
  flashback_screen_draw_header(ctx, cell_layer, "Season", s_subtitle_text);
}

// Window lifecycle
static void window_load(Window *window) {
  s_menu_layer = flashback_screen_create_menu_layer(window);

  menu_layer_set_callbacks(s_menu_layer, NULL,
                           (MenuLayerCallbacks){
                               .get_num_sections = flashback_screen_num_sections_callback,
                               .get_num_rows = get_num_rows_callback,
                               .draw_row = draw_row_callback,
                               .get_cell_height = flashback_screen_cell_height_callback,
                               .select_click = select_callback,
                               .draw_header = draw_header_callback,
                               .get_header_height = flashback_screen_header_height_callback,
                           });

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
