#include "app/rom_menu.h"

namespace app {

void RomMenu::setEntries(const std::vector<std::string>& entries) {
  entries_ = entries;
  selected_ = 0;
  confirm_pressed_ = false;
}

bool RomMenu::update(const drivers::InputState& input) {
  confirm_pressed_ = false;
  if (entries_.empty()) {
    return false;
  }

  if (input.up && !last_up_) {
    selected_ = (selected_ == 0) ? static_cast<int>(entries_.size() - 1) : selected_ - 1;
  }
  if (input.down && !last_down_) {
    selected_ = (selected_ + 1) % static_cast<int>(entries_.size());
  }
  if (input.a && !last_a_) {
    confirm_pressed_ = true;
  }

  last_up_ = input.up;
  last_down_ = input.down;
  last_a_ = input.a;
  return true;
}

std::string RomMenu::selectedPath() const {
  if (entries_.empty()) {
    return "";
  }
  return entries_[selected_];
}

std::string RomMenu::renderText() const {
  if (entries_.empty()) {
    return "ROM: <none>";
  }
  return "ROM: " + entries_[selected_];
}

}  // namespace app
