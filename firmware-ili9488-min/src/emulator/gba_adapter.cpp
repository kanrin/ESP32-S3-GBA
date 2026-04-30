#include "emulator/gba_adapter.h"

// VBA emulator core declarations (from gba.h / globals.h)
extern "C" {
#include "emulator/vba/gba.h"
#include "emulator/vba/globals.h"
#include "emulator/vba/memory.h"
#include "emulator/vba/system.h"
#include "emulator/vba/sound.h"
}

// Global pointer to the active GbaAdapter instance for platform callbacks
static emulator::GbaAdapter* g_active_gba = nullptr;

// ============================================================
// Platform function implementations required by VBA core
// These are declared in system.h and called from gba.cpp
// ============================================================

extern "C" void systemDrawScreen(void) {
  if (g_active_gba) {
    g_active_gba->onDrawScreen();
  }
}

extern "C" bool systemReadJoypads(void) {
  if (g_active_gba) {
    return g_active_gba->onReadJoypads();
  }
  return false;
}

extern "C" uint32_t systemGetClock(void) {
  if (g_active_gba) {
    return g_active_gba->onGetClock();
  }
  return micros();
}

extern "C" void systemOnWriteDataToSoundBuffer(int16_t* finalWave, int length) {
  if (g_active_gba) {
    g_active_gba->onWriteDataToSoundBuffer(finalWave, length);
  }
}

extern "C" void systemMessage(const char* msg, ...) {
  // VBA uses this for error messages - log to serial
  char buf[256];
  va_list args;
  va_start(args, msg);
  vsnprintf(buf, sizeof(buf), msg, args);
  va_end(args);
  Serial.printf("VBA: %s\n", buf);
  if (g_active_gba) {
    g_active_gba->onMessage(buf);
  }
}

#ifdef USE_MOTION_SENSOR
extern "C" void systemUpdateMotionSensor(void) {}
extern "C" int systemGetAccelX(void) { return 0; }
extern "C" int systemGetAccelY(void) { return 0; }
extern "C" int systemGetGyroZ(void) { return 0; }
extern "C" void systemSetSensorState(bool) {}
#endif

// ============================================================
// Memory allocation helpers (VBA uses memalign_alloc_aligned)
// ============================================================

extern "C" void* memalign_alloc_aligned(size_t size) {
  // On ESP32, heap_caps_aligned_alloc is available
  // For simplicity, use malloc with alignment
  void* ptr = nullptr;
  if (posix_memalign(&ptr, 16, size) != 0) {
    return nullptr;
  }
  return ptr;
}

extern "C" void memalign_free(void* ptr) {
  free(ptr);
}

// ============================================================
// GbaAdapter implementation
// ============================================================

namespace emulator {

GbaAdapter::GbaAdapter() {
  g_active_gba = this;
}

GbaAdapter::~GbaAdapter() {
  if (g_active_gba == this) {
    g_active_gba = nullptr;
  }
  CPUCleanUp();
}

bool GbaAdapter::begin(drivers::DisplayILI9488* display, drivers::AudioPwm* audio) {
  display_ = display;
  audio_ = audio;

  // Initialize VBA core with HLE BIOS (no external BIOS file needed)
  CPUInit(nullptr, false);

  return display_ != nullptr;
}

bool GbaAdapter::loadRom(drivers::StorageSd& storage, const String& rom_path) {
  // Read ROM file from SD card
  std::vector<uint8_t> rom_buffer;
  if (!storage.readFile(rom_path, &rom_buffer) || rom_buffer.empty()) {
    Serial.println("GBA: Failed to read ROM file");
    loaded_ = false;
    return false;
  }

  Serial.printf("GBA: Loaded ROM %d bytes\n", rom_buffer.size());

  // Load ROM data into VBA core
  int result = CPULoadRomData(reinterpret_cast<const char*>(rom_buffer.data()),
                               static_cast<int>(rom_buffer.size()));
  if (result == 0) {
    Serial.println("GBA: CPULoadRomData failed");
    loaded_ = false;
    return false;
  }

  // Reset the CPU to start execution
  CPUReset();

  loaded_ = true;
  frame_count_ = 0;
  frame_ready_ = false;
  initialized_ = true;

  Serial.println("GBA: ROM loaded and CPU reset");
  return true;
}

void GbaAdapter::stepFrame(const drivers::InputState& input) {
  if (!loaded_ || display_ == nullptr) {
    return;
  }

  // Map input state to GBA joypad bits
  // GBA joypad bits (active low):
  // Bit 0: A, Bit 1: B, Bit 2: Select, Bit 3: Start
  // Bit 4: Right, Bit 5: Left, Bit 6: Up, Bit 7: Down
  // Bit 8: R, Bit 9: L
  joypad_state_ = 0x03FF; // all released
  if (input.a)      joypad_state_ &= ~(1 << 0);
  if (input.b)      joypad_state_ &= ~(1 << 1);
  if (input.select) joypad_state_ &= ~(1 << 2);
  if (input.start)  joypad_state_ &= ~(1 << 3);
  if (input.right)  joypad_state_ &= ~(1 << 4);
  if (input.left)   joypad_state_ &= ~(1 << 5);
  if (input.up)     joypad_state_ &= ~(1 << 6);
  if (input.down)   joypad_state_ &= ~(1 << 7);
  if (input.r)      joypad_state_ &= ~(1 << 8);
  if (input.l)      joypad_state_ &= ~(1 << 9);

  // Set the joypad state for VBA
  joy = joypad_state_;

  // Run one frame of emulation
  frame_ready_ = false;
  CPULoop();

  // If PWM feedback is enabled and A button is pressed, play a tone
  if (pwm_feedback_enabled_ && input.a && audio_ != nullptr) {
    audio_->playTone(880, 20);
  }

  frame_count_++;
}

void GbaAdapter::onDrawScreen() {
  // VBA has rendered a frame into the pix[] buffer
  // pix[] is 256 * 160 pixels (PIX_BUFFER_SCREEN_WIDTH = 256)
  // GBA active area is 240x160, pixels are RGB565
  frame_ready_ = true;

  if (display_ == nullptr) return;

  // Convert pix buffer to 240x160 frame and push to display
  // pix is uint16_t[2 * PIX_BUFFER_SCREEN_WIDTH * 160]
  // The active GBA area is 240x160, starting at offset 0 in each line
  // PIX_BUFFER_SCREEN_WIDTH = 256, so each line has 256 pixels but only 240 are visible

  static std::vector<uint16_t> frame(240 * 160);
  for (int y = 0; y < 160; ++y) {
    for (int x = 0; x < 240; ++x) {
      frame[(y * 240) + x] = pix[(y * PIX_BUFFER_SCREEN_WIDTH) + x];
    }
  }

  display_->pushFrame240x160(frame);
}

bool GbaAdapter::onReadJoypads() {
  // Joypad state is already set in stepFrame via the 'joy' variable
  // This is called by VBA's UpdateJoypad() internally
  return true;
}

uint32_t GbaAdapter::onGetClock() {
  return micros();
}

void GbaAdapter::onWriteDataToSoundBuffer(int16_t* finalWave, int length) {
  // Audio output - for now, we don't have a proper audio pipeline
  // This will be implemented when audio support is added
  (void)finalWave;
  (void)length;
}

void GbaAdapter::onMessage(const char* msg) {
  Serial.printf("GBA Msg: %s\n", msg);
}

String GbaAdapter::statusText() const {
  if (!loaded_) {
    return "GBA: ROM not loaded";
  }
  return "GBA: running frame=" + String(frame_count_);
}

}  // namespace emulator
