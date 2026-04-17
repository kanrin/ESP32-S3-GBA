#pragma once

#include <Arduino.h>
#include <vector>

namespace drivers {

class StorageSd {
 public:
  bool begin();
  std::vector<String> listRomFiles(const String& folder) const;
  bool readFile(const String& path, std::vector<uint8_t>* out) const;

 private:
  bool ready_ = false;
};

}  // namespace drivers
