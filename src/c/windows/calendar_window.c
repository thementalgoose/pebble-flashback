#include "calendar_window.h"
#include "flashback_screen.h"
#include "../colors.h"
#include "../ui_constants.h"
#include "../data_models.h"
#include "../message_handler.h"
#include "../utils.h"
#include "race_window.h"
#include <pebble.h>

#define MAX_RACES 32

static Window *s_window;
static MenuLayer *s_menu_layer;
static char s_subtitle_text[16];

// Race data storage
static Race s_races[MAX_RACES];
static int s_race_count = 0;
static int s_selected_row = -1;
static bool s_data_loaded = false;

static void update_initial_selection(void);

static bool parse_iso_date(const char *iso_date, int *year, int *month, int *day) {
  if (!iso_date || !year || !month || !day) {
    return false;
  }

  if (strlen(iso_date) < 10) {
    return false;
  }

  char year_str[5] = {0};
  char month_str[3] = {0};
  char day_str[3] = {0};

  strncpy(year_str, iso_date, 4);
  strncpy(month_str, iso_date + 5, 2);
  strncpy(day_str, iso_date + 8, 2);

  *year = atoi(year_str);
  *month = atoi(month_str);
  *day = atoi(day_str);

  return *year > 0 && *month > 0 && *day > 0;
}

static bool iso_date_to_key(const char *iso_date, int *key) {
  int year = 0;
  int month = 0;
  int day = 0;

  if (!parse_iso_date(iso_date, &year, &month, &day) || !key) {
    return false;
  }

  *key = year * 10000 + month * 100 + day;
  return true;
}

static bool is_race_today_or_future(const char *race_date, const char *today_date) {
  int race_key = 0;
  int today_key = 0;

  if (!iso_date_to_key(race_date, &race_key) ||
      !iso_date_to_key(today_date, &today_key)) {
    return false;
  }

  return race_key >= today_key;
}

static void get_today_date(char *output, size_t output_size) {
  if (!output || output_size < 11) {
    return;
  }

  time_t now = time(NULL);
  struct tm *local_tm = localtime(&now);
  if (!local_tm) {
    output[0] = '\0';
    return;
  }

  int year = local_tm->tm_year + 1900;
  int month = local_tm->tm_mon + 1;
  int day = local_tm->tm_mday;

  output[0] = '0' + ((year / 1000) % 10);
  output[1] = '0' + ((year / 100) % 10);
  output[2] = '0' + ((year / 10) % 10);
  output[3] = '0' + (year % 10);
  output[4] = '-';
  output[5] = '0' + (month / 10);
  output[6] = '0' + (month % 10);
  output[7] = '-';
  output[8] = '0' + (day / 10);
  output[9] = '0' + (day % 10);
  output[10] = '\0';
}

static int find_upcoming_race_index(void) {
  char today_date[11] = {0};
  get_today_date(today_date, sizeof(today_date));
  if (!today_date[0]) {
    return s_race_count > 0 ? 0 : -1;
  }

  for (int i = 0; i < s_race_count; i++) {
    if (is_race_today_or_future(s_races[i].date, today_date)) {
      return i;
    }
  }

  return s_race_count > 0 ? s_race_count - 1 : -1;
}

static void update_initial_selection(void) {
  if (!s_menu_layer || !s_data_loaded || s_race_count <= 0) {
    return;
  }

  // if (s_selected_row < 0 || s_selected_row >= s_race_count) {
  //   s_selected_row = 0;
  // }

  // MenuIndex index = {
  //     .section = 0,
  //     .row = (uint16_t)s_selected_row,
  // };
  // menu_layer_set_selected_index(s_menu_layer, index, MenuRowAlignCenter, false);
}

// Parse pipe-delimited race data
static void parse_race_data(const char *data) {
  if (!data) {
    return;
  }

  // Reset count
  s_race_count = 0;
  s_selected_row = -1;

  const char *ptr = data;
  char line_buffer[256];

  // Parse each line manually
  while (*ptr && s_race_count < MAX_RACES) {
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

    // Parse pipe-delimited fields: round|name|location|date
    char round_str[16] = {0};
    char name[64] = {0};
    char location[64] = {0};
    char date[MAX_EXTRA_LENGTH] = {0};

    // Find first pipe (round|name)
    const char *pipe1 = strchr(line_buffer, '|');
    if (!pipe1) continue;

    // Find second pipe (name|location)
    const char *pipe2 = strchr(pipe1 + 1, '|');
    if (!pipe2) continue;

    // Find third pipe (location|date)
    const char *pipe3 = strchr(pipe2 + 1, '|');
    if (!pipe3) continue;

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

    // Extract location
    size_t location_len = pipe3 - pipe2 - 1;
    if (location_len >= sizeof(location)) location_len = sizeof(location) - 1;
    strncpy(location, pipe2 + 1, location_len);
    location[location_len] = '\0';

    // Extract date (rest of the line)
    strncpy(date, pipe3 + 1, sizeof(date) - 1);
    date[sizeof(date) - 1] = '\0';

    // Store the data
    s_races[s_race_count].round = atoi(round_str);
    snprintf(s_races[s_race_count].name, sizeof(s_races[s_race_count].name), "%s", name);
    snprintf(s_races[s_race_count].location, sizeof(s_races[s_race_count].location), "%s", location);
    snprintf(s_races[s_race_count].date, sizeof(s_races[s_race_count].date), "%s", date);
    s_races[s_race_count].index = s_race_count;

    s_race_count++;
  }

  // s_selected_row = find_upcoming_race_index();
  s_data_loaded = true;
  APP_LOG(APP_LOG_LEVEL_INFO, "Parsed %d races from data", s_race_count);
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

  // Get the formatted race text
  Tuple *title_tuple = dict_find(iterator, MESSAGE_KEY_DATA_TITLE);
  if (!title_tuple) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "No title in calendar message");
    return;
  }

  const char *race_text = title_tuple->value->cstring;
  APP_LOG(APP_LOG_LEVEL_INFO, "Received calendar data (%d chars)", strlen(race_text));

  // Parse the pipe-delimited data
  parse_race_data(race_text);

  // Reload the menu
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
    update_initial_selection();
  }
}

