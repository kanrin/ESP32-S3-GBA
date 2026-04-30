#include "emulator/vba_adapter.h"

// VBA core headers
#include "emulator/vba/gba.h"
#include "emulator/vba/globals.h"
#include "emulator/vba/memory.h"
#include "emulator/vba/sound.h"
#include "emulator/vba/system.h"

// ============================================================
// VBA system callbacks - these are called by the VBA core
// ============================================================

// Pointer to the adapter instance so system callbacks can reach it
static emulator::VbaAdapter* g_adapter_instance = nullptr;

// Forward declare the system callback implementations
extern "C" {

void systemDrawScreen(void) {
  if (g_adapter_instance) {
    g_adapter_instance->onFrameReady();
  }
}

bool systemReadJoypads(void) {
  // Joypad state is set via UpdateJoypad() before CPULoop
  return true;
}

uint32_t systemGetClock(void) {
  return micros();
}

void systemMessage(const char* msg, ...) {
  // Silently ignore debug messages on embedded
  (void)msg;
}

void systemOnWriteDataToSoundBuffer(int16_t* finalWave, int length) {
  // Audio output - can be implemented later
  (void)finalWave;
  (void)length;
}

}  // extern "C"

// ============================================================
// VbaAdapter implementation
// ============================================================

namespace emulator {

// VBA global variables are defined in gba.cpp (lines 94-99).
// We just need to set them to point to our pre-allocated buffers.

VbaAdapter::VbaAdapter() {}


VbaAdapter::~VbaAdapter() {
  if (initialized_) {
    CPUCleanUp();
    initialized_ = false;
  }
  g_adapter_instance = nullptr;
}

bool VbaAdapter::begin(drivers::DisplayILI9488* display, drivers::AudioPwm* audio) {
  display_ = display;
  audio_ = audio;

  if (display_ == nullptr) {
    return false;
  }

  // Point VBA globals to our pre-allocated buffers
  // Note: internalRAM, oam, ioMem, paletteRAM are static arrays in gba.cpp
  // and don't need to be set here.
  pix = pix_buffer_;
  rom = rom_buffer_;
  vram = vram_buffer_;
  workRAM = workRAM_buffer_;
  bios = bios_buffer_;
  libretro_save_buf = save_buffer_;


  // Initialize VBA core
  CPUInit(nullptr, false);
  initialized_ = true;

  return true;
}

bool VbaAdapter::loadRom(drivers::StorageSd& storage, const String& rom_path) {
  if (!initialized_) {
    return false;
  }

  // Read ROM file into buffer
  std::vector<uint8_t> rom_data;
  if (!storage.readFile(rom_path, &rom_data) || rom_data.empty()) {
    loaded_ = false;
    return false;
  }

  // Load ROM into VBA using CPULoadRomData (loads from memory)
  // This works for .gba, .gb, .gbc files since it just copies raw data
  int result = CPULoadRomData(reinterpret_cast<const char*>(rom_data.data()),
                               static_cast<int>(rom_data.size()));
  if (result <= 0) {
    loaded_ = false;
    return false;
  }

  // Reset the emulator
  CPUReset();

  // Set up memory mapping for GB/GBC compatibility
  // The VBA core handles GB/GBC through the GBA's backward compatibility mode
  // No special setup needed - the CPU will execute the ROM as-is

  loaded_ = true;
  frame_count_ = 0;
  return true;
}

void VbaAdapter::stepFrame(const drivers::InputState& input) {
  if (!loaded_ || display_ == nullptr) {
    return;
  }

  // Set up joypad state for VBA
  // VBA uses a 64-bit joy variable where bits represent button states
  // Bit layout (from gba.cpp):
  //   0-9: Button states (0 = pressed, 1 = released)
  //   10+: Extended flags
  joy = 0xFFFF;  // Start with all released
  if (input.a)      joy &= ~(1ULL << 0);
  if (input.b)      joy &= ~(1ULL << 1);
  if (input.select) joy &= ~(1ULL << 2);
  if (input.start)  joy &= ~(1ULL << 3);
  if (input.right)  joy &= ~(1ULL << 4);
  if (input.left)   joy &= ~(1ULL << 5);
  if (input.up)     joy &= ~(1ULL << 6);
  if (input.down)   joy &= ~(1ULL << 7);
  if (input.r)      joy &= ~(1ULL << 8);
  if (input.l)      joy &= ~(1ULL << 9);

  // Register this adapter instance for system callbacks
  g_adapter_instance = this;

  // Run one frame of emulation
  // CPULoop runs until a frame is complete (systemDrawScreen is called)
  CPULoop();

  // PWM audio feedback (simple beep on A button)
  if (pwm_feedback_enabled_ && input.a && audio_ != nullptr) {
    audio_->playTone(880, 20);
  }

  frame_count_++;
}

void VbaAdapter::onFrameReady() {
  if (display_ == nullptr) {
    return;
  }

  // VBA renders into pix[] buffer.
  // The GBA screen is 240x160 pixels, stored in a 256-wide buffer.
  // pix layout: PIX_BUFFER_SCREEN_WIDTH (256) pixels per line, 160 lines.
  // The visible area starts at offset 8 on each line (centered in 256).
  //
  // For GB/GBC games, the VBA core renders the 160x144 GB screen
  // centered in the 240x160 GBA buffer at offset (40, 8).
  // We extract the 160x144 GB screen area.
  static std::vector<uint16_t> frame_160x144(160 * 144);

  constexpr int kPixWidth = 256;  // PIX_BUFFER_SCREEN_WIDTH
  constexpr int kGbaWidth = 240;
  constexpr int kGbaHeight = 160;
  constexpr int kGbWidth = 160;
  constexpr int kGbHeight = 144;
  constexpr int kXOffset = (kPixWidth - kGbaWidth) / 2;  // 8
  constexpr int kGbXOffset = kXOffset + (kGbaWidth - kGbWidth) / 2;  // 8 + 40 = 48
  constexpr int kGbYOffset = (kGbaHeight - kGbHeight) / 2;  // 8

  for (int y = 0; y < kGbHeight; ++y) {
    for (int x = 0; x < kGbWidth; ++x) {
      frame_160x144[(y * kGbWidth) + x] = pix_buffer_[((y + kGbYOffset) * kPixWidth) + x + kGbXOffset];
    }
  }

  display_->pushFrame160x144(frame_160x144);
}


String VbaAdapter::statusText() const {
  if (!loaded_) {
    return "VBA: ROM not loaded";
  }
  return "VBA: running frame=" + String(frame_count_);
}

}  // namespace emulator
