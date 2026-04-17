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

  static std::vector<uint16_t> scaled(320 * 288);
  for (int y = 0; y < 144; ++y) {
    for (int x = 0; x < 160; ++x) {
      const uint16_t px = frame[(y * 160) + x];
      const int dstIndex = (y * 2 * 320) + (x * 2);
      scaled[dstIndex] = px;
      scaled[dstIndex + 1] = px;
      scaled[dstIndex + 320] = px;
      scaled[dstIndex + 321] = px;
    }
  }

  const int xOffset = (480 - 320) / 2;
  const int yOffset = (320 - 288) / 2;
  tft_.startWrite();
  tft_.setAddrWindow(xOffset, yOffset, 320, 288);
  tft_.pushColors(reinterpret_cast<uint16_t*>(scaled.data()), static_cast<int32_t>(scaled.size()), true);
  tft_.endWrite();
}

}  // namespace drivers
