// A standalone game built on the SDK. It does not link the LotA RPG module.
#include "evengine/core.hpp"
#include "evengine/platform.hpp"
#include <filesystem>
#include <iostream>

class Starter {
public:
  evengine::Project project;
  evengine::Vec2 feet;
  int crystals = 0;
  std::vector<std::string> collected;
  bool quit = false, help = true;
  explicit Starter(const std::string &path)
      : project(evengine::Project::load(path)) {
    feet = project.scenes.at(project.start).spawn;
  }
  void update(const evengine::Input &input, float dt) {
    if (input.hit(evengine::Action::Quit))
      quit = true;
    if (input.hit(evengine::Action::Menu))
      help = !help;
    auto direction =
        evengine::normalized({float(input.down(evengine::Action::Right)) -
                              float(input.down(evengine::Action::Left)),
                          float(input.down(evengine::Action::Down)) -
                              float(input.down(evengine::Action::Up))});
    auto speed = input.down(evengine::Action::Run) ? 80.0f : 48.0f;
    const auto &scene = project.scenes.at(project.start);
    feet = evengine::moveBody(project, scene, feet,
                          {direction.x * speed * dt, direction.y * speed * dt});
    for (const auto &object : scene.objects)
      if (object.kind == "crystal" &&
          std::hypot(feet.x - object.position.x, feet.y - object.position.y) <
              14 &&
          std::find(collected.begin(), collected.end(), object.id) ==
              collected.end()) {
        collected.push_back(object.id);
        ++crystals;
      }
  }
  evengine::Canvas draw() const {
    evengine::Canvas canvas;
    canvas.clear({24, 32, 44});
    const auto &scene = project.scenes.at(project.start);
    evengine::drawScene(canvas, project, scene, evengine::cameraFor(scene, feet), feet,
                    project.sprites.at("player").source, collected, false);
    canvas.rect({8, 8, 210, 29}, {24, 32, 44, 235});
    canvas.text("YOUR NEXT GAME", 16, 14, {229, 223, 191});
    canvas.text("CRYSTALS " + std::to_string(crystals) + " / 3", 16, 25,
                {138, 208, 213});
    // Scaled images and nested clip regions are available to every SDK project.
    canvas.rect({310, 8, 80, 58}, {24, 32, 44});
    canvas.pushClip({316, 14, 68, 46});
    canvas.image(project.tiles.at('.').source, {316, 14, 68, 46});
    canvas.rect({316 + feet.x / scene.width / 16 * 68 - 1,
                 14 + feet.y / scene.height / 16 * 46 - 1, 3, 3},
                {229, 223, 191});
    canvas.popClip();
    if (help) {
      canvas.rect({8, 211, 384, 21}, {24, 32, 44, 235});
      canvas.text("MOVE WASD  SHIFT RUN  Q HELP  F5 SAVE", 16, 218,
                  {138, 208, 213});
    }
    return canvas;
  }
  bool save(const std::string &path, std::string &error) const {
    return evengine::saveState(
        path, {project.id, project.start, feet, crystals, collected}, error);
  }
  bool load(const std::string &path, std::string &error) {
    evengine::SavedState state;
    if (!evengine::loadState(path, project, state, error))
      return false;
    feet = state.player;
    crystals = state.score;
    collected = state.removed;
    return true;
  }
};
int main(int argc, char **argv) {
  try {
    std::string path = (std::filesystem::path(evengine::defaultProjectPath())
                            .parent_path()
                            .parent_path()
                            .parent_path() /
                        "starter/content/game.project")
                           .string();
    std::string capture, save;
    int ticks = 0;
    bool smoke = false;
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      auto value = [&]() {
        if (++i >= argc)
          throw std::runtime_error("Missing option value");
        return std::string(argv[i]);
      };
      if (arg == "--project")
        path = value();
      else if (arg == "--capture")
        capture = value();
      else if (arg == "--ticks")
        ticks = std::stoi(value());
      else if (arg == "--smoke")
        smoke = true;
      else if (arg == "--save")
        save = value();
      else
        throw std::runtime_error("Unknown option: " + arg);
    }
    Starter game(path);
    auto atlas = evengine::Atlas::load(game.project.atlasPath);
    if (smoke || !capture.empty()) {
      evengine::Input input;
      input.set(evengine::Action::Left, true);
      auto before = game.feet;
      for (int n = 0; n < ticks; ++n) {
        game.update(input, evengine::Tick);
        input.consume();
      }
      if (ticks > 0 && game.feet.x >= before.x)
        throw std::runtime_error("Starter movement failed");
      if (!save.empty()) {
        std::string error;
        if (!game.save(save, error) || !game.load(save, error))
          throw std::runtime_error(error);
      }
      evengine::SoftwareRenderer render;
      render.render(game.draw(), atlas);
      if (!capture.empty())
        render.writePPM(capture);
      std::cout << "SDK starter: project, collision, movement, clipping, "
                   "rendering and save/load OK\n";
      return 0;
    }
    auto platform = evengine::createPlatform(atlas, game.project.title);
    if (save.empty())
      save = platform->savePath(game.project.id);
    evengine::Input input;
    double previous = platform->seconds(), accumulator = 0;
    while (!game.quit && platform->poll(input)) {
      double now = platform->seconds();
      accumulator += std::clamp(now - previous, 0.0, .1);
      previous = now;
      while (accumulator >= evengine::Tick) {
        game.update(input, evengine::Tick);
        std::string error;
        if (input.hit(evengine::Action::Save) && !game.save(save, error))
          std::cerr << error << '\n';
        if (input.hit(evengine::Action::Load) && !game.load(save, error))
          std::cerr << error << '\n';
        input.consume();
        accumulator -= evengine::Tick;
      }
      platform->present(game.draw());
      platform->idle();
    }
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
