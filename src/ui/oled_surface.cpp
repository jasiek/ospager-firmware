#include "ui/oled_surface.h"

#include <Arduino.h>
#include <Wire.h>

// 6x8 character cells using the classic Adafruit GFX font.
static constexpr int CELL_W = 6;
static constexpr int CELL_H = 8;

bool OledSurface::begin() {
  // reset = false (no reset pin), periphBegin = false (Wire already started).
  if (!display_.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, false, false)) {
    return false;
  }
  display_.clearDisplay();
  display_.display();
  full_ = true;
  return true;
}

void OledSurface::render(const FrameBuffer& fb) {
  // Diff against the shadow; only touch the panel when something changed.
  bool changed = full_;
  for (uint8_t r = 0; r < UI_ROWS && !changed; ++r) {
    for (uint8_t c = 0; c < UI_COLS; ++c) {
      if (fb.at(c, r) != shadow_[r][c]) {
        changed = true;
        break;
      }
    }
  }
  if (!changed) return;

  display_.clearDisplay();
  for (uint8_t r = 0; r < UI_ROWS; ++r) {
    for (uint8_t c = 0; c < UI_COLS; ++c) {
      const Cell& cell = fb.at(c, r);
      shadow_[r][c] = cell;
      bool inv = cell.style & STYLE_INVERSE;
      uint16_t fg = inv ? SSD1306_BLACK : SSD1306_WHITE;
      uint16_t bg = inv ? SSD1306_WHITE : SSD1306_BLACK;
      display_.drawChar(c * CELL_W, r * CELL_H, cell.ch, fg, bg, 1);
    }
  }
  display_.display();
  full_ = false;
}
