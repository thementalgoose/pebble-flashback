#include "home_window.h"
#include "../data_models.h"
#include "calendar_window.h"
#include <pebble.h>

// Current season (will be set to current year)
int g_current_season = 2025;

static Window *s_window;
static MenuLayer *s_menu_layer;
static StatusBarLayer *s_status_bar;
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
  switch (cell_index->row) {
  case MENU_ITEM_CALENDAR:
    menu_cell_basic_draw(ctx, cell_layer, "Calendar", NULL, NULL);
    break;
  case MENU_ITEM_DRIVER_STANDINGS:
    menu_cell_basic_draw(ctx, cell_layer, "Driver Standings", NULL, NULL);
    break;
  case MENU_ITEM_TEAM_STANDINGS:
    menu_cell_basic_draw(ctx, cell_layer, "Team Standings", NULL, NULL);
    break;
  }
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
    APP_LOG(APP_LOG_LEVEL_INFO, "Driver Standings selected (not implemented)");
    // TODO: Push driver standings window
    break;
  case MENU_ITEM_TEAM_STANDINGS:
    APP_LOG(APP_LOG_LEVEL_INFO, "Team Standings selected (not implemented)");
    // TODO: Push team standings window
    break;
  }
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer,
                                 uint16_t section_index, void *context) {
  // Draw season header
  menu_cell_basic_header_draw(ctx, cell_layer, s_season_text);
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

  // Create status bar
  s_status_bar = status_bar_layer_create();
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));

  // Calculate menu layer bounds (below status bar)
  GRect menu_bounds = bounds;
  menu_bounds.origin.y = STATUS_BAR_LAYER_HEIGHT;
  menu_bounds.size.h -= STATUS_BAR_LAYER_HEIGHT;

  // Create menu layer
  s_menu_layer = menu_layer_create(menu_bounds);
  menu_layer_set_click_config_onto_window(s_menu_layer, window);

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
  status_bar_layer_destroy(s_status_bar);
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
