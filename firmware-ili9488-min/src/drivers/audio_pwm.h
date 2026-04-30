#pragma once

#include <stdint.h>

namespace drivers {

class AudioPwm {
 public:
  void begin();
  void stop();
  void playTone(uint32_t frequency_hz, uint16_t duration_ms);

 private:
  int channel_ = 0;
};

}  // namespace drivers