// Menu layer callbacks
static uint16_t get_num_sections_callback(struct MenuLayer *menu_layer,
                                          void *context) {
  return 1;
}

static uint16_t get_num_rows_callback(MenuLayer *menu_layer,
                                      uint16_t section_index, void *context) {
  if (!s_data_loaded) {
    return 1; // Show loading
  }
  return s_race_count > 0 ? s_race_count : 1;
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer,
                                 uint16_t section_index, void *context) {
  flashback_screen_draw_header(ctx, cell_layer, "Calendar", s_subtitle_text);
}

static int16_t get_header_height_callback(struct MenuLayer *menu_layer,
                                          uint16_t section_index,
                                          void *context) {
  return MENU_HEADER_HEIGHT;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer,
                              MenuIndex *cell_index, void *context) {
  Race *race = NULL;

  if (!s_data_loaded) {
    menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
    return;
  }

  if (cell_index->row < s_race_count) {
    race = &s_races[cell_index->row];
  }

  if (race) {
    GRect bounds = layer_get_bounds(cell_layer);
    bool selected = menu_layer_is_index_selected(s_menu_layer, cell_index);

    if (selected) {
      graphics_context_set_fill_color(ctx, HIGHLIGHT_BG);
      graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    }

    GColor text_color = selected ? TEXT_COLOR_SELECTED : TEXT_COLOR_UNSELECTED;
    graphics_context_set_text_color(ctx, text_color);

    // Round number in fixed-width left column
    char round_text[4];
    snprintf(round_text, sizeof(round_text), "%d", race->round);
    GRect round_rect = GRect(H_INSET, 2, MENU_ROW_POS_WIDTH, bounds.size.h - 4);
    graphics_draw_text(ctx, round_text,
                      CALENDAR_WINDOW_ROW_FONT,
                      round_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    // Race name starting at fixed offset after round column
    const int name_x = H_INSET + MENU_ROW_POS_WIDTH + MENU_ROW_POS_GAP;
    GRect name_rect = GRect(name_x, 2, bounds.size.w - name_x - H_INSET, bounds.size.h - 4);
    graphics_draw_text(ctx, race->name,
                      CALENDAR_WINDOW_ROW_FONT,
                      name_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);
  } else {
    menu_cell_basic_draw(ctx, cell_layer, "No races", NULL, NULL);
  }
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index,
                            void *context) {
  Race *race = NULL;

  if (cell_index->row < s_race_count) {
    race = &s_races[cell_index->row];
  }

  if (race) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Selected race round: %d", race->round);
    race_window_push(race->round, race->name);
  }
}

// Window lifecycle
static void window_load(Window *window) {
  s_menu_layer = flashback_screen_create_menu_layer(window);

  menu_layer_set_callbacks(s_menu_layer, NULL,
                           (MenuLayerCallbacks){
                               .get_num_sections = get_num_sections_callback,
                               .get_num_rows = get_num_rows_callback,
                               .draw_row = draw_row_callback,
                               .draw_header = draw_header_callback,
                               .get_header_height = get_header_height_callback,
                               .get_cell_height = flashback_screen_cell_height_callback,
                               .select_click = select_callback,
                           });

  if (!s_data_loaded) {
    message_handler_set_overview_callbacks(on_race_data_received,
                                           on_race_count_received);
    app_message_register_inbox_received(calendar_inbox_received);
    message_handler_request_overview();
  } else {
    menu_layer_reload_data(s_menu_layer);
    update_initial_selection();
  }
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  s_menu_layer = NULL;
  flashback_screen_destroy_header_background();
}

static void window_appear(Window *window) {
  snprintf(s_subtitle_text, sizeof(s_subtitle_text), "%d", g_current_season);
  update_initial_selection();
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
  s_selected_row = -1;
}
