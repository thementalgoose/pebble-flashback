#pragma once

#include <pebble.h>

// Create and show the race results window
void results_window_push(int race_round);

// Destroy the race results window
void results_window_destroy(void);
