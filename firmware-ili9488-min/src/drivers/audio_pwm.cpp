#include "drivers/audio_pwm.h"

#include "esp_log.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "boards/pin_config.h"

static const char* TAG = "AudioPWM";

namespace drivers {

void AudioPwm::begin() {
  ESP_LOGI(TAG, "begin()");

  ledc_timer_config_t timer_cfg = {};
  timer_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
  timer_cfg.duty_resolution = LEDC_TIMER_10_BIT;
  timer_cfg.timer_num = LEDC_TIMER_0;
  timer_cfg.freq_hz = 2000;
  timer_cfg.clk_cfg = LEDC_AUTO_CLK;
  ledc_timer_config(&timer_cfg);

  ledc_channel_config_t chan_cfg = {};
  chan_cfg.gpio_num = board::audio::kPwmOut;
  chan_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
  chan_cfg.channel = static_cast<ledc_channel_t>(channel_);
  chan_cfg.intr_type = LEDC_INTR_DISABLE;
  chan_cfg.timer_sel = LEDC_TIMER_0;
  chan_cfg.duty = 0;
  chan_cfg.hpoint = 0;
  ledc_channel_config(&chan_cfg);
}

void AudioPwm::stop() {
  ledc_set_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(channel_), 0);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(channel_));
}

void AudioPwm::playTone(uint32_t frequency_hz, uint16_t duration_ms) {
  // Set frequency
  ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, frequency_hz);
  // Set duty to 50%
  ledc_set_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(channel_), 512);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(channel_));

  // Wait for duration
  vTaskDelay(pdMS_TO_TICKS(duration_ms));

  // Stop
  stop();
}

}  // namespace drivers
