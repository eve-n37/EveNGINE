#include "common.hpp"
#include "png.hpp"
#include <algorithm>
#include <fstream>
#include <random>
#include <stdexcept>
#include <vector>
namespace fs = std::filesystem;
namespace eve {
namespace {
int hex(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}
} // namespace
void need(bool ok, const std::string &message) {
  if (!ok)
    throw std::runtime_error(message);
}
void writeText(const fs::path &path, const std::string &text) {
  std::ofstream out(path, std::ios::binary);
  out << text;
  out.close();
  need(bool(out), "Cannot write " + path.filename().string());
}
bool writeIcon(const lota::Json &project, const fs::path &output) {
  if (!project.has("appIcon"))
    return false;
  auto &icon = project["appIcon"];
  need(icon.type == lota::Json::Object, "Invalid game icon.");
  auto &sizeValue = icon["size"];
  need(sizeValue.type == lota::Json::Number && icon["pixels"].type == lota::Json::String,
       "Invalid game icon dimensions or pixels.");
  int size = int(sizeValue.number);
  auto &pixels = icon["pixels"].string;
  need(sizeValue.number == size && size >= 16 && size <= 512 &&
           pixels.size() == size_t(size) * size * 8,
       "Game icons must be square, 16–512 pixels, with RGBA artwork.");
  std::vector<unsigned char> source(size_t(size) * size * 4);
  bool visible = false;
  for (size_t i = 0; i < source.size(); i++) {
    int a = hex(pixels[i * 2]), b = hex(pixels[i * 2 + 1]);
    need(a >= 0 && b >= 0, "Invalid game icon pixel.");
    source[i] = static_cast<unsigned char>(a << 4 | b);
    visible |= i % 4 == 3 && source[i];
  }
  if (!visible)
    return false;
  const int target = 256;
  std::vector<unsigned char> image(size_t(target) * target * 4);
  // Nearest-neighbour scaling keeps pixel art sharp.
  for (int y = 0; y < target; y++)
    for (int x = 0; x < target; x++)
      std::copy_n(&source[(size_t(y * size / target) * size + x * size / target) * 4], 4,
                  &image[(size_t(y) * target + x) * 4]);
  writePng(output.string(), target, target, image);
  return true;
}
std::string appName(const lota::Json &project) {
  if (project.has("export") && project["export"].type == lota::Json::Object &&
      project["export"].has("appName") && !project["export"]["appName"].text().empty())
    return project["export"]["appName"].text();
  return project["title"].text("My Game");
}
std::string folderName(std::string name) {
  for (char &c : name)
    if (c == '/' || c == '\\' || c == ':' || static_cast<unsigned char>(c) < 0x20)
      c = '-';
  if (name.empty() || name == "." || name == "..")
    name = "Game";
  return name;
}
std::string projectId(const lota::Json &project) {
  need(project.type == lota::Json::Object && project["format"].text() == "lota-studio",
       "Invalid project.");
  auto id = project["id"].text();
  need(!id.empty() && id.size() <= 64 &&
           id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "0123456789_-") == std::string::npos &&
           id[0] != '-',
       "The project ID can only use letters, numbers, hyphens and underscores.");
  return id;
}
fs::path stagingFolder(const fs::path &destination) {
  std::random_device random;
  return destination.parent_path() / (".eve-export-" + std::to_string(random()));
}
void replaceWith(const fs::path &result, const fs::path &destination) {
  if (!fs::exists(destination)) {
    fs::rename(result, destination);
    return;
  }
  std::random_device random;
  auto old = destination.parent_path() / (".eve-replaced-" + std::to_string(random()));
  fs::rename(destination, old);
  try {
    fs::rename(result, destination);
  } catch (...) {
    fs::rename(old, destination);
    throw;
  }
  fs::remove_all(old);
}
} // namespace eve
