#pragma once
#include <pebble.h>

#define MAX_ITEMS 30
#define MAX_TITLE_LEN 32
#define MAX_SUBTITLE_LEN 32
#define MAX_EXTRA_LEN 128

typedef struct {
  char title[MAX_TITLE_LEN];
  char subtitle[MAX_SUBTITLE_LEN];
  char extra[MAX_EXTRA_LEN];
} DataItem;

typedef struct {
  DataItem items[MAX_ITEMS];
  int count;
} DataList;

extern DataList g_calendar_data;
extern DataList g_driver_data;
extern DataList g_team_data;

void data_init(void);
void data_add_item(int type, int index, char *title, char *subtitle,
                   char *extra);
void data_set_count(int type, int count);
DataList *data_get_list(int type);
