#include "data_models.h"
#include "message_handler.h"
#include "windows/home_window.h"
#include <pebble.h>

static void init(void) {
  // Get current year using localtime()
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);

  // tm_year is years since 1900
  g_current_season = current_time->tm_year + 1900;

  APP_LOG(APP_LOG_LEVEL_INFO, "F1 Flashback initialized for season %d",
          g_current_season);

  // Initialize message handler
  message_handler_init();

  // Push home window
  home_window_push();
}

static void deinit(void) {
  // Cleanup windows
  home_window_destroy();

  // Cleanup message handler
  message_handler_deinit();

  APP_LOG(APP_LOG_LEVEL_INFO, "F1 Flashback deinitialized");
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
