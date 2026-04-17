#pragma once

#include <Arduino.h>
#include <vector>

#include "drivers/input_keys.h"

namespace app {

class RomMenu {
 public:
  void setEntries(const std::vector<String>& entries);
  bool update(const drivers::InputState& input);
  int selectedIndex() const { return selected_; }
  bool confirmedSelection() const { return confirm_pressed_; }
  String selectedPath() const;
  String renderText() const;

 private:
  std::vector<String> entries_;
  int selected_ = 0;
  bool last_up_ = false;
  bool last_down_ = false;
  bool last_a_ = false;
  bool confirm_pressed_ = false;
};

}  // namespace app
