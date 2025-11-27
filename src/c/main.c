#include "data_models.h"
#include "message_handler.h"
#include "windows/home_window.h"
#include <pebble.h>

static void init(void) {
  // Set current season to current year
  // Calculate year manually to avoid localtime() linker issues
  time_t now = time(NULL);
  int days_since_epoch = now / 86400;
  // Approximate year calculation (365.25 days per year on average)
  g_current_season = 1970 + (days_since_epoch / 365);

  // Adjust for more accurate year (add leap years)
  int years_elapsed = g_current_season - 1970;
  int approx_leap_days = years_elapsed / 4;
  g_current_season = 1970 + ((days_since_epoch - approx_leap_days) / 365);

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
