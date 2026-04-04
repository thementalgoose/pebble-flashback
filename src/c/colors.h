#pragma once

#include <pebble.h>

// Text colors for Black & White displays
#ifdef PBL_COLOR
  #define TEXT_COLOR_UNSELECTED GColorBlack
  #define TEXT_COLOR_SELECTED GColorWhite
  #define HIGHLIGHT_BG GColorVividCerulean
  #define SELECTED_ICON_LIGHT true
#else
  #define TEXT_COLOR_UNSELECTED GColorBlack
  #define TEXT_COLOR_SELECTED GColorWhite
  #define HIGHLIGHT_BG GColorBlack
  #define SELECTED_ICON_LIGHT true
#endif

