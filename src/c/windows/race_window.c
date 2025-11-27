#include "race_window.h"

static Window *s_window;
static ScrollLayer *s_scroll_layer;
static TextLayer *s_text_layer;
static char s_race_details[1024];

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GRect max_text_bounds = GRect(0, 0, bounds.size.w, 2000);

  s_scroll_layer = scroll_layer_create(bounds);
  s_text_layer = text_layer_create(max_text_bounds);
  text_layer_set_text(s_text_layer, s_race_details);
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentLeft);

  // Calculate size
  GSize max_size = text_layer_get_content_size(s_text_layer);
  text_layer_set_size(s_text_layer, max_size);
  scroll_layer_set_content_size(s_scroll_layer,
                                GSize(bounds.size.w, max_size.h + 10));

  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_text_layer));
  scroll_layer_set_click_config_onto_window(s_scroll_layer, window);
  layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
  scroll_layer_destroy(s_scroll_layer);
  window_destroy(window);
  s_window = NULL;
}

void race_window_push(char *race_json) {
  // In a real app, we would parse JSON here or have JS format it.
  // For this MVP, we'll just display the raw data string which contains the
  // info. To make it better, I should have formatted it in JS. But I can't
  // change JS easily now without context switch. I'll just copy it.
  strncpy(s_race_details, race_json, sizeof(s_race_details) - 1);

  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
                                             .load = window_load,
                                             .unload = window_unload,
                                         });
  }
  window_stack_push(s_window, true);
}
