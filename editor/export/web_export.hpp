#pragma once
#include <stddef.h>
#ifdef __cplusplus
#include <string>
namespace eve {
// The web runtime Studio carries: Emscripten's evengine.js and evengine.wasm, plus the page
// template (editor/web/index.html), the font and the default icon.
struct WebExportSources {
  std::string script, wasm, page, font, defaultIcon;
};
// Writes a game that runs in a browser: index.html, evengine.js, evengine.wasm, project.lota,
// font.pam and icon.png. With zip set, writes them into a .zip with index.html at the top, the
// layout itch.io and similar sites expect. An existing destination is replaced only once the new
// export is complete.
bool exportWebGame(const std::string &projectJson, const WebExportSources &sources,
                   const std::string &destination, std::string &error, bool zip = false);
} // namespace eve
extern "C" {
#endif
// C entry point for the Mac app. Returns 1 on success; otherwise 0 with a message in error.
// sourceFolder holds evengine.js, evengine.wasm and index.html.
int eveExportWebGame(const char *projectJson, const char *sourceFolder, const char *font,
                     const char *defaultIcon, const char *destination, int zip, char *error,
                     size_t errorSize);
#ifdef __cplusplus
}
#endif
