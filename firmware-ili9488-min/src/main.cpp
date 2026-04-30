#include <Arduino.h>

#include "app/rom_menu.h"
#include "drivers/audio_i2s_pcm5102.h"
#include "drivers/audio_pwm.h"
#include "drivers/display_ili9488.h"
#include "drivers/input_keys.h"
#include "drivers/storage_sd.h"
#include "emulator/gbc_adapter.h"

namespace {

drivers::DisplayILI9488 g_display;
drivers::InputKeys g_input;
drivers::StorageSd g_storage;
drivers::AudioPwm g_audio_pwm;
drivers::AudioI2sPcm5102 g_audio_i2s;
app::RomMenu g_menu;
emulator::GbcAdapter g_gbc;

bool g_rom_loaded = false;
bool g_use_i2s_audio = false;
uint32_t g_last_ui_update_ms = 0;

void drawStatus(const String& text) {
  g_display.fillScreen(TFT_WHITE);
  g_display.drawText(12, 12, "ESP32-S3 GBC MVP", TFT_WHITE, TFT_RED);
  g_display.drawText(12, 44, text, TFT_YELLOW, TFT_BLACK);
  g_display.drawText(12, 76, "A:load/run B:audio mode", TFT_CYAN, TFT_BLACK);
  g_display.drawText(12, 104, g_use_i2s_audio ? "Audio: I2S PCM5102" : "Audio: PWM quick path", TFT_YELLOW,
                     TFT_BLACK);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Booting handheld MVP...");

  g_display.begin();
  g_display.showSplash("Bring-up");

  g_input.begin();
  g_audio_pwm.begin();
  g_audio_i2s.begin();

  if (!g_storage.begin()) {
    Serial.println("SD init failed, check TF wiring.");
    drawStatus("SD init failed, check TF wiring.");
  } else {
    auto roms = g_storage.listRomFiles("/roms");
    g_menu.setEntries(roms);
    Serial.println(g_menu.renderText());
    drawStatus(g_menu.renderText());
  }

  g_gbc.begin(&g_display, &g_audio_pwm);
  Serial.println("init ok");
}

void loop() {
  const drivers::InputState input = g_input.poll();
  g_menu.update(input);

  if (!g_rom_loaded && g_menu.confirmedSelection()) {
    g_rom_loaded = g_gbc.loadRom(g_storage, g_menu.selectedPath());
    if (g_rom_loaded) {
      Serial.println("ROM loaded");
      g_display.showSplash("ROM loaded");
      delay(250);
    } else {
      Serial.println("ROM load failed");
      drawStatus("ROM load failed");
      delay(250);
    }
  }

  if (g_rom_loaded) {
    g_gbc.setPwmFeedbackEnabled(!g_use_i2s_audio);
    if (input.a && g_use_i2s_audio) {
      g_audio_i2s.playTestTone(880, 24);
    }
    g_gbc.stepFrame(input);
  } else {
    const uint32_t now = millis();
    if (now - g_last_ui_update_ms > 120) {
      drawStatus(g_menu.renderText());
      g_last_ui_update_ms = now;
    }
  }

  if (input.start && input.select) {
    g_rom_loaded = false;
    drawStatus(g_menu.renderText());
  }

  static bool last_b = false;
  if (input.b && !last_b) {
    g_use_i2s_audio = !g_use_i2s_audio;
    drawStatus(g_menu.renderText());
  }
  last_b = input.b;

  delay(16);
  // Serial.println("loop one");
}
