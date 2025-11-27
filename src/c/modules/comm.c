#include "comm.h"
#include "data.h"

static DataChangedHandler s_data_changed_handler = NULL;

static void inbox_received_callback(DictionaryIterator *iterator,
                                    void *context) {
  Tuple *type_tuple = dict_find(iterator, MESSAGE_KEY_REQUEST_TYPE);
  Tuple *count_tuple = dict_find(iterator, MESSAGE_KEY_DATA_COUNT);
  Tuple *index_tuple = dict_find(iterator, MESSAGE_KEY_DATA_INDEX);
  Tuple *title_tuple = dict_find(iterator, MESSAGE_KEY_DATA_TITLE);
  Tuple *subtitle_tuple = dict_find(iterator, MESSAGE_KEY_DATA_SUBTITLE);
  Tuple *extra_tuple = dict_find(iterator, MESSAGE_KEY_DATA_EXTRA);

  if (type_tuple) {
    int type = type_tuple->value->int32;

    if (count_tuple) {
      data_set_count(type, count_tuple->value->int32);
      if (s_data_changed_handler) {
        s_data_changed_handler();
      }
      // Reload window if needed (using custom callback system if we had one)
      // For now, we rely on the UI updating when the user enters the menu
    }

    if (index_tuple && title_tuple) {
      char *extra = extra_tuple ? extra_tuple->value->cstring : "";
      char *subtitle = subtitle_tuple ? subtitle_tuple->value->cstring : "";

      data_add_item(type, index_tuple->value->int32,
                    title_tuple->value->cstring, subtitle, extra);

      if (s_data_changed_handler) {
        s_data_changed_handler();
      }
      // Refresh UI logic would go here
      // Since we don't have a global event system, we might need to expose a
      // callback For this MVP, we assume data loads fast enough or user
      // refreshes
      APP_LOG(APP_LOG_LEVEL_INFO, "Received item %d for type %d",
              (int)index_tuple->value->int32, type);
    }
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator,
                                   AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

void comm_init(void) {
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  const int inbox_size = 1024;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
}

void comm_set_data_changed_handler(DataChangedHandler handler) {
  s_data_changed_handler = handler;
}

void comm_request_data(RequestType type) {
  DictionaryIterator *out_iter;
  AppMessageResult result = app_message_outbox_begin(&out_iter);

  if (result == APP_MSG_OK) {
    dict_write_int(out_iter, MESSAGE_KEY_REQUEST_TYPE, &type, sizeof(int),
                   true);
    result = app_message_outbox_send();
    if (result != APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the outbox: %d", (int)result);
    }
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the outbox: %d", (int)result);
  }
}
