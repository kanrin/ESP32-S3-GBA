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

  // Load and display a raw RGB565 image from SD card
  // The raw file should be 480x320 pixels, 16-bit RGB565, no header
  // Total size: 480 * 320 * 2 = 307,200 bytes
  bool pushRawImage(const String& filepath);

  // Display the embedded default background image (Pokemon.jpg compiled into firmware)
  void pushDefaultBackground();

  // Draw ROM menu overlay on top of current display content
  void drawMenuOverlay(const std::vector<String>& entries, int selected);


 private:
  TFT_eSPI tft_;
  bool ready_ = false;
};

}  // namespace drivers
