// Concrete pager screens. All render only into the FrameBuffer, so they are
// identical across the serial terminal, the OLED, and any future LCD.
#pragma once

#include <stdint.h>

#include "ui/ui.h"

class Clock;
class MessageStore;

// Home: clock, date and sync/traffic status.
class HomeScreen : public Screen {
 public:
  HomeScreen(Clock& clock, MessageStore& store) : clock_(clock), store_(store) {}
  void setMessagesScreen(Screen* s) { messages_ = s; }

  void onInput(InputEvent ev, UiManager& ui) override;
  void render(FrameBuffer& fb) override;

 private:
  Clock& clock_;
  MessageStore& store_;
  Screen* messages_ = nullptr;
};

class DetailScreen;

// Messages: scrollable list of received pager messages.
class MessagesScreen : public Screen {
 public:
  explicit MessagesScreen(MessageStore& store) : store_(store) {}
  void setDetailScreen(DetailScreen* s) { detail_ = s; }

  void onEnter(UiManager& ui) override;
  void onInput(InputEvent ev, UiManager& ui) override;
  void render(FrameBuffer& fb) override;

 private:
  MessageStore& store_;
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
