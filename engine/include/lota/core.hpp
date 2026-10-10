#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace lota {
constexpr const char *Version = "0.6.0";
constexpr int Width = 400, Height = 240, TileSize = 16;
constexpr float Tick = 1.0f / 60.0f;
struct Vec2 {
  float x = 0, y = 0;
};
struct Rect {
  float x = 0, y = 0, w = 0, h = 0;
};
bool overlaps(Rect a, Rect b);
Vec2 normalized(Vec2 v);
struct Color {
  uint8_t r = 255, g = 255, b = 255, a = 255;
};
enum class Action {
  Left,
  Right,
  Up,
  Down,
  Run,
  Interact,
  Menu,
  Save,
  Load,
  Debug,
  Quit,
  Attack,
  Dive,
  Potion,
  Hop,
  Reset,
  One,
  Two,
  Three,
  Clear,
  Count
};
struct Input {
  std::array<bool, static_cast<size_t>(Action::Count)> held{}, pressed{};
  void set(Action action, bool value);
  bool down(Action a) const { return held[static_cast<size_t>(a)]; }
  bool hit(Action a) const { return pressed[static_cast<size_t>(a)]; }
  void consume() { pressed.fill(false); }
};
struct Atlas {
  int width = 0, height = 0;
  std::vector<Color> pixels;
  static Atlas load(const std::string &path); // P6 RGB (magenta cutout) or P7
                                              // RGBA PAM (full alpha).
};
struct Sprite {
  Rect source;
  Rect collider;
  std::vector<Rect> extraColliders;
};
struct Tile {
  Rect source;
  bool solid = false;
};
struct Object {
  std::string kind, sprite, id, text;
  Vec2 position;
};
struct Portal {
  Rect bounds;
  std::string target;
  Vec2 arrival;
};
struct Scene {
  std::string id, title;
  int width = 0, height = 0;
  Vec2 spawn;
  std::vector<char> cells;
  std::vector<Object> objects;
  std::vector<Portal> portals;
  char at(int x, int y) const;
};
struct Project {
  std::string id, title, mode, atlasPath, start;
  std::map<char, Tile> tiles;
  std::map<std::string, Sprite> sprites;
  std::map<std::string, Scene> scenes;
  static Project load(const std::string &path);
};
struct Draw {
  bool sprite = false;
  Rect destination, source;
  Color color;
  bool flip = false;
  bool clipped = false;
  Rect clip{};
};
class Canvas {
public:
  std::vector<Draw> draws;
  void clear(Color color);
  void rect(Rect destination, Color color);
  void sprite(Rect source, Vec2 position, bool flip = false, Color tint = {});
  void image(Rect source, Rect destination, bool flip = false, Color tint = {});
  void pushClip(Rect bounds);
  void popClip();
  void text(const std::string &value, int x, int y, Color color = {},
            int scale = 1);

private:
  std::vector<Rect> clips;
  void append(Draw draw);
};
// Reference renderer: deterministic screenshots and tests without a display.
class SoftwareRenderer {
public:
  // Keep the 375 KiB framebuffer off the small handheld thread stack.
  // A local renderer can reserve its frame even in a branch never executed.
  SoftwareRenderer() : pixels(static_cast<size_t>(Width) * Height) {}
  std::vector<Color> pixels;
  void render(const Canvas &canvas, const Atlas &atlas);
  void writePPM(const std::string &path) const;
};
static_assert(sizeof(SoftwareRenderer) <= 64,
              "Renderer storage must remain off-stack");
Rect body(Vec2 feet);
bool blocked(const Project &project, const Scene &scene, Rect rectangle);
Vec2 moveBody(const Project &project, const Scene &scene, Vec2 feet,
              Vec2 delta);
Vec2 cameraFor(const Scene &scene, Vec2 feet);
void drawScene(Canvas &canvas, const Project &project, const Scene &scene,
               Vec2 camera, Vec2 player, Rect playerSprite,
               const std::vector<std::string> &removed, bool debug);
struct SavedState {
  std::string project, scene;
  Vec2 player;
  int score = 0;
  std::vector<std::string> removed;
};
bool saveState(const std::string &path, const SavedState &state,
               std::string &error);
bool loadState(const std::string &path, const Project &project,
               SavedState &state, std::string &error);
} // namespace lota
