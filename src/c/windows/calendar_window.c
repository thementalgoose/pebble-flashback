#include "calendar_window.h"
#include "../data_models.h"
#include "../message_handler.h"
#include "../utils.h"
#include "race_window.h"
#include <pebble.h>

#define MAX_RACES 30

static Window *s_window;
static MenuLayer *s_menu_layer;
static char s_header_text[16];

// Race data storage
static Race s_races[MAX_RACES];
static int s_race_count = 0;
static int s_upcoming_count = 0;
static int s_previous_count = 0;
static bool s_data_loaded = false;

// Section indices
#define SECTION_UPCOMING 0
#define SECTION_PREVIOUS 1

// Callbacks from message handler
static void on_race_data_received(int index, const char *title,
                                  const char *subtitle, const char *extra) {
  if (index >= 0 && index < MAX_RACES) {
    snprintf(s_races[index].name, sizeof(s_races[index].name), "%s", title);
    snprintf(s_races[index].location, sizeof(s_races[index].location), "%s",
             subtitle);
    snprintf(s_races[index].date, sizeof(s_races[index].date), "%s", extra);
    s_races[index].index = index;

    // Update section counts based on date
    int comparison = utils_compare_date_with_now(extra);
    if (comparison >= 0) {
      s_upcoming_count++;
    } else {
      s_previous_count++;
    }

    // Reload menu to show new race data
    if (s_menu_layer) {
      menu_layer_reload_data(s_menu_layer);
    }
  }
}

static void on_race_count_received(int count) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Expected %d races", count);
  s_race_count = count;
  s_upcoming_count = 0;
  s_previous_count = 0;
  s_data_loaded = true;

  // Reload menu to update from "Loading..." state
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
}

// Menu layer callbacks
static uint16_t get_num_sections_callback(struct MenuLayer *menu_layer,
                                          void *context) {
  return 2; // Upcoming and Previous
}

static uint16_t get_num_rows_callback(MenuLayer *menu_layer,
                                      uint16_t section_index, void *context) {
  if (!s_data_loaded) {
    return 1; // Show loading
  }

  if (section_index == SECTION_UPCOMING) {
    return s_upcoming_count > 0 ? s_upcoming_count : 1;
  } else {
    return s_previous_count > 0 ? s_previous_count : 1;
  }
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer,
                                 uint16_t section_index, void *context) {
  if (section_index == SECTION_UPCOMING) {
    menu_cell_basic_header_draw(ctx, cell_layer, "Upcoming Races");
  } else {
    menu_cell_basic_header_draw(ctx, cell_layer, "Previous Races");
  }
}

static int16_t get_header_height_callback(struct MenuLayer *menu_layer,
                                          uint16_t section_index,
                                          void *context) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer,
                              MenuIndex *cell_index, void *context) {
  if (!s_data_loaded) {
    menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
    return;
  }

  Race *race = NULL;

  if (cell_index->section == SECTION_UPCOMING) {
    // For upcoming races, iterate backwards to show chronologically (soonest first)
    // Since races arrive in reverse order, iterating backwards gives us chronological order
    int upcoming_index = 0;
    for (int i = s_race_count - 1; i >= 0; i--) {
      int comparison = utils_compare_date_with_now(s_races[i].date);
      if (comparison >= 0) {
        if (upcoming_index == cell_index->row) {
          race = &s_races[i];
          break;
        }
        upcoming_index++;
      }
    }
  } else {
    // For previous races, iterate forwards to show reverse chronologically (most recent first)
    // Since races arrive in reverse order, iterating forwards gives us reverse chronological order
    int previous_index = 0;
    for (int i = 0; i < s_race_count; i++) {
      int comparison = utils_compare_date_with_now(s_races[i].date);
      if (comparison < 0) {
        if (previous_index == cell_index->row) {
          race = &s_races[i];
          break;
        }
        previous_index++;
      }
    }
  }

  if (race) {
    menu_cell_basic_draw(ctx, cell_layer, race->name, race->location, NULL);
  } else {
    menu_cell_basic_draw(ctx, cell_layer, "No races", NULL, NULL);
  }
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer,
                                        MenuIndex *cell_index, void *context) {
  return 44;
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index,
                            void *context) {
  if (!s_data_loaded) {
    return;
  }

  int selected_index = -1;

  if (cell_index->section == SECTION_UPCOMING) {
    // For upcoming races, iterate backwards to match display order (chronological)
    int upcoming_index = 0;
    for (int i = s_race_count - 1; i >= 0; i--) {
      int comparison = utils_compare_date_with_now(s_races[i].date);
      if (comparison >= 0) {
        if (upcoming_index == cell_index->row) {
          selected_index = i;
          break;
        }
        upcoming_index++;
      }
    }
  } else {
    // For previous races, iterate forwards to match display order (reverse chronological)
    int previous_index = 0;
    for (int i = 0; i < s_race_count; i++) {
      int comparison = utils_compare_date_with_now(s_races[i].date);
      if (comparison < 0) {
        if (previous_index == cell_index->row) {
          selected_index = i;
          break;
        }
        previous_index++;
      }
    }
  }

  if (selected_index >= 0) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Selected race index: %d", selected_index);
    race_window_push(selected_index, s_races[selected_index].name);
  }
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
                               .select_click = select_callback,
                           });

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

  // Request overview data if not loaded
  if (!s_data_loaded) {
    message_handler_set_overview_callbacks(on_race_data_received,
                                           on_race_count_received);
    message_handler_request_overview();
  }
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
}

static void window_appear(Window *window) {
  // Update header
  extern int g_current_season;
  snprintf(s_header_text, sizeof(s_header_text), "%d Season", g_current_season);
}

void calendar_window_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
                                             .load = window_load,
                                             .unload = window_unload,
                                             .appear = window_appear,
                                         });
  }

  window_stack_push(s_window, true);
}

void calendar_window_destroy(void) {
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }

  // Clear data
  s_data_loaded = false;
  s_race_count = 0;
  s_upcoming_count = 0;
  s_previous_count = 0;
}
