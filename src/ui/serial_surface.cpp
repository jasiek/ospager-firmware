#include "ui/serial_surface.h"

#include <Arduino.h>

// The grid is framed by a 1-char border, so content cell (col,row) maps to
// terminal (1-based) row = row + 2, col = col + 2.
static constexpr uint8_t CONTENT_ROW0 = 2;
static constexpr uint8_t CONTENT_COL0 = 2;

void SerialSurface::moveTo(uint8_t termRow, uint8_t termCol) {
  out_.printf("\033[%u;%uH", termRow, termCol);
}

void SerialSurface::emitStyle(uint8_t style) {
  if (style == curStyle_) return;
  out_.print("\033[0");
  if (style & STYLE_BOLD)    out_.print(";1");
  if (style & STYLE_DIM)     out_.print(";2");
  if (style & STYLE_INVERSE) out_.print(";7");
  out_.print("m");
  curStyle_ = style;
}

void SerialSurface::begin() {
  out_.print("\033[?25l");   // hide cursor
  out_.print("\033[2J");     // clear screen
  curStyle_ = 0xFF;
  emitStyle(STYLE_NORMAL);

  // Top border (with a small title) and bottom border.
  moveTo(1, 1);
  out_.print('+');
  for (uint8_t i = 0; i < UI_COLS; ++i) out_.print('-');
  out_.print('+');
  moveTo(1, 3);
  out_.print(" ospager ");

  moveTo(UI_ROWS + CONTENT_ROW0, 1);
  out_.print('+');
  for (uint8_t i = 0; i < UI_COLS; ++i) out_.print('-');
  out_.print('+');

  // Side walls.
  for (uint8_t r = 0; r < UI_ROWS; ++r) {
    moveTo(r + CONTENT_ROW0, 1);
    out_.print('|');
    moveTo(r + CONTENT_ROW0, UI_COLS + CONTENT_COL0);
    out_.print('|');
  }

  full_ = true;
}

void SerialSurface::render(const FrameBuffer& fb) {
  for (uint8_t r = 0; r < UI_ROWS; ++r) {
    for (uint8_t c = 0; c < UI_COLS; ++c) {
      const Cell& cell = fb.at(c, r);
      if (!full_ && cell == shadow_[r][c]) continue;
      shadow_[r][c] = cell;
      moveTo(r + CONTENT_ROW0, c + CONTENT_COL0);
      emitStyle(cell.style);
      out_.write((uint8_t)cell.ch);
    }
  }
  emitStyle(STYLE_NORMAL);
  moveTo(UI_ROWS + CONTENT_ROW0 + 1, 1);   // park cursor below the frame
  full_ = false;
}
