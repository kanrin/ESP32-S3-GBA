#include "drivers/display_ili9488.h"

#include <SD.h>

#include "boards/pin_config.h"
#include "drivers/default_bg.h"


namespace drivers {

bool DisplayILI9488::begin() {
  Serial.println("[Display] begin()");

  // Backlight on
  if (board::display::kBacklightOnGpio) {
    pinMode(board::display::kBacklight, OUTPUT);
    digitalWrite(board::display::kBacklight, HIGH);
    Serial.println("[Display] Backlight ON (GPIO 21)");
  }

  // Hardware reset before init
  pinMode(board::display::kRst, OUTPUT);
  digitalWrite(board::display::kRst, HIGH);
  delay(10);
  digitalWrite(board::display::kRst, LOW);
  delay(20);
  digitalWrite(board::display::kRst, HIGH);
  delay(150);
  Serial.println("[Display] Hardware reset done");

  // Initialize TFT
  tft_.init();
  Serial.println("[Display] tft_.init() done");

  tft_.setRotation(1);  // Landscape: 480x320
  tft_.invertDisplay(0);
  tft_.setSwapBytes(true);

  // Test pattern: RGB color bars
  Serial.println("[Display] Drawing test pattern...");
  tft_.fillScreen(TFT_RED);
  delay(300);
  tft_.fillScreen(TFT_GREEN);
  delay(300);
  tft_.fillScreen(TFT_BLUE);
  delay(300);
  tft_.fillScreen(TFT_BLACK);
  delay(200);

  // Draw color bars
  int w = tft_.width();   // 480
  int h = tft_.height();  // 320
  int bar_w = w / 4;
  tft_.fillRect(0,      0, bar_w, h, TFT_RED);
  tft_.fillRect(bar_w,  0, bar_w, h, TFT_GREEN);
  tft_.fillRect(bar_w*2, 0, bar_w, h, TFT_BLUE);
  tft_.fillRect(bar_w*3, 0, bar_w, h, TFT_YELLOW);
  delay(500);

  tft_.fillScreen(TFT_BLACK);
  Serial.println("[Display] Test pattern done");

  ready_ = true;
  return true;
}

void DisplayILI9488::showSplash(const String& text) {
  if (!ready_) {
    return;
  }
  const int w = tft_.width();
  tft_.fillScreen(TFT_BLACK);

  tft_.fillRect(0, 0, w, 72, TFT_WHITE);
  tft_.setTextDatum(TL_DATUM);
  tft_.setTextColor(TFT_BLACK, TFT_WHITE);
  tft_.setTextSize(3);
  tft_.setCursor(10, 16);
  tft_.println("ESP32-S3 OK");

  tft_.fillRect(0, 72, w, 48, TFT_YELLOW);
  tft_.setTextColor(TFT_BLACK, TFT_YELLOW);
  tft_.setTextSize(2);
  tft_.setCursor(10, 84);
  tft_.println(text);
}

void DisplayILI9488::fillScreen(uint16_t color) {
  if (ready_) {
    tft_.fillScreen(color);
  }
}

void DisplayILI9488::drawText(int16_t x, int16_t y, const String& text, uint16_t fg, uint16_t bg) {
  if (!ready_) {
    return;
  }
  constexpr int kLineH = 22;
  tft_.setTextDatum(TL_DATUM);
  tft_.setTextSize(2);
  tft_.fillRect(x, y, tft_.width() - x, kLineH, bg);
  tft_.setTextColor(fg, bg);
  tft_.setCursor(x, y);
  tft_.print(text);
}

void DisplayILI9488::pushFrame160x144(const std::vector<uint16_t>& frame) {
  if (!ready_ || frame.size() != 160 * 144) {
    return;
  }

  // Full-screen rendering: 160x144 -> 480x320
  // Scale 3x horizontally (160*3 = 480), ~2.222x vertically (144*2.222 = 320)
  // Use nearest-neighbor interpolation for performance
  static std::vector<uint16_t> scaled(480 * 320);
  for (int dstY = 0; dstY < 320; ++dstY) {
    const int srcY = (dstY * 144) / 320;
    for (int dstX = 0; dstX < 480; ++dstX) {
      const int srcX = (dstX * 160) / 480;
      scaled[(dstY * 480) + dstX] = frame[(srcY * 160) + srcX];
    }
  }

  tft_.startWrite();
  tft_.setAddrWindow(0, 0, 480, 320);
  tft_.pushColors(reinterpret_cast<uint16_t*>(scaled.data()), static_cast<int32_t>(scaled.size()), true);
  tft_.endWrite();
}

void DisplayILI9488::pushFrame240x160(const std::vector<uint16_t>& frame) {
  if (!ready_ || frame.size() != 240 * 160) {
    return;
  }

  // Full-screen rendering: 240x160 -> 480x320
  // Scale 2x horizontally (240*2 = 480), 2x vertically (160*2 = 320)
  static std::vector<uint16_t> scaled(480 * 320);
  for (int y = 0; y < 160; ++y) {
    for (int x = 0; x < 240; ++x) {
      const uint16_t px = frame[(y * 240) + x];
      const int dstIndex = (y * 2 * 480) + (x * 2);
      scaled[dstIndex] = px;
      scaled[dstIndex + 1] = px;
      scaled[dstIndex + 480] = px;
      scaled[dstIndex + 481] = px;
    }
  }

  tft_.startWrite();
  tft_.setAddrWindow(0, 0, 480, 320);
  tft_.pushColors(reinterpret_cast<uint16_t*>(scaled.data()), static_cast<int32_t>(scaled.size()), true);
  tft_.endWrite();
}

bool DisplayILI9488::pushRawImage(const String& filepath) {
  if (!ready_) {
    return false;
  }

  File file = SD.open(filepath, FILE_READ);
  if (!file) {
    Serial.printf("[Display] Failed to open raw image: %s\n", filepath.c_str());
    return false;
  }

  // Expected: 480 * 320 * 2 = 307,200 bytes (RGB565, no header)
  constexpr size_t kExpectedSize = 480 * 320 * 2;
  size_t fileSize = file.size();

  if (fileSize < kExpectedSize) {
    Serial.printf("[Display] Raw image too small: %u < %u\n", fileSize, kExpectedSize);
    file.close();
    return false;
  }

  // Read the raw pixel data in chunks to avoid large stack allocations
  constexpr size_t kChunkSize = 480 * 16;  // 16 lines at a time
  static std::vector<uint16_t> chunk(kChunkSize);

  tft_.startWrite();
  tft_.setAddrWindow(0, 0, 480, 320);

  for (int y = 0; y < 320; y += 16) {
    int lines = (y + 16 <= 320) ? 16 : (320 - y);
    size_t bytesToRead = lines * 480 * 2;
    size_t pixelsToPush = lines * 480;

    // Read chunk from file
    size_t bytesRead = file.read(reinterpret_cast<uint8_t*>(chunk.data()), bytesToRead);
    if (bytesRead != bytesToRead) {
      Serial.printf("[Display] Read error at line %d: got %u, expected %u\n", y, bytesRead, bytesToRead);
      break;
    }

    tft_.pushColors(chunk.data(), static_cast<int32_t>(pixelsToPush), true);
  }

  tft_.endWrite();
  file.close();
  Serial.println("[Display] Raw image displayed successfully");
  return true;
}

void DisplayILI9488::pushDefaultBackground() {
  if (!ready_) {
    return;
  }

  Serial.println("[Display] Pushing embedded default background (Pokemon.jpg)");
  tft_.startWrite();
  tft_.setAddrWindow(0, 0, kDefaultBgWidth, kDefaultBgHeight);
  tft_.pushColors(const_cast<uint16_t*>(kDefaultBg), kDefaultBgWidth * kDefaultBgHeight, true);
  tft_.endWrite();
  Serial.println("[Display] Default background displayed");
}

void DisplayILI9488::drawMenuOverlay(const std::vector<String>& entries, int selected) {

  if (!ready_) {
    return;
  }

  const int w = tft_.width();   // 480
  const int h = tft_.height();  // 320

  // Top bar: device name
  tft_.fillRect(0, 0, w, 24, TFT_BLACK);
  tft_.setTextDatum(TL_DATUM);
  tft_.setTextColor(TFT_CYAN, TFT_BLACK);
  tft_.setTextSize(1);
  tft_.setCursor(8, 6);
  tft_.print("ESP32-S3 GBA Emulator");

  // Bottom bar: ROM selection overlay
  const int barY = h - 64;
  const int barH = 64;
  tft_.fillRect(0, barY, w, barH, TFT_BLACK);

  if (entries.empty()) {
    // No ROMs found
    tft_.setTextColor(TFT_RED, TFT_BLACK);
    tft_.setTextSize(2);
    tft_.setCursor(10, barY + 12);
    tft_.print("No ROMs found!");
    tft_.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft_.setTextSize(1);
    tft_.setCursor(10, barY + 40);
    tft_.print("Place .gba/.gb/.gbc files in /roms/");
    return;
  }

  // Draw title with ROM count
  tft_.setTextColor(TFT_WHITE, TFT_BLACK);
  tft_.setTextSize(2);
  tft_.setCursor(10, barY + 2);
  tft_.printf("Select ROM  [%d/%d]", selected + 1, (int)entries.size());

  // Draw current selection with highlight
  String entryName = entries[selected];
  // Extract filename only (remove path)
  int slashPos = entryName.lastIndexOf('/');
  if (slashPos >= 0) {
    entryName = entryName.substring(slashPos + 1);
  }

  tft_.fillRect(10, barY + 26, w - 20, 24, TFT_NAVY);
  tft_.setTextColor(TFT_YELLOW, TFT_NAVY);
  tft_.setTextSize(2);
  tft_.setCursor(14, barY + 28);
  tft_.print("> ");
  tft_.print(entryName);

  // Draw navigation hint
  tft_.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft_.setTextSize(1);
  tft_.setCursor(10, barY + 54);
  tft_.print("UP/DOWN: navigate   A: select   B: audio");
}


}  // namespace drivers
