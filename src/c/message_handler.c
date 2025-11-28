#include "message_handler.h"
#include <pebble.h>

// Callback storage
static OverviewDataCallback s_overview_data_callback = NULL;
static OverviewCompleteCallback s_overview_complete_callback = NULL;
static RaceDetailsDataCallback s_race_details_data_callback = NULL;
static RaceDetailsCompleteCallback s_race_details_complete_callback = NULL;
static DriverStandingsDataCallback s_driver_standings_data_callback = NULL;
static DriverStandingsCompleteCallback s_driver_standings_complete_callback = NULL;
static TeamStandingsDataCallback s_team_standings_data_callback = NULL;
static TeamStandingsCompleteCallback s_team_standings_complete_callback = NULL;

// Message received handler
static void inbox_received_callback(DictionaryIterator *iterator,
                                    void *context) {
  // Read request type
  Tuple *request_type_tuple = dict_find(iterator, MESSAGE_KEY_REQUEST_TYPE);
  if (!request_type_tuple) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "No request type in message");
    return;
  }

  int request_type = request_type_tuple->value->int32;

  // Read other fields
  Tuple *index_tuple = dict_find(iterator, MESSAGE_KEY_DATA_INDEX);
  Tuple *count_tuple = dict_find(iterator, MESSAGE_KEY_DATA_COUNT);
  Tuple *title_tuple = dict_find(iterator, MESSAGE_KEY_DATA_TITLE);
  Tuple *subtitle_tuple = dict_find(iterator, MESSAGE_KEY_DATA_SUBTITLE);
  Tuple *extra_tuple = dict_find(iterator, MESSAGE_KEY_DATA_EXTRA);

  switch (request_type) {
  case REQUEST_TYPE_GET_OVERVIEW:
    if (count_tuple) {
      // This is the count message
      int count = count_tuple->value->int32;
      APP_LOG(APP_LOG_LEVEL_INFO, "Received overview count: %d", count);
      if (s_overview_complete_callback) {
        s_overview_complete_callback(count);
      }
    } else if (index_tuple && title_tuple && subtitle_tuple) {
      // This is a data message
      int index = index_tuple->value->int32;
      const char *title = title_tuple->value->cstring;
      const char *subtitle = subtitle_tuple->value->cstring;
      const char *extra = extra_tuple ? extra_tuple->value->cstring : "";

      // Read round number
      Tuple *round_tuple = dict_find(iterator, MESSAGE_KEY_DATA_ROUND);
      int round = round_tuple ? round_tuple->value->int32 : 0;

      APP_LOG(APP_LOG_LEVEL_INFO, "Received race %d: %s (round %d)", index, title, round);

      if (s_overview_data_callback) {
        s_overview_data_callback(index, title, subtitle, extra, round);
      }
    }
    break;

  case REQUEST_TYPE_GET_RACE_DETAILS:
    if (count_tuple) {
      // This is the count message
      int count = count_tuple->value->int32;
      APP_LOG(APP_LOG_LEVEL_INFO, "Received race details count: %d", count);
      if (s_race_details_complete_callback) {
        s_race_details_complete_callback(count);
      }
    } else if (index_tuple && title_tuple && subtitle_tuple) {
      // This is a data message
      int index = index_tuple->value->int32;
      const char *title = title_tuple->value->cstring;
      const char *subtitle = subtitle_tuple->value->cstring;
      const char *extra = extra_tuple ? extra_tuple->value->cstring : "";

      APP_LOG(APP_LOG_LEVEL_INFO, "Received event %d: %s", index, title);

      if (s_race_details_data_callback) {
        s_race_details_data_callback(index, title, subtitle, extra);
      }
    }
    break;

  case REQUEST_TYPE_GET_DRIVER_STANDINGS:
    if (count_tuple) {
      // This is the count message
      int count = count_tuple->value->int32;
      APP_LOG(APP_LOG_LEVEL_INFO, "Received driver standings count: %d", count);
      if (s_driver_standings_complete_callback) {
        s_driver_standings_complete_callback(count);
      }
    } else if (index_tuple && title_tuple && subtitle_tuple) {
      // This is a data message
      int index = index_tuple->value->int32;
      const char *name = title_tuple->value->cstring;
      const char *code = subtitle_tuple->value->cstring;

      // Read points and position
      Tuple *points_tuple = dict_find(iterator, MESSAGE_KEY_DATA_POINTS);
      Tuple *position_tuple = dict_find(iterator, MESSAGE_KEY_DATA_POSITION);
      int points = points_tuple ? points_tuple->value->int32 : 0;
      int position = position_tuple ? position_tuple->value->int32 : 0;

      APP_LOG(APP_LOG_LEVEL_INFO, "Received driver %d: %s (%s) - %d pts, P%d",
              index, name, code, points, position);

      if (s_driver_standings_data_callback) {
        s_driver_standings_data_callback(index, name, code, points, position);
      }
    }
    break;

  case REQUEST_TYPE_GET_TEAM_STANDINGS:
    if (count_tuple) {
      // This is the count message
      int count = count_tuple->value->int32;
      APP_LOG(APP_LOG_LEVEL_INFO, "Received team standings count: %d", count);
      if (s_team_standings_complete_callback) {
        s_team_standings_complete_callback(count);
      }
    } else if (index_tuple && title_tuple) {
      // This is a data message
      int index = index_tuple->value->int32;
      const char *name = title_tuple->value->cstring;

      // Read points and position
      Tuple *points_tuple = dict_find(iterator, MESSAGE_KEY_DATA_POINTS);
      Tuple *position_tuple = dict_find(iterator, MESSAGE_KEY_DATA_POSITION);
      int points = points_tuple ? points_tuple->value->int32 : 0;
      int position = position_tuple ? position_tuple->value->int32 : 0;

      APP_LOG(APP_LOG_LEVEL_INFO, "Received team %d: %s - %d pts, P%d",
              index, name, points, position);

      if (s_team_standings_data_callback) {
        s_team_standings_data_callback(index, name, points, position);
      }
    }
    break;

  default:
    APP_LOG(APP_LOG_LEVEL_WARNING, "Unknown request type: %d",
            (int)request_type);
    break;
  }
}

