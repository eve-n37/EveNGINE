#include "archive.hpp"
#include <array>
#include <fstream>
#include <stdexcept>
namespace eve {
uint32_t crc32(const void *data, size_t size, uint32_t previous) {
  static const auto table = [] {
    std::array<uint32_t, 256> t{};
    for (uint32_t n = 0; n < 256; n++) {
      uint32_t c = n;
      for (int k = 0; k < 8; k++)
        c = c & 1 ? 0xedb88320u ^ (c >> 1) : c >> 1;
      t[n] = c;
    }
    return t;
  }();
  auto *bytes = static_cast<const unsigned char *>(data);
  uint32_t value = ~previous;
  for (size_t i = 0; i < size; i++)
    value = table[(value ^ bytes[i]) & 0xff] ^ (value >> 8);
  return ~value;
}
namespace {
void put16(std::string &out, uint32_t v) {
  out += char(v & 0xff);
  out += char((v >> 8) & 0xff);
}
void put32(std::string &out, uint32_t v) {
  put16(out, v & 0xffff);
  put16(out, v >> 16);
}
} // namespace
void writeZip(const std::filesystem::path &path, const std::vector<ZipEntry> &entries) {
  // Fixed timestamps (1 January 1980) keep archives reproducible.
  const uint32_t time = 0, date = 1 << 5 | 1, utf8 = 0x0800;
  std::string zip, central;
  for (auto &e : entries) {
    if (e.data.size() >= 0xffffffffu || e.name.size() > 0xffff || zip.size() >= 0xffffffffu)
      throw std::runtime_error("The export is too large for a ZIP file.");
    uint32_t crc = crc32(e.data.data(), e.data.size()), size = uint32_t(e.data.size()),
             offset = uint32_t(zip.size());
    put32(zip, 0x04034b50);
    put16(zip, 10); // version needed: stored entries
    put16(zip, utf8);
    put16(zip, 0); // stored, no compression
    put16(zip, time);
    put16(zip, date);
    put32(zip, crc);
    put32(zip, size);
    put32(zip, size);
    put16(zip, uint32_t(e.name.size()));
    put16(zip, 0);
    zip += e.name;
    zip += e.data;
    uint32_t mode = 0100000 | (e.executable ? 0755 : 0644);
    put32(central, 0x02014b50);
    put16(central, 3 << 8 | 20); // made on Unix, so unzip tools apply the permissions
    put16(central, 10);
    put16(central, utf8);
    put16(central, 0);
    put16(central, time);
    put16(central, date);
    put32(central, crc);
    put32(central, size);
    put32(central, size);
    put16(central, uint32_t(e.name.size()));
    put16(central, 0);
    put16(central, 0);
    put16(central, 0);
    put16(central, 0);
    put32(central, mode << 16);
    put32(central, offset);
    central += e.name;
  }
  if (zip.size() + central.size() >= 0xffffffffu || entries.size() > 0xffff)
    throw std::runtime_error("The export is too large for a ZIP file.");
  uint32_t start = uint32_t(zip.size());
  zip += central;
  put32(zip, 0x06054b50);
  put16(zip, 0);
  put16(zip, 0);
  put16(zip, uint32_t(entries.size()));
  put16(zip, uint32_t(entries.size()));
  put32(zip, uint32_t(central.size()));
  put32(zip, start);
  put16(zip, 0);
  std::ofstream file(path, std::ios::binary);
  file.write(zip.data(), std::streamsize(zip.size()));
  file.close();
  if (!file)
    throw std::runtime_error("Cannot write " + path.filename().string());
}
} // namespace eve
