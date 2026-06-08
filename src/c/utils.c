#include "utils.h"
#include <pebble.h>
#include <stdint.h>

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

static bool parse_iso_datetime(const char *iso_datetime, int *year, int *month,
                               int *day, int *hour, int *minute,
                               int *second) {
  if (!iso_datetime || !year || !month || !day || !hour || !minute || !second) {
    return false;
  }

  int pos = 0;
  *year = parse_int(iso_datetime, &pos, 4);
  if (iso_datetime[pos] != '-') return false;
  pos++;
  *month = parse_int(iso_datetime, &pos, 2);
  if (iso_datetime[pos] != '-') return false;
  pos++;
  *day = parse_int(iso_datetime, &pos, 2);
  if (iso_datetime[pos] != 'T') return false;
  pos++;
  *hour = parse_int(iso_datetime, &pos, 2);
  if (iso_datetime[pos] != ':') return false;
  pos++;
  *minute = parse_int(iso_datetime, &pos, 2);
  *second = 0;

  if (iso_datetime[pos] == ':') {
    pos++;
    *second = parse_int(iso_datetime, &pos, 2);
  }

  return true;
}

static bool iso_datetime_to_epoch_utc(const char *iso_datetime, time_t *output) {
  if (!iso_datetime || !output) {
    return false;
  }

  int year, month, day, hour, minute, second;
  if (!parse_iso_datetime(iso_datetime, &year, &month, &day, &hour, &minute, &second)) {
    return false;
  }

  int64_t y = year;
  int64_t m = month;
  int64_t d = day;
  int64_t H = hour;
  int64_t M = minute;
  int64_t S = second;

  if (m <= 2) {
    y -= 1;
    m += 12;
  }

  int64_t era = y / 400;
  int64_t yoe = y - era * 400;
  int64_t doy = (153 * (m - 3) + 2) / 5 + d - 1;
  int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  int64_t days_since_epoch = era * 146097 + doe - 719468;
  int64_t seconds_since_epoch = days_since_epoch * 86400 + H * 3600 + M * 60 + S;

  *output = (time_t)seconds_since_epoch;
  return true;
}

void utils_format_datetime(const char *iso_datetime, char *output,
                           size_t output_size) {
  if (!iso_datetime || !output || output_size < 20) {
    return;
  }

  time_t utc_epoch;
  if (!iso_datetime_to_epoch_utc(iso_datetime, &utc_epoch)) {
    output[0] = '\0';
    return;
  }

  struct tm *local_tm = localtime(&utc_epoch);
  if (!local_tm) {
    output[0] = '\0';
    return;
  }

  int month = local_tm->tm_mon + 1;
  int day = local_tm->tm_mday;
  int hour = local_tm->tm_hour;
  int minute = local_tm->tm_min;
  const char *am_pm = hour >= 12 ? "PM" : "AM";
  int display_hour = hour % 12;
  if (display_hour == 0) {
    display_hour = 12;
  }

  const char *month_abbr = utils_get_month_abbr(month);
  size_t out_pos = 0;

  for (size_t i = 0; month_abbr[i] && out_pos < output_size - 1; i++) {
    output[out_pos++] = month_abbr[i];
  }
  output[out_pos++] = ' ';

  if (day >= 10) {
    output[out_pos++] = '0' + (day / 10);
  }
  output[out_pos++] = '0' + (day % 10);
  output[out_pos++] = ',';
  output[out_pos++] = ' ';

  if (display_hour >= 10) {
    output[out_pos++] = '0' + (display_hour / 10);
  }
  output[out_pos++] = '0' + (display_hour % 10);
  output[out_pos++] = ':';
  output[out_pos++] = '0' + (minute / 10);
  output[out_pos++] = '0' + (minute % 10);
  output[out_pos++] = ' ';
  output[out_pos++] = am_pm[0];
  output[out_pos++] = am_pm[1];
  output[out_pos] = '\0';
}

void utils_format_datetime_preferred(const char *iso_datetime, char *output,
                                     size_t output_size) {
  if (!iso_datetime || !output || output_size < 20) {
    return;
  }

  time_t utc_epoch;
  if (!iso_datetime_to_epoch_utc(iso_datetime, &utc_epoch)) {
    output[0] = '\0';
    return;
  }

  struct tm *local_tm = localtime(&utc_epoch);
  if (!local_tm) {
    output[0] = '\0';
    return;
  }

  int month = local_tm->tm_mon + 1;
  int day = local_tm->tm_mday;
  int hour = local_tm->tm_hour;
  int minute = local_tm->tm_min;
  const char *month_abbr = utils_get_month_abbr(month);
  size_t out_pos = 0;

  for (size_t i = 0; month_abbr[i] && out_pos < output_size - 1; i++) {
    output[out_pos++] = month_abbr[i];
  }
  output[out_pos++] = ' ';

  if (day >= 10) {
    output[out_pos++] = '0' + (day / 10);
  }
  output[out_pos++] = '0' + (day % 10);
  output[out_pos++] = ',';
  output[out_pos++] = ' ';

  if (clock_is_24h_style()) {
    output[out_pos++] = '0' + (hour / 10);
    output[out_pos++] = '0' + (hour % 10);
  } else {
    const char *am_pm = hour >= 12 ? "PM" : "AM";
    int display_hour = hour % 12;
    if (display_hour == 0) {
      display_hour = 12;
    }

    if (display_hour >= 10) {
      output[out_pos++] = '0' + (display_hour / 10);
    }
    output[out_pos++] = '0' + (display_hour % 10);
    output[out_pos++] = ' ';
    output[out_pos++] = am_pm[0];
    output[out_pos++] = am_pm[1];
  }

  output[out_pos++] = ':';
  output[out_pos++] = '0' + (minute / 10);
  output[out_pos++] = '0' + (minute % 10);
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

  time_t utc_epoch;
  if (!iso_datetime_to_epoch_utc(iso_datetime, &utc_epoch)) {
    output[0] = '\0';
    return;
  }

  struct tm *local_tm = localtime(&utc_epoch);
  if (!local_tm) {
    output[0] = '\0';
    return;
  }

  int month = local_tm->tm_mon + 1;
  int day = local_tm->tm_mday;
  int hour = local_tm->tm_hour;
  int minute = local_tm->tm_min;

  snprintf(output, output_size, "%02d/%02d %02d:%02d", day, month, hour, minute);
}

void utils_format_datetime_large(const char *iso_datetime, char *output,
                                 size_t output_size) {
  if (!iso_datetime || !output || output_size < 14) {
    return;
  }

  time_t utc_epoch;
  if (!iso_datetime_to_epoch_utc(iso_datetime, &utc_epoch)) {
    output[0] = '\0';
    return;
  }

  struct tm *local_tm = localtime(&utc_epoch);
  if (!local_tm) {
    output[0] = '\0';
    return;
  }

  int month = local_tm->tm_mon + 1;
  int day = local_tm->tm_mday;
  int hour = local_tm->tm_hour;
  int minute = local_tm->tm_min;
  const char *month_abbr = utils_get_month_abbr(month);

  snprintf(output, output_size, "%s %d, %02d:%02d",
           month_abbr, day, hour, minute);
}
