#include "ui/input.h"

#include <Arduino.h>

void SerialInput::push(InputEvent ev) {
  if (qCount_ >= QN) return;
  queue_[qTail_] = ev;
  qTail_ = (qTail_ + 1) % QN;
  qCount_++;
}

bool SerialInput::pop(InputEvent& ev) {
  if (qCount_ == 0) return false;
  ev = queue_[qHead_];
  qHead_ = (qHead_ + 1) % QN;
  qCount_--;
  return true;
}

void SerialInput::update() {
  while (in_.available() > 0) {
    char c = (char)in_.read();

    // Collapse a CRLF/LFCR pair into a single Enter: a terminal that sends
    // "\r\n" must not look like two presses. Swallow the second byte of a
    // pair; a non-newline byte ends any pending pair.
    bool isEol = (c == '\r' || c == '\n');
    if (esc_ == 0 && isEol) {
      if (eolPair_ && c != eolPair_) {
        eolPair_ = 0;   // second half of a \r\n / \n\r pair -> ignore
      } else {
        push(InputEvent::Select);
        eolPair_ = c;
      }
      continue;
    }
    if (!isEol) eolPair_ = 0;

    switch (esc_) {
      case 0:  // normal
        if (c == 0x1b) {
          esc_ = 1;
        } else if (c == 'k') {
          push(InputEvent::Up);
        } else if (c == 'j') {
          push(InputEvent::Down);
        } else if (c == 'h') {
          push(InputEvent::Left);
        } else if (c == 'l') {
          push(InputEvent::Right);
        } else if (c == 'd') {
          push(InputEvent::Delete);
        } else if (c == 'q') {
          push(InputEvent::Back);
        } else if (c == 0x0c) {   // Ctrl-L
          push(InputEvent::Redraw);
        }
        break;

      case 1:  // saw ESC
        if (c == '[') {
          esc_ = 2;
        } else {
          // Lone ESC -> Back; reprocess this byte as a normal key.
          push(InputEvent::Back);
          esc_ = 0;
          if (c == '\r' || c == '\n') push(InputEvent::Select);
        }
        break;

      case 2:  // saw ESC '[' -> final byte selects the arrow
        switch (c) {
          case 'A': push(InputEvent::Up); break;
          case 'B': push(InputEvent::Down); break;
          case 'C': push(InputEvent::Right); break;
          case 'D': push(InputEvent::Left); break;
          default: break;  // ignore other CSI sequences
        }
        esc_ = 0;
        break;
    }
  }
}
