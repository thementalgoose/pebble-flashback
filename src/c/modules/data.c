#include "data.h"
#include "comm.h"

DataList g_calendar_data;
DataList g_driver_data;
DataList g_team_data;

void data_init(void) {
  g_calendar_data.count = 0;
  g_driver_data.count = 0;
  g_team_data.count = 0;
}

DataList *data_get_list(int type) {
  switch (type) {
  case REQUEST_TYPE_CALENDAR:
    return &g_calendar_data;
  case REQUEST_TYPE_DRIVERS:
    return &g_driver_data;
  case REQUEST_TYPE_TEAMS:
    return &g_team_data;
  default:
    return NULL;
  }
}

void data_set_count(int type, int count) {
  DataList *list = data_get_list(type);
  if (list) {
    list->count = count;
  }
}

void data_add_item(int type, int index, char *title, char *subtitle,
                   char *extra) {
  DataList *list = data_get_list(type);
  if (list && index < MAX_ITEMS) {
    strncpy(list->items[index].title, title, MAX_TITLE_LEN - 1);
    strncpy(list->items[index].subtitle, subtitle, MAX_SUBTITLE_LEN - 1);
    strncpy(list->items[index].extra, extra, MAX_EXTRA_LEN - 1);
  }
}
