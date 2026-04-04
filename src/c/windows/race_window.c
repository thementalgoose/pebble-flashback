#include "race_window.h"
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

// Map a full event label to its shorthand code
static void abbreviate_event(const char *label, char *out, size_t out_size) {
  if (!label || !out || out_size < 4) return;

  if      (strstr(label, "Free Practice 1") || strstr(label, "Practice 1")) snprintf(out, out_size, "FP1");
  else if (strstr(label, "Free Practice 2") || strstr(label, "Practice 2")) snprintf(out, out_size, "FP2");
  else if (strstr(label, "Free Practice 3") || strstr(label, "Practice 3")) snprintf(out, out_size, "FP3");
  else if (strstr(label, "Sprint Qualifying") || strstr(label, "Sprint Shootout")) snprintf(out, out_size, "SQ");
  else if (strstr(label, "Sprint"))      snprintf(out, out_size, "SR");
  else if (strstr(label, "Qualifying"))  snprintf(out, out_size, "Q");
  else if (strstr(label, "Race"))        snprintf(out, out_size, "R");
  else {
    strncpy(out, label, 3);
    out[3] = '\0';
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
    char code[4];
    abbreviate_event(event->label, code, sizeof(code));
    GRect code_rect = GRect(H_INSET, 2, EVENT_CODE_WIDTH, bounds.size.h - 4);
    graphics_draw_text(ctx, code,
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

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer,
                                        MenuIndex *cell_index, void *context) {
  return MENU_CELL_HEIGHT;
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer,
                                 uint16_t section_index, void *context) {
  GRect bounds = layer_get_bounds(cell_layer);
  graphics_context_set_text_color(ctx, GColorBlack);

  GRect title_rect = GRect(HDR_INSET, 0, bounds.size.w - 2 * HDR_INSET, MENU_HEADER_DIVIDER_Y);
  graphics_draw_text(ctx, APP_TITLE,
                    MENU_HEADER_TITLE_FONT,
                    title_rect,
                    GTextOverflowModeTrailingEllipsis,
                    GTextAlignmentCenter,
                    NULL);

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_line(ctx, GPoint(HDR_INSET, MENU_HEADER_DIVIDER_Y), GPoint(bounds.size.w - HDR_INSET, MENU_HEADER_DIVIDER_Y));

  // Race name as subtitle, spanning full width
  GRect subtitle_rect = GRect(HDR_INSET, MENU_HEADER_SUBTITLE_Y, bounds.size.w - 2 * HDR_INSET, 14);
  graphics_draw_text(ctx, s_race_name,
                    MENU_HEADER_SUBTITLE_FONT,
                    subtitle_rect,
                    GTextOverflowModeTrailingEllipsis,
                    GTextAlignmentLeft,
                    NULL);
}

static int16_t get_header_height_callback(struct MenuLayer *menu_layer,
                                          uint16_t section_index,
                                          void *context) {
  return MENU_HEADER_HEIGHT;
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
                               .draw_header = draw_header_callback,
                               .get_header_height = get_header_height_callback,
                               .get_num_sections = get_num_sections_callback,
                           });

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

  // Register our custom inbox handler for the formatted text (only once)
  app_message_register_inbox_received(race_inbox_received);

  // Request race details if not loaded
  if (s_current_race_index >= 0 && !s_data_loaded) {
    // Set the legacy callbacks (for compatibility, though not used)
    message_handler_set_race_details_callbacks(on_event_data_received,
                                               on_event_count_received);

    // Request the data
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
