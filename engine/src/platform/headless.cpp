#include "lota/platform.hpp"
#include <stdexcept>
namespace lota {
std::unique_ptr<Platform> createPlatform(const Atlas&,const std::string&) {throw std::runtime_error("Built headless: use --headless or --capture, or enable the SDL3 build");}
std::string defaultProjectPath() {return "samples/valley/game.project";}
}
