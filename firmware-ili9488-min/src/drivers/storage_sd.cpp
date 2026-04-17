#include "drivers/storage_sd.h"

#include <SD.h>
#include <SPI.h>

#include "boards/pin_config.h"

namespace {

bool hasRomExtension(const String& name) {
  const String lower = name;
  return lower.endsWith(".gb") || lower.endsWith(".gbc") || lower.endsWith(".zip");
}

}  // namespace

namespace drivers {

bool StorageSd::begin() {
  ready_ = SD.begin(board::storage::kSdCs);
  return ready_;
}

std::vector<String> StorageSd::listRomFiles(const String& folder) const {
  std::vector<String> files;
  if (!ready_) {
    return files;
  }

  File dir = SD.open(folder);
  if (!dir || !dir.isDirectory()) {
    return files;
  }

  File entry = dir.openNextFile();
  while (entry) {
    if (!entry.isDirectory()) {
      String name = String(entry.name());
      name.toLowerCase();
      if (hasRomExtension(name)) {
        files.push_back(String(entry.path()));
      }
    }
    entry = dir.openNextFile();
  }
  return files;
}

bool StorageSd::readFile(const String& path, std::vector<uint8_t>* out) const {
  if (!ready_ || out == nullptr) {
    return false;
  }

  File file = SD.open(path, FILE_READ);
  if (!file) {
    return false;
  }

  out->clear();
  out->reserve(file.size());
  while (file.available()) {
    out->push_back(static_cast<uint8_t>(file.read()));
  }
  return true;
}

}  // namespace drivers
