#pragma once

#include <filesystem>

namespace geg {
  namespace fs = std::filesystem;

  bool is_file(const fs::path &path);
  bool is_directory(const fs::path &path);
  bool is_empty(const fs::path &path);
  std::string read_file(const fs::path &path);
  std::vector<uint8_t> read_file_binary(const fs::path &path);
}    // namespace geg
