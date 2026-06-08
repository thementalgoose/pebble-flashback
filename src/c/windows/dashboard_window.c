#include "dashboard_window.h"
#include "calendar_window.h"
#include "driver_standings_window.h"
#include "flashback_screen.h"
#include "race_window.h"
#include "team_standings_window.h"
#include "../colors.h"
#include "../data_models.h"
#include "../message_handler.h"
#include "../ui_constants.h"
#include "../utils.h"
#include <pebble.h>

#define DASHBOARD_ITEM_COUNT 4
#define DASHBOARD_OVERVIEW_HEIGHT 72
#define DASHBOARD_ICON_SIZE 14
#define DASHBOARD_ICON_GAP 4

static Window *s_window;
static MenuLayer *s_menu_layer;
static AppTimer *s_overview_retry_timer;
static char s_subtitle_text[32];

static bool s_overview_loaded = false;
static int s_race_round = 0;
static char s_race_name[MAX_TITLE_LENGTH] = "";
static char s_race_datetime[64] = "";

typedef enum {
  DASHBOARD_ICON_NONE = 0,
  DASHBOARD_ICON_CALENDAR,
  DASHBOARD_ICON_DRIVERS,
  DASHBOARD_ICON_TEAMS,
} DashboardIcon;

static void parse_overview_data(const char *data) {
  if (!data) {
    return;
  }

  const char *pipe1 = strchr(data, '|');
  if (!pipe1) {
    return;
  }

  const char *pipe2 = strchr(pipe1 + 1, '|');
  if (!pipe2) {
    return;
  }

  char round_str[16] = {0};
  size_t round_len = pipe1 - data;
  if (round_len >= sizeof(round_str)) {
    round_len = sizeof(round_str) - 1;
  }
  strncpy(round_str, data, round_len);
  round_str[round_len] = '\0';

  size_t name_len = pipe2 - pipe1 - 1;
  if (name_len >= sizeof(s_race_name)) {
    name_len = sizeof(s_race_name) - 1;
  }
  strncpy(s_race_name, pipe1 + 1, name_len);
  s_race_name[name_len] = '\0';

  strncpy(s_race_datetime, pipe2 + 1, sizeof(s_race_datetime) - 1);
  s_race_datetime[sizeof(s_race_datetime) - 1] = '\0';

  s_race_round = atoi(round_str);
  s_overview_loaded = true;

  APP_LOG(APP_LOG_LEVEL_INFO, "Parsed dashboard overview: round %d, %s",
          s_race_round, s_race_name);
}

static void dashboard_overview_received(const char *overview_text) {
  parse_overview_data(overview_text);

  if (s_overview_retry_timer) {
    app_timer_cancel(s_overview_retry_timer);
    s_overview_retry_timer = NULL;
  }

  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
}

static void request_overview_retry(void *context) {
  s_overview_retry_timer = NULL;

  if (!s_overview_loaded) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Retrying dashboard overview request");
    message_handler_request_overview();
  }
}

static uint16_t get_num_rows_callback(MenuLayer *menu_layer,
                                      uint16_t section_index, void *context) {
  return DASHBOARD_ITEM_COUNT;
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer,
                                        MenuIndex *cell_index, void *context) {
  if (cell_index->row == 0) {
    return DASHBOARD_OVERVIEW_HEIGHT;
  }

  return MENU_CELL_HEIGHT;
}

