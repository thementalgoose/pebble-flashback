#include "race_window.h"
#include "flashback_screen.h"
#include "../data_models.h"
#include "../message_handler.h"
#include "../utils.h"
#include "../colors.h"
#include "../ui_constants.h"
#include <pebble.h>

#define MAX_EVENTS 10
// Width of the fixed event-code column (fits up to 3 chars e.g. "FP1")
#define EVENT_CODE_WIDTH 28

static Window *s_window;
static MenuLayer *s_menu_layer;

// Event data storage
static RaceEvent s_events[MAX_EVENTS];
static int s_event_count = 0;
static bool s_data_loaded = false;
static int s_current_race_index = -1;
static char s_race_name[64] = "Race Schedule";

// Parse pipe-delimited event data
static void parse_event_data(const char *data) {
  if (!data) {
    return;
  }

  // Reset event count
  s_event_count = 0;

  const char *ptr = data;
  char line_buffer[256];

  // Parse each line manually
  while (*ptr && s_event_count < MAX_EVENTS) {
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

    // Parse pipe-delimited fields: label|datetime
    char label[64] = {0};
    char datetime[64] = {0};

    // Find pipe delimiter (label|datetime)
    const char *pipe = strchr(line_buffer, '|');
    if (!pipe) continue;

    // Extract label
    size_t label_len = pipe - line_buffer;
    if (label_len >= sizeof(label)) label_len = sizeof(label) - 1;
    strncpy(label, line_buffer, label_len);
    label[label_len] = '\0';

    // Extract datetime (rest of the line)
    strncpy(datetime, pipe + 1, sizeof(datetime) - 1);
    datetime[sizeof(datetime) - 1] = '\0';

    // Store the data
    snprintf(s_events[s_event_count].label, sizeof(s_events[s_event_count].label), "%s", label);
    snprintf(s_events[s_event_count].datetime, sizeof(s_events[s_event_count].datetime), "%s", datetime);
    s_events[s_event_count].index = s_event_count;

    s_event_count++;
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "Parsed %d events from data", s_event_count);
  s_data_loaded = true;
}

// Callbacks from message handler
static void on_event_data_received(int index, const char *title,
                                   const char *subtitle, const char *extra) {
  // Legacy callback - not used with new format
}

static void on_event_count_received(int count) {
  // Legacy callback - not used with new format
}

// Custom inbox handler for race event text
static void race_inbox_received(DictionaryIterator *iterator, void *context) {
  Tuple *request_type_tuple = dict_find(iterator, MESSAGE_KEY_REQUEST_TYPE);
  if (!request_type_tuple) {
    return;
  }

  int request_type = request_type_tuple->value->int32;
  if (request_type != REQUEST_TYPE_GET_RACE_DETAILS) {
    return;
  }

  // Get the formatted event text
  Tuple *title_tuple = dict_find(iterator, MESSAGE_KEY_DATA_TITLE);
  if (!title_tuple) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "No title in race details message");
    return;
  }

  const char *events_text = title_tuple->value->cstring;
  APP_LOG(APP_LOG_LEVEL_INFO, "Received race events text (%d chars)", strlen(events_text));

  // Parse the pipe-delimited data
  parse_event_data(events_text);

  // Reload the menu
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
    GRect bounds = layer_get_bounds(cell_layer);
    bool selected = menu_layer_is_index_selected(s_menu_layer, cell_index);

    if (selected) {
      graphics_context_set_fill_color(ctx, HIGHLIGHT_BG);
      graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    }

    GColor text_color = selected ? TEXT_COLOR_SELECTED : TEXT_COLOR_UNSELECTED;
    graphics_context_set_text_color(ctx, text_color);

    // Event shorthand code in fixed-width left column
    GRect code_rect = GRect(H_INSET, 2, EVENT_CODE_WIDTH, bounds.size.h - 4);
    graphics_draw_text(ctx, event->label,
                      MENU_ROW_FONT,
                      code_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    // Compact date/time right-aligned
    char compact_time[12];
    utils_format_datetime_compact(event->datetime, compact_time, sizeof(compact_time));
    GRect time_rect = GRect(bounds.size.w - 80 - H_INSET, 2, 80, bounds.size.h - 4);
    graphics_draw_text(ctx, compact_time,
                      MENU_ROW_FONT,
                      time_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentRight,
                      NULL);
  } else {
    menu_cell_basic_draw(ctx, cell_layer, "No events", NULL, NULL);
  }
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer,
                                 uint16_t section_index, void *context) {
  flashback_screen_draw_header(ctx, cell_layer, s_race_name, NULL);
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
                               .draw_header = draw_header_callback,
                               .get_header_height = flashback_screen_header_height_callback,
                           });

  app_message_register_inbox_received(race_inbox_received);

  if (s_current_race_index >= 0 && !s_data_loaded) {
    message_handler_set_race_details_callbacks(on_event_data_received,
                                               on_event_count_received);
    message_handler_request_race_details(s_current_race_index);
  }
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  s_menu_layer = NULL;
}

void race_window_push(int race_index, const char *race_name) {
  // Check if this is a different race than the currently loaded one
  bool is_different_race = (race_index != s_current_race_index);

  s_current_race_index = race_index;

  // Only clear data if switching to a different race
  if (is_different_race) {
    s_data_loaded = false;
    s_event_count = 0;

    // Clear old event data to prevent showing stale data
    memset(s_events, 0, sizeof(s_events));

    // Store race name for header display
    if (race_name) {
      snprintf(s_race_name, sizeof(s_race_name), "%s", race_name);
    } else {
      snprintf(s_race_name, sizeof(s_race_name), "Race Schedule");
    }

    // If window is already loaded, reload menu and request new data
    if (s_menu_layer) {
      menu_layer_reload_data(s_menu_layer);
      // Request race details for the new race
      message_handler_set_race_details_callbacks(on_event_data_received,
                                                 on_event_count_received);
      message_handler_request_race_details(s_current_race_index);
    }
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
