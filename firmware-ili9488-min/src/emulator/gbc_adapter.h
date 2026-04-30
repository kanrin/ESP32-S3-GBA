#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "drivers/audio_pwm.h"
#include "drivers/display_ili9488.h"
#include "drivers/input_keys.h"
#include "drivers/storage_sd.h"

namespace emulator {

class GbcAdapter {
 public:
  bool begin(drivers::DisplayILI9488* display, drivers::AudioPwm* audio);
  bool loadRom(drivers::StorageSd& storage, const std::string& rom_path);
  void setPwmFeedbackEnabled(bool enabled) { pwm_feedback_enabled_ = enabled; }
  void stepFrame(const drivers::InputState& input);
  bool isLoaded() const { return loaded_; }
  std::string statusText() const;

 private:
  std::vector<uint8_t> rom_buffer_;
  std::vector<uint16_t> frame_buffer_;
  drivers::DisplayILI9488* display_ = nullptr;
  drivers::AudioPwm* audio_ = nullptr;
  bool loaded_ = false;
  bool pwm_feedback_enabled_ = true;
  uint32_t frame_tick_ = 0;
};

}  // namespace emulator
