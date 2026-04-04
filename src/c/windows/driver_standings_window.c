#include "driver_standings_window.h"
#include "../colors.h"
#include "../ui_constants.h"
#include "../data_models.h"
#include "../message_handler.h"
#include <pebble.h>

#define MAX_DRIVERS 30

static Window *s_window;
static MenuLayer *s_menu_layer;
static char s_subtitle_text[32];

// Driver standings data storage
static DriverStanding s_drivers[MAX_DRIVERS];
static int s_driver_count = 0;
static bool s_data_loaded = false;

// Parse pipe-delimited driver standings data
static void parse_standings_data(const char *data) {
  if (!data) {
    return;
  }

  // Reset driver count
  s_driver_count = 0;

  const char *ptr = data;
  char line_buffer[256];

  // Parse each line manually
  while (*ptr && s_driver_count < MAX_DRIVERS) {
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

    // Parse pipe-delimited fields: position|name|code|points
    char position_str[16] = {0};
    char name[64] = {0};
    char code[8] = {0};
    char points_str[16] = {0};

    // Find first pipe (position|name)
    const char *pipe1 = strchr(line_buffer, '|');
    if (!pipe1) continue;

    // Find second pipe (name|code)
    const char *pipe2 = strchr(pipe1 + 1, '|');
    if (!pipe2) continue;

    // Find third pipe (code|points)
    const char *pipe3 = strchr(pipe2 + 1, '|');
    if (!pipe3) continue;

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

    // Extract code
    size_t code_len = pipe3 - pipe2 - 1;
    if (code_len >= sizeof(code)) code_len = sizeof(code) - 1;
    strncpy(code, pipe2 + 1, code_len);
    code[code_len] = '\0';

    // Extract points (rest of the line)
    strncpy(points_str, pipe3 + 1, sizeof(points_str) - 1);
    points_str[sizeof(points_str) - 1] = '\0';

    // Store the data
    s_drivers[s_driver_count].position = atoi(position_str);
    s_drivers[s_driver_count].points = atoi(points_str);
    snprintf(s_drivers[s_driver_count].name, sizeof(s_drivers[s_driver_count].name), "%s", name);
    snprintf(s_drivers[s_driver_count].code, sizeof(s_drivers[s_driver_count].code), "%s", code);
    s_drivers[s_driver_count].index = s_driver_count;

    s_driver_count++;
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "Parsed %d drivers from standings data", s_driver_count);
  s_data_loaded = true;
}

// Callback from message handler
static void on_driver_data_received(int index, const char *name,
                                    const char *code, int points,
                                    int position) {
  // Legacy callback - not used with new format
}

static void on_driver_count_received(int count) {
  // Legacy callback - not used with new format
}

// Custom inbox handler for driver standings text
static void driver_standings_inbox_received(DictionaryIterator *iterator, void *context) {
  Tuple *request_type_tuple = dict_find(iterator, MESSAGE_KEY_REQUEST_TYPE);
  if (!request_type_tuple) {
    return;
  }

  int request_type = request_type_tuple->value->int32;
  if (request_type != REQUEST_TYPE_GET_DRIVER_STANDINGS) {
    return;
  }

  // Get the formatted standings text
  Tuple *title_tuple = dict_find(iterator, MESSAGE_KEY_DATA_TITLE);
  if (!title_tuple) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "No title in driver standings message");
    return;
  }

  const char *standings_text = title_tuple->value->cstring;
  APP_LOG(APP_LOG_LEVEL_INFO, "Received driver standings text (%d chars)", strlen(standings_text));

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
  return s_driver_count > 0 ? s_driver_count : 1;
}

