#pragma once

#include <pebble.h>

// Text colors for Black & White displays
#ifdef PBL_COLOR
  #define TEXT_COLOR_UNSELECTED GColorBlack
  #define TEXT_COLOR_SELECTED GColorWhite
  #define DIVIDER_COLOR GColorLightGray
  #define HIGHLIGHT_BG GColorCobaltBlue
  #define SELECTED_ICON_LIGHT true
  #define HEADER_COLOR GColorOxfordBlue
  #define HEADER_CONTENT_COLOR GColorWhite
#else
  #define TEXT_COLOR_UNSELECTED GColorBlack
  #define TEXT_COLOR_SELECTED GColorWhite
  #define DIVIDER_COLOR GColorLightGray
  #define HIGHLIGHT_BG GColorBlack
  #define SELECTED_ICON_LIGHT true
  #define HEADER_COLOR GColorLightGray
  #define HEADER_CONTENT_COLOR GColorBlack
#endif
