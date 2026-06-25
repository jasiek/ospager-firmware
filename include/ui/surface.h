// Core UI primitives: a character-cell FrameBuffer that screens draw into, and
// a Surface interface that each output backend (serial terminal, OLED, future
// LCD) implements to render that buffer.
#pragma once

#include <stdint.h>
#include <string.h>

#include "board.h"

// Cell attributes. Backends render what they can and ignore the rest.
enum CellStyle : uint8_t {
  STYLE_NORMAL  = 0,
  STYLE_BOLD    = 1 << 0,
  STYLE_INVERSE = 1 << 1,   // selection highlight
  STYLE_DIM     = 1 << 2,
};

struct Cell {
  char    ch;
  uint8_t style;
  Cell(char c = ' ', uint8_t s = STYLE_NORMAL) : ch(c), style(s) {}
  bool operator==(const Cell& o) const { return ch == o.ch && style == o.style; }
  bool operator!=(const Cell& o) const { return !(*this == o); }
};

// Fixed-size character grid. Sized at UI_COLS x UI_ROWS (see board.h).
class FrameBuffer {
 public:
  uint8_t cols() const { return UI_COLS; }
  uint8_t rows() const { return UI_ROWS; }

  void clear() {
    for (auto& c : cells_) c = Cell{};
  }

  const Cell& at(uint8_t col, uint8_t row) const {
    return cells_[row * UI_COLS + col];
  }

  void putChar(uint8_t col, uint8_t row, char ch, uint8_t style = STYLE_NORMAL) {
    if (col >= UI_COLS || row >= UI_ROWS) return;
    cells_[row * UI_COLS + col] = Cell{ch, style};
  }

  void putText(uint8_t col, uint8_t row, const char* s,
               uint8_t style = STYLE_NORMAL) {
    for (; *s && col < UI_COLS; ++s, ++col) putChar(col, row, *s, style);
  }

  void putTextCentered(uint8_t row, const char* s,
                       uint8_t style = STYLE_NORMAL) {
    int len = (int)strlen(s);
    int col = (UI_COLS - len) / 2;
    if (col < 0) col = 0;
    putText((uint8_t)col, row, s, style);
  }

  // Fill an entire row (e.g. a selection bar or separator).
  void fillRow(uint8_t row, char ch, uint8_t style = STYLE_NORMAL) {
    for (uint8_t col = 0; col < UI_COLS; ++col) putChar(col, row, ch, style);
  }

 private:
  Cell cells_[UI_COLS * UI_ROWS];
};

// An output device that can render the FrameBuffer.
class Surface {
 public:
  virtual ~Surface() {}
  virtual void render(const FrameBuffer& fb) = 0;
  // Force a full repaint on the next render (e.g. a terminal reconnected and
  // missed the static frame). Default: no-op.
  virtual void reset() {}
};
