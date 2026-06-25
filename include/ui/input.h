// Logical input events and a serial-keystroke decoder. The same InputEvent set
// will later be produced by GPIO buttons, so screens never see raw keys.
#pragma once

#include <stdint.h>

class Stream;

enum class InputEvent : uint8_t {
  None,
  Up,
  Down,
  Left,
  Right,
  Select,   // Enter / OK
  Back,     // Esc / cancel
  Redraw,   // Ctrl-L: force a full repaint (e.g. after reconnecting a terminal)
};

// Decodes a serial terminal's byte stream into InputEvents. Arrow keys (CSI
// sequences), Enter, Esc, and vim-style j/k/q are recognised.
class SerialInput {
 public:
  explicit SerialInput(Stream& in) : in_(in) {}

  // Read available bytes and queue decoded events. Call once per loop.
  void update();
  // Pop the next decoded event; returns false when the queue is empty.
  bool pop(InputEvent& ev);

 private:
  void push(InputEvent ev);

  Stream& in_;

  static constexpr uint8_t QN = 16;
  InputEvent queue_[QN];
  uint8_t qHead_ = 0, qTail_ = 0, qCount_ = 0;

  // CSI escape-sequence parser state.
  uint8_t esc_ = 0;   // 0 = normal, 1 = saw ESC, 2 = saw ESC[
};
