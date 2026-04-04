#pragma once
#include <pebble.h>

// Creates a full-screen MenuLayer on window with standard Flashback styling.
// Configures click-config, PBL_ROUND centre-focus, and highlight colours,
// and adds the layer to the window's root layer.
// The caller is responsible for setting callbacks and calling menu_layer_destroy.
MenuLayer *flashback_screen_create_menu_layer(Window *window);

// Draws the standard Flashback header: APP_TITLE centred at the top, a divider
// line, then subtitle_left on the left. When subtitle_right is non-NULL it is
// drawn right-aligned; otherwise subtitle_left spans the full available width.
void flashback_screen_draw_header(GContext *ctx, const Layer *cell_layer,
                                  const char *subtitle_left,
                                  const char *subtitle_right);

// Standard MenuLayer callbacks that screens can reference directly in a
// MenuLayerCallbacks struct when they don't need custom behaviour.
int16_t flashback_screen_cell_height_callback(struct MenuLayer *menu_layer,
                                              MenuIndex *cell_index,
                                              void *context);
int16_t flashback_screen_header_height_callback(struct MenuLayer *menu_layer,
                                                uint16_t section_index,
                                                void *context);
uint16_t flashback_screen_num_sections_callback(struct MenuLayer *menu_layer,
                                                void *context);
