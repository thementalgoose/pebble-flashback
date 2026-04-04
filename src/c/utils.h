#pragma once

#include <pebble.h>

// Format datetime from ISO string to readable format
// Input: "2025-03-14T01:30:00Z"
// Output: "Mar 14, 1:30 AM" (in local timezone)
void utils_format_datetime(const char *iso_datetime, char *output,
                           size_t output_size);

// Format date from ISO string
// Input: "2025-03-14"
// Output: "Mar 14, 2025"
void utils_format_date(const char *iso_date, char *output, size_t output_size);

// Get month abbreviation
const char *utils_get_month_abbr(int month);

// Format datetime from ISO string to compact form
// Input: "2025-07-12T18:30:00Z"
// Output: "12/07 18:30"
void utils_format_datetime_compact(const char *iso_datetime, char *output,
                                   size_t output_size);
