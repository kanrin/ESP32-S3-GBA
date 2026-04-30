#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <vector>

namespace drivers {

class DisplayILI9488 {
 public:
  bool begin();
  void showSplash(const String& text);
  void fillScreen(uint16_t color);
  void drawText(int16_t x, int16_t y, const String& text, uint16_t fg, uint16_t bg);
  void pushFrame160x144(const std::vector<uint16_t>& frame);
  void pushFrame240x160(const std::vector<uint16_t>& frame);

 private:
  TFT_eSPI tft_;
  bool ready_ = false;
};

}  // namespace drivers
