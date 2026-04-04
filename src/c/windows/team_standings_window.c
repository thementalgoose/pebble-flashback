#include "team_standings_window.h"
#include "flashback_screen.h"
#include "../data_models.h"
#include "../message_handler.h"
#include "../colors.h"
#include "../ui_constants.h"
#include <pebble.h>

#define MAX_TEAMS 15

static Window *s_window;
static MenuLayer *s_menu_layer;
static char s_subtitle_text[32];

// Team standings data storage
static ConstructorStanding s_teams[MAX_TEAMS];
static int s_team_count = 0;
static bool s_data_loaded = false;

// Parse pipe-delimited team standings data
static void parse_standings_data(const char *data) {
  if (!data) {
    return;
  }

  // Reset team count
  s_team_count = 0;

  const char *ptr = data;
  char line_buffer[128];

  // Parse each line manually
  while (*ptr && s_team_count < MAX_TEAMS) {
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

    // Parse pipe-delimited fields: position|name|points
    char position_str[16] = {0};
    char name[64] = {0};
    char points_str[16] = {0};

    // Find first pipe (position|name)
    const char *pipe1 = strchr(line_buffer, '|');
    if (!pipe1) continue;

    // Find second pipe (name|points)
    const char *pipe2 = strchr(pipe1 + 1, '|');
    if (!pipe2) continue;

    // Extract position
    size_t pos_len = pipe1 - line_buffer;
    if (pos_len >= sizeof(position_str)) pos_len = sizeof(position_str) - 1;
    strncpy(position_str, line_buffer, pos_len);
    position_str[pos_len] = '\0';

    // Extract name
    size_t name_len = pipe2 - pipe1 - 1;
    if (name_len >= sizeof(name)) name_len = sizeof(name) - 1;
    strncpy(name, pipe1 + 1, name_len);
    name[name_len] = '\0';

    // Extract points (rest of the line)
    strncpy(points_str, pipe2 + 1, sizeof(points_str) - 1);
    points_str[sizeof(points_str) - 1] = '\0';

    // Store the data
    s_teams[s_team_count].position = atoi(position_str);
    s_teams[s_team_count].points = atoi(points_str);
    snprintf(s_teams[s_team_count].name, sizeof(s_teams[s_team_count].name), "%s", name);
    s_teams[s_team_count].index = s_team_count;

    s_team_count++;
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "Parsed %d teams from standings data", s_team_count);
  s_data_loaded = true;
}

// Callback from message handler
static void on_team_standings_received(int index, const char *name, int points,
                                       int position) {
  // Legacy callback - not used with new format
}

static void on_team_standings_complete(int count) {
  // Legacy callback - not used with new format
}

// Custom inbox handler for team standings text
static void team_standings_inbox_received(DictionaryIterator *iterator, void *context) {
  Tuple *request_type_tuple = dict_find(iterator, MESSAGE_KEY_REQUEST_TYPE);
  if (!request_type_tuple) {
    return;
  }

  int request_type = request_type_tuple->value->int32;
  if (request_type != REQUEST_TYPE_GET_TEAM_STANDINGS) {
    return;
  }

  // Get the formatted standings text
  Tuple *title_tuple = dict_find(iterator, MESSAGE_KEY_DATA_TITLE);
  if (!title_tuple) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "No title in team standings message");
    return;
  }

  const char *standings_text = title_tuple->value->cstring;
  APP_LOG(APP_LOG_LEVEL_INFO, "Received team standings text (%d chars)", strlen(standings_text));

  // Parse the pipe-delimited data
  parse_standings_data(standings_text);

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
  return s_team_count > 0 ? s_team_count : 1;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer,
                              MenuIndex *cell_index, void *context) {
  if (!s_data_loaded) {
    menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
    return;
  }

  if (cell_index->row < s_team_count) {
    ConstructorStanding *team = &s_teams[cell_index->row];

    GRect bounds = layer_get_bounds(cell_layer);

    bool selected = menu_layer_is_index_selected(s_menu_layer, cell_index);

    if (selected) {
      graphics_context_set_fill_color(ctx, HIGHLIGHT_BG);
      graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    }

    GColor text_color = selected ? TEXT_COLOR_SELECTED : TEXT_COLOR_UNSELECTED;
    graphics_context_set_text_color(ctx, text_color);

    // Draw position number in a fixed-width column (right-aligned so digits line up)
    char position_text[4];
    snprintf(position_text, sizeof(position_text), "%d", team->position);
    GRect pos_rect = GRect(H_INSET, 2, MENU_ROW_POS_WIDTH, bounds.size.h - 4);
    graphics_draw_text(ctx, position_text,
                      MENU_ROW_FONT,
                      pos_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    // Draw name starting at a fixed offset after the position column
    const int name_x = H_INSET + MENU_ROW_POS_WIDTH + MENU_ROW_POS_GAP;
    GRect text_rect = GRect(name_x, 2, bounds.size.w - name_x - 46, bounds.size.h - 4);
    graphics_draw_text(ctx, team->name,
                      MENU_ROW_FONT,
                      text_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    char points_text[16];
    snprintf(points_text, sizeof(points_text), "%d", team->points);
    GRect points_rect = GRect(bounds.size.w - 44 - H_INSET, 2, 42, bounds.size.h - 4);
    graphics_draw_text(ctx, points_text,
                      MENU_ROW_FONT,
                      points_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentRight,
                      NULL);
  } else {
    menu_cell_basic_draw(ctx, cell_layer, "No teams", NULL, NULL);
  }
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer,
                                 uint16_t section_index, void *context) {
  flashback_screen_draw_header(ctx, cell_layer, "Teams", s_subtitle_text);
}

// Window lifecycle
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

  snprintf(s_subtitle_text, sizeof(s_subtitle_text), "%d", g_current_season);

  if (!s_data_loaded) {
    message_handler_set_team_standings_callbacks(on_team_standings_received,
                                                 on_team_standings_complete);
    app_message_register_inbox_received(team_standings_inbox_received);
    message_handler_request_team_standings();
  }
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  s_menu_layer = NULL;
}

void team_standings_window_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
                                             .load = window_load,
                                             .unload = window_unload,
                                         });
  }

  window_stack_push(s_window, true);
}

void team_standings_window_destroy(void) {
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }

  // Clear data
  s_data_loaded = false;
  s_team_count = 0;
}
