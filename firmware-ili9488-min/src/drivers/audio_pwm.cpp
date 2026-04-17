#include "drivers/audio_pwm.h"

#include "boards/pin_config.h"

namespace drivers {

void AudioPwm::begin() {
  ledcSetup(channel_, 2000, 10);
  ledcAttachPin(board::audio::kPwmOut, channel_);
}

void AudioPwm::stop() {
  ledcWriteTone(channel_, 0);
}

void AudioPwm::playTone(uint32_t frequency_hz, uint16_t duration_ms) {
  ledcWriteTone(channel_, frequency_hz);
  delay(duration_ms);
  stop();
}

}  // namespace drivers
