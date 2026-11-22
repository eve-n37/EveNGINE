#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace eve {
// CRC-32 as used by PNG and ZIP. Pass a previous result to continue it.
uint32_t crc32(const void *data, size_t size, uint32_t previous = 0);
struct ZipEntry {
  std::string name, data;
  bool executable = false;
};
// Writes an uncompressed ZIP with UTF-8 names and Unix permissions. Entries go in the order given.
// Throws std::runtime_error when the file can't be written or an entry is too large.
void writeZip(const std::filesystem::path &path, const std::vector<ZipEntry> &entries);
} // namespace eve
