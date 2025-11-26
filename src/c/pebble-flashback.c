#include <pebble.h>

#define MAX_RACES 24
#define KEY_RACE_COUNT 0
#define KEY_RACE_NAME 1
#define KEY_RACE_LOCATION 2
#define KEY_RACE_DATE 3
#define KEY_RACE_INDEX 4

typedef struct {
  char name[32];
  char location[32];
  char date[16];
} F1Race;

static Window *s_main_window;
static MenuLayer *s_menu_layer;
static TextLayer *s_loading_layer;

static F1Race races[MAX_RACES];
static int race_count = 0;
static bool data_loaded = false;

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return race_count;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  if (cell_index->row >= race_count) return;
  
  F1Race *race = &races[cell_index->row];
  menu_cell_basic_draw(ctx, cell_layer, race->name, race->location, NULL);
  
  // Draw date in top-right
  GRect bounds = layer_get_bounds(cell_layer);
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, race->date, 
                    fonts_get_system_font(FONT_KEY_GOTHIC_14), 
                    GRect(bounds.size.w - 50, 2, 48, 20),
                    GTextOverflowModeTrailingEllipsis,
                    GTextAlignmentRight, NULL);
}

static int16_t get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return 44;
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *race_count_tuple = dict_find(iter, KEY_RACE_COUNT);
  Tuple *race_index_tuple = dict_find(iter, KEY_RACE_INDEX);
  Tuple *race_name_tuple = dict_find(iter, KEY_RACE_NAME);
  Tuple *race_location_tuple = dict_find(iter, KEY_RACE_LOCATION);
  Tuple *race_date_tuple = dict_find(iter, KEY_RACE_DATE);
  
  if (race_count_tuple) {
    race_count = race_count_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_INFO, "Received race count: %d", race_count);
  }
  
  if (race_index_tuple && race_name_tuple && race_location_tuple && race_date_tuple) {
    int index = race_index_tuple->value->int32;
    if (index >= 0 && index < MAX_RACES) {
      snprintf(races[index].name, sizeof(races[index].name), "%s", race_name_tuple->value->cstring);
      snprintf(races[index].location, sizeof(races[index].location), "%s", race_location_tuple->value->cstring);
      snprintf(races[index].date, sizeof(races[index].date), "%s", race_date_tuple->value->cstring);
      
      APP_LOG(APP_LOG_LEVEL_INFO, "Received race %d: %s", index, races[index].name);
      
      // Hide loading text and show menu when we have data
      if (!data_loaded && race_count > 0) {
        data_loaded = true;
        layer_set_hidden(text_layer_get_layer(s_loading_layer), true);
        layer_set_hidden(menu_layer_get_layer(s_menu_layer), false);
        menu_layer_reload_data(s_menu_layer);
      } else {
        menu_layer_reload_data(s_menu_layer);
      }
    }
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", (int)reason);
}

static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox failed: %d", (int)reason);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create loading text layer
  s_loading_layer = text_layer_create(GRect(0, bounds.size.h / 2 - 20, bounds.size.w, 40));
  text_layer_set_text(s_loading_layer, "Loading F1\nCalendar...");
  text_layer_set_text_alignment(s_loading_layer, GTextAlignmentCenter);
  text_layer_set_font(s_loading_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_loading_layer));
  
  // Create MenuLayer (hidden initially)
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_rows = get_num_rows_callback,
    .draw_row = draw_row_callback,
    .get_cell_height = get_cell_height_callback,
  });
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  layer_set_hidden(menu_layer_get_layer(s_menu_layer), true);
  
  // Request data from phone
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_uint8(iter, 0, 0); // Trigger data request
  app_message_outbox_send();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_loading_layer);
  menu_layer_destroy(s_menu_layer);
}

static void init(void) {
  // Register AppMessage handlers
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  
  // Open AppMessage with sufficient buffer size
  app_message_open(512, 512);
  
  // Create main window
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  window_stack_push(s_main_window, true);
}

static void deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}