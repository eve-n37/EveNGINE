#include "png.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <stdexcept>
namespace eve {
namespace {
uint32_t crc(const unsigned char *data, size_t size, uint32_t value = 0) {
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
  value = ~value;
  for (size_t i = 0; i < size; i++)
    value = table[(value ^ data[i]) & 0xff] ^ (value >> 8);
  return ~value;
}
void put32(std::string &out, uint32_t v) {
  for (int shift = 24; shift >= 0; shift -= 8)
    out += char((v >> shift) & 0xff);
}
void chunk(std::string &out, const char *type, const std::string &data) {
  put32(out, uint32_t(data.size()));
  std::string body = std::string(type, 4) + data;
  out += body;
  put32(out, crc(reinterpret_cast<const unsigned char *>(body.data()), body.size()));
}
} // namespace
void writePng(const std::string &path, int width, int height,
              const std::vector<unsigned char> &rgba) {
  if (width < 1 || height < 1 || width > 4096 || height > 4096 ||
      rgba.size() != size_t(width) * height * 4)
    throw std::runtime_error("Invalid PNG image size.");
  // Each row starts with filter type 0 (none).
  std::string raw;
  raw.reserve(size_t(height) * (width * 4 + 1));
  for (int y = 0; y < height; y++) {
    raw += '\0';
    raw.append(reinterpret_cast<const char *>(&rgba[size_t(y) * width * 4]), size_t(width) * 4);
  }
  // zlib stream of stored deflate blocks, each up to 65535 bytes, then an Adler-32 checksum.
  std::string zlib = "\x78\x01";
  uint32_t a = 1, b = 0;
  // raw is never empty: every row has at least its filter byte.
  for (size_t at = 0;;) {
    size_t size = std::min<size_t>(65535, raw.size() - at);
    bool last = at + size == raw.size();
    zlib += char(last ? 1 : 0);
    zlib += char(size & 0xff);
    zlib += char(size >> 8);
    zlib += char(~size & 0xff);
    zlib += char((~size >> 8) & 0xff);
    zlib.append(raw, at, size);
    for (size_t i = at; i < at + size; i++) {
      a = (a + static_cast<unsigned char>(raw[i])) % 65521;
      b = (b + a) % 65521;
    }
    at += size;
    if (last)
      break;
  }
  put32(zlib, b << 16 | a);
  std::string png = "\x89PNG\r\n\x1a\n", header;
  put32(header, uint32_t(width));
  put32(header, uint32_t(height));
  header += std::string("\x08\x06\x00\x00\x00", 5); // 8-bit RGBA, no interlace
  chunk(png, "IHDR", header);
  chunk(png, "IDAT", zlib);
  chunk(png, "IEND", "");
  std::ofstream file(path, std::ios::binary);
  file.write(png.data(), std::streamsize(png.size()));
  file.close();
  if (!file)
    throw std::runtime_error("Cannot write " + path);
}
} // namespace eve
