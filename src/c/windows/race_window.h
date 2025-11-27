#pragma once

#include <pebble.h>

// Create and show the race window for a specific race
void race_window_push(int race_index);

// Destroy the race window
void race_window_destroy(void);
