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
                                     const char *subtitle, const char *extra,
                                     int round);
typedef void (*OverviewCompleteCallback)(int count);
typedef void (*RaceDetailsDataCallback)(int index, const char *title,
                                        const char *subtitle,
                                        const char *extra);
typedef void (*RaceDetailsCompleteCallback)(int count);
typedef void (*DriverStandingsDataCallback)(int index, const char *name,
                                            const char *code, int points,
                                            int position);
typedef void (*DriverStandingsCompleteCallback)(int count);
typedef void (*TeamStandingsDataCallback)(int index, const char *name,
                                          int points, int position);
typedef void (*TeamStandingsCompleteCallback)(int count);

// Initialize message handler
void message_handler_init(void);

// Deinitialize message handler
void message_handler_deinit(void);

// Request data from JS
void message_handler_request_overview(void);
void message_handler_request_race_details(int race_index);
void message_handler_request_driver_standings(void);
void message_handler_request_team_standings(void);

// Register callbacks
void message_handler_set_overview_callbacks(
    OverviewDataCallback data_cb, OverviewCompleteCallback complete_cb);
void message_handler_set_race_details_callbacks(
    RaceDetailsDataCallback data_cb, RaceDetailsCompleteCallback complete_cb);
void message_handler_set_driver_standings_callbacks(
    DriverStandingsDataCallback data_cb,
    DriverStandingsCompleteCallback complete_cb);
void message_handler_set_team_standings_callbacks(
    TeamStandingsDataCallback data_cb, TeamStandingsCompleteCallback complete_cb);
