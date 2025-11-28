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

typedef struct {
  char name[MAX_TITLE_LENGTH];     // Full name (first + last)
  char code[4];                     // Driver code (e.g., "NOR")
  int points;
  int position;
  int index;
} DriverStanding;

typedef struct {
  char name[MAX_TITLE_LENGTH];
  int points;
  int position;
  int index;
} ConstructorStanding;

// Global season
extern int g_current_season;
