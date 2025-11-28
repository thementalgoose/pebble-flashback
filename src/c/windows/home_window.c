#include "home_window.h"
#include "../data_models.h"
#include "../colors.h"
#include "calendar_window.h"
#include "driver_standings_window.h"
#include "team_standings_window.h"
#include <pebble.h>

// Current season (will be set to current year)
int g_current_season = 2025;

static Window *s_window;
static MenuLayer *s_menu_layer;
static char s_season_text[16];

// Menu icons
static GBitmap *s_icon_calendar_light;
static GBitmap *s_icon_drivers_light;
static GBitmap *s_icon_teams_light;
static GBitmap *s_icon_calendar_dark;
static GBitmap *s_icon_drivers_dark;
static GBitmap *s_icon_teams_dark;

// Menu items
#define MENU_ITEM_CALENDAR 0
#define MENU_ITEM_DRIVER_STANDINGS 1
#define MENU_ITEM_TEAM_STANDINGS 2
#define NUM_MENU_ITEMS 3

// Menu layer callbacks
static uint16_t get_num_rows_callback(MenuLayer *menu_layer,
                                      uint16_t section_index, void *context) {
  return NUM_MENU_ITEMS;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer,
                              MenuIndex *cell_index, void *context) {
  // Check if this row is selected
  bool selected = menu_layer_is_index_selected(s_menu_layer, cell_index);

  // Draw the cell with custom icon compositing
  GRect bounds = layer_get_bounds(cell_layer);

  // Draw custom color selection background on color displays
  if (selected) {
    graphics_context_set_fill_color(ctx, HIGHLIGHT_BG);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  }

  // Set text color based on selection
  GColor text_color = selected ? TEXT_COLOR_SELECTED : TEXT_COLOR_UNSELECTED;

  // Get the icon and title for this row
  GBitmap *icon = NULL;
  const char *title = NULL;

  switch (cell_index->row) {
  case MENU_ITEM_CALENDAR:
    icon = (selected && SELECTED_ICON_LIGHT) ? s_icon_calendar_light : s_icon_calendar_dark;
    title = "Calendar";
    break;
  case MENU_ITEM_DRIVER_STANDINGS:
    icon = (selected && SELECTED_ICON_LIGHT) ? s_icon_drivers_light : s_icon_drivers_dark;
    title = "Driver Standings";
    break;
  case MENU_ITEM_TEAM_STANDINGS:
    icon = (selected && SELECTED_ICON_LIGHT) ? s_icon_teams_light : s_icon_teams_dark;
    title = "Team Standings";
    break;
  }

  // Draw the icon with proper compositing mode to match text color
  if (icon) {
    GRect icon_bounds = gbitmap_get_bounds(icon);
    GRect icon_rect = GRect(2, (bounds.size.h - icon_bounds.size.h) / 2,
                           icon_bounds.size.w, icon_bounds.size.h);

    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_context_set_text_color(ctx, text_color);
    graphics_draw_bitmap_in_rect(ctx, icon, icon_rect);
  }

  // Draw the text
  GRect text_rect = GRect(32, (bounds.size.h - 32) / 2, bounds.size.w - 32 - 4, 28);
  graphics_context_set_text_color(ctx, text_color);
  graphics_draw_text(ctx, title,
                    fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                    text_rect,
                    GTextOverflowModeTrailingEllipsis,
                    GTextAlignmentLeft,
                    NULL);
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer,
                                        MenuIndex *cell_index, void *context) {
  return 44; // Taller cells for easier touch
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index,
                            void *context) {
  switch (cell_index->row) {
  case MENU_ITEM_CALENDAR:
    APP_LOG(APP_LOG_LEVEL_INFO, "Calendar selected");
    calendar_window_push();
    break;
  case MENU_ITEM_DRIVER_STANDINGS:
    APP_LOG(APP_LOG_LEVEL_INFO, "Driver Standings selected");
    driver_standings_window_push();
    break;
  case MENU_ITEM_TEAM_STANDINGS:
    APP_LOG(APP_LOG_LEVEL_INFO, "Team Standings selected");
    team_standings_window_push();
    break;
  }
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer,
                                 uint16_t section_index, void *context) {
  // Draw season header centered on round displays
  GRect bounds = layer_get_bounds(cell_layer);
  GTextAlignment alignment = PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft);

  graphics_context_set_text_color(ctx, TEXT_COLOR_UNSELECTED);
  graphics_draw_text(ctx, s_season_text,
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

  // Load menu icons
  s_icon_calendar_light = gbitmap_create_with_resource(RESOURCE_ID_ICON_CALENDAR_LIGHT);
  s_icon_drivers_light = gbitmap_create_with_resource(RESOURCE_ID_ICON_DRIVERS_LIGHT);
  s_icon_teams_light = gbitmap_create_with_resource(RESOURCE_ID_ICON_TEAMS_LIGHT);
  s_icon_calendar_dark = gbitmap_create_with_resource(RESOURCE_ID_ICON_CALENDAR_DARK);
  s_icon_drivers_dark = gbitmap_create_with_resource(RESOURCE_ID_ICON_DRIVERS_DARK);
  s_icon_teams_dark = gbitmap_create_with_resource(RESOURCE_ID_ICON_TEAMS_DARK);

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
                               .get_num_rows = get_num_rows_callback,
                               .draw_row = draw_row_callback,
                               .get_cell_height = get_cell_height_callback,
                               .select_click = select_callback,
                               .draw_header = draw_header_callback,
                               .get_header_height = get_header_height_callback,
                               .get_num_sections = get_num_sections_callback,
                           });

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

  // Set season text
  snprintf(s_season_text, sizeof(s_season_text), "%d Season", g_current_season);
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);

  // Destroy menu icons
  gbitmap_destroy(s_icon_calendar_light);
  gbitmap_destroy(s_icon_drivers_light);
  gbitmap_destroy(s_icon_teams_light);
  gbitmap_destroy(s_icon_calendar_dark);
  gbitmap_destroy(s_icon_drivers_dark);
  gbitmap_destroy(s_icon_teams_dark);
}

void home_window_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
                                             .load = window_load,
                                             .unload = window_unload,
                                         });
  }

  window_stack_push(s_window, true);
}

void home_window_destroy(void) {
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }
}
