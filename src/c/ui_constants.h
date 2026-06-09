#pragma once

#include <pebble.h>

// ---------------------------------------------------------------------------
// Circular bezel insets
// Rows near the top/bottom of a round display are clipped by the bezel.
// HDR_INSET is larger because the header sits near the top arc.
// ---------------------------------------------------------------------------
#ifdef PBL_ROUND
  #define H_INSET   16
  #define HDR_INSET 22
#else
  #define H_INSET   4
  #define HDR_INSET 4
#endif

// ---------------------------------------------------------------------------
// Menu layout
// ---------------------------------------------------------------------------
#define MENU_CELL_HEIGHT    28
#define MENU_HEADER_HEIGHT  50
// Fixed width reserved for the position number column (fits "20" in GOTHIC_18_BOLD)
#define MENU_ROW_POS_WIDTH  22
// Gap between position column and name
#define MENU_ROW_POS_GAP    4

// Pixel rows within the header cell
#define MENU_HEADER_TITLE_Y     4
#define MENU_HEADER_SUBTITLE_Y  24
#define MENU_HEADER_LOGO_SIZE   16
#define MENU_HEADER_LOGO_GAP    8
#define MENU_HEADER_RIGHT_WIDTH 40

// ---------------------------------------------------------------------------
// Fonts
// ---------------------------------------------------------------------------
#define MENU_ROW_FONT               fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)
#define MENU_ROW_SECONDARY_FONT     fonts_get_system_font(FONT_KEY_GOTHIC_14)
#define MENU_HEADER_TITLE_FONT      fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD)
#define MENU_HEADER_SUBTITLE_FONT   fonts_get_system_font(FONT_KEY_GOTHIC_14)

#define FLASHBACK_SCREEN_HEADER_TITLE_FONT MENU_HEADER_TITLE_FONT
#define FLASHBACK_SCREEN_HEADER_SUBTITLE_FONT MENU_ROW_FONT
#define HOME_WINDOW_ROW_FONT        MENU_ROW_FONT
#define DASHBOARD_HEADER_TITLE_FONT MENU_HEADER_TITLE_FONT
#define DASHBOARD_HEADER_SUBTITLE_FONT MENU_HEADER_SUBTITLE_FONT
#define DASHBOARD_ROW_FONT          MENU_ROW_FONT
#define RACE_WINDOW_ROW_FONT        MENU_ROW_FONT
#define RACE_WINDOW_SECONDARY_FONT  MENU_ROW_SECONDARY_FONT
#define RESULTS_RACE_WINDOW_ROW_FONT MENU_ROW_FONT
#define RESULTS_RACE_WINDOW_SECONDARY_FONT MENU_ROW_SECONDARY_FONT
#define RESULTS_QUALIFYING_WINDOW_ROW_FONT MENU_ROW_FONT
#define RESULTS_QUALIFYING_WINDOW_SECONDARY_FONT MENU_ROW_SECONDARY_FONT
#define TEAM_STANDINGS_WINDOW_ROW_FONT MENU_ROW_FONT
#define DRIVER_STANDINGS_WINDOW_ROW_FONT MENU_ROW_FONT
#define CALENDAR_WINDOW_ROW_FONT    MENU_ROW_FONT

// fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ROBOTO_MONO_LIGHT_14))
// fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)

// ---------------------------------------------------------------------------
// Shared strings
// ---------------------------------------------------------------------------
#define APP_TITLE "Flashback"
