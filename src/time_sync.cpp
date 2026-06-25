#include "time_sync.h"

#include <Arduino.h>
#include <sys/time.h>

const char* const DISPLAY_TZ = "CET-1CEST,M3.5.0,M10.5.0/3";

// Convert a UTC calendar time to a Unix epoch, independent of the current TZ
// (newlib here has no timegm()). Howard Hinnant's days_from_civil algorithm.
static time_t timegmUtc(const struct tm& t) {
  int year  = t.tm_year + 1900;
  int month = t.tm_mon + 1;                  // 1..12
  year -= (month <= 2);
  int era = (year >= 0 ? year : year - 399) / 400;
  unsigned yoe = (unsigned)(year - era * 400);
  unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + t.tm_mday - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  long days = (long)era * 146097 + (long)doe - 719468;   // days since 1970-01-01
  long long secs = (long long)days * 86400LL +
                   t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
  return (time_t)secs;
}

void Clock::begin() {
  setenv("TZ", DISPLAY_TZ, 1);
  tzset();
}

void Clock::apply(time_t epoch, uint32_t capcode) {
  struct timeval tv = {.tv_sec = epoch, .tv_usec = 0};
  settimeofday(&tv, nullptr);
  synced_ = true;
  lastSyncMs_ = millis();
  lastCapcode_ = capcode;
}

void Clock::syncFromUtcFields(const struct tm& fields, uint32_t capcode) {
  struct tm tmp = fields;
  apply(timegmUtc(tmp), capcode);
}

void Clock::syncFromLocalFields(const struct tm& fields, uint32_t capcode) {
  struct tm tmp = fields;
  tmp.tm_isdst = -1;            // let mktime resolve DST under DISPLAY_TZ
  apply(mktime(&tmp), capcode);
}

time_t Clock::now() const { return time(nullptr); }

void Clock::localNow(struct tm& out) const {
  time_t t = now();
  localtime_r(&t, &out);
}

uint32_t Clock::secondsSinceSync() const {
  if (!synced_) return 0;
  return (millis() - lastSyncMs_) / 1000UL;
}
