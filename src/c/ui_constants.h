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
#define MENU_HEADER_HEIGHT  42
// Fixed width reserved for the position number column (fits "20" in GOTHIC_18_BOLD)
#define MENU_ROW_POS_WIDTH  22
// Gap between position column and name
#define MENU_ROW_POS_GAP    4

// Pixel rows within the header cell
#define MENU_HEADER_DIVIDER_Y  20
#define MENU_HEADER_SUBTITLE_Y 22

// ---------------------------------------------------------------------------
// Fonts
// ---------------------------------------------------------------------------
#define MENU_ROW_FONT             fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)
#define MENU_HEADER_TITLE_FONT    fonts_get_system_font(FONT_KEY_GOTHIC_14)
#define MENU_HEADER_SUBTITLE_FONT fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD)
// fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ROBOTO_MONO_LIGHT_14))
// fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)

// ---------------------------------------------------------------------------
// Shared strings
// ---------------------------------------------------------------------------
#define APP_TITLE "Flashback"
