#include "calendar_window.h"
#include "../data_models.h"
#include "../message_handler.h"
#include "../utils.h"
#include "race_window.h"
#include <pebble.h>

#define MAX_UPCOMING_RACES 15
#define MAX_PAST_RACES 15

static Window *s_window;
static MenuLayer *s_menu_layer;
static char s_header_text[16];

// Race data storage
static Race s_upcoming_races[MAX_UPCOMING_RACES];
static Race s_past_races[MAX_PAST_RACES];
static int s_upcoming_count = 0;
static int s_past_count = 0;
static bool s_upcoming_loaded = false;
static bool s_past_loaded = false;

// Section indices
#define SECTION_UPCOMING 0
#define SECTION_PREVIOUS 1

// Parse pipe-delimited race data
static void parse_race_data(const char *data, Race *races, int *count, int max_count) {
  if (!data || !races || !count) {
    return;
  }

  // Reset count
  *count = 0;

  const char *ptr = data;
  char line_buffer[256];

  // Parse each line manually
  while (*ptr && *count < max_count) {
    // Extract one line
    size_t line_len = 0;
    while (*ptr && *ptr != '\n' && line_len < sizeof(line_buffer) - 1) {
      line_buffer[line_len++] = *ptr++;
    }
    line_buffer[line_len] = '\0';

    // Skip the newline
    if (*ptr == '\n') {
      ptr++;
    }

    // Skip empty lines
    if (line_len == 0) {
      continue;
    }

    // Parse pipe-delimited fields: round|name|location
    char round_str[16] = {0};
    char name[64] = {0};
    char location[64] = {0};

    // Find first pipe (round|name)
    const char *pipe1 = strchr(line_buffer, '|');
    if (!pipe1) continue;

    // Find second pipe (name|location)
    const char *pipe2 = strchr(pipe1 + 1, '|');
    if (!pipe2) continue;

    // Extract round
    size_t round_len = pipe1 - line_buffer;
    if (round_len >= sizeof(round_str)) round_len = sizeof(round_str) - 1;
    strncpy(round_str, line_buffer, round_len);
    round_str[round_len] = '\0';

    // Extract name
    size_t name_len = pipe2 - pipe1 - 1;
    if (name_len >= sizeof(name)) name_len = sizeof(name) - 1;
    strncpy(name, pipe1 + 1, name_len);
    name[name_len] = '\0';

    // Extract location (rest of the line)
    strncpy(location, pipe2 + 1, sizeof(location) - 1);
    location[sizeof(location) - 1] = '\0';

    // Store the data
    races[*count].round = atoi(round_str);
    snprintf(races[*count].name, sizeof(races[*count].name), "%s", name);
    snprintf(races[*count].location, sizeof(races[*count].location), "%s", location);
    races[*count].index = *count;
    races[*count].date[0] = '\0'; // Not used in new format

    (*count)++;
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "Parsed %d races from data", *count);
}

// Callback from message handler
static void on_race_data_received(int index, const char *title,
                                  const char *subtitle, const char *extra,
                                  int round) {
  // Legacy callback - not used with new format
}

static void on_race_count_received(int count) {
  // Legacy callback - not used with new format
}

// Custom inbox handler for race calendar text
static void calendar_inbox_received(DictionaryIterator *iterator, void *context) {
  Tuple *request_type_tuple = dict_find(iterator, MESSAGE_KEY_REQUEST_TYPE);
  if (!request_type_tuple) {
    return;
  }

  int request_type = request_type_tuple->value->int32;
  if (request_type != REQUEST_TYPE_GET_OVERVIEW) {
    return;
  }

  // Get the data index (0 = upcoming, 1 = past)
  Tuple *index_tuple = dict_find(iterator, MESSAGE_KEY_DATA_INDEX);
  if (!index_tuple) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "No index in calendar message");
    return;
  }

  int data_index = index_tuple->value->int32;

  // Get the formatted race text
  Tuple *title_tuple = dict_find(iterator, MESSAGE_KEY_DATA_TITLE);
  if (!title_tuple) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "No title in calendar message");
    return;
  }

  const char *race_text = title_tuple->value->cstring;
  APP_LOG(APP_LOG_LEVEL_INFO, "Received calendar data (index %d, %d chars)", data_index, strlen(race_text));

  // Parse the pipe-delimited data
  if (data_index == 0) {
    // Upcoming races
    parse_race_data(race_text, s_upcoming_races, &s_upcoming_count, MAX_UPCOMING_RACES);
    s_upcoming_loaded = true;
  } else if (data_index == 1) {
    // Past races
    parse_race_data(race_text, s_past_races, &s_past_count, MAX_PAST_RACES);
    s_past_loaded = true;
  }

  // Reload the menu
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
  if (section_index == SECTION_UPCOMING) {
    if (!s_upcoming_loaded) {
      return 1; // Show loading
    }
    return s_upcoming_count > 0 ? s_upcoming_count : 1;
  } else {
    if (!s_past_loaded) {
      return 1; // Show loading
    }
    return s_past_count > 0 ? s_past_count : 1;
  }
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer,
                                 uint16_t section_index, void *context) {
  const char *header_text = (section_index == SECTION_UPCOMING) ? "Upcoming Races" : "Previous Races";

  // Draw header centered on round displays
  GRect bounds = layer_get_bounds(cell_layer);
  GTextAlignment alignment = PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft);

  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, header_text,
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

