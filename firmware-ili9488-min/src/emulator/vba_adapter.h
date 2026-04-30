#pragma once

#include <Arduino.h>
#include <vector>

#include "drivers/audio_pwm.h"
#include "drivers/display_ili9488.h"
#include "drivers/input_keys.h"
#include "drivers/storage_sd.h"

// Forward declare VBA globals (matching globals.h declarations)
// These are defined in gba.cpp
extern uint16_t *pix;
extern uint8_t *rom;
extern uint8_t *vram;
extern uint8_t *workRAM;
extern uint8_t *bios;
extern uint8_t *libretro_save_buf;
extern uint8_t internalRAM[];
extern uint8_t paletteRAM[];
extern uint8_t oam[];
extern uint8_t ioMem[];


namespace emulator {

class VbaAdapter {
 public:
  VbaAdapter();
  ~VbaAdapter();

  bool begin(drivers::DisplayILI9488* display, drivers::AudioPwm* audio);
  bool loadRom(drivers::StorageSd& storage, const String& rom_path);
  void setPwmFeedbackEnabled(bool enabled) { pwm_feedback_enabled_ = enabled; }
  void stepFrame(const drivers::InputState& input);
  bool isLoaded() const { return loaded_; }
  String statusText() const;

 private:
  drivers::DisplayILI9488* display_ = nullptr;
  drivers::AudioPwm* audio_ = nullptr;
  bool loaded_ = false;
  bool pwm_feedback_enabled_ = true;
  uint32_t frame_count_ = 0;
  bool initialized_ = false;

  // VBA state buffers (static allocation for embedded)
  // Note: internalRAM, oam, ioMem, paletteRAM are static arrays in gba.cpp
  uint16_t pix_buffer_[256 * 160];  // VBA pix buffer (256 wide, 160 tall)
  uint8_t rom_buffer_[32 * 1024 * 1024];  // 32MB ROM buffer
  uint8_t vram_buffer_[0x20000];
  uint8_t workRAM_buffer_[0x40000];
  uint8_t bios_buffer_[0x4000];
  uint8_t save_buffer_[0x22000];  // flash + eeprom

};

}  // namespace emulator
