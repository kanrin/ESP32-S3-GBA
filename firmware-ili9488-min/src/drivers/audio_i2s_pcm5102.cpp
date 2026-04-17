#include "drivers/audio_i2s_pcm5102.h"

#include <I2S.h>
#include <math.h>

#include "boards/pin_config.h"

namespace drivers {

bool AudioI2sPcm5102::begin(uint32_t sample_rate_hz) {
  I2S.setSckPin(board::audio::kI2sBclk);
  I2S.setFsPin(board::audio::kI2sLrck);
  I2S.setDataPin(board::audio::kI2sDout);
  ready_ = I2S.begin(I2S_PHILIPS_MODE, static_cast<int>(sample_rate_hz), 16) == 1;
  sample_rate_hz_ = sample_rate_hz;
  return ready_;
}

bool AudioI2sPcm5102::writeMonoSamples(const int16_t* samples, size_t sample_count) {
  if (!ready_ || samples == nullptr || sample_count == 0) {
    return false;
  }

  static int16_t stereo[1024];
  const size_t mono_count = sample_count > 512 ? 512 : sample_count;
  for (size_t i = 0; i < mono_count; ++i) {
    stereo[i * 2] = samples[i];
    stereo[(i * 2) + 1] = samples[i];
  }

  const size_t written = I2S.write(reinterpret_cast<const uint8_t*>(stereo), mono_count * sizeof(int16_t) * 2);
  return written > 0;
}

bool AudioI2sPcm5102::playTestTone(uint32_t frequency_hz, uint16_t duration_ms) {
  if (!ready_ || frequency_hz == 0 || duration_ms == 0) {
    return false;
  }

  constexpr size_t kChunkSize = 128;
  int16_t mono[kChunkSize];
  const uint32_t total_samples = (sample_rate_hz_ * duration_ms) / 1000;
  const float two_pi = 6.28318530718f;

  uint32_t produced = 0;
  while (produced < total_samples) {
    const uint32_t remain = total_samples - produced;
    const size_t chunk = remain > kChunkSize ? kChunkSize : remain;
    for (size_t i = 0; i < chunk; ++i) {
      const float phase = two_pi * static_cast<float>(produced + i) * static_cast<float>(frequency_hz) /
                          static_cast<float>(sample_rate_hz_);
      mono[i] = static_cast<int16_t>(sinf(phase) * 2400.0f);
    }
    if (!writeMonoSamples(mono, chunk)) {
      return false;
    }
    produced += chunk;
  }
  return true;
}

void AudioI2sPcm5102::end() {
  I2S.end();
  ready_ = false;
}

}  // namespace drivers
