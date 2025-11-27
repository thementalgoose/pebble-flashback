#include "standings_window.h"
#include "../modules/data.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static RequestType s_type;

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer,
                                           uint16_t section_index, void *data) {
  DataList *list = data_get_list(s_type);
  return list ? list->count : 0;
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer,
                                   MenuIndex *cell_index, void *data) {
  DataList *list = data_get_list(s_type);
  if (list && cell_index->row < list->count) {
    menu_cell_basic_draw(ctx, cell_layer, list->items[cell_index->row].title,
                         list->items[cell_index->row].subtitle, NULL);
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
                           });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

  // Request data
  comm_set_data_changed_handler(reload_data);
  comm_request_data(s_type);
}

static void window_unload(Window *window) {
  comm_set_data_changed_handler(NULL);
  menu_layer_destroy(s_menu_layer);
  window_destroy(window);
  s_window = NULL;
}

void standings_window_push(RequestType type) {
  s_type = type;
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
                                             .load = window_load,
                                             .unload = window_unload,
                                         });
  }
  window_stack_push(s_window, true);
}
