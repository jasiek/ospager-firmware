#include "ui/screens.h"

#include <Arduino.h>
#include <time.h>

#include "board.h"
#include "message_store.h"
#include "power.h"
#include "time_sync.h"

// ---------------------------------------------------------------- HomeScreen
void HomeScreen::onInput(InputEvent ev, UiManager& ui) {
  // Any key opens the main menu.
  if (menu_) ui.push(menu_);
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
    case InputEvent::Delete:
      // Selection stays put; render() clamps it to the shrunken list, which
      // lands on the next-older message (or the new last one).
      if (n > 0) store_.remove((size_t)sel_);
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
  snprintf(hdr, sizeof(hdr), "%s %d", title_, n);
  fb.putText(0, 0, hdr, STYLE_BOLD);

  const int VIS = 6;   // list occupies rows 1..6
  if (sel_ < 0) sel_ = 0;
  if (sel_ > n - 1) sel_ = (n > 0) ? n - 1 : 0;
  if (sel_ < top_) top_ = sel_;
  if (sel_ >= top_ + VIS) top_ = sel_ - VIS + 1;
  if (top_ < 0) top_ = 0;

  if (n == 0) {
    fb.putTextCentered(3, "(empty)");
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

  fb.putText(0, 7, "j/k sel  d del  q back", STYLE_DIM);
}

// -------------------------------------------------------------- DetailScreen
void DetailScreen::onInput(InputEvent ev, UiManager& ui) {
  if (ev == InputEvent::Delete) {
    if (index_ < (int)store_.count()) store_.remove((size_t)index_);
    ui.pop();   // back to the list, which reflects the shorter store
    return;
  }
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

  fb.putText(0, 7, "d del  q back", STYLE_DIM);
}

// ---------------------------------------------------------------- MenuScreen
void MenuScreen::addItem(const char* label, Screen* target) {
  if (count_ >= MAX_ITEMS) return;
  labels_[count_]  = label;
  targets_[count_] = target;
  count_++;
}

void MenuScreen::onInput(InputEvent ev, UiManager& ui) {
  switch (ev) {
    case InputEvent::Up:
      if (sel_ > 0) sel_--;
      break;
    case InputEvent::Down:
      if (sel_ < (int)count_ - 1) sel_++;
      break;
    case InputEvent::Select:
    case InputEvent::Right:
      if (count_ > 0) ui.push(targets_[sel_]);
      break;
    case InputEvent::Back:
    case InputEvent::Left:
      ui.pop();
      break;
    default:
      break;
  }
}

void MenuScreen::render(FrameBuffer& fb) {
  fb.putText(0, 0, title_, STYLE_BOLD);
  fb.fillRow(1, '-');

  if (sel_ < 0) sel_ = 0;
  if (sel_ > (int)count_ - 1) sel_ = (count_ > 0) ? count_ - 1 : 0;

  for (uint8_t i = 0; i < count_; ++i) {
    uint8_t row = 2 + i;
    bool selected = (i == sel_);
    if (selected) fb.fillRow(row, ' ', STYLE_INVERSE);
    fb.putText(0, row, labels_[i], selected ? STYLE_INVERSE : STYLE_NORMAL);
  }

  fb.putText(0, 7, "j/k sel  q back", STYLE_DIM);
}

// ------------------------------------------------------------- ConfirmScreen
void ConfirmScreen::onInput(InputEvent ev, UiManager& ui) {
  switch (ev) {
    case InputEvent::Select:
    case InputEvent::Right:
      if (action_) action_();   // reset / power off: does not return
      break;
    case InputEvent::Back:
    case InputEvent::Left:
      ui.pop();
      break;
    default:
      break;
  }
}

void ConfirmScreen::render(FrameBuffer& fb) {
  fb.putText(0, 0, title_, STYLE_BOLD);
  fb.fillRow(1, '-');
  fb.putTextCentered(3, prompt_);
  fb.putText(0, 7, "enter ok  q back", STYLE_DIM);
}

// --------------------------------------------------------------- PowerScreen
void PowerScreen::onInput(InputEvent ev, UiManager& ui) {
  if (ev == InputEvent::Back || ev == InputEvent::Left) ui.pop();
}

void PowerScreen::render(FrameBuffer& fb) {
  PowerReadings p = powerRead();

  fb.putText(0, 0, "Power", STYLE_BOLD);
  fb.fillRow(1, '-');

  char line[24];
  snprintf(line, sizeof(line), "Discharge %6.1f mA", p.dischargeMa);
  fb.putText(0, 3, line);
  snprintf(line, sizeof(line), "Charge    %6.1f mA", p.chargeMa);
  fb.putText(0, 4, line);
  snprintf(line, sizeof(line), "VBUS in   %6.1f mA", p.vbusMa);
  fb.putText(0, 5, line);

  fb.putText(0, 7, "q back", STYLE_DIM);
}
