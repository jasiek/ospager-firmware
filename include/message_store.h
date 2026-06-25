// In-memory ring buffer of the most recently received pager messages.
#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <time.h>

struct PagerMessage {
  uint32_t capcode = 0;
  time_t   epoch = 0;     // RTC time when received (0 if clock not yet synced)
  String   text;
};

class MessageStore {
 public:
  static constexpr size_t CAP = 16;

  void add(uint32_t capcode, time_t epoch, const String& text);

  // Remove the message at `index` (0 == newest), compacting the ring so the
  // remaining messages keep their order. No-op if index is out of range.
  void remove(size_t index);

  size_t count() const { return count_; }
  // index 0 == newest. Returns a reference into the ring; valid until next add.
  const PagerMessage& get(size_t index) const;

  // Bumps on every add() so the UI can detect new traffic cheaply.
  uint32_t revision() const { return revision_; }

 private:
  // Physical buffer slot holding logical `index` (0 == newest).
  size_t slotOf(size_t index) const {
    size_t newest = (head_ + CAP - 1) % CAP;
    return (newest + CAP - (index % CAP)) % CAP;
  }

  PagerMessage buf_[CAP];
  size_t   head_ = 0;       // index of the next slot to write
  size_t   count_ = 0;
  uint32_t revision_ = 0;
};
