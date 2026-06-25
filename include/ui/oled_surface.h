// Surface backend that renders the FrameBuffer to an SSD1306 OLED. Each grid
// cell maps to a 6x8 character cell; the panel is redrawn only when a cell
// changes. This proves the same screens render to a graphic display.
#pragma once

#include <stdint.h>

#include <Adafruit_SSD1306.h>

#include "board.h"
#include "ui/surface.h"

class OledSurface : public Surface {
 public:
  // Initialise the panel. Returns false if no SSD1306 is present.
  bool begin();

  void render(const FrameBuffer& fb) override;

 private:
  Adafruit_SSD1306 display_{OLED_W, OLED_H, &Wire, -1};
  Cell shadow_[UI_ROWS][UI_COLS];
  bool full_ = true;
};
