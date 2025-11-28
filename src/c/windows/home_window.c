#include "home_window.h"
#include "../colors.h"
#include "../data_models.h"
#include "calendar_window.h"
#include "driver_standings_window.h"
#include "team_standings_window.h"
#include <pebble.h>

// Current season (will be set to current year)
int g_current_season = 2025;

static Window *s_window;
static MenuLayer *s_menu_layer;
static char s_season_text[16];

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
  // Get the icon and title for this row
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

  // Use Pebble's built-in drawing which handles icon inversion automatically
  menu_cell_basic_draw(ctx, cell_layer, title, NULL, NULL);
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer,
                                        MenuIndex *cell_index, void *context) {
  return 44; // Taller cells for easier touch
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
  // Draw season header centered on round displays
  GRect bounds = layer_get_bounds(cell_layer);
  GTextAlignment alignment =
      PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft);

  graphics_context_set_text_color(ctx, TEXT_COLOR_UNSELECTED);
  graphics_draw_text(
      ctx, s_season_text, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
      GRect(PBL_IF_ROUND_ELSE(0, 5), 0, bounds.size.w - PBL_IF_ROUND_ELSE(0, 5),
            bounds.size.h),
      GTextOverflowModeTrailingEllipsis, alignment, NULL);
}

static int16_t get_header_height_callback(struct MenuLayer *menu_layer,
                                          uint16_t section_index,
                                          void *context) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
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

  // Set custom color highlight on color displays
  menu_layer_set_highlight_colors(s_menu_layer, HIGHLIGHT_BG, GColorWhite);

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

  // Set season text
  snprintf(s_season_text, sizeof(s_season_text), "%d Season", g_current_season);
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
