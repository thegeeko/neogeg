#pragma once

#include <filesystem>

namespace geg {
	namespace fs = std::filesystem;

	auto is_file(const fs::path &path) -> bool;
	auto is_directory(const fs::path &path) -> bool;
	auto is_empty(const fs::path &path) -> bool;
	auto read_file(const fs::path &path) -> std::string;
	auto read_file_binary(const fs::path &path) -> std::vector<uint8_t>;
}		 // namespace geg