static void draw_overview_row(GContext *ctx, const Layer *cell_layer,
                              MenuIndex *cell_index) {
  GRect bounds = layer_get_bounds(cell_layer);
  bool selected = menu_layer_is_index_selected(s_menu_layer, cell_index);

  if (selected) {
    graphics_context_set_fill_color(ctx, HIGHLIGHT_BG);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  }

  graphics_context_set_stroke_color(ctx, DIVIDER_COLOR);
  graphics_draw_line(ctx,
                     GPoint(H_INSET, 0),
                     GPoint(bounds.size.w - H_INSET, 0));
  graphics_draw_line(ctx,
                     GPoint(H_INSET, bounds.size.h - 1),
                     GPoint(bounds.size.w - H_INSET, bounds.size.h - 1));

  graphics_context_set_text_color(ctx,
                                  selected ? TEXT_COLOR_SELECTED : TEXT_COLOR_UNSELECTED);

  if (!s_overview_loaded) {
    menu_cell_basic_draw(ctx, cell_layer, "Loading upcoming race...", NULL, NULL);
    return;
  }

  char round_text[20];
  snprintf(round_text, sizeof(round_text), "Round %d", s_race_round);

  char datetime_text[64];
  utils_format_datetime(s_race_datetime, datetime_text, sizeof(datetime_text));
  if (datetime_text[0] == '\0') {
    snprintf(datetime_text, sizeof(datetime_text), "%s", s_race_datetime);
  }

  GRect round_rect = GRect(H_INSET, 4, bounds.size.w - 2 * H_INSET, 16);
  graphics_draw_text(ctx, round_text,
                     MENU_HEADER_SUBTITLE_FONT,
                     round_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft,
                     NULL);

  GRect name_rect = GRect(H_INSET, 20, bounds.size.w - 2 * H_INSET, 24);
  graphics_draw_text(ctx, s_race_name,
                     MENU_ROW_FONT,
                     name_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft,
                     NULL);

  GRect datetime_rect = GRect(H_INSET, 44, bounds.size.w - 2 * H_INSET, 18);
  graphics_draw_text(ctx, datetime_text,
                     MENU_HEADER_SUBTITLE_FONT,
                     datetime_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft,
                     NULL);
}

static void draw_regular_row(GContext *ctx, const Layer *cell_layer,
                             MenuIndex *cell_index, const char *title,
                             DashboardIcon icon) {
  GRect bounds = layer_get_bounds(cell_layer);
  bool selected = menu_layer_is_index_selected(s_menu_layer, cell_index);

  if (selected) {
    graphics_context_set_fill_color(ctx, HIGHLIGHT_BG);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  }

  graphics_context_set_text_color(ctx,
                                  selected ? TEXT_COLOR_SELECTED : TEXT_COLOR_UNSELECTED);

  int text_x = H_INSET;
  if (icon != DASHBOARD_ICON_NONE) {
    GColor icon_color = selected ? TEXT_COLOR_SELECTED : TEXT_COLOR_UNSELECTED;
    graphics_context_set_stroke_color(ctx, icon_color);
    graphics_context_set_fill_color(ctx, icon_color);

    const int icon_y = (bounds.size.h - DASHBOARD_ICON_SIZE) / 2;
    GRect icon_rect = GRect(H_INSET, icon_y, DASHBOARD_ICON_SIZE, DASHBOARD_ICON_SIZE);

    switch (icon) {
    case DASHBOARD_ICON_CALENDAR:
      graphics_draw_rect(ctx, icon_rect);
      graphics_fill_rect(ctx, GRect(icon_rect.origin.x, icon_rect.origin.y, icon_rect.size.w, 3), 0, GCornerNone);
      graphics_draw_line(ctx,
                         GPoint(icon_rect.origin.x + 3, icon_rect.origin.y + 5),
                         GPoint(icon_rect.origin.x + icon_rect.size.w - 4, icon_rect.origin.y + 5));
      break;
    case DASHBOARD_ICON_DRIVERS:
      graphics_fill_circle(ctx, GPoint(icon_rect.origin.x + 4, icon_rect.origin.y + 5), 3);
      graphics_fill_circle(ctx, GPoint(icon_rect.origin.x + 10, icon_rect.origin.y + 5), 3);
      graphics_draw_line(ctx,
                         GPoint(icon_rect.origin.x + 2, icon_rect.origin.y + 11),
                         GPoint(icon_rect.origin.x + 12, icon_rect.origin.y + 11));
      break;
    case DASHBOARD_ICON_TEAMS:
      graphics_draw_rect(ctx, GRect(icon_rect.origin.x + 1, icon_rect.origin.y + 2, 12, 10));
      graphics_fill_rect(ctx, GRect(icon_rect.origin.x + 3, icon_rect.origin.y + 5, 3, 7), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(icon_rect.origin.x + 8, icon_rect.origin.y + 3, 3, 9), 0, GCornerNone);
      break;
    default:
      break;
    }

    text_x += DASHBOARD_ICON_SIZE + DASHBOARD_ICON_GAP;
  }

  GRect text_rect = GRect(text_x, 2, bounds.size.w - text_x - H_INSET, bounds.size.h - 4);
  graphics_draw_text(ctx, title,
                     MENU_ROW_FONT,
                     text_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft,
                     NULL);
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer,
                              MenuIndex *cell_index, void *context) {
  switch (cell_index->row) {
  case 0:
    draw_overview_row(ctx, cell_layer, cell_index);
    break;
  case 1:
    draw_regular_row(ctx, cell_layer, cell_index, "Calendar",
                     DASHBOARD_ICON_CALENDAR);
    break;
  case 2:
    draw_regular_row(ctx, cell_layer, cell_index, "Driver Standings",
                     DASHBOARD_ICON_DRIVERS);
    break;
  case 3:
    draw_regular_row(ctx, cell_layer, cell_index, "Team Standings",
                     DASHBOARD_ICON_TEAMS);
    break;
  }
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index,
                            void *context) {
  switch (cell_index->row) {
  case 1:
    APP_LOG(APP_LOG_LEVEL_INFO, "Calendar selected from dashboard");
    calendar_window_push();
    break;
  case 2:
    APP_LOG(APP_LOG_LEVEL_INFO, "Driver standings selected from dashboard");
    driver_standings_window_push();
    break;
  case 3:
    APP_LOG(APP_LOG_LEVEL_INFO, "Team standings selected from dashboard");
    team_standings_window_push();
    break;
  default:
    break;
  }
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer,
                                 uint16_t section_index, void *context) {
  flashback_screen_draw_header(ctx, cell_layer, "Dashboard", s_subtitle_text);
}

