#include "export.hpp"
#include "lota/json.hpp"
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <random>
#include <stdexcept>
#include <vector>
namespace fs = std::filesystem;
namespace eve {
namespace {
void need(bool ok, const std::string &message) {
  if (!ok)
    throw std::runtime_error(message);
}
int hex(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}
void writeText(const fs::path &path, const std::string &text) {
  std::ofstream out(path, std::ios::binary);
  out << text;
  out.close();
  need(bool(out), "Cannot write " + path.filename().string());
}
// Single-quotes a value for /bin/sh.
std::string shellQuote(const std::string &s) {
  std::string out = "'";
  for (char c : s)
    out += c == '\'' ? std::string("'\\''") : std::string(1, c);
  return out + "'";
}
// Escapes a value for a .desktop file string field.
std::string desktopText(const std::string &s) {
  std::string out;
  for (char c : s)
    if (c == '\\')
      out += "\\\\";
    else if (c == '\n' || c == '\r' || c == '\t')
      out += ' ';
    else if (static_cast<unsigned char>(c) >= 0x20)
      out += c;
  return out;
}
// Returns true when the project has a visible custom icon and writes it as a 256 px PNG.
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
  GdkPixbuf *image = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, target, target);
  need(image, "Cannot create game icon image.");
  auto *out = gdk_pixbuf_get_pixels(image);
  int stride = gdk_pixbuf_get_rowstride(image);
  // Nearest-neighbour scaling keeps pixel art sharp.
  for (int y = 0; y < target; y++)
    for (int x = 0; x < target; x++)
      std::copy_n(&source[(size_t(y * size / target) * size + x * size / target) * 4], 4,
                  out + y * stride + x * 4);
  GError *problem = nullptr;
  bool ok = gdk_pixbuf_save(image, output.c_str(), "png", &problem, nullptr);
  g_object_unref(image);
  std::string message = problem ? problem->message : "Cannot write game icon.";
  if (problem)
    g_error_free(problem);
  need(ok, message);
  return true;
}
} // namespace

bool exportLinuxGame(const std::string &projectJson, const LinuxExportSources &sources,
                     const std::string &destination, std::string &error) {
  fs::path dest = fs::absolute(destination), stage;
  try {
    auto project = lota::Json::parse(projectJson);
    need(project.type == lota::Json::Object && project["format"].text() == "lota-studio",
         "Invalid project.");
    auto id = project["id"].text(), title = project["title"].text("My Game");
    need(!id.empty() && id.size() <= 64 &&
             id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  "0123456789_-") == std::string::npos &&
             id[0] != '-',
         "The project ID can only use letters, numbers, hyphens and underscores.");
    for (auto *file : {&sources.runtime, &sources.font, &sources.sdlLibrary})
      need(fs::is_regular_file(*file), "Missing export resource: " + fs::path(*file).string());
    need(!dest.filename().empty(), "Choose a folder name for the exported game.");
    std::random_device random;
    stage = dest.parent_path() / (".eve-export-" + std::to_string(random()));
    fs::create_directories(stage / "lib");
    auto exe = stage / id;
    fs::copy_file(sources.runtime, exe);
    fs::permissions(exe, fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec |
                             fs::perms::others_read | fs::perms::others_exec);
    writeText(stage / "project.lota", projectJson);
    fs::copy_file(sources.font, stage / "font.pam");
    // Copy the real file behind any libSDL3.so symlink under the soname the runtime needs.
    fs::copy_file(fs::canonical(sources.sdlLibrary), stage / "lib" / "libSDL3.so.0");
    if (!writeIcon(project, stage / "icon.png") && fs::is_regular_file(sources.defaultIcon))
      fs::copy_file(sources.defaultIcon, stage / "icon.png");
    auto name = desktopText(title);
    writeText(stage / "install-desktop-entry.sh",
              "#!/bin/sh\n"
              "# Adds this game to your application menu. Run it again after moving the folder.\n"
              "set -e\n"
              "here=$(cd \"$(dirname \"$0\")\" && pwd)\n"
              "apps=\"${XDG_DATA_HOME:-$HOME/.local/share}/applications\"\n"
              "mkdir -p \"$apps\"\n"
              "{\n"
              "  printf '%s\\n' '[Desktop Entry]' 'Type=Application' 'Categories=Game;' "
              "'Terminal=false'\n"
              "  printf 'Name=%s\\n' " +
                  shellQuote(name) +
                  "\n"
                  "  printf 'Exec=\"%s/" +
                  id +
                  "\"\\n' \"$here\"\n"
                  "  printf 'Icon=%s/icon.png\\n' \"$here\"\n"
                  "} > \"$apps/evengine-" +
                  id +
                  ".desktop\"\n"
                  "echo 'Added '" +
                  shellQuote(name) + "' to your applications menu.'\n");
    fs::permissions(stage / "install-desktop-entry.sh",
                    fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                    fs::perm_options::add);
    writeText(stage / "README.txt",
              name +
                  "\n\nMade with EveNGINE Studio 1.1.\n\n"
                  "Run ./" +
                  id +
                  " to play. Keep the files in this folder together.\n"
                  "Run ./install-desktop-entry.sh to add the game to your application menu.\n"
                  "Saves are kept in ~/.local/share/LotAEngine/" +
                  id + "/.\n");
    if (fs::exists(dest)) {
      auto old = dest.parent_path() / (".eve-replaced-" + std::to_string(random()));
      fs::rename(dest, old);
      try {
        fs::rename(stage, dest);
      } catch (...) {
        fs::rename(old, dest);
        throw;
      }
      fs::remove_all(old);
    } else
      fs::rename(stage, dest);
    error.clear();
    return true;
  } catch (const std::exception &e) {
    std::error_code ignored;
    if (!stage.empty())
      fs::remove_all(stage, ignored);
    error = e.what();
    return false;
  }
}
} // namespace eve
