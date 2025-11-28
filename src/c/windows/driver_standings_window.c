#include "driver_standings_window.h"
#include "../data_models.h"
#include "../message_handler.h"
#include <pebble.h>

#define MAX_DRIVERS 30

static Window *s_window;
static MenuLayer *s_menu_layer;
static char s_header_text[32];

// Driver standings data storage
static DriverStanding s_drivers[MAX_DRIVERS];
static int s_driver_count = 0;
static bool s_data_loaded = false;

// Callbacks from message handler
static void on_driver_data_received(int index, const char *name,
                                    const char *code, int points,
                                    int position) {
  if (index >= 0 && index < MAX_DRIVERS) {
    snprintf(s_drivers[index].name, sizeof(s_drivers[index].name), "%s", name);
    snprintf(s_drivers[index].code, sizeof(s_drivers[index].code), "%s", code);
    s_drivers[index].points = points;
    s_drivers[index].position = position;
    s_drivers[index].index = index;

    // Reload menu to show new driver data
    if (s_menu_layer) {
      menu_layer_reload_data(s_menu_layer);
    }
  }
}

static void on_driver_count_received(int count) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Expected %d drivers", count);
  s_driver_count = count;
  s_data_loaded = true;

  // Reload menu to update from "Loading..." state
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
}

// Menu layer callbacks
static uint16_t get_num_rows_callback(MenuLayer *menu_layer,
                                      uint16_t section_index, void *context) {
  if (!s_data_loaded) {
    return 1; // Show loading
  }
  return s_driver_count > 0 ? s_driver_count : 1;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer,
                              MenuIndex *cell_index, void *context) {
  if (!s_data_loaded) {
    menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
    return;
  }

  if (cell_index->row < s_driver_count) {
    DriverStanding *driver = &s_drivers[cell_index->row];

    // Get bounds and calculate positions
    GRect bounds = layer_get_bounds(cell_layer);

    // Check if this cell is selected to invert text color
    bool selected = menu_layer_is_index_selected(s_menu_layer, cell_index);
    GColor text_color = selected ? GColorWhite : GColorBlack;

    // Draw position number on the left
    char position_text[4];
    snprintf(position_text, sizeof(position_text), "%d", driver->position);

    GRect position_rect = GRect(4, 4, 28, bounds.size.h - 8);
    graphics_context_set_text_color(ctx, text_color);
    graphics_draw_text(ctx, position_text,
                      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                      position_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentCenter,
                      NULL);

    // Draw driver name and points with offset for position number
    const int text_offset_x = 36;
    GRect name_rect = GRect(text_offset_x, 2, bounds.size.w - text_offset_x - 4, 20);

    // Format points display
    char points_text[32];
    snprintf(points_text, sizeof(points_text), "%d pts", driver->points);
    GRect points_rect = GRect(text_offset_x, 22, bounds.size.w - text_offset_x - 4, 18);

    graphics_draw_text(ctx, driver->name,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                      name_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    graphics_draw_text(ctx, points_text,
                      fonts_get_system_font(FONT_KEY_GOTHIC_14),
                      points_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);
  } else {
    menu_cell_basic_draw(ctx, cell_layer, "No drivers", NULL, NULL);
  }
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer,
                                        MenuIndex *cell_index, void *context) {
  return 44;
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer,
                                 uint16_t section_index, void *context) {
  // Draw header centered on round displays
  GRect bounds = layer_get_bounds(cell_layer);
  GTextAlignment alignment = PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft);

  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, s_header_text,
                    fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                    GRect(PBL_IF_ROUND_ELSE(0, 5), 0, bounds.size.w - PBL_IF_ROUND_ELSE(0, 5), bounds.size.h),
                    GTextOverflowModeTrailingEllipsis,
                    alignment,
                    NULL);
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

  // Set callbacks
  menu_layer_set_callbacks(s_menu_layer, NULL,
                           (MenuLayerCallbacks){
                               .get_num_sections = get_num_sections_callback,
                               .get_num_rows = get_num_rows_callback,
                               .draw_row = draw_row_callback,
                               .draw_header = draw_header_callback,
                               .get_header_height = get_header_height_callback,
                               .get_cell_height = get_cell_height_callback,
                           });

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

  // Set header text
  snprintf(s_header_text, sizeof(s_header_text), "%d Driver Standings", g_current_season);

  // Request driver standings data if not loaded
  if (!s_data_loaded) {
    message_handler_set_driver_standings_callbacks(on_driver_data_received,
                                                   on_driver_count_received);
    message_handler_request_driver_standings();
  }
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
}

void driver_standings_window_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
                                             .load = window_load,
                                             .unload = window_unload,
                                         });
  }

  window_stack_push(s_window, true);
}

void driver_standings_window_destroy(void) {
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }

  // Clear data
  s_data_loaded = false;
  s_driver_count = 0;
}
