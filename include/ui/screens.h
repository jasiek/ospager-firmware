// Concrete pager screens. All render only into the FrameBuffer, so they are
// identical across the serial terminal, the OLED, and any future LCD.
#pragma once

#include <stdint.h>

#include "ui/ui.h"

class Clock;
class MessageStore;

// Home: clock, date and sync/traffic status. Any key opens the main menu.
class HomeScreen : public Screen {
 public:
  HomeScreen(Clock& clock, MessageStore& store) : clock_(clock), store_(store) {}
  void setMenuScreen(Screen* s) { menu_ = s; }

  void onInput(InputEvent ev, UiManager& ui) override;
  void render(FrameBuffer& fb) override;

 private:
  Clock& clock_;
  MessageStore& store_;
  Screen* menu_ = nullptr;
};

// Generic vertical menu: a title and a list of label -> target-screen items.
// Up/Down move the highlight, Select/Right opens the item, Back/Left pops.
class MenuScreen : public Screen {
 public:
  explicit MenuScreen(const char* title) : title_(title) {}
  // Items render on rows 2..6, so at most 5 fit beneath the title.
  void addItem(const char* label, Screen* target);

  void onInput(InputEvent ev, UiManager& ui) override;
  void render(FrameBuffer& fb) override;

 private:
  static constexpr uint8_t MAX_ITEMS = 5;
  const char* title_;
  const char* labels_[MAX_ITEMS];
  Screen*     targets_[MAX_ITEMS];
  uint8_t     count_ = 0;
  int         sel_ = 0;
};

// Power: live AXP192 current readings (discharge / charge / VBUS in).
class PowerScreen : public Screen {
 public:
  void onInput(InputEvent ev, UiManager& ui) override;
  void render(FrameBuffer& fb) override;
};

class DetailScreen;

// A scrollable list of received messages from one store. Used for both the
// pager's own Messages and the broadcast Rubrics; the title is the only
// difference, the navigation/delete logic is identical.
class MessagesScreen : public Screen {
 public:
  MessagesScreen(MessageStore& store, const char* title = "Messages")
      : store_(store), title_(title) {}
  void setDetailScreen(DetailScreen* s) { detail_ = s; }

  void onEnter(UiManager& ui) override;
  void onInput(InputEvent ev, UiManager& ui) override;
  void render(FrameBuffer& fb) override;

 private:
  MessageStore& store_;
  const char* title_;
  DetailScreen* detail_ = nullptr;
  int sel_ = 0;   // selected index (0 == newest)
  int top_ = 0;   // first visible index
};

// Detail: full text of a single message.
class DetailScreen : public Screen {
 public:
  explicit DetailScreen(MessageStore& store) : store_(store) {}
  void setIndex(int index) { index_ = index; }

  void onInput(InputEvent ev, UiManager& ui) override;
  void render(FrameBuffer& fb) override;

 private:
  MessageStore& store_;
  int index_ = 0;
};
