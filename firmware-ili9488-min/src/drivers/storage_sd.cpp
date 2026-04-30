#include "drivers/storage_sd.h"

#include <cstring>
#include "esp_log.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "vfs_fat.h"

#include "boards/pin_config.h"

static const char* TAG = "StorageSD";

namespace {

bool hasRomExtension(const std::string& name) {
  // Convert to lowercase for comparison
  std::string lower = name;
  for (auto& c : lower) c = tolower(c);
  return lower.ends_with(".gba") || lower.ends_with(".gb") ||
         lower.ends_with(".gbc") || lower.ends_with(".zip");
}

}  // namespace

namespace drivers {

bool StorageSd::begin() {
  ESP_LOGI(TAG, "begin()");

  // Initialize SPI bus for SD card
  // Note: SD card shares SPI2_HOST with display, but uses different CS
  // We need to re-configure the bus or use a separate bus.
  // For simplicity, we use SPI3_HOST for SD card to avoid conflicts.
  
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SPI3_HOST;

  spi_bus_config_t bus_cfg = {};
  bus_cfg.mosi_io_num = board::display::kMosi;   // Share MOSI with display
  bus_cfg.miso_io_num = board::display::kMiso;   // Share MISO with display
  bus_cfg.sclk_io_num = board::display::kSclk;   // Share SCLK with display
  bus_cfg.quadwp_io_num = -1;
  bus_cfg.quadhd_io_num = -1;
  bus_cfg.max_transfer_sz = 4096;

  esp_err_t ret = spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "SPI bus init failed: %d", ret);
    return false;
  }

  // This initializes the slot without card detect (CD) and write protect (WP) pins
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = static_cast<gpio_num_t>(board::storage::kSdCs);
  slot_config.host_id = SPI3_HOST;

  // Mount FAT filesystem
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
  mount_config.format_if_mount_failed = false;
  mount_config.max_files = 5;
  mount_config.allocation_unit_size = 16 * 1024;

  sdmmc_card_t* card;
  ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "SD card mount failed: %d", ret);
    return false;
  }

  // Card info
  sdmmc_card_print_info(stdout, card);
  ESP_LOGI(TAG, "SD card mounted at /sdcard");

  ready_ = true;
  return true;
}

std::vector<std::string> StorageSd::listRomFiles(const std::string& folder) const {
  std::vector<std::string> files;
  if (!ready_) return files;

  // Open directory using stdio
  std::string path = "/sdcard" + folder;
  DIR* dir = opendir(path.c_str());
  if (!dir) {
    ESP_LOGE(TAG, "Failed to open directory: %s", path.c_str());
    return files;
  }

  struct dirent* entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_type == DT_REG) {
      std::string name(entry->d_name);
      if (hasRomExtension(name)) {
        files.push_back(folder + "/" + name);
      }
    }
  }
  closedir(dir);

  return files;
}

bool StorageSd::readFile(const std::string& path, std::vector<uint8_t>* out) const {
  if (!ready_ || out == nullptr) return false;

  std::string full_path = "/sdcard" + path;
  FILE* file = fopen(full_path.c_str(), "rb");
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file: %s", full_path.c_str());
    return false;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (file_size <= 0) {
    fclose(file);
    return false;
  }

  // Read file
  out->clear();
  out->resize(file_size);
  size_t bytes_read = fread(out->data(), 1, file_size, file);
  fclose(file);

  if (bytes_read != static_cast<size_t>(file_size)) {
    ESP_LOGE(TAG, "Read %u bytes, expected %ld", bytes_read, file_size);
    return false;
  }

  return true;
}

}  // namespace drivers
