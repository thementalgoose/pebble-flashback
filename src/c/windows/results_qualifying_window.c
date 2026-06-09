#include "results_qualifying_window.h"
#include "flashback_screen.h"
#include "../data_models.h"
#include "../message_handler.h"
#include "../colors.h"
#include "../ui_constants.h"
#include <pebble.h>

#define MAX_RESULTS 30

static Window *s_window;
static MenuLayer *s_menu_layer;
static char s_subtitle_text[32];
static char s_race_name[32] = "R1 Qualifying";

// Qualifying results data storage
static QualifyingResult s_results[MAX_RESULTS];
static int s_result_count = 0;
static bool s_data_loaded = false;
static int s_current_race_round = 1;

// Helper to format driver name as "M.Verstapp." like driver standings
static void format_driver_name(const char *full_name, char *output, size_t output_size) {
  if (!full_name || !output || output_size < 4) return;

  const char *space = strchr(full_name, ' ');
  if (!space) {
    strncpy(output, full_name, output_size - 1);
    output[output_size - 1] = '\0';
    return;
  }

  char first_initial = full_name[0];
  const char *last_name = space + 1;
  size_t max_last_len = output_size - 4;
  size_t pos = 0;
  output[pos++] = first_initial;
  output[pos++] = '.';

  size_t copied = 0;
  while (*last_name && copied < max_last_len && pos < output_size - 2) {
    output[pos++] = *last_name++;
    copied++;
  }

  output[pos] = '\0';
}

// Parse pipe-delimited qualifying data
static void parse_results_data(const char *data) {
  if (!data) {
    return;
  }

  s_result_count = 0;

  const char *ptr = data;
  char line_buffer[256];

  while (*ptr && s_result_count < MAX_RESULTS) {
    size_t line_len = 0;
    while (*ptr && *ptr != '\n' && line_len < sizeof(line_buffer) - 1) {
      line_buffer[line_len++] = *ptr++;
    }
    line_buffer[line_len] = '\0';

    if (*ptr == '\n') {
      ptr++;
    }

    if (line_len == 0) {
      continue;
    }

    char position_str[16] = {0};
    char name[64] = {0};
    char time_str[32] = {0};

    const char *pipe1 = strchr(line_buffer, '|');
    if (!pipe1) continue;
    const char *pipe2 = strchr(pipe1 + 1, '|');
    if (!pipe2) continue;

    size_t pos_len = pipe1 - line_buffer;
    if (pos_len >= sizeof(position_str)) pos_len = sizeof(position_str) - 1;
    strncpy(position_str, line_buffer, pos_len);
    position_str[pos_len] = '\0';

    size_t name_len = pipe2 - pipe1 - 1;
    if (name_len >= sizeof(name)) name_len = sizeof(name) - 1;
    strncpy(name, pipe1 + 1, name_len);
    name[name_len] = '\0';

    strncpy(time_str, pipe2 + 1, sizeof(time_str) - 1);
    time_str[sizeof(time_str) - 1] = '\0';

    s_results[s_result_count].position = atoi(position_str);
    snprintf(s_results[s_result_count].name, sizeof(s_results[s_result_count].name), "%s", name);
    snprintf(s_results[s_result_count].time, sizeof(s_results[s_result_count].time), "%s", time_str);
    s_results[s_result_count].index = s_result_count;

    s_result_count++;
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "Parsed %d qualifying results", s_result_count);
  s_data_loaded = true;
}

static void qualifying_inbox_received(DictionaryIterator *iterator, void *context) {
  Tuple *request_type_tuple = dict_find(iterator, MESSAGE_KEY_REQUEST_TYPE);
  if (!request_type_tuple) {
    return;
  }

  int request_type = request_type_tuple->value->int32;
  if (request_type != REQUEST_TYPE_GET_QUALIFYING_RESULTS) {
    return;
  }

  Tuple *qualifying_tuple = dict_find(iterator, MESSAGE_KEY_DATA_QUALIFYING);
  if (!qualifying_tuple) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "No qualifying data in message");
    return;
  }

  const char *results_text = qualifying_tuple->value->cstring;
  APP_LOG(APP_LOG_LEVEL_INFO, "Received qualifying results text (%d chars)", (int)strlen(results_text));

  parse_results_data(results_text);

  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
}

