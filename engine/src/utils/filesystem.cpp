#include "filesystem.hpp"

#include <fstream>
#include "pch.hpp"

namespace geg {
  auto is_file(const fs::path &path) -> bool {
    return fs::is_regular_file(path);
  }

  auto is_directory(const fs::path &path) -> bool {
    return fs::is_directory(path);
  }

  auto is_empty(const fs::path &path) -> bool {
    return fs::is_empty(path);
  }

  auto read_file(const fs::path &path) -> std::string {
    GEG_CORE_ASSERT(is_file(path), "Trying to read a file that does not exist");

    std::ifstream file{path, std::ios::ate};
    GEG_CORE_ASSERT(file.is_open(), "Failed to open file");

    auto size = file.tellg();
    std::string buffer(size, ' ');
    file.seekg(0);
    file.read(buffer.data(), size);
    file.close();

    return buffer;
  }

}    // namespace geg
