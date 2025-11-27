#include "race_window.h"
#include "../data_models.h"
#include "../message_handler.h"
#include "../utils.h"
#include <pebble.h>

#define MAX_EVENTS 10

static Window *s_window;
static MenuLayer *s_menu_layer;

// Event data storage
static RaceEvent s_events[MAX_EVENTS];
static int s_event_count = 0;
static bool s_data_loaded = false;
static int s_current_race_index = -1;
static char s_race_name[64] = "Race Schedule";

// Callbacks from message handler
static void on_event_data_received(int index, const char *title,
                                   const char *subtitle, const char *extra) {
  if (index >= 0 && index < MAX_EVENTS) {
    snprintf(s_events[index].label, sizeof(s_events[index].label), "%s", title);
    snprintf(s_events[index].datetime, sizeof(s_events[index].datetime), "%s",
             subtitle);
    s_events[index].index = index;

    // Reload menu to show new event data
    if (s_menu_layer) {
      menu_layer_reload_data(s_menu_layer);
    }
  }
}

static void on_event_count_received(int count) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Expected %d events", count);
  s_event_count = count;
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
  return s_event_count > 0 ? s_event_count : 1;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer,
                              MenuIndex *cell_index, void *context) {
  if (!s_data_loaded) {
    menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
    return;
  }

  if (cell_index->row < s_event_count) {
    RaceEvent *event = &s_events[cell_index->row];

    // Format datetime for display
    char formatted_time[64];
    utils_format_datetime(event->datetime, formatted_time,
                          sizeof(formatted_time));

    menu_cell_basic_draw(ctx, cell_layer, event->label, formatted_time, NULL);
  } else {
    menu_cell_basic_draw(ctx, cell_layer, "No events", NULL, NULL);
  }
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer,
                                        MenuIndex *cell_index, void *context) {
  return 44;
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer,
                                 uint16_t section_index, void *context) {
  menu_cell_basic_header_draw(ctx, cell_layer, s_race_name);
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
                               .get_num_rows = get_num_rows_callback,
                               .draw_row = draw_row_callback,
                               .get_cell_height = get_cell_height_callback,
                               .draw_header = draw_header_callback,
                               .get_header_height = get_header_height_callback,
                               .get_num_sections = get_num_sections_callback,
                           });

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

  // Request race details
  if (s_current_race_index >= 0) {
    message_handler_set_race_details_callbacks(on_event_data_received,
                                               on_event_count_received);
    message_handler_request_race_details(s_current_race_index);
  }
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
}

void race_window_push(int race_index, const char *race_name) {
  s_current_race_index = race_index;
  s_data_loaded = false;
  s_event_count = 0;

  // Store race name for header display
  if (race_name) {
    snprintf(s_race_name, sizeof(s_race_name), "%s", race_name);
  } else {
    snprintf(s_race_name, sizeof(s_race_name), "Race Schedule");
  }

  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
                                             .load = window_load,
                                             .unload = window_unload,
                                         });
  }

  window_stack_push(s_window, true);
}

void race_window_destroy(void) {
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }

  s_data_loaded = false;
  s_event_count = 0;
  s_current_race_index = -1;
}