static void draw_row_callback(GContext *ctx, const Layer *cell_layer,
                              MenuIndex *cell_index, void *context) {
  Race *race = NULL;

  if (cell_index->section == SECTION_UPCOMING) {
    if (!s_upcoming_loaded) {
      menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
      return;
    }
    if (cell_index->row < s_upcoming_count) {
      race = &s_upcoming_races[cell_index->row];
    }
  } else {
    if (!s_past_loaded) {
      menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
      return;
    }
    if (cell_index->row < s_past_count) {
      race = &s_past_races[cell_index->row];
    }
  }

  if (race) {
    // Get bounds and calculate positions
    GRect bounds = layer_get_bounds(cell_layer);

    // Check if this cell is selected to invert text color
    bool selected = menu_layer_is_index_selected(s_menu_layer, cell_index);

    // Draw custom color selection background on color displays
    if (selected) {
#ifdef PBL_COLOR
      graphics_context_set_fill_color(ctx, GColorFromHEX(0x489bb0));
      graphics_fill_rect(ctx, bounds, 0, GCornerNone);
#endif
    }

    GColor text_color = selected ? GColorWhite : GColorBlack;

    // Draw round number in icon position (left side)
    char round_text[4];
    snprintf(round_text, sizeof(round_text), "%d", race->round);

    GRect round_rect = GRect(4, 4, 28, bounds.size.h - 8);
    graphics_context_set_text_color(ctx, text_color);
    graphics_draw_text(ctx, round_text,
                      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                      round_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentCenter,
                      NULL);

    // Draw race name and location with offset for round number
    const int text_offset_x = 36;
    GRect title_rect = GRect(text_offset_x, 0, bounds.size.w - text_offset_x - 4, 22);
    GRect subtitle_rect = GRect(text_offset_x, 22, bounds.size.w - text_offset_x - 4, 20);

    graphics_draw_text(ctx, race->name,
                      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                      title_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    graphics_draw_text(ctx, race->location,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18),
                      subtitle_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);
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
  Race *race = NULL;

  if (cell_index->section == SECTION_UPCOMING) {
    if (cell_index->row < s_upcoming_count) {
      race = &s_upcoming_races[cell_index->row];
    }
  } else {
    if (cell_index->row < s_past_count) {
      race = &s_past_races[cell_index->row];
    }
  }

  if (race) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Selected race round: %d", race->round);
    race_window_push(race->round, race->name);
  }
}

// Window lifecycle
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create menu layer (full screen, no status bar)
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_click_config_onto_window(s_menu_layer, window);

  // Set custom color highlight on color displays
#ifdef PBL_COLOR
  menu_layer_set_highlight_colors(s_menu_layer, GColorFromHEX(0x489bb0), GColorWhite);
#endif

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
  if (!s_upcoming_loaded || !s_past_loaded) {
    // Set the legacy callbacks (for compatibility, though not used)
    message_handler_set_overview_callbacks(on_race_data_received,
                                           on_race_count_received);

    // Register our custom inbox handler for the formatted text
    app_message_register_inbox_received(calendar_inbox_received);

    // Request the data
    message_handler_request_overview();
  }
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  s_menu_layer = NULL;
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
  s_upcoming_loaded = false;
  s_past_loaded = false;
  s_upcoming_count = 0;
  s_past_count = 0;
}
