#include "team_standings_window.h"
#include "../data_models.h"
#include "../message_handler.h"
#include "../colors.h"
#include <pebble.h>

#define MAX_TEAMS 15

#ifdef PBL_ROUND
  #define H_INSET 16
  #define HDR_INSET 22
#else
  #define H_INSET 4
  #define HDR_INSET 4
#endif

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

    // "1  TeamName" (extra space for single-digit alignment)
    char row_text[64];
    if (team->position < 10) {
      snprintf(row_text, sizeof(row_text), "%d  %s", team->position, team->name);
    } else {
      snprintf(row_text, sizeof(row_text), "%d %s", team->position, team->name);
    }

    GRect text_rect = GRect(H_INSET, 2, bounds.size.w - H_INSET - 46, bounds.size.h - 4);
    graphics_draw_text(ctx, row_text,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                      text_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    char points_text[16];
    snprintf(points_text, sizeof(points_text), "%d", team->points);
    GRect points_rect = GRect(bounds.size.w - 44 - H_INSET, 2, 42, bounds.size.h - 4);
    graphics_draw_text(ctx, points_text,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                      points_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentRight,
                      NULL);
  } else {
    menu_cell_basic_draw(ctx, cell_layer, "No teams", NULL, NULL);
  }
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer,
                                        MenuIndex *cell_index, void *context) {
  return 28;
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer,
                                 uint16_t section_index, void *context) {
  GRect bounds = layer_get_bounds(cell_layer);
  graphics_context_set_text_color(ctx, GColorBlack);

  GRect title_rect = GRect(HDR_INSET, 0, bounds.size.w - 2 * HDR_INSET, 20);
  graphics_draw_text(ctx, "Flashback",
                    fonts_get_system_font(FONT_KEY_GOTHIC_14),
                    title_rect,
                    GTextOverflowModeTrailingEllipsis,
                    GTextAlignmentCenter,
                    NULL);

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_line(ctx, GPoint(HDR_INSET, 20), GPoint(bounds.size.w - HDR_INSET, 20));

  GRect subtitle_left = GRect(HDR_INSET, 22, 80, 14);
  graphics_draw_text(ctx, "Teams",
                    fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                    subtitle_left,
                    GTextOverflowModeTrailingEllipsis,
                    GTextAlignmentLeft,
                    NULL);

  GRect subtitle_right = GRect(bounds.size.w - 80 - HDR_INSET, 22, 76, 14);
  graphics_draw_text(ctx, s_subtitle_text,
                    fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                    subtitle_right,
                    GTextOverflowModeTrailingEllipsis,
                    GTextAlignmentRight,
                    NULL);
}

static int16_t get_header_height_callback(struct MenuLayer *menu_layer,
                                          uint16_t section_index,
                                          void *context) {
  return 42;
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
                               .get_num_sections = get_num_sections_callback,
                               .get_num_rows = get_num_rows_callback,
                               .draw_row = draw_row_callback,
                               .draw_header = draw_header_callback,
                               .get_header_height = get_header_height_callback,
                               .get_cell_height = get_cell_height_callback,
                           });

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

  snprintf(s_subtitle_text, sizeof(s_subtitle_text), "%d", g_current_season);

  // Request team standings data if not loaded
  if (!s_data_loaded) {
    // Set the legacy callbacks (for compatibility, though not used)
    message_handler_set_team_standings_callbacks(on_team_standings_received,
                                                 on_team_standings_complete);

    // Register our custom inbox handler for the formatted text
    app_message_register_inbox_received(team_standings_inbox_received);

    // Request the data
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
