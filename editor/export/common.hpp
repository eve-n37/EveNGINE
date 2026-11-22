#pragma once
// Helpers shared by the Linux and web exporters.
#include "lota/json.hpp"
#include <filesystem>
#include <string>
namespace eve {
void need(bool ok, const std::string &message);
void writeText(const std::filesystem::path &path, const std::string &text);
// Returns true when the project has a visible custom icon and writes it as a 256 px PNG.
bool writeIcon(const lota::Json &project, const std::filesystem::path &output);
// The name players see: the export setting, else the game title.
std::string appName(const lota::Json &project);
// The app name made safe as a single file or folder name.
std::string folderName(std::string name);
// Checks the project is a Studio project with a usable ID, and returns that ID.
std::string projectId(const lota::Json &project);
// Moves a finished export (file or folder) to destination. Anything already there is kept until
// the move succeeds.
void replaceWith(const std::filesystem::path &result, const std::filesystem::path &destination);
// A unique hidden folder beside destination to assemble an export in.
std::filesystem::path stagingFolder(const std::filesystem::path &destination);
} // namespace eve
