#pragma once
#include <string>
namespace eve {
// Files an exported Linux game is assembled from, normally in Studio's resources folder.
struct LinuxExportSources {
  std::string runtime, font, sdlLibrary, defaultIcon;
};
// Writes a self-contained game folder: the runtime (named after the project ID), project.lota,
// font.pam, lib/libSDL3.so.0, icon.png, an installer for an application-menu entry and a README.
// An existing destination is replaced only once the new folder is complete.
bool exportLinuxGame(const std::string &projectJson, const LinuxExportSources &sources,
                     const std::string &destination, std::string &error);
} // namespace eve
