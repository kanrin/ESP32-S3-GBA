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

  // Called by VBA core via systemDrawScreen() callback
  void onFrameReady();

 private:
  bool allocateBuffers();
  void freeBuffers();

  drivers::DisplayILI9488* display_ = nullptr;
  drivers::AudioPwm* audio_ = nullptr;
  bool loaded_ = false;
  bool pwm_feedback_enabled_ = true;
  uint32_t frame_count_ = 0;
  bool initialized_ = false;

  // VBA state buffers (heap-allocated to avoid DRAM overflow)
  uint16_t* pix_buffer_ = nullptr;       // 256 * 160 * 2 = 81,920 bytes
  uint8_t* vram_buffer_ = nullptr;       // 0x20000 = 131,072 bytes
  uint8_t* workRAM_buffer_ = nullptr;    // 0x40000 = 262,144 bytes
  uint8_t* bios_buffer_ = nullptr;       // 0x4000 = 16,384 bytes
  uint8_t* save_buffer_ = nullptr;       // 0x22000 = 139,264 bytes
  // ROM buffer is dynamically allocated based on actual ROM size
  uint8_t* rom_buffer_ = nullptr;
  uint32_t rom_buffer_size_ = 0;
};

}  // namespace emulator