static void window_load(Window *window) {
  s_menu_layer = flashback_screen_create_menu_layer(window);

  menu_layer_set_callbacks(s_menu_layer, NULL,
                           (MenuLayerCallbacks){
                               .get_num_sections = flashback_screen_num_sections_callback,
                               .get_num_rows = get_num_rows_callback,
                               .draw_row = draw_row_callback,
                               .draw_header = draw_header_callback,
                               .get_header_height = flashback_screen_header_height_callback,
                               .get_cell_height = get_cell_height_callback,
                               .select_click = select_callback,
  });

  snprintf(s_subtitle_text, sizeof(s_subtitle_text), "%d", g_current_season);
  message_handler_set_overview_message_callback(dashboard_overview_received);

  if (!s_overview_loaded) {
    message_handler_request_overview();
    if (!s_overview_retry_timer) {
      s_overview_retry_timer = app_timer_register(500, request_overview_retry, NULL);
    }
  }
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  s_menu_layer = NULL;
}

static void window_appear(Window *window) {
  snprintf(s_subtitle_text, sizeof(s_subtitle_text), "%d", g_current_season);
}

void dashboard_window_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
                                             .load = window_load,
                                             .unload = window_unload,
                                             .appear = window_appear,
                                         });
  }

  window_stack_push(s_window, true);
}

void dashboard_window_destroy(void) {
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }

  s_overview_loaded = false;
  s_race_round = 0;
  s_race_name[0] = '\0';
  s_race_datetime[0] = '\0';
  if (s_overview_retry_timer) {
    app_timer_cancel(s_overview_retry_timer);
    s_overview_retry_timer = NULL;
  }
  message_handler_set_overview_message_callback(NULL);
}