// Helper function to format driver name as "M.Verstapp."
static void format_driver_name(const char *full_name, char *output, size_t output_size) {
  if (!full_name || !output || output_size < 4) return;

  // Find the space between first and last name
  const char *space = strchr(full_name, ' ');
  if (!space) {
    // No space found, just copy the name
    strncpy(output, full_name, output_size - 1);
    output[output_size - 1] = '\0';
    return;
  }

  // Get first initial
  char first_initial = full_name[0];

  // Get last name (skip the space)
  const char *last_name = space + 1;

  // Format as "F.Lastname." (truncate if needed)
  // Leave room for "F." + lastname + "." + null terminator
  size_t max_last_len = output_size - 4; // Room for "F", ".", ".", "\0"

  size_t pos = 0;
  output[pos++] = first_initial;
  output[pos++] = '.';

  // Copy last name (truncate if necessary)
  size_t copied = 0;
  while (*last_name && copied < max_last_len && pos < output_size - 2) {
    output[pos++] = *last_name++;
    copied++;
  }

  output[pos] = '\0';
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer,
                              MenuIndex *cell_index, void *context) {
  if (!s_data_loaded) {
    menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
    return;
  }

  if (cell_index->row < s_driver_count) {
    DriverStanding *driver = &s_drivers[cell_index->row];

    // Get bounds
    GRect bounds = layer_get_bounds(cell_layer);

    // Check if this cell is selected
    bool selected = menu_layer_is_index_selected(s_menu_layer, cell_index);

    // Draw selection background (black for selected)
    if (selected) {
      graphics_context_set_fill_color(ctx, HIGHLIGHT_BG);
      graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    }

    GColor text_color = selected ? TEXT_COLOR_SELECTED : TEXT_COLOR_UNSELECTED;
    graphics_context_set_text_color(ctx, text_color);

    // Format: "1  M.Verstapp.  321" (note extra space for single digits)
    // Add space after single-digit positions to align names

    // Draw position number in a fixed-width column (right-aligned so digits line up)
    char position_text[4];
    snprintf(position_text, sizeof(position_text), "%d", driver->position);
    GRect pos_rect = GRect(H_INSET, 2, MENU_ROW_POS_WIDTH, bounds.size.h - 4);
    graphics_draw_text(ctx, position_text,
                      fonts_get_system_font(MENU_ROW_FONT),
                      pos_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    // Draw name starting at a fixed offset after the position column
    char abbreviated_name[16];
    format_driver_name(driver->name, abbreviated_name, sizeof(abbreviated_name));
    const int name_x = H_INSET + MENU_ROW_POS_WIDTH + MENU_ROW_POS_GAP;
    GRect text_rect = GRect(name_x, 2, bounds.size.w - name_x - 46, bounds.size.h - 4);
    graphics_draw_text(ctx, abbreviated_name,
                      fonts_get_system_font(MENU_ROW_FONT),
                      text_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    // Draw points on right
    char points_text[16];
    snprintf(points_text, sizeof(points_text), "%d", driver->points);
    GRect points_rect = GRect(bounds.size.w - 44 - H_INSET, 2, 42, bounds.size.h - 4);
    graphics_draw_text(ctx, points_text,
                      fonts_get_system_font(MENU_ROW_FONT),
                      points_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentRight,
                      NULL);
  } else {
    menu_cell_basic_draw(ctx, cell_layer, "No drivers", NULL, NULL);
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
                    fonts_get_system_font(MENU_HEADER_TITLE_FONT),
                    title_rect,
                    GTextOverflowModeTrailingEllipsis,
                    GTextAlignmentCenter,
                    NULL);

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_line(ctx, GPoint(HDR_INSET, MENU_HEADER_DIVIDER_Y), GPoint(bounds.size.w - HDR_INSET, MENU_HEADER_DIVIDER_Y));

  GRect subtitle_left = GRect(HDR_INSET, MENU_HEADER_SUBTITLE_Y, 80, 14);
  graphics_draw_text(ctx, "Drivers",
                    fonts_get_system_font(MENU_HEADER_SUBTITLE_FONT),
                    subtitle_left,
                    GTextOverflowModeTrailingEllipsis,
                    GTextAlignmentLeft,
                    NULL);

  GRect subtitle_right = GRect(bounds.size.w - 80 - HDR_INSET, MENU_HEADER_SUBTITLE_Y, 76, 14);
  graphics_draw_text(ctx, s_subtitle_text,
                    fonts_get_system_font(MENU_HEADER_SUBTITLE_FONT),
                    subtitle_right,
                    GTextOverflowModeTrailingEllipsis,
                    GTextAlignmentRight,
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
  // On round displays, centre the focused row so the selected item always
  // sits in the widest part of the circle, with minimal clipping.
  menu_layer_set_center_focused(s_menu_layer, true);
#endif

  // Set black/white highlight for selected row
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

  // Request driver standings data if not loaded
  if (!s_data_loaded) {
    // Set the legacy callbacks (for compatibility, though not used)
    message_handler_set_driver_standings_callbacks(on_driver_data_received,
                                                   on_driver_count_received);

    // Register our custom inbox handler for the formatted text
    app_message_register_inbox_received(driver_standings_inbox_received);

    // Request the data
    message_handler_request_driver_standings();
  }
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  s_menu_layer = NULL;
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
