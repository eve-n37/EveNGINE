#include "evengine/studio.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace evengine;
namespace fs = std::filesystem;
// Writes a copy of the project with an export block inserted after its opening brace.
static std::string withExport(const std::string &source, const fs::path &folder,
                              const std::string &name, const std::string &block) {
  auto first = source.find('{');
  if (first == std::string::npos)
    throw std::runtime_error("Project is not a JSON object");
  auto path = (folder / name).string();
  std::ofstream(path) << source.substr(0, first + 1) << "\"export\":" << block << ","
                      << source.substr(first + 1);
  return path;
}
int main(int argc, char **argv) {
  try {
    if (argc != 3)
      throw std::runtime_error("Expected project and output folder");
    std::ifstream in(argv[1]);
    std::stringstream text;
    text << in.rdbuf();
    fs::path out = argv[2];
    int checks = 0;
    auto check = [&](bool ok, const char *message) {
      if (!ok)
        throw std::runtime_error(message);
      ++checks;
      std::cout << "OK " << message << '\n';
    };
    {
      StudioGame plain(argv[1]);
      check(plain.windowScale == 3 && !plain.defaultFullscreen &&
                plain.appName == plain.project.title,
            "projects without export settings keep the old window defaults");
    }
    auto path = withExport(text.str(), out, "export-settings.eve",
                           R"({"appName":"Frontier","windowScale":2,"fullscreen":true,)"
                           R"("bundleId":"com.example.frontier","version":"2.0.1",)"
                           R"("macArchitecture":"arm64"})");
    StudioGame g(path);
    check(g.appName == "Frontier" && g.windowScale == 2 && g.defaultFullscreen,
          "export settings load");
    auto save = (out / "export-settings.dat").string();
    fs::remove(save);
    fs::remove(save + ".settings");
    g.beginSession(save, true);
    check(!g.fullscreen, "playtests ignore the fullscreen default");
    g.beginSession(save);
    check(g.fullscreen, "exported games start fullscreen when asked");
    g.fullscreen = false;
    g.savePreferences();
    StudioGame again(path);
    again.beginSession(save);
    check(!again.fullscreen, "a player's saved fullscreen choice wins");
    for (auto bad :
         {R"({"windowScale":9})", R"({"windowScale":2.5})", R"({"fullscreen":1})", R"([])"}) {
      bool rejected = false;
      try {
        StudioGame broken(withExport(text.str(), out, "export-bad.eve", bad));
      } catch (const std::exception &) {
        rejected = true;
      }
      if (!rejected)
        throw std::runtime_error(std::string("accepted invalid export settings: ") + bad);
    }
    check(true, "invalid export settings are rejected");
    std::cout << checks << " export settings checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
