#pragma once

#include <pebble.h>

// Request types (must match JS)
typedef enum {
  REQUEST_TYPE_GET_OVERVIEW = 1,
  REQUEST_TYPE_GET_RACE_DETAILS = 2,
  REQUEST_TYPE_GET_DRIVER_STANDINGS = 3,
  REQUEST_TYPE_GET_TEAM_STANDINGS = 4
} RequestType;

// Callback types for different data
typedef void (*OverviewDataCallback)(int index, const char *title,
                                     const char *subtitle, const char *extra);
typedef void (*OverviewCompleteCallback)(int count);
typedef void (*RaceDetailsDataCallback)(int index, const char *title,
                                        const char *subtitle,
                                        const char *extra);
typedef void (*RaceDetailsCompleteCallback)(int count);

// Initialize message handler
void message_handler_init(void);

// Deinitialize message handler
void message_handler_deinit(void);

// Request data from JS
void message_handler_request_overview(void);
void message_handler_request_race_details(int race_index);

// Register callbacks
void message_handler_set_overview_callbacks(
    OverviewDataCallback data_cb, OverviewCompleteCallback complete_cb);
void message_handler_set_race_details_callbacks(
    RaceDetailsDataCallback data_cb, RaceDetailsCompleteCallback complete_cb);
