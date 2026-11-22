#include "web_export.hpp"
#include "archive.hpp"
#include "common.hpp"
#include <cstring>
#include <fstream>
#include <sstream>
namespace fs = std::filesystem;
namespace eve {
namespace {
std::string readFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  need(bool(in), "Missing export resource: " + path);
  std::stringstream out;
  out << in.rdbuf();
  return out.str();
}
std::string htmlText(const std::string &s) {
  std::string out;
  for (char c : s)
    if (c == '&')
      out += "&amp;";
    else if (c == '<')
      out += "&lt;";
    else if (c == '>')
      out += "&gt;";
    else if (c == '"')
      out += "&quot;";
    else if (static_cast<unsigned char>(c) >= 0x20)
      out += c;
  return out;
}
void replaceAll(std::string &text, const std::string &from, const std::string &to) {
  for (size_t at = text.find(from); at != std::string::npos; at = text.find(from, at + to.size()))
    text.replace(at, from.size(), to);
}
} // namespace

bool exportWebGame(const std::string &projectJson, const WebExportSources &sources,
                   const std::string &destination, std::string &error, bool zip) {
  fs::path dest = fs::absolute(destination), stage;
  try {
    auto project = lota::Json::parse(projectJson);
    projectId(project);
    need(!dest.filename().empty(), "Choose a name for the exported game.");
    int scale = 3;
    if (project.has("export") && project["export"].has("windowScale"))
      scale = int(project["export"]["windowScale"].number);
    need(scale >= 1 && scale <= 6, "Window size must be 1× to 6×.");
    auto page = readFile(sources.page);
    replaceAll(page, "{{TITLE}}", htmlText(appName(project)));
    replaceAll(page, "{{WIDTH}}", std::to_string(400 * scale));
    std::vector<ZipEntry> files{{"index.html", page},
                                {"evengine.js", readFile(sources.script)},
                                {"evengine.wasm", readFile(sources.wasm)},
                                {"project.lota", projectJson},
                                {"font.pam", readFile(sources.font)}};
    stage = stagingFolder(dest);
    fs::create_directories(stage);
    if (writeIcon(project, stage / "icon.png"))
      files.push_back({"icon.png", readFile((stage / "icon.png").string())});
    else
      files.push_back({"icon.png", readFile(sources.defaultIcon)});
    fs::path result = stage;
    if (zip) {
      result = stage / "game.zip";
      writeZip(result, files);
    } else
      for (auto &f : files)
        writeText(stage / f.name, f.data);
    replaceWith(result, dest);
    if (zip)
      fs::remove_all(stage);
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

extern "C" int eveExportWebGame(const char *projectJson, const char *sourceFolder, const char *font,
                                const char *defaultIcon, const char *destination, int zip,
                                char *error, size_t errorSize) {
  fs::path folder = sourceFolder;
  std::string message;
  bool ok =
      eve::exportWebGame(projectJson,
                         {(folder / "evengine.js").string(), (folder / "evengine.wasm").string(),
                          (folder / "index.html").string(), font, defaultIcon},
                         destination, message, zip != 0);
  if (!ok && error && errorSize) {
    std::strncpy(error, message.c_str(), errorSize - 1);
    error[errorSize - 1] = '\0';
  }
  return ok;
}
