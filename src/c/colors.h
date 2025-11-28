#pragma once

#include <pebble.h>

// Text colors for Black & White displays
#ifdef PBL_COLOR
  #define TEXT_COLOR_UNSELECTED GColorBlack
  #define TEXT_COLOR_SELECTED GColorBlack
  #define HIGHLIGHT_BG GColorFromHEX(0x489bb0)
#else
  #define TEXT_COLOR_UNSELECTED GColorBlack
  #define TEXT_COLOR_SELECTED GColorWhite
  #define HIGHLIGHT_BG GColorFromHEX(0x000000)
#endif

