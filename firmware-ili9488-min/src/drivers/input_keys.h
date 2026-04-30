#pragma once

#include <stdint.h>

namespace drivers {

struct InputState {
  bool up = false;
  bool down = false;
  bool left = false;
  bool right = false;
  bool a = false;
  bool b = false;
  bool x = false;
  bool y = false;
  bool start = false;
  bool select = false;
  bool l = false;
  bool r = false;
};

class InputKeys {
 public:
  void begin();
  InputState poll();

 private:
  bool readActiveLow(int pin) const;

  static constexpr uint16_t kDebounceMs = 15;
  bool stable_state_[12] = {false};
  bool sampled_state_[12] = {false};
  uint32_t sampled_at_ms_[12] = {0};
};

}  // namespace drivers
