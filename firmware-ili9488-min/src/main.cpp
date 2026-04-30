#include <Arduino.h>

#include "app/rom_menu.h"
#include "drivers/audio_i2s_pcm5102.h"
#include "drivers/audio_pwm.h"
#include "drivers/display_ili9488.h"
#include "drivers/input_keys.h"
#include "drivers/logging.h"
#include "drivers/storage_sd.h"
#include "emulator/gbc_adapter.h"
#include "emulator/vba_adapter.h"


namespace {

drivers::DisplayILI9488 g_display;
drivers::InputKeys g_input;
drivers::StorageSd g_storage;
drivers::AudioPwm g_audio_pwm;
drivers::AudioI2sPcm5102 g_audio_i2s;
app::RomMenu g_menu;
emulator::GbcAdapter g_gbc;
emulator::VbaAdapter g_vba;


bool g_rom_loaded = false;
bool g_use_i2s_audio = false;
bool g_use_vba = false;  // true if using VBA for GBA ROMs
uint32_t g_last_ui_update_ms = 0;
bool g_background_loaded = false;
std::vector<String> g_rom_list;  // Cached ROM list

bool hasGbaExtension(const String& path) {
  String lower = path;
  lower.toLowerCase();
  return lower.endsWith(".gba");
}

void drawStatus(const String& text) {
  // Load background image (once): try SD card first, fall back to embedded default
  if (!g_background_loaded) {
    if (g_display.pushRawImage("/roms/bg.raw")) {
      Serial.println("[App] Using SD card background: /roms/bg.raw");
    } else {
      Serial.println("[App] SD background not found, using embedded default (Pokemon.jpg)");
      g_display.pushDefaultBackground();
    }
    g_background_loaded = true;
  }

  // Draw menu overlay on top of background image
  g_display.drawMenuOverlay(g_rom_list, g_menu.selectedIndex());
}



}  // namespace



void setup() {
  Serial.begin(115200);
  delay(200);
  LOG_I("Booting handheld MVP...");

  g_display.begin();
  g_display.showSplash("Bring-up");

  g_input.begin();
  g_audio_pwm.begin();
  g_audio_i2s.begin();

  if (!g_storage.begin()) {
    LOG_E("SD init failed, check TF wiring.");
    drawStatus("SD init failed, check TF wiring.");
  } else {
    g_rom_list = g_storage.listRomFiles("/roms");
    g_menu.setEntries(g_rom_list);
    LOG_I("ROM list:\r\n%s", g_menu.renderText().c_str());
    drawStatus(g_menu.renderText());
  }


  g_gbc.begin(&g_display, &g_audio_pwm);
  if (!g_vba.begin(&g_display, &g_audio_pwm)) {
    LOG_W("VBA init failed (no PSRAM?) - GBA ROMs will not be playable");
  }
  LOG_I("init ok");
}

void loop() {
  const drivers::InputState input = g_input.poll();
  g_menu.update(input);

  if (!g_rom_loaded && g_menu.confirmedSelection()) {
    const String& path = g_menu.selectedPath();
    g_use_vba = hasGbaExtension(path);

    if (g_use_vba) {
      g_rom_loaded = g_vba.loadRom(g_storage, path);
    } else {
      g_rom_loaded = g_gbc.loadRom(g_storage, path);
    }

    if (g_rom_loaded) {
      LOG_I("%s", g_use_vba ? "GBA ROM loaded (VBA)" : "ROM loaded");
      g_display.showSplash(g_use_vba ? "GBA ROM loaded" : "ROM loaded");
      delay(250);
    } else {
      LOG_E("ROM load failed");
      drawStatus("ROM load failed");
      delay(250);
    }
  }

  if (g_rom_loaded) {
    if (g_use_vba) {
      g_vba.setPwmFeedbackEnabled(!g_use_i2s_audio);
      g_vba.stepFrame(input);
    } else {
      g_gbc.setPwmFeedbackEnabled(!g_use_i2s_audio);
      if (input.a && g_use_i2s_audio) {
        g_audio_i2s.playTestTone(880, 24);
      }
      g_gbc.stepFrame(input);
    }
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
}
