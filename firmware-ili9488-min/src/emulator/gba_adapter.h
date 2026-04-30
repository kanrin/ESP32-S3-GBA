#pragma once

#include <Arduino.h>
#include <vector>

#include "drivers/audio_pwm.h"
#include "drivers/display_ili9488.h"
#include "drivers/input_keys.h"
#include "drivers/storage_sd.h"

namespace emulator {

class GbaAdapter {
 public:
  GbaAdapter();
  ~GbaAdapter();

  bool begin(drivers::DisplayILI9488* display, drivers::AudioPwm* audio);
  bool loadRom(drivers::StorageSd& storage, const String& rom_path);
  void setPwmFeedbackEnabled(bool enabled) { pwm_feedback_enabled_ = enabled; }
  void stepFrame(const drivers::InputState& input);
  bool isLoaded() const { return loaded_; }
  String statusText() const;

  // Called from platform glue (system.cpp)
  void onDrawScreen();
  bool onReadJoypads();
  uint32_t onGetClock();
  void onWriteDataToSoundBuffer(int16_t* finalWave, int length);
  void onMessage(const char* msg);

 private:
  drivers::DisplayILI9488* display_ = nullptr;
  drivers::AudioPwm* audio_ = nullptr;
  bool loaded_ = false;
  bool pwm_feedback_enabled_ = true;
  uint32_t frame_count_ = 0;
  bool frame_ready_ = false;
  bool initialized_ = false;

  // Joypad state mapping
  uint16_t joypad_state_ = 0x03FF; // GBA: all bits high = no buttons pressed
};

}  // namespace emulator
