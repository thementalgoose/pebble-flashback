#pragma once

#include <pebble.h>

// Text colors for Black & White displays
#ifdef PBL_COLOR
  #define TEXT_COLOR_UNSELECTED GColorBlack
  #define TEXT_COLOR_SELECTED GColorBlack
  #define DIVIDER_COLOR GColorLightGray
  #define HIGHLIGHT_BG GColorTiffanyBlue
  #define SELECTED_ICON_LIGHT true
#else
  #define TEXT_COLOR_UNSELECTED GColorBlack
  #define TEXT_COLOR_SELECTED GColorWhite
  #define DIVIDER_COLOR GColorLightGray
  #define HIGHLIGHT_BG GColorBlack
  #define SELECTED_ICON_LIGHT true
#endif