static uint16_t get_num_rows_callback(MenuLayer *menu_layer,
                                      uint16_t section_index, void *context) {
  if (!s_data_loaded) {
    return 1;
  }
  return s_result_count > 0 ? s_result_count : 1;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer,
                              MenuIndex *cell_index, void *context) {
  if (!s_data_loaded) {
    menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
    return;
  }

  if (cell_index->row < s_result_count) {
    QualifyingResult *result = &s_results[cell_index->row];
    GRect bounds = layer_get_bounds(cell_layer);
    bool selected = menu_layer_is_index_selected(s_menu_layer, cell_index);

    if (selected) {
      graphics_context_set_fill_color(ctx, HIGHLIGHT_BG);
      graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    }

    GColor text_color = selected ? TEXT_COLOR_SELECTED : TEXT_COLOR_UNSELECTED;
    graphics_context_set_text_color(ctx, text_color);

    char position_text[4];
    snprintf(position_text, sizeof(position_text), "%d", result->position);
    GRect pos_rect = GRect(H_INSET, 2, MENU_ROW_POS_WIDTH, bounds.size.h - 4);
    graphics_draw_text(ctx, position_text,
                      RESULTS_QUALIFYING_WINDOW_ROW_FONT,
                      pos_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    char abbreviated_name[16];
    format_driver_name(result->name, abbreviated_name, sizeof(abbreviated_name));
    const int name_x = H_INSET + MENU_ROW_POS_WIDTH + MENU_ROW_POS_GAP;
    const int secondary_width = 50;
    GRect text_rect = GRect(name_x, 2, bounds.size.w - name_x - secondary_width - H_INSET, bounds.size.h - 4);
    graphics_draw_text(ctx, abbreviated_name,
                      RESULTS_QUALIFYING_WINDOW_ROW_FONT,
                      text_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    GRect time_rect = GRect(bounds.size.w - secondary_width - H_INSET, 4, secondary_width, bounds.size.h - 4);
    graphics_draw_text(ctx, result->time,
                      RESULTS_QUALIFYING_WINDOW_SECONDARY_FONT,
                      time_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentRight,
                      NULL);
  } else {
    menu_cell_basic_draw(ctx, cell_layer, "No qualifying results", NULL, NULL);
  }
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer,
                                 uint16_t section_index, void *context) {
  flashback_screen_draw_header(ctx, cell_layer, s_race_name, s_subtitle_text);
}

static void window_load(Window *window) {
  s_menu_layer = flashback_screen_create_menu_layer(window);

  menu_layer_set_callbacks(s_menu_layer, NULL,
                           (MenuLayerCallbacks){
                               .get_num_sections = flashback_screen_num_sections_callback,
                               .get_num_rows = get_num_rows_callback,
                               .draw_row = draw_row_callback,
                               .draw_header = draw_header_callback,
                               .get_header_height = flashback_screen_header_height_callback,
                               .get_cell_height = flashback_screen_cell_height_callback,
                           });

  app_message_register_inbox_received(qualifying_inbox_received);

  snprintf(s_subtitle_text, sizeof(s_subtitle_text), "%d", g_current_season);

  if (!s_data_loaded) {
    message_handler_request_qualifying_results(s_current_race_round);
  }
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  s_menu_layer = NULL;
  flashback_screen_destroy_header_background();
}

void results_qualifying_window_push(int race_round) {
  bool is_different_round = (race_round != s_current_race_round);
  s_current_race_round = race_round;
  snprintf(s_race_name, sizeof(s_race_name), "R%d Qualifying", s_current_race_round);
  snprintf(s_subtitle_text, sizeof(s_subtitle_text), "%d R%d", g_current_season, s_current_race_round);

  if (is_different_round) {
    s_data_loaded = false;
    s_result_count = 0;
    memset(s_results, 0, sizeof(s_results));

    if (s_menu_layer) {
      menu_layer_reload_data(s_menu_layer);
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

void results_qualifying_window_destroy(void) {
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }

  s_data_loaded = false;
  s_result_count = 0;
  s_current_race_round = 1;
}
