#include "studio.hpp"
#include "lota/platform.hpp"
#include <iostream>
#include <filesystem>
#include <memory>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace {
// Everything one running game needs between frames. Kept on the heap because in a browser main()
// returns before the first frame.
struct Session {
  explicit Session(const std::string &path) : game(path) {}
  lota::StudioGame game;
  std::unique_ptr<lota::Platform> platform;
  bool fullscreen = false, suppressCapture = false;
  lota::Input input;
  double previous = 0, accumulator = 0;
  // Runs one frame. Returns false once the game has quit.
  bool frame() {
    if (game.quit || !platform->poll(input))
      return false;
    if (platform->takeCloseRequest())
      game.requestWindowClose();
    if (game.quit)
      return false;
    game.controllerMode = platform->usingGamepad();
    auto key = platform->takeKeyPress();
    if (!game.pendingBinding.empty() && !key.empty()) {
      game.captureBinding(key);
      platform->configureInput(game.keyBindings, game.buttonBindings);
      // A captured key must be released before it can confirm another menu action.
      input = lota::Input{};
      for (auto &binding : game.keyBindings)
        if (binding.second == key)
          input.held[size_t(binding.first)] = true;
      suppressCapture = true;
    }
    if (suppressCapture) {
      input.consume();
      if (std::none_of(input.held.begin(), input.held.end(), [](bool held) { return held; }))
        suppressCapture = false;
    }
    if (game.fullscreen != fullscreen) {
      if (platform->setFullscreen(game.fullscreen))
        fullscreen = game.fullscreen;
      else {
        game.fullscreen = fullscreen;
        game.toast = "Fullscreen is unavailable.";
        game.toastTime = 3;
      }
    }
    double now = platform->seconds();
    game.frameMs = float((now - previous) * 1000);
    accumulator += std::clamp(now - previous, 0., .1);
    previous = now;
    while (accumulator >= lota::Tick) {
      game.update(input, lota::Tick);
      input.consume();
      accumulator -= lota::Tick;
    }
    platform->setMusic(game.music(), game.musicVolume * game.userMusic);
    for (auto &id : game.soundEvents)
      platform->playSound(game.audioAssets.at(id), game.sfxVolume * game.userEffects);
    game.soundEvents.clear();
    platform->present(game.draw());
    platform->idle();
    return true;
  }
};
} // namespace

int main(int argc, char **argv) {
  try {
    auto base =
        std::filesystem::path(lota::defaultProjectPath()).parent_path().parent_path().parent_path();
    std::string path = (base / "project.lota").string(), capture, scene;
    int ticks = 0;
    bool check = false;
    for (int i = 1; i < argc; i++) {
      std::string argument = argv[i];
      auto value = [&]() {
        if (++i >= argc)
          throw std::runtime_error("Missing value");
        return std::string(argv[i]);
      };
      if (argument == "--project")
        path = value();
      else if (argument == "--scene")
        scene = value();
      else if (argument == "--capture")
        capture = value();
      else if (argument == "--ticks")
        ticks = std::stoi(value());
      else if (argument == "--validate")
        check = true;
      else
        throw std::runtime_error("Unknown option");
    }
    auto session = std::make_unique<Session>(path);
    auto &game = session->game;
    if (!scene.empty()) {
      if (!game.project.scenes.count(scene))
        throw std::runtime_error("Unknown playtest map");
      game.scene = scene;
      game.feet = game.project.scenes.at(scene).spawn;
    }
    if (check || !capture.empty()) {
      lota::Input input;
      input.set(lota::Action::Right, true);
      for (int i = 0; i < ticks; i++) {
        game.update(input, lota::Tick);
        input.consume();
      }
      if (!capture.empty()) {
        lota::SoftwareRenderer renderer;
        renderer.render(game.draw(), game.atlas);
        renderer.writePPM(capture);
      }
      std::cout << "Project validated: " << game.project.scenes.size() << " maps, "
                << game.clips.size() << " clips\n";
      return 0;
    }
    session->platform = lota::createPlatform(game.atlas, game.appName, game.windowScale);
    if (!game.audioAssets.empty() && !session->platform->audioAvailable())
      std::cerr << "Audio device unavailable; continuing silently.\n";
    game.beginSession(session->platform->savePath(game.project.id), !scene.empty());
    session->platform->configureInput(game.keyBindings, game.buttonBindings);
    session->platform->confirmWindowClose(true);
    session->previous = session->platform->seconds();
#ifdef __EMSCRIPTEN__
    // The browser calls frame() once per display refresh; main returns straight away.
    emscripten_set_main_loop_arg(
        [](void *data) {
          auto *s = static_cast<Session *>(data);
          if (!s->frame()) {
            emscripten_cancel_main_loop();
            emscripten_run_script("if (typeof eveGameEnded === 'function') eveGameEnded();");
          }
        },
        session.release(), 0, false);
    return 0;
#else
    while (session->frame()) {
    }
#endif
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
