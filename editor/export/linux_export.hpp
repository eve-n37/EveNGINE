#pragma once
#include <stddef.h>
#ifdef __cplusplus
#include <string>
namespace eve {
// Files an exported Linux game is assembled from: on Linux, Studio's resources folder; on a Mac,
// the prebuilt Linux runtime and SDL3 bundled inside the app.
struct LinuxExportSources {
  std::string runtime, font, sdlLibrary, defaultIcon;
};
// Writes a self-contained game folder: the runtime (named after the project ID), project.lota,
// font.pam, lib/libSDL3.so.0, icon.png, an installer for an application-menu entry and a README.
// With archive set, writes a .tar.gz holding that folder instead, which keeps the executable
// permissions Linux needs wherever it was made. An existing destination is replaced only once the
// new export is complete. Works on Linux and macOS.
bool exportLinuxGame(const std::string &projectJson, const LinuxExportSources &sources,
                     const std::string &destination, std::string &error, bool archive = false);
} // namespace eve
extern "C" {
#endif
// C entry point for the Mac app. Returns 1 on success; otherwise 0 with a message in error.
int eveExportLinuxGame(const char *projectJson, const char *runtime, const char *font,
                       const char *sdlLibrary, const char *defaultIcon, const char *destination,
                       int archive, char *error, size_t errorSize);
#ifdef __cplusplus
}
#endif
