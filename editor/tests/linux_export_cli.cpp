// Test driver for the shared Linux exporter: exports a project the way Studio does.
#include "../export/linux_export.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
int main(int argc, char **argv) {
  if (argc != 7 && argc != 8) {
    std::cerr << "usage: " << argv[0]
              << " project runtime font libSDL3 default-icon destination [--archive]\n";
    return 2;
  }
  std::ifstream in(argv[1], std::ios::binary);
  std::stringstream json;
  json << in.rdbuf();
  char error[512] = {0};
  if (!eveExportLinuxGame(json.str().c_str(), argv[2], argv[3], argv[4], argv[5], argv[6],
                          argc == 8 && std::string(argv[7]) == "--archive", error, sizeof error)) {
    std::cerr << error << '\n';
    return 1;
  }
  return 0;
}
