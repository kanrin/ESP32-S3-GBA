#include "drivers/audio_i2s_pcm5102.h"

#include <cmath>
#include <cstring>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "boards/pin_config.h"

static const char* TAG = "AudioI2S";

namespace drivers {

bool AudioI2sPcm5102::begin(uint32_t sample_rate_hz) {
  ESP_LOGI(TAG, "begin() sample_rate=%lu", sample_rate_hz);

  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);

  esp_err_t ret = i2s_new_channel(&chan_cfg, &tx_handle_, nullptr);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "I2S new channel failed: %d", ret);
    return false;
  }

  i2s_std_config_t std_cfg = {};
  std_cfg.clk_cfg.sample_rate_hz = sample_rate_hz;
  std_cfg.clk_cfg.clk_src = I2S_CLK_SRC_DEFAULT;
  std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
  std_cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_STEREO;
  std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;
  std_cfg.slot_cfg.bit_width = I2S_DATA_BIT_WIDTH_16BIT;
  std_cfg.slot_cfg.ws_width = I2S_DATA_BIT_WIDTH_16BIT;
  std_cfg.slot_cfg.ws_pol = false;
  std_cfg.slot_cfg.bit_shift = true;
  std_cfg.gpio_cfg.bclk = static_cast<gpio_num_t>(board::audio::kI2sBclk);
  std_cfg.gpio_cfg.ws = static_cast<gpio_num_t>(board::audio::kI2sLrck);
  std_cfg.gpio_cfg.dout = static_cast<gpio_num_t>(board::audio::kI2sDout);
  std_cfg.gpio_cfg.din = I2S_GPIO_UNUSED;
  std_cfg.gpio_cfg.invert_flags = {};

  ret = i2s_channel_init_std_mode(tx_handle_, &std_cfg);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "I2S init std mode failed: %d", ret);
    i2s_del_channel(tx_handle_);
    tx_handle_ = nullptr;
    return false;
  }

  ret = i2s_channel_enable(tx_handle_);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "I2S channel enable failed: %d", ret);
    i2s_del_channel(tx_handle_);
    tx_handle_ = nullptr;
    return false;
  }

  sample_rate_hz_ = sample_rate_hz;
  ready_ = true;
  return true;
}

bool AudioI2sPcm5102::writeMonoSamples(const int16_t* samples, size_t sample_count) {
  if (!ready_ || samples == nullptr || sample_count == 0) return false;

  // Convert mono to stereo by duplicating each sample
  static int16_t stereo[1024];
  const size_t mono_count = sample_count > 512 ? 512 : sample_count;
  for (size_t i = 0; i < mono_count; ++i) {
    stereo[i * 2] = samples[i];
    stereo[(i * 2) + 1] = samples[i];
  }

  size_t bytes_written = 0;
  esp_err_t ret = i2s_channel_write(tx_handle_, stereo,
                                     mono_count * sizeof(int16_t) * 2,
                                     &bytes_written, portMAX_DELAY);
  return ret == ESP_OK && bytes_written > 0;
}

bool AudioI2sPcm5102::playTestTone(uint32_t frequency_hz, uint16_t duration_ms) {
  if (!ready_ || frequency_hz == 0 || duration_ms == 0) return false;

  constexpr size_t kChunkSize = 128;
  int16_t mono[kChunkSize];
  const uint32_t total_samples = (sample_rate_hz_ * duration_ms) / 1000;
  const float two_pi = 6.28318530718f;

  uint32_t produced = 0;
  while (produced < total_samples) {
    const uint32_t remain = total_samples - produced;
    const size_t chunk = remain > kChunkSize ? kChunkSize : remain;
    for (size_t i = 0; i < chunk; ++i) {
      const float phase = two_pi * static_cast<float>(produced + i) *
                          static_cast<float>(frequency_hz) /
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
  if (ready_) {
    i2s_channel_disable(tx_handle_);
    i2s_del_channel(tx_handle_);
    tx_handle_ = nullptr;
    ready_ = false;
  }
}

}  // namespace drivers
