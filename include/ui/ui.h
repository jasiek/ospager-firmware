// Screen abstraction and the manager that owns the screen stack and fans the
// rendered FrameBuffer out to every attached Surface.
#pragma once

#include <stdint.h>

#include "ui/input.h"
#include "ui/surface.h"

class UiManager;

class Screen {
 public:
  virtual ~Screen() {}
  virtual void onEnter(UiManager&) {}
  virtual void onInput(InputEvent ev, UiManager& ui) {}
  virtual void onTick(uint32_t nowMs, UiManager& ui) {}
  virtual void render(FrameBuffer& fb) = 0;
};

class UiManager {
 public:
  void addSurface(Surface* s);

  void push(Screen* s);
  void pop();
  Screen* top() const;

  void onInput(InputEvent ev);
  void onTick(uint32_t nowMs);

  void markDirty() { dirty_ = true; }
  bool dirty() const { return dirty_; }
  void render();   // rebuild the FrameBuffer and push it to all surfaces

 private:
  static constexpr uint8_t MAX_SURFACES = 4;
  static constexpr uint8_t MAX_SCREENS = 8;

  Surface* surfaces_[MAX_SURFACES];
  uint8_t  surfaceCount_ = 0;

  Screen*  stack_[MAX_SCREENS];
  uint8_t  depth_ = 0;

  FrameBuffer fb_;
  bool dirty_ = true;
};
