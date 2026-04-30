#include "drivers/input_keys.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "boards/pin_config.h"

static const char* TAG = "InputKeys";

namespace drivers {

void InputKeys::begin() {
  ESP_LOGI(TAG, "begin()");

  // Configure all input pins as inputs with pull-up
  // ESP32-S3 has internal pull-ups that can be enabled
  const int input_pins[] = {
      board::input::kUp, board::input::kDown, board::input::kLeft, board::input::kRight,
      board::input::kA, board::input::kB, board::input::kX, board::input::kY,
      board::input::kStart, board::input::kSelect, board::input::kL, board::input::kR};

  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

  uint64_t pin_mask = 0;
  for (int pin : input_pins) {
    pin_mask |= (1ULL << pin);
  }
  io_conf.pin_bit_mask = pin_mask;
  gpio_config(&io_conf);
}

InputState InputKeys::poll() {
  InputState state;
  state.up = readActiveLow(board::input::kUp);
  state.down = readActiveLow(board::input::kDown);
  state.left = readActiveLow(board::input::kLeft);
  state.right = readActiveLow(board::input::kRight);

  state.a = readActiveLow(board::input::kA);
  state.b = readActiveLow(board::input::kB);
  state.x = readActiveLow(board::input::kX);
  state.y = readActiveLow(board::input::kY);

  state.start = readActiveLow(board::input::kStart);
  state.select = readActiveLow(board::input::kSelect);
  state.l = readActiveLow(board::input::kL);
  state.r = readActiveLow(board::input::kR);
  return state;
}

bool InputKeys::readActiveLow(int pin) const {
  return gpio_get_level(static_cast<gpio_num_t>(pin)) == 0;
}

}  // namespace drivers
