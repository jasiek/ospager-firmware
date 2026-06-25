// Surface backend that renders the FrameBuffer to an ANSI/VT100 terminal over
// a serial stream. A fixed border frames the grid; only changed cells are
// transmitted (per-cell diffing) so updates stay cheap at 115200 baud.
#pragma once

#include <stdint.h>

#include "ui/surface.h"

class Print;

class SerialSurface : public Surface {
 public:
  explicit SerialSurface(Print& out) : out_(out) {}

  // Clear the terminal, draw the frame, hide the cursor, force a full repaint.
  void begin();

  void render(const FrameBuffer& fb) override;

 private:
  void moveTo(uint8_t termRow, uint8_t termCol);
  void emitStyle(uint8_t style);

  Print& out_;
  Cell    shadow_[UI_ROWS][UI_COLS];
  bool    full_ = true;       // force a full repaint on the next render
  uint8_t curStyle_ = 0xFF;   // last SGR state emitted (0xFF = unknown)
};
