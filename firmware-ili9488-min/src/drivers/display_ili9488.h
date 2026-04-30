#pragma once

#include <stdint.h>
#include <vector>
#include <string>

#include "driver/spi_master.h"

namespace drivers {

class DisplayILI9488 {
 public:
  bool begin();
  void showSplash(const std::string& text);
  void fillScreen(uint16_t color);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void drawText(int16_t x, int16_t y, const std::string& text, uint16_t fg, uint16_t bg);
  void pushFrame160x144(const std::vector<uint16_t>& frame);
  void pushFrame240x160(const std::vector<uint16_t>& frame);

  // Load and display a raw RGB565 image from SD card
  // The raw file should be 480x320 pixels, 16-bit RGB565, no header
  // Total size: 480 * 320 * 2 = 307,200 bytes
  bool pushRawImage(const std::string& filepath);

  // Display the embedded default background image (Pokemon.jpg compiled into firmware)
  void pushDefaultBackground();

  // Draw ROM menu overlay on top of current display content
  void drawMenuOverlay(const std::vector<std::string>& entries, int selected);

 private:
  // Low-level SPI helpers
  void spiWriteCommand(uint8_t cmd);
  void spiWriteData(const uint8_t* data, size_t len);
  void spiWriteData16(const uint16_t* data, size_t len);
  void spiWriteData16Repeat(uint16_t color, size_t count);
  void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
  void hardwareReset();
  void backlightOn();

  spi_device_handle_t spi_handle_ = nullptr;
  bool ready_ = false;
};

}  // namespace drivers
