#include "ui/ui.h"

void UiManager::addSurface(Surface* s) {
  if (surfaceCount_ < MAX_SURFACES) surfaces_[surfaceCount_++] = s;
}

void UiManager::push(Screen* s) {
  if (depth_ >= MAX_SCREENS || s == nullptr) return;
  stack_[depth_++] = s;
  s->onEnter(*this);
  markDirty();
}

void UiManager::pop() {
  if (depth_ <= 1) return;            // never pop the root screen
  depth_--;
  stack_[depth_ - 1]->onEnter(*this);  // let the revealed screen refresh
  markDirty();
}

Screen* UiManager::top() const {
  return depth_ ? stack_[depth_ - 1] : nullptr;
}

void UiManager::onInput(InputEvent ev) {
  if (ev == InputEvent::Redraw) {
    redraw();
    return;
  }
  if (Screen* s = top()) {
    s->onInput(ev, *this);
    markDirty();
  }
}

void UiManager::redraw() {
  for (uint8_t i = 0; i < surfaceCount_; ++i) surfaces_[i]->reset();
  markDirty();
}

void UiManager::onTick(uint32_t nowMs) {
  if (Screen* s = top()) s->onTick(nowMs, *this);
}

void UiManager::render() {
  Screen* s = top();
  if (!s) return;
  fb_.clear();
  s->render(fb_);
  for (uint8_t i = 0; i < surfaceCount_; ++i) surfaces_[i]->render(fb_);
  dirty_ = false;
}
