#include "calendar_window.h"
#include "../modules/comm.h"
#include "../modules/data.h"
#include "race_window.h"

static Window *s_window;
static MenuLayer *s_menu_layer;

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer,
                                           uint16_t section_index, void *data) {
  return g_calendar_data.count;
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer,
                                   MenuIndex *cell_index, void *data) {
  if (cell_index->row < g_calendar_data.count) {
    menu_cell_basic_draw(ctx, cell_layer,
                         g_calendar_data.items[cell_index->row].title,
                         g_calendar_data.items[cell_index->row].subtitle, NULL);
  }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index,
                                 void *data) {
  if (cell_index->row < g_calendar_data.count) {
    race_window_push(g_calendar_data.items[cell_index->row].extra);
  }
}

static void reload_data(void) { menu_layer_reload_data(s_menu_layer); }

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL,
                           (MenuLayerCallbacks){
                               .get_num_rows = menu_get_num_rows_callback,
                               .draw_row = menu_draw_row_callback,
                               .select_click = menu_select_callback,
                           });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

  comm_set_data_changed_handler(reload_data);
  comm_request_data(REQUEST_TYPE_CALENDAR);
}

static void window_unload(Window *window) {
  comm_set_data_changed_handler(NULL);
  menu_layer_destroy(s_menu_layer);
  window_destroy(window);
  s_window = NULL;
}

void calendar_window_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
                                             .load = window_load,
                                             .unload = window_unload,
                                         });
  }
  window_stack_push(s_window, true);
}
