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

void MessageStore::remove(size_t index) {
  if (index >= count_) return;
  // Slide every message newer than `index` one slot toward the older end,
  // overwriting the deleted slot: logical j takes the contents of logical j-1.
  for (size_t j = index; j > 0; --j) {
    buf_[slotOf(j)] = buf_[slotOf(j - 1)];
  }
  // The old newest slot is now a duplicate; pull head_ back onto it and clear
  // it so the String's heap is released.
  head_ = (head_ + CAP - 1) % CAP;
  buf_[head_] = PagerMessage{};
  count_--;
  revision_++;
}

const PagerMessage& MessageStore::get(size_t index) const {
  // index 0 == newest == the slot just before head_.
  return buf_[slotOf(index)];
}
