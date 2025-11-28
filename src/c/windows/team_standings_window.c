#include "team_standings_window.h"
#include "../data_models.h"
#include "../message_handler.h"
#include <pebble.h>

#define MAX_TEAMS 15

static Window *s_window;
static MenuLayer *s_menu_layer;
static char s_header_text[32];

// Team standings data storage
static ConstructorStanding s_teams[MAX_TEAMS];
static int s_team_count = 0;
static bool s_data_loaded = false;

// Callbacks from message handler
static void on_team_data_received(int index, const char *name, int points,
                                  int position) {
  if (index >= 0 && index < MAX_TEAMS) {
    snprintf(s_teams[index].name, sizeof(s_teams[index].name), "%s", name);
    s_teams[index].points = points;
    s_teams[index].position = position;
    s_teams[index].index = index;

    // Reload menu to show new team data
    if (s_menu_layer) {
      menu_layer_reload_data(s_menu_layer);
    }
  }
}

static void on_team_count_received(int count) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Expected %d teams", count);
  s_team_count = count;
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

    // Draw position number on the left
    char position_text[4];
    snprintf(position_text, sizeof(position_text), "%d", team->position);

    GRect position_rect = GRect(4, 4, 28, bounds.size.h - 8);
    graphics_context_set_text_color(ctx, text_color);
    graphics_draw_text(ctx, position_text,
                      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                      position_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentCenter,
                      NULL);

    // Draw team name and points with offset for position number
    const int text_offset_x = 36;
    GRect name_rect = GRect(text_offset_x, 0, bounds.size.w - text_offset_x - 4, 22);

    // Format points display
    char points_text[32];
    snprintf(points_text, sizeof(points_text), "%d pts", team->points);
    GRect points_rect = GRect(text_offset_x, 22, bounds.size.w - text_offset_x - 4, 20);

    graphics_draw_text(ctx, team->name,
                      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                      name_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    graphics_draw_text(ctx, points_text,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18),
                      points_rect,
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);
  } else {
    menu_cell_basic_draw(ctx, cell_layer, "No teams", NULL, NULL);
  }
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer,
                                        MenuIndex *cell_index, void *context) {
  return 44;
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer,
                                 uint16_t section_index, void *context) {
  // Draw header centered on round displays
  GRect bounds = layer_get_bounds(cell_layer);
  GTextAlignment alignment = PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft);

  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, s_header_text,
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
                           });

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

  // Set header text
  snprintf(s_header_text, sizeof(s_header_text), "%d Team Standings", g_current_season);

  // Request team standings data if not loaded
  if (!s_data_loaded) {
    message_handler_set_team_standings_callbacks(on_team_data_received,
                                                 on_team_count_received);
    message_handler_request_team_standings();
  }
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
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
