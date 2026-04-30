#include "emulator/gbc_adapter.h"

#include <cstdio>

namespace {

uint16_t gbPalette(uint8_t value) {
  switch (value & 0x3) {
    case 0:
      return 0xE79C;
    case 1:
      return 0x9E4F;
    case 2:
      return 0x34A6;
    default:
      return 0x0000;
  }
}

}  // namespace

namespace emulator {

bool GbcAdapter::begin(drivers::DisplayILI9488* display, drivers::AudioPwm* audio) {
  display_ = display;
  audio_ = audio;
  frame_buffer_.assign(160 * 144, 0);
  return display_ != nullptr && audio_ != nullptr;
}

bool GbcAdapter::loadRom(drivers::StorageSd& storage, const std::string& rom_path) {
  rom_buffer_.clear();
  loaded_ = storage.readFile(rom_path, &rom_buffer_) && !rom_buffer_.empty();
  frame_tick_ = 0;
  return loaded_;
}

void GbcAdapter::stepFrame(const drivers::InputState& input) {
  if (!loaded_ || display_ == nullptr) {
    return;
  }

  if (pwm_feedback_enabled_ && input.a && audio_ != nullptr) {
    audio_->playTone(880, 20);
  }

  // This adapter owns the platform glue path (input/audio/display cadence).
  // A full GB core can replace this test renderer without changing app code.
  for (int y = 0; y < 144; ++y) {
    for (int x = 0; x < 160; ++x) {
      const uint8_t sample = rom_buffer_[(x + y + frame_tick_) % rom_buffer_.size()];
      frame_buffer_[(y * 160) + x] = gbPalette(sample + (frame_tick_ / 10));
    }
  }

  display_->pushFrame160x144(frame_buffer_);
  ++frame_tick_;
}

std::string GbcAdapter::statusText() const {
  if (!loaded_) {
    return "GBC: ROM not loaded";
  }
  char buf[64];
  snprintf(buf, sizeof(buf), "GBC: running frame=%lu", frame_tick_);
  return std::string(buf);
}

}  // namespace emulator
