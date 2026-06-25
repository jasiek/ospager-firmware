#include "message_store.h"

void MessageStore::add(uint32_t capcode, time_t epoch, const String& text) {
  PagerMessage& slot = buf_[head_];
  slot.capcode = capcode;
  slot.epoch = epoch;
  slot.text = text;
  head_ = (head_ + 1) % CAP;
  if (count_ < CAP) count_++;
  revision_++;
}

const PagerMessage& MessageStore::get(size_t index) const {
  // index 0 == newest == the slot just before head_.
  size_t newest = (head_ + CAP - 1) % CAP;
  size_t pos = (newest + CAP - (index % CAP)) % CAP;
  return buf_[pos];
}
