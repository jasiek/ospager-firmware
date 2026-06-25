#include "ui/screens.h"

#include <Arduino.h>
#include <time.h>

#include "board.h"
#include "message_store.h"
#include "time_sync.h"

// ---------------------------------------------------------------- HomeScreen
void HomeScreen::onInput(InputEvent ev, UiManager& ui) {
  if (messages_ && (ev == InputEvent::Down || ev == InputEvent::Select ||
                    ev == InputEvent::Right)) {
    ui.push(messages_);
  }
}

void HomeScreen::render(FrameBuffer& fb) {
  fb.putTextCentered(0, "OSPAGER", STYLE_BOLD);
  fb.fillRow(1, '-');

  char date[16] = "----------";
  char tnow[16] = "--:--:--";
  if (clock_.isSynced()) {
    struct tm lt;
    clock_.localNow(lt);
    strftime(date, sizeof(date), "%Y-%m-%d", &lt);
    snprintf(tnow, sizeof(tnow), "%02d:%02d:%02d", lt.tm_hour, lt.tm_min,
             lt.tm_sec);
  }
  fb.putTextCentered(3, date);
  fb.putTextCentered(4, tnow, STYLE_BOLD);
  fb.fillRow(6, '-');

  char status[24];
  if (clock_.isSynced()) {
    snprintf(status, sizeof(status), "sync %lus  msgs %u",
             (unsigned long)clock_.secondsSinceSync(),
             (unsigned)store_.count());
  } else {
    snprintf(status, sizeof(status), "no sync  msgs %u",
             (unsigned)store_.count());
  }
  fb.putTextCentered(7, status);
}

// ------------------------------------------------------------ MessagesScreen
void MessagesScreen::onEnter(UiManager&) {
  sel_ = 0;
  top_ = 0;
}

void MessagesScreen::onInput(InputEvent ev, UiManager& ui) {
  int n = (int)store_.count();
  switch (ev) {
    case InputEvent::Up:
      if (sel_ > 0) sel_--;
      break;
    case InputEvent::Down:
      if (sel_ < n - 1) sel_++;
      break;
    case InputEvent::Select:
    case InputEvent::Right:
      if (n > 0 && detail_) {
        detail_->setIndex(sel_);
        ui.push(detail_);
      }
      break;
    case InputEvent::Back:
    case InputEvent::Left:
      ui.pop();
      break;
    default:
      break;
  }
}

void MessagesScreen::render(FrameBuffer& fb) {
  int n = (int)store_.count();

  char hdr[24];
  snprintf(hdr, sizeof(hdr), "Messages %d", n);
  fb.putText(0, 0, hdr, STYLE_BOLD);

  const int VIS = 6;   // list occupies rows 1..6
  if (sel_ < 0) sel_ = 0;
  if (sel_ > n - 1) sel_ = (n > 0) ? n - 1 : 0;
  if (sel_ < top_) top_ = sel_;
  if (sel_ >= top_ + VIS) top_ = sel_ - VIS + 1;
  if (top_ < 0) top_ = 0;

  if (n == 0) {
    fb.putTextCentered(3, "(no messages)");
  } else {
    for (int row = 0; row < VIS; ++row) {
      int idx = top_ + row;
      if (idx >= n) break;
      const PagerMessage& m = store_.get((size_t)idx);

      char hm[8] = "--:--";
      if (m.epoch != 0) {
        struct tm lt;
        localtime_r(&m.epoch, &lt);
        snprintf(hm, sizeof(hm), "%02d:%02d", lt.tm_hour, lt.tm_min);
      }
      char line[48];
      snprintf(line, sizeof(line), "%s %lu %s", hm, (unsigned long)m.capcode,
               m.text.c_str());

      bool selected = (idx == sel_);
      uint8_t style = selected ? STYLE_INVERSE : STYLE_NORMAL;
      if (selected) fb.fillRow(1 + row, ' ', STYLE_INVERSE);
      fb.putText(0, 1 + row, line, style);
    }
  }

  fb.putText(0, 7, "j/k sel  q back", STYLE_DIM);
}

// -------------------------------------------------------------- DetailScreen
void DetailScreen::onInput(InputEvent ev, UiManager& ui) {
  if (ev == InputEvent::Back || ev == InputEvent::Left) ui.pop();
}

void DetailScreen::render(FrameBuffer& fb) {
  int n = (int)store_.count();
  if (n == 0) {
    fb.putTextCentered(3, "(no message)");
    fb.putText(0, 7, "q back", STYLE_DIM);
    return;
  }

  int idx = index_;
  if (idx < 0) idx = 0;
  if (idx > n - 1) idx = n - 1;
  const PagerMessage& m = store_.get((size_t)idx);

  char ts[16] = "--:--:--";
  if (m.epoch != 0) {
    struct tm lt;
    localtime_r(&m.epoch, &lt);
    snprintf(ts, sizeof(ts), "%02d:%02d:%02d", lt.tm_hour, lt.tm_min, lt.tm_sec);
  }
  char hdr[24];
  snprintf(hdr, sizeof(hdr), "cc%lu %s", (unsigned long)m.capcode, ts);
  fb.putText(0, 0, hdr, STYLE_BOLD);
  fb.fillRow(1, '-');

  // Char-wrap the body into rows 2..6.
  const char* s = m.text.c_str();
  int len = (int)strlen(s);
  int pos = 0;
  for (int row = 2; row <= 6 && pos < len; ++row) {
    int take = len - pos;
    if (take > UI_COLS) take = UI_COLS;
    char line[UI_COLS + 1];
    memcpy(line, s + pos, take);
    line[take] = '\0';
    fb.putText(0, (uint8_t)row, line);
    pos += take;
  }

  fb.putText(0, 7, "q back", STYLE_DIM);
}
