// RTC / clock service. The ESP32 system clock is kept in UTC; callers read it
// back as local time via the configured POSIX timezone (DST-aware).
#pragma once

#include <stdint.h>
#include <time.h>

class Clock {
 public:
  // Configure the display timezone. Call once at startup.
  void begin();

  // Sync from a broadcast whose fields are LOCAL wall time (resolves DST).
  void syncFromLocalFields(const struct tm& fields, uint32_t capcode);
  // Sync from a broadcast whose fields are UTC.
  void syncFromUtcFields(const struct tm& fields, uint32_t capcode);

  bool     isSynced() const { return synced_; }
  time_t   now() const;                       // epoch seconds (UTC)
  void     localNow(struct tm& out) const;    // current time in DISPLAY_TZ
  uint32_t secondsSinceSync() const;          // wall time since last sync
  uint32_t lastCapcode() const { return lastCapcode_; }

 private:
  void apply(time_t epoch, uint32_t capcode);

  bool     synced_ = false;
  uint32_t lastSyncMs_ = 0;
  uint32_t lastCapcode_ = 0;
};

// POSIX timezone used for display (Central European w/ EU DST by default).
extern const char* const DISPLAY_TZ;
