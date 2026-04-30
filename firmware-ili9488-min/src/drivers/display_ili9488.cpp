#include "drivers/display_ili9488.h"

#include <cstring>
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "boards/pin_config.h"
#include "drivers/default_bg.h"

static const char* TAG = "Display";

namespace drivers {

// ============================================================
// Low-level SPI helpers
// ============================================================

void DisplayILI9488::spiWriteCommand(uint8_t cmd) {
  // DC low = command
  gpio_set_level(static_cast<gpio_num_t>(board::display::kDc), 0);

  spi_transaction_t trans = {};
  trans.length = 8;  // 8 bits
  trans.tx_buffer = &cmd;
  spi_device_transmit(spi_handle_, &trans);
}

void DisplayILI9488::spiWriteData(const uint8_t* data, size_t len) {
  if (len == 0) return;

  // DC high = data
  gpio_set_level(static_cast<gpio_num_t>(board::display::kDc), 1);

  spi_transaction_t trans = {};
  trans.length = len * 8;  // bits
  trans.tx_buffer = data;
  spi_device_transmit(spi_handle_, &trans);
}

void DisplayILI9488::spiWriteData16(const uint16_t* data, size_t len) {
  if (len == 0) return;

  // DC high = data
  gpio_set_level(static_cast<gpio_num_t>(board::display::kDc), 1);

  // We send 16-bit data directly (MSB first for ILI9488)
  spi_transaction_t trans = {};
  trans.length = len * 16;  // bits
  trans.tx_buffer = data;
  spi_device_transmit(spi_handle_, &trans);
}

void DisplayILI9488::spiWriteData16Repeat(uint16_t color, size_t count) {
  if (count == 0) return;

  // DC high = data
  gpio_set_level(static_cast<gpio_num_t>(board::display::kDc), 1);

  // Use a small buffer to send repeated color in chunks
  constexpr size_t kChunk = 512;
  uint16_t buf[kChunk];
  for (size_t i = 0; i < kChunk; i++) {
    buf[i] = color;
  }

  size_t remaining = count;
  while (remaining > 0) {
    size_t send = (remaining > kChunk) ? kChunk : remaining;
    spi_transaction_t trans = {};
    trans.length = send * 16;
    trans.tx_buffer = buf;
    spi_device_transmit(spi_handle_, &trans);
    remaining -= send;
  }
}

void DisplayILI9488::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  spiWriteCommand(0x2A);  // Column Address Set
  uint8_t col_data[] = {
      static_cast<uint8_t>(x0 >> 8), static_cast<uint8_t>(x0 & 0xFF),
      static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1 & 0xFF)};
  spiWriteData(col_data, 4);

  spiWriteCommand(0x2B);  // Row Address Set
  uint8_t row_data[] = {
      static_cast<uint8_t>(y0 >> 8), static_cast<uint8_t>(y0 & 0xFF),
      static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1 & 0xFF)};
  spiWriteData(row_data, 4);

  spiWriteCommand(0x2C);  // Memory Write
}

void DisplayILI9488::hardwareReset() {
  gpio_set_level(static_cast<gpio_num_t>(board::display::kRst), 1);
  vTaskDelay(pdMS_TO_TICKS(10));
  gpio_set_level(static_cast<gpio_num_t>(board::display::kRst), 0);
  vTaskDelay(pdMS_TO_TICKS(20));
  gpio_set_level(static_cast<gpio_num_t>(board::display::kRst), 1);
  vTaskDelay(pdMS_TO_TICKS(150));
}

void DisplayILI9488::backlightOn() {
  if (board::display::kBacklightOnGpio) {
    gpio_set_level(static_cast<gpio_num_t>(board::display::kBacklight), 1);
    ESP_LOGI(TAG, "Backlight ON (GPIO %d)", board::display::kBacklight);
  }
}

// ============================================================
// Public API
// ============================================================

