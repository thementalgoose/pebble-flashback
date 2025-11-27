#pragma once
#include <pebble.h>

typedef enum {
  REQUEST_TYPE_CALENDAR = 0,
  REQUEST_TYPE_DRIVERS = 1,
  REQUEST_TYPE_TEAMS = 2
} RequestType;

typedef void (*DataChangedHandler)(void);

void comm_init(void);
void comm_request_data(RequestType type);
void comm_set_data_changed_handler(DataChangedHandler handler);