// Message dropped handler
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", (int)reason);
}

// Outbox sent handler
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Message sent successfully");
}

// Outbox failed handler
static void outbox_failed_callback(DictionaryIterator *iterator,
                                   AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message send failed: %d", (int)reason);
}

void message_handler_init(void) {
  // Open AppMessage with appropriate buffer sizes
  app_message_open(512, 512);

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_register_outbox_failed(outbox_failed_callback);

  APP_LOG(APP_LOG_LEVEL_INFO, "Message handler initialized");
}

void message_handler_deinit(void) { app_message_deregister_callbacks(); }

void message_handler_request_overview(void) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);

  if (result == APP_MSG_OK) {
    dict_write_uint8(iter, MESSAGE_KEY_REQUEST_TYPE, REQUEST_TYPE_GET_OVERVIEW);
    result = app_message_outbox_send();

    if (result == APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Requested overview data");
    } else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to send overview request: %d",
              (int)result);
    }
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to begin overview request: %d",
            (int)result);
  }
}

void message_handler_request_race_details(int race_index) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);

  if (result == APP_MSG_OK) {
    dict_write_uint8(iter, MESSAGE_KEY_REQUEST_TYPE,
                     REQUEST_TYPE_GET_RACE_DETAILS);
    dict_write_int32(iter, MESSAGE_KEY_DATA_INDEX, race_index);
    result = app_message_outbox_send();

    if (result == APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Requested race details for index %d",
              race_index);
    } else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to send race details request: %d",
              (int)result);
    }
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to begin race details request: %d",
            (int)result);
  }
}

void message_handler_set_overview_callbacks(
    OverviewDataCallback data_cb, OverviewCompleteCallback complete_cb) {
  s_overview_data_callback = data_cb;
  s_overview_complete_callback = complete_cb;
}

void message_handler_set_race_details_callbacks(
    RaceDetailsDataCallback data_cb, RaceDetailsCompleteCallback complete_cb) {
  s_race_details_data_callback = data_cb;
  s_race_details_complete_callback = complete_cb;
}

void message_handler_request_driver_standings(void) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);

  if (result == APP_MSG_OK) {
    dict_write_uint8(iter, MESSAGE_KEY_REQUEST_TYPE,
                     REQUEST_TYPE_GET_DRIVER_STANDINGS);
    result = app_message_outbox_send();

    if (result == APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Requested driver standings");
    } else {
      APP_LOG(APP_LOG_LEVEL_ERROR,
              "Failed to send driver standings request: %d", (int)result);
    }
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Failed to begin driver standings request: %d", (int)result);
  }
}

void message_handler_request_team_standings(void) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);

  if (result == APP_MSG_OK) {
    dict_write_uint8(iter, MESSAGE_KEY_REQUEST_TYPE,
                     REQUEST_TYPE_GET_TEAM_STANDINGS);
    result = app_message_outbox_send();

    if (result == APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Requested team standings");
    } else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to send team standings request: %d",
              (int)result);
    }
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to begin team standings request: %d",
            (int)result);
  }
}

void message_handler_set_driver_standings_callbacks(
    DriverStandingsDataCallback data_cb,
    DriverStandingsCompleteCallback complete_cb) {
  s_driver_standings_data_callback = data_cb;
  s_driver_standings_complete_callback = complete_cb;
}

void message_handler_set_team_standings_callbacks(
    TeamStandingsDataCallback data_cb, TeamStandingsCompleteCallback complete_cb) {
  s_team_standings_data_callback = data_cb;
  s_team_standings_complete_callback = complete_cb;
}
