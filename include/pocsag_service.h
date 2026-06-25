// POCSAG receive service: owns the radio + pager, decodes frames, syncs the
// clock from time broadcasts, and records every message in the store.
#pragma once

#include <stdint.h>

class Clock;
class MessageStore;

class PocsagService {
 public:
  PocsagService(Clock& clock, MessageStore& store);

  // Initialise the SX1278 in FSK/direct mode and start receiving. Returns false
  // on radio failure (with a reason printed to Serial before the UI starts).
  bool begin();

  // Drain any decoded frames. Call frequently from loop().
  void poll();

 private:
  Clock& clock_;
  MessageStore& store_;
};
