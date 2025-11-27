#include "modules/comm.h"
#include "modules/data.h"
#include "windows/main_window.h"
#include <pebble.h>

static void init(void) {
  comm_init();
  data_init();
  main_window_push();
}

static void deinit(void) {
  // Destroy windows if needed, but usually they handle themselves on pop
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
