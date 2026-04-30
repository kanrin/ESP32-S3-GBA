#include "drivers/display_ili9488.h"

#include "boards/pin_config.h"

namespace drivers {

bool DisplayILI9488::begin() {
  if (board::display::kBacklightOnGpio) {
    pinMode(board::display::kBacklight, OUTPUT);
    digitalWrite(board::display::kBacklight, HIGH);
  }

  tft_.init();
  tft_.setRotation(1);
  tft_.invertDisplay(false);
  tft_.setSwapBytes(true);

  tft_.fillScreen(TFT_RED);
  delay(200);
  tft_.fillScreen(TFT_GREEN);
  delay(200);
  tft_.fillScreen(TFT_BLUE);
  delay(200);
  tft_.fillScreen(TFT_BLACK);
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

}  // namespace drivers
