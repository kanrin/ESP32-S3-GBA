#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace drivers {

class StorageSd {
 public:
  bool begin();
  std::vector<std::string> listRomFiles(const std::string& folder) const;
  bool readFile(const std::string& path, std::vector<uint8_t>* out) const;

 private:
  bool ready_ = false;
};

}  // namespace drivers
