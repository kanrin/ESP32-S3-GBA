#pragma once

#include <stdint.h>
#include <stddef.h>

#include "driver/i2s_std.h"

namespace drivers {

class AudioI2sPcm5102 {
 public:
  bool begin(uint32_t sample_rate_hz = 44100);
  bool writeMonoSamples(const int16_t* samples, size_t sample_count);
  bool playTestTone(uint32_t frequency_hz, uint16_t duration_ms);
  void end();

 private:
  bool ready_ = false;
  uint32_t sample_rate_hz_ = 44100;
  i2s_chan_handle_t tx_handle_ = nullptr;
};

}  // namespace drivers
