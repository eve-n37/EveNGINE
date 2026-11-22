#pragma once
#include <string>
#include <vector>
namespace eve {
// Writes 8-bit RGBA pixels as a PNG. Uses uncompressed deflate blocks, so it needs no zlib; meant
// for small images such as game icons. Throws std::runtime_error when the file can't be written.
void writePng(const std::string &path, int width, int height,
              const std::vector<unsigned char> &rgba);
} // namespace eve