bool DisplayILI9488::begin() {
  ESP_LOGI(TAG, "begin()");

  // Configure GPIOs
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1ULL << board::display::kDc) |
                          (1ULL << board::display::kRst) |
                          (1ULL << board::display::kCs);
  if (board::display::kBacklightOnGpio) {
    io_conf.pin_bit_mask |= (1ULL << board::display::kBacklight);
  }
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  // Set CS high initially
  gpio_set_level(static_cast<gpio_num_t>(board::display::kCs), 1);

  // Backlight on
  backlightOn();

  // Hardware reset
  hardwareReset();
  ESP_LOGI(TAG, "Hardware reset done");

  // Initialize SPI bus
  spi_bus_config_t bus_cfg = {};
  bus_cfg.mosi_io_num = board::display::kMosi;
  bus_cfg.miso_io_num = board::display::kMiso;
  bus_cfg.sclk_io_num = board::display::kSclk;
  bus_cfg.quadwp_io_num = -1;
  bus_cfg.quadhd_io_num = -1;
  bus_cfg.max_transfer_sz = 480 * 320 * 2 + 8;  // Full screen buffer + overhead

  esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "SPI bus init failed: %d", ret);
    return false;
  }

  // Attach display device to SPI bus
  spi_device_interface_config_t dev_cfg = {};
  dev_cfg.clock_speed_hz = 10 * 1000 * 1000;  // 10 MHz
  dev_cfg.mode = 0;                             // SPI mode 0
  dev_cfg.spics_io_num = board::display::kCs;
  dev_cfg.queue_size = 1;
  dev_cfg.flags = SPI_DEVICE_HALFDUPLEX;  // No need for MISO for display writes

  ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_handle_);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "SPI device add failed: %d", ret);
    return false;
  }

  // ============================================================
  // ILI9488 Initialization Sequence
  // ============================================================
  vTaskDelay(pdMS_TO_TICKS(120));  // Wait after reset

  // Software Reset
  spiWriteCommand(0x01);
  vTaskDelay(pdMS_TO_TICKS(120));

  // Power Control A
  spiWriteCommand(0xCB);
  uint8_t pwr_a[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
  spiWriteData(pwr_a, 5);

  // Power Control B
  spiWriteCommand(0xCF);
  uint8_t pwr_b[] = {0x00, 0xC1, 0x30};
  spiWriteData(pwr_b, 3);

  // Driver Timing Control A
  spiWriteCommand(0xE8);
  uint8_t dtca[] = {0x85, 0x00, 0x78};
  spiWriteData(dtca, 3);

  // Driver Timing Control B
  spiWriteCommand(0xEA);
  uint8_t dtcb[] = {0x00, 0x00};
  spiWriteData(dtcb, 2);

  // Power On Sequence Control
  spiWriteCommand(0xED);
  uint8_t pwr_on[] = {0x64, 0x03, 0x12, 0x81};
  spiWriteData(pwr_on, 4);

  // Pump Ratio Control
  spiWriteCommand(0xF7);
  uint8_t pump[] = {0x20};
  spiWriteData(pump, 1);

  // Power Control 1
  spiWriteCommand(0xC0);
  uint8_t pwr1[] = {0x23};
  spiWriteData(pwr1, 1);

  // Power Control 2
  spiWriteCommand(0xC1);
  uint8_t pwr2[] = {0x10};
  spiWriteData(pwr2, 1);

  // VCOM Control 1
  spiWriteCommand(0xC5);
  uint8_t vcom1[] = {0x3E, 0x28};
  spiWriteData(vcom1, 2);

  // VCOM Control 2
  spiWriteCommand(0xC7);
  uint8_t vcom2[] = {0x86};
  spiWriteData(vcom2, 1);

  // Memory Access Control
  spiWriteCommand(0x36);
  uint8_t madctl[] = {0x48};  // MX, BGR
  spiWriteData(madctl, 1);

  // Pixel Interface Format - 16-bit RGB565
  spiWriteCommand(0x3A);
  uint8_t pixfmt[] = {0x55};
  spiWriteData(pixfmt, 1);

  // Frame Rate Control (60Hz)
  spiWriteCommand(0xB1);
  uint8_t frmctr[] = {0x00, 0x18};
  spiWriteData(frmctr, 2);

  // Display Function Control
  spiWriteCommand(0xB6);
  uint8_t dfc[] = {0x08, 0x82, 0x27};
  spiWriteData(dfc, 3);

  // 3Gamma Function Disable
  spiWriteCommand(0xF2);
  uint8_t gamma_dis[] = {0x00};
  spiWriteData(gamma_dis, 1);

  // Gamma Curve Selection
  spiWriteCommand(0x26);
  uint8_t gamma_curve[] = {0x01};
  spiWriteData(gamma_curve, 1);

  // Positive Gamma Correction
  spiWriteCommand(0xE0);
  uint8_t pos_gamma[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                         0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
  spiWriteData(pos_gamma, 15);

  // Negative Gamma Correction
  spiWriteCommand(0xE1);
  uint8_t neg_gamma[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                         0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
  spiWriteData(neg_gamma, 15);

  // Sleep Out
  spiWriteCommand(0x11);
  vTaskDelay(pdMS_TO_TICKS(120));

  // Display On
  spiWriteCommand(0x29);
  vTaskDelay(pdMS_TO_TICKS(50));

  ESP_LOGI(TAG, "ILI9488 init done");

  // Set rotation to landscape (480x320)
  // MADCTL: MX=1, MY=0, MV=0, ML=0, BGR=1, MH=0
  spiWriteCommand(0x36);
  uint8_t rot[] = {0x48};  // Landscape: MX | BGR
  spiWriteData(rot, 1);

  // Test pattern: RGB color bars
  ESP_LOGI(TAG, "Drawing test pattern...");
  fillScreen(0xF800);  // RED
  vTaskDelay(pdMS_TO_TICKS(300));
  fillScreen(0x07E0);  // GREEN
  vTaskDelay(pdMS_TO_TICKS(300));
  fillScreen(0x001F);  // BLUE
  vTaskDelay(pdMS_TO_TICKS(300));
  fillScreen(0x0000);  // BLACK
  vTaskDelay(pdMS_TO_TICKS(200));

  // Draw color bars
  int w = 480;
  int h = 320;
  int bar_w = w / 4;
  fillRect(0, 0, bar_w, h, 0xF800);     // RED
  fillRect(bar_w, 0, bar_w, h, 0x07E0); // GREEN
  fillRect(bar_w*2, 0, bar_w, h, 0x001F); // BLUE
  fillRect(bar_w*3, 0, bar_w, h, 0xFFE0); // YELLOW
  vTaskDelay(pdMS_TO_TICKS(500));

  fillScreen(0x0000);  // BLACK
  ESP_LOGI(TAG, "Test pattern done");

  ready_ = true;
  return true;
}

void DisplayILI9488::fillScreen(uint16_t color) {
  if (!ready_) return;
  setAddrWindow(0, 0, 479, 319);
  spiWriteData16Repeat(color, 480 * 320);
}

void DisplayILI9488::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if (!ready_) return;
  setAddrWindow(x, y, x + w - 1, y + h - 1);
  spiWriteData16Repeat(color, w * h);
}

void DisplayILI9488::showSplash(const std::string& text) {
  if (!ready_) return;
  const int w = 480;
  fillScreen(0x0000);  // BLACK

  // White top bar
  fillRect(0, 0, w, 72, 0xFFFF);  // WHITE
  // Text would need a font renderer - for now just show colored bars
  fillRect(0, 72, w, 48, 0xFFE0);  // YELLOW bar
}

void DisplayILI9488::drawText(int16_t x, int16_t y, const std::string& text, uint16_t fg, uint16_t bg) {
  if (!ready_) return;
  constexpr int kLineH = 22;
  fillRect(x, y, 480 - x, kLineH, bg);
  // Text rendering would need a bitmap font - placeholder
  (void)text;
  (void)fg;
}

void DisplayILI9488::pushFrame160x144(const std::vector<uint16_t>& frame) {
  if (!ready_ || frame.size() != 160 * 144) return;

  // Full-screen rendering: 160x144 -> 480x320
  // Scale 3x horizontally (160*3 = 480), ~2.222x vertically (144*2.222 = 320)
  static std::vector<uint16_t> scaled(480 * 320);
  for (int dstY = 0; dstY < 320; ++dstY) {
    const int srcY = (dstY * 144) / 320;
    for (int dstX = 0; dstX < 480; ++dstX) {
      const int srcX = (dstX * 160) / 480;
      scaled[(dstY * 480) + dstX] = frame[(srcY * 160) + srcX];
    }
  }

  setAddrWindow(0, 0, 479, 319);
  spiWriteData16(scaled.data(), scaled.size());
}

void DisplayILI9488::pushFrame240x160(const std::vector<uint16_t>& frame) {
  if (!ready_ || frame.size() != 240 * 160) return;

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

  setAddrWindow(0, 0, 479, 319);
  spiWriteData16(scaled.data(), scaled.size());
}

bool DisplayILI9488::pushRawImage(const std::string& filepath) {
  if (!ready_) return false;

  // This function requires SD card access - the SD card driver will provide
  // a file read callback. For now, this is a placeholder that will be
  // implemented when the SD card driver is integrated.
  ESP_LOGW(TAG, "pushRawImage not yet implemented (needs SD integration): %s", filepath.c_str());
  (void)filepath;
  return false;
}

void DisplayILI9488::pushDefaultBackground() {
  if (!ready_) return;

  ESP_LOGI(TAG, "Pushing embedded default background");
  setAddrWindow(0, 0, kDefaultBgWidth - 1, kDefaultBgHeight - 1);
  spiWriteData16(kDefaultBg, kDefaultBgWidth * kDefaultBgHeight);
  ESP_LOGI(TAG, "Default background displayed");
}

void DisplayILI9488::drawMenuOverlay(const std::vector<std::string>& entries, int selected) {
  if (!ready_) return;

  const int w = 480;
  const int h = 320;

  // Top bar: device name
  fillRect(0, 0, w, 24, 0x0000);  // BLACK

  // Bottom bar: ROM selection overlay
  const int barY = h - 64;
  const int barH = 64;
  fillRect(0, barY, w, barH, 0x0000);  // BLACK

  if (entries.empty()) {
    // No ROMs found - just show a colored indicator
    fillRect(10, barY + 12, 200, 16, 0xF800);  // RED bar
    return;
  }

  // Draw selection highlight
  fillRect(10, barY + 26, w - 20, 24, 0x000F);  // NAVY bar
}

}  // namespace drivers
