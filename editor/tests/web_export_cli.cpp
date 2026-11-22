// Test driver for the shared web exporter: exports a project the way Studio does.
#include "../export/web_export.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
int main(int argc, char **argv) {
  if (argc != 6 && argc != 7) {
    std::cerr << "usage: " << argv[0]
              << " project runtime-folder font default-icon destination [--zip]\n";
    return 2;
  }
  std::ifstream in(argv[1], std::ios::binary);
  std::stringstream json;
  json << in.rdbuf();
  char error[512] = {0};
  if (!eveExportWebGame(json.str().c_str(), argv[2], argv[3], argv[4], argv[5],
                        argc == 7 && std::string(argv[6]) == "--zip", error, sizeof error)) {
    std::cerr << error << '\n';
    return 1;
  }
  return 0;
}
