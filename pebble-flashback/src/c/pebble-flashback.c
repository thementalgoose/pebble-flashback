#include <pebble.h>

#include "modules/data.h"
#include "window/main-menu.h"

static void js_ready_callback() {
  main_window_set_background_color(COLOR_FALLBACK(GColorGreen, GColorWhite));
}

static void init() {
  const int array_size = 16;
  data_init(array_size);

  main_window_push();
}

static void deinit() {
  data_deinit();
}

int main() {
  init();
  app_event_loop();
  deinit();
}