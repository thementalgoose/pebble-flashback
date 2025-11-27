#include "main_window.h"
#include "../modules/comm.h"
#include "calendar_window.h"
#include "standings_window.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static GBitmap *s_icon_calendar;
static GBitmap *s_icon_helmet;
static GBitmap *s_icon_trophy;

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer,
                                           uint16_t section_index, void *data) {
  return 3;
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer,
                                   MenuIndex *cell_index, void *data) {
  switch (cell_index->row) {
  case 0:
    menu_cell_basic_draw(ctx, cell_layer, "Calendar", "Upcoming & Past",
                         s_icon_calendar);
    break;
  case 1:
    menu_cell_basic_draw(ctx, cell_layer, "Drivers", "Standings",
                         s_icon_helmet);
    break;
  case 2:
    menu_cell_basic_draw(ctx, cell_layer, "Teams", "Standings", s_icon_trophy);
    break;
  }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index,
                                 void *data) {
  switch (cell_index->row) {
  case 0:
    calendar_window_push();
    break;
  case 1:
    standings_window_push(REQUEST_TYPE_DRIVERS);
    break;
  case 2:
    standings_window_push(REQUEST_TYPE_TEAMS);
    break;
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_icon_calendar = gbitmap_create_with_resource(RESOURCE_ID_ICON_CALENDAR);
  s_icon_helmet = gbitmap_create_with_resource(RESOURCE_ID_ICON_HELMET);
  s_icon_trophy = gbitmap_create_with_resource(RESOURCE_ID_ICON_TROPHY);

  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL,
                           (MenuLayerCallbacks){
                               .get_num_rows = menu_get_num_rows_callback,
                               .draw_row = menu_draw_row_callback,
                               .select_click = menu_select_callback,
                           });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  gbitmap_destroy(s_icon_calendar);
  gbitmap_destroy(s_icon_helmet);
  gbitmap_destroy(s_icon_trophy);
  window_destroy(window);
  s_window = NULL;
}

void main_window_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
                                             .load = window_load,
                                             .unload = window_unload,
                                         });
  }
  window_stack_push(s_window, true);
}
