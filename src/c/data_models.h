#pragma once

#include <pebble.h>

// Maximum sizes for data strings
#define MAX_TITLE_LENGTH 64
#define MAX_SUBTITLE_LENGTH 64
#define MAX_EXTRA_LENGTH 32

// Data models
typedef struct {
  char name[MAX_TITLE_LENGTH];
  char location[MAX_SUBTITLE_LENGTH];
  char date[MAX_EXTRA_LENGTH]; // ISO format date
  int index;
  int round; // Round number in the season (1-24)
} Race;

typedef struct {
  char label[MAX_TITLE_LENGTH];
  char datetime[MAX_SUBTITLE_LENGTH]; // ISO format datetime
  int index;
} RaceEvent;

// Global season
extern int g_current_season;
