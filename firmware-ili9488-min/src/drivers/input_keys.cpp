#include "drivers/input_keys.h"

#include "boards/pin_config.h"

namespace drivers {

void InputKeys::begin() {
  pinMode(board::input::kUp, INPUT_PULLUP);
  pinMode(board::input::kDown, INPUT_PULLUP);
  pinMode(board::input::kLeft, INPUT_PULLUP);
  pinMode(board::input::kRight, INPUT_PULLUP);

  pinMode(board::input::kA, INPUT_PULLUP);
  pinMode(board::input::kB, INPUT_PULLUP);
  pinMode(board::input::kX, INPUT_PULLUP);
  pinMode(board::input::kY, INPUT_PULLUP);

  pinMode(board::input::kStart, INPUT_PULLUP);
  pinMode(board::input::kSelect, INPUT_PULLUP);
  pinMode(board::input::kL, INPUT_PULLUP);
  pinMode(board::input::kR, INPUT_PULLUP);
}

InputState InputKeys::poll() {
  InputState state;
  state.up = readDebounced(board::input::kUp, 0);
  state.down = readDebounced(board::input::kDown, 1);
  state.left = readDebounced(board::input::kLeft, 2);
  state.right = readDebounced(board::input::kRight, 3);

  state.a = readDebounced(board::input::kA, 4);
  state.b = readDebounced(board::input::kB, 5);
  state.x = readDebounced(board::input::kX, 6);
  state.y = readDebounced(board::input::kY, 7);

  state.start = readDebounced(board::input::kStart, 8);
  state.select = readDebounced(board::input::kSelect, 9);
  state.l = readDebounced(board::input::kL, 10);
  state.r = readDebounced(board::input::kR, 11);
  return state;
}

bool InputKeys::readActiveLow(int pin) const {
  return digitalRead(pin) == LOW;
}

bool InputKeys::readDebounced(int pin, uint8_t slot) {
  const bool raw = readActiveLow(pin);
  const uint32_t now = millis();
  if (raw != sampled_state_[slot]) {
    sampled_state_[slot] = raw;
    sampled_at_ms_[slot] = now;
  }
  if ((now - sampled_at_ms_[slot]) >= kDebounceMs) {
    stable_state_[slot] = sampled_state_[slot];
  }
  return stable_state_[slot];
}

}  // namespace drivers
