#include "utils.h"
#include <pebble.h>

const char *utils_get_month_abbr(int month) {
  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  if (month >= 1 && month <= 12) {
    return months[month - 1];
  }
  return "???";
}

// Simple manual string parsing to avoid sscanf
static int parse_int(const char *str, int *pos, int len) {
  int val = 0;
  for (int i = 0; i < len && str[*pos] >= '0' && str[*pos] <= '9';
       i++, (*pos)++) {
    val = val * 10 + (str[*pos] - '0');
  }
  return val;
}

void utils_format_datetime(const char *iso_datetime, char *output,
                           size_t output_size) {
  if (!iso_datetime || !output || output_size < 20) {
    return;
  }

  // Parse ISO datetime manually: "2025-03-14T01:30:00Z"
  int pos = 0;
  parse_int(iso_datetime, &pos, 4); // year (not needed for display)
  pos++;                            // skip '-'
  int month = parse_int(iso_datetime, &pos, 2);
  pos++; // skip '-'
  int day = parse_int(iso_datetime, &pos, 2);
  pos++; // skip 'T'
  int hour = parse_int(iso_datetime, &pos, 2);
  pos++; // skip ':'
  int minute = parse_int(iso_datetime, &pos, 2);

  // Format manually
  const char *am_pm = hour >= 12 ? "PM" : "AM";
  int display_hour = hour % 12;
  if (display_hour == 0)
    display_hour = 12;

  // Build string manually to avoid snprintf
  const char *month_abbr = utils_get_month_abbr(month);
  size_t out_pos = 0;

  // Copy month abbreviation
  for (size_t i = 0; month_abbr[i] && out_pos < output_size - 1; i++) {
    output[out_pos++] = month_abbr[i];
  }
  output[out_pos++] = ' ';

  // Day
  if (day >= 10)
    output[out_pos++] = '0' + (day / 10);
  output[out_pos++] = '0' + (day % 10);
  output[out_pos++] = ',';
  output[out_pos++] = ' ';

  // Hour
  if (display_hour >= 10)
    output[out_pos++] = '0' + (display_hour / 10);
  output[out_pos++] = '0' + (display_hour % 10);
  output[out_pos++] = ':';

  // Minute
  output[out_pos++] = '0' + (minute / 10);
  output[out_pos++] = '0' + (minute % 10);
  output[out_pos++] = ' ';

  // AM/PM
  output[out_pos++] = am_pm[0];
  output[out_pos++] = am_pm[1];

  output[out_pos] = '\0';
}

void utils_format_date(const char *iso_date, char *output, size_t output_size) {
  if (!iso_date || !output || output_size < 15) {
    return;
  }

  // Parse ISO date manually: "2025-03-14"
  int pos = 0;
  int year = parse_int(iso_date, &pos, 4);
  pos++; // skip '-'
  int month = parse_int(iso_date, &pos, 2);
  pos++; // skip '-'
  int day = parse_int(iso_date, &pos, 2);

  // Format manually
  const char *month_abbr = utils_get_month_abbr(month);
  size_t out_pos = 0;

  // Month
  for (size_t i = 0; month_abbr[i] && out_pos < output_size - 1; i++) {
    output[out_pos++] = month_abbr[i];
  }
  output[out_pos++] = ' ';

  // Day
  if (day >= 10)
    output[out_pos++] = '0' + (day / 10);
  output[out_pos++] = '0' + (day % 10);
  output[out_pos++] = ',';
  output[out_pos++] = ' ';

  // Year
  output[out_pos++] = '0' + (year / 1000);
  output[out_pos++] = '0' + ((year / 100) % 10);
  output[out_pos++] = '0' + ((year / 10) % 10);
  output[out_pos++] = '0' + (year % 10);

  output[out_pos] = '\0';
}

void utils_format_datetime_compact(const char *iso_datetime, char *output,
                                   size_t output_size) {
  if (!iso_datetime || !output || output_size < 12) {
    return;
  }

  // Parse ISO datetime: "2025-07-12T18:30:00Z"
  int pos = 0;
  parse_int(iso_datetime, &pos, 4); // year (unused)
  pos++;                             // skip '-'
  int month = parse_int(iso_datetime, &pos, 2);
  pos++;                             // skip '-'
  int day = parse_int(iso_datetime, &pos, 2);
  pos++;                             // skip 'T'
  int hour = parse_int(iso_datetime, &pos, 2);
  pos++;                             // skip ':'
  int minute = parse_int(iso_datetime, &pos, 2);

  snprintf(output, output_size, "%02d/%02d %02d:%02d", day, month, hour, minute);
}