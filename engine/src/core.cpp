#include "lota/core.hpp"
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif

namespace lota {
namespace {
std::string folder(const std::string &path) {
  auto i = path.find_last_of("/\\");
  return i == std::string::npos ? "" : path.substr(0, i + 1);
}
void require(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}
std::string token(std::istream &input) {
  std::string result;
  for (;;) {
    input >> std::ws;
    if (input.peek() != '#')
      break;
    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  input >> result;
  return result;
}
Scene loadScene(const std::string &path, const std::string &id) {
  std::ifstream input(path);
  require(bool(input), "Cannot open scene: " + path);
  Scene scene;
  scene.id = id;
  std::string line;
  int number = 0;
  bool grid = false;
  while (std::getline(input, line)) {
    ++number;
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    std::istringstream row(line);
    std::string key;
    row >> key;
    if (key.empty() || key[0] == '#')
      continue;
    if (key == "title")
      row >> std::quoted(scene.title);
    else if (key == "size") {
      row >> scene.width >> scene.height;
      require(scene.width > 0 && scene.height > 0 && scene.width <= 256 &&
                  scene.height <= 256,
              "Scene size outside 1..256");
    } else if (key == "spawn")
      row >> scene.spawn.x >> scene.spawn.y;
    else if (key == "tiles") {
      require(scene.width > 0 && !grid, "Size must precede one tile grid");
      grid = true;
      for (int y = 0; y < scene.height; ++y) {
        require(bool(std::getline(input, line)),
                "Truncated tile grid: " + path);
        ++number;
        if (!line.empty() && line.back() == '\r')
          line.pop_back();
        require(int(line.size()) == scene.width,
                "Tile row width mismatch: " + path);
        scene.cells.insert(scene.cells.end(), line.begin(), line.end());
      }
    } else if (key == "object") {
      Object o;
      row >> o.kind >> o.sprite >> o.id >> o.position.x >> o.position.y >>
          std::quoted(o.text);
      scene.objects.push_back(o);
    } else if (key == "portal") {
      Portal p;
      row >> p.bounds.x >> p.bounds.y >> p.bounds.w >> p.bounds.h >> p.target >>
          p.arrival.x >> p.arrival.y;
      require(p.bounds.w > 0 && p.bounds.h > 0, "Invalid portal bounds");
      scene.portals.push_back(p);
    } else
      throw std::runtime_error("Unknown scene field: " + key);
    require(!row.fail(),
            path + ":" + std::to_string(number) + " invalid " + key);
  }
  require(grid && !scene.title.empty(), "Incomplete scene: " + path);
  return scene;
}
bool validPoint(Vec2 v) { return std::isfinite(v.x) && std::isfinite(v.y); }
} // namespace
bool overlaps(Rect a, Rect b) {
  return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h &&
         a.y + a.h > b.y;
}
Vec2 normalized(Vec2 v) {
  float length = std::sqrt(v.x * v.x + v.y * v.y);
  return length > 0 ? Vec2{v.x / length, v.y / length} : Vec2{};
}
void Input::set(Action action, bool value) {
  auto i = static_cast<size_t>(action);
  if (value && !held[i])
    pressed[i] = true;
  held[i] = value;
}
Atlas Atlas::load(const std::string &path) {
  std::ifstream input(path, std::ios::binary);
  require(bool(input), "Cannot open atlas: " + path);
  Atlas atlas;
  std::string format = token(input);
  bool rgba = format == "P7";
  if (rgba) {
    std::string key;
    int depth = 0, maxval = 0;
    bool header = false;
    while (input >> key) {
      if (key == "ENDHDR") {
        header = true;
        break;
      }
      if (key[0] == '#') {
        std::string ignored;
        std::getline(input, ignored);
        continue;
      }
      if (key == "WIDTH")
        input >> atlas.width;
      else if (key == "HEIGHT")
        input >> atlas.height;
      else if (key == "DEPTH")
        input >> depth;
      else if (key == "MAXVAL")
        input >> maxval;
      else if (key == "TUPLTYPE") {
        std::string type;
        input >> type;
        require(type == "RGB_ALPHA", "Unsupported PAM tuple");
      } else
        throw std::runtime_error("Unknown PAM header field");
      require(bool(input), "Invalid PAM header");
    }
    require(header && depth == 4 && maxval == 255, "Unsupported PAM atlas");
  } else {
    require(format == "P6", "Atlas must be P6 PPM or P7 RGBA PAM");
    atlas.width = std::stoi(token(input));
    atlas.height = std::stoi(token(input));
    require(token(input) == "255", "Atlas must be 8 bit");
  }
  require(atlas.width > 0 && atlas.height > 0 && atlas.width <= 4096 &&
              atlas.height <= 4096,
          "Atlas too large");
  char separator = char(input.get());
  require(separator == '\n' || separator == '\r' || separator == ' ',
          "Missing atlas separator");
  if (separator == '\r' && input.peek() == '\n')
    input.get();
  atlas.pixels.resize(size_t(atlas.width) * atlas.height);
  for (auto &pixel : atlas.pixels) {
    unsigned char bytes[4] = {0, 0, 0, 255};
    input.read(reinterpret_cast<char *>(bytes), rgba ? 4 : 3);
    require(bool(input), "Truncated atlas");
    pixel = {bytes[0], bytes[1], bytes[2], bytes[3]};
    if (!rgba && bytes[0] == 255 && bytes[1] == 0 && bytes[2] == 255)
      pixel.a = 0;
  }
  return atlas;
}

char Scene::at(int x, int y) const {
  return x < 0 || y < 0 || x >= width || y >= height
             ? '\0'
             : cells[size_t(y) * width + x];
}
Project Project::load(const std::string &path) {
  std::ifstream input(path);
  require(bool(input), "Cannot open project: " + path);
  Project p;
  std::string line;
  std::map<std::string, std::string> files;
  while (std::getline(input, line)) {
    std::istringstream row(line);
    std::string key;
    row >> key;
    if (key.empty() || key[0] == '#')
      continue;
    if (key == "id")
      row >> p.id;
    else if (key == "title")
      row >> std::quoted(p.title);
    else if (key == "mode")
      row >> p.mode;
    else if (key == "atlas") {
      std::string v;
      row >> v;
      p.atlasPath = folder(path) + v;
    } else if (key == "start")
      row >> p.start;
    else if (key == "tile") {
      char c;
      Tile t;
      int solid;
      row >> c >> t.source.x >> t.source.y >> solid;
      t.source.w = t.source.h = TileSize;
      t.solid = solid != 0;
      require(p.tiles.emplace(c, t).second, "Duplicate tile");
    } else if (key == "sprite") {
      std::string id;
      Sprite s;
      row >> id >> s.source.x >> s.source.y >> s.source.w >> s.source.h >>
          s.collider.x >> s.collider.y >> s.collider.w >> s.collider.h;
      require(p.sprites.emplace(id, s).second, "Duplicate sprite");
    } else if (key == "collider") {
      std::string id;
      Rect r;
      row >> id >> r.x >> r.y >> r.w >> r.h;
      require(p.sprites.count(id) && r.w > 0 && r.h > 0,
              "Collider needs a defined sprite and positive size");
      p.sprites.at(id).extraColliders.push_back(r);
    } else if (key == "scene") {
      std::string id, file;
      row >> id >> file;
      require(files.emplace(id, file).second, "Duplicate scene");
    } else
      throw std::runtime_error("Unknown project field: " + key);
    require(!row.fail(), "Invalid project field: " + key);
  }
  require(!p.id.empty() && p.id.size() < 48 &&
              p.id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNO"
                                     "PQRSTUVWXYZ0123456789_- ") ==
                  std::string::npos,
          "Invalid project id");
  require(!p.title.empty() && !p.atlasPath.empty() && files.count(p.start),
          "Project needs title, atlas and start scene");
  for (auto &file : files)
    p.scenes.emplace(file.first,
                     loadScene(folder(path) + file.second, file.first));
  auto atlas = Atlas::load(p.atlasPath);
  auto validSprite = [&](Rect r) {
    return validPoint({r.x, r.y}) && r.x >= 0 && r.y >= 0 && r.w > 0 &&
           r.h > 0 && r.x + r.w <= atlas.width && r.y + r.h <= atlas.height;
  };
  for (const auto &t : p.tiles)
    require(validSprite(t.second.source), "Tile source outside atlas");
  for (const auto &s : p.sprites)
    require(validSprite(s.second.source), "Sprite source outside atlas");
  std::set<std::string> objectIds;
  for (const auto &pair : p.scenes) {
    const auto &s = pair.second;
    for (char c : s.cells)
      require(p.tiles.count(c), "Undefined tile in " + s.id);
    for (const auto &o : s.objects) {
      require(p.sprites.count(o.sprite) && validPoint(o.position),
              "Invalid object in " + s.id);
      require(!o.id.empty() && o.id.size() <= 128 &&
                  objectIds.insert(o.id).second,
              "Object IDs must be unique across the project");
    }
    require(validPoint(s.spawn) && !blocked(p, s, body(s.spawn)),
            "Blocked spawn in " + s.id);
    for (const auto &portal : s.portals) {
      require(p.scenes.count(portal.target),
              "Missing portal target: " + portal.target);
      require(validPoint(portal.arrival) &&
                  !blocked(p, p.scenes.at(portal.target), body(portal.arrival)),
              "Blocked portal arrival");
    }
  }
  return p;
}
void Canvas::append(Draw d) {
  if (!clips.empty()) {
    d.clipped = true;
    d.clip = clips.back();
  }
  if (d.destination.w > 0 && d.destination.h > 0 &&
      overlaps(d.destination, {0, 0, Width, Height}))
    draws.push_back(d);
}
void Canvas::pushClip(Rect r) {
  if (!clips.empty()) {
    auto p = clips.back();
    float x = std::max(r.x, p.x), y = std::max(r.y, p.y);
    r = {x, y, std::max(0.0f, std::min(r.x + r.w, p.x + p.w) - x),
         std::max(0.0f, std::min(r.y + r.h, p.y + p.h) - y)};
  }
  clips.push_back(r);
}
void Canvas::popClip() {
  if (clips.empty())
    throw std::runtime_error("Unbalanced canvas clip");
  clips.pop_back();
}
void Canvas::clear(Color c) {
  draws.clear();
  clips.clear();
  rect({0, 0, Width, Height}, c);
}
void Canvas::rect(Rect r, Color c) { append({false, r, {}, c, false}); }
void Canvas::image(Rect source, Rect destination, bool flip, Color tint) {
  append({true, destination, source, tint, flip});
}
void Canvas::sprite(Rect source, Vec2 pos, bool flip, Color tint) {
  image(source, {std::floor(pos.x), std::floor(pos.y), source.w, source.h},
        flip, tint);
}
void Canvas::text(const std::string &value, int x, int y, Color color,
                  int scale) {
  int cursor = x;
  for (unsigned char c : value) {
    if (c == '\n') {
      cursor = x;
      y += 9 * scale;
      continue;
    }
    if (c < 32 || c > 126)
      c = '?';
    int i = c - 32;
    Rect s{float((i % 32) * 6), float(128 + (i / 32) * 8), 6, 8};
    Rect d{float(cursor), float(y), float(6 * scale), float(8 * scale)};
    image(s, d, false, color);
    cursor += 6 * scale;
  }
}
void SoftwareRenderer::render(const Canvas &canvas, const Atlas &atlas) {
  std::fill(pixels.begin(), pixels.end(), Color{0, 0, 0, 255});
  for (const auto &d : canvas.draws) {
    int left = std::max(0, int(std::floor(d.destination.x))),
        top = std::max(0, int(std::floor(d.destination.y)));
    int right =
            std::min(Width, int(std::ceil(d.destination.x + d.destination.w))),
        bottom =
            std::min(Height, int(std::ceil(d.destination.y + d.destination.h)));
    if (d.clipped) {
      left = std::max(left, int(std::ceil(d.clip.x)));
      top = std::max(top, int(std::ceil(d.clip.y)));
      right = std::min(right, int(std::ceil(d.clip.x + d.clip.w)));
      bottom = std::min(bottom, int(std::ceil(d.clip.y + d.clip.h)));
    }
    for (int y = top; y < bottom; ++y)
      for (int x = left; x < right; ++x) {
        Color c = d.color;
        if (d.sprite) {
          int sx = int((x - d.destination.x) * d.source.w / d.destination.w);
          if (d.flip)
            sx = int(d.source.w) - 1 - sx;
          sx += int(d.source.x);
          int sy = int(d.source.y +
                       (y - d.destination.y) * d.source.h / d.destination.h);
          if (sx < 0 || sy < 0 || sx >= atlas.width || sy >= atlas.height)
            continue;
          auto t = atlas.pixels[size_t(sy) * atlas.width + sx];
          c = {uint8_t(t.r * c.r / 255), uint8_t(t.g * c.g / 255),
               uint8_t(t.b * c.b / 255), uint8_t(t.a * c.a / 255)};
        }
        auto &dst = pixels[size_t(y) * Width + x];
        unsigned a = c.a, b = 255 - a;
        dst = {uint8_t((c.r * a + dst.r * b) / 255),
               uint8_t((c.g * a + dst.g * b) / 255),
               uint8_t((c.b * a + dst.b * b) / 255), 255};
      }
  }
}
void SoftwareRenderer::writePPM(const std::string &path) const {
  std::ofstream output(path, std::ios::binary);
  require(bool(output), "Cannot write screenshot");
  output << "P6\n" << Width << " " << Height << "\n255\n";
  for (auto p : pixels) {
    char b[] = {char(p.r), char(p.g), char(p.b)};
    output.write(b, 3);
  }
}
Rect body(Vec2 feet) { return {feet.x - 5, feet.y - 7, 10, 7}; }
bool blocked(const Project &p, const Scene &s, Rect r) {
  if (!validPoint({r.x, r.y}) || !validPoint({r.w, r.h}) || r.w <= 0 ||
      r.h <= 0 || r.x < 0 || r.y < 0 || r.x + r.w > s.width * TileSize ||
      r.y + r.h > s.height * TileSize)
    return true;
  int x0 = int(std::floor(r.x / TileSize)),
      y0 = int(std::floor(r.y / TileSize));
  int x1 = int(std::floor((r.x + r.w - 0.001f) / TileSize)),
      y1 = int(std::floor((r.y + r.h - 0.001f) / TileSize));
  for (int y = y0; y <= y1; ++y)
    for (int x = x0; x <= x1; ++x) {
      auto tile = p.tiles.find(s.at(x, y));
      if (tile == p.tiles.end() || tile->second.solid)
        return true;
    }
  for (const auto &o : s.objects) {
    const auto &sprite = p.sprites.at(o.sprite);
    auto c = sprite.collider;
    if (c.w > 0 && c.h > 0 &&
        overlaps(r, {o.position.x + c.x, o.position.y + c.y, c.w, c.h}))
      return true;
    for (auto extra : sprite.extraColliders)
      if (overlaps(r, {o.position.x + extra.x, o.position.y + extra.y, extra.w,
                       extra.h}))
        return true;
  }
  return false;
}
Vec2 moveBody(const Project &p, const Scene &s, Vec2 pos, Vec2 delta) {
  if (!validPoint(pos) || !validPoint(delta) ||
      std::max(std::abs(delta.x), std::abs(delta.y)) > 12288.0f)
    return pos;
  // Substeps prevent tunnelling, including callers that supply unusually large
  // deltas.
  int steps = std::max(
      1, int(std::ceil(std::max(std::abs(delta.x), std::abs(delta.y)) / 3.0f)));
  steps = std::min(steps, 4096);
  delta.x /= steps;
  delta.y /= steps;
  for (int i = 0; i < steps; ++i) {
    Vec2 next{pos.x + delta.x, pos.y};
    if (!blocked(p, s, body(next)))
      pos = next;
    next = {pos.x, pos.y + delta.y};
    if (!blocked(p, s, body(next)))
      pos = next;
  }
  return pos;
}
Vec2 cameraFor(const Scene &s, Vec2 p) {
  return {
      std::floor(std::clamp(p.x - Width / 2.0f, 0.0f,
                            std::max(0.0f, float(s.width * TileSize - Width)))),
      std::floor(
          std::clamp(p.y - Height / 2.0f, 0.0f,
                     std::max(0.0f, float(s.height * TileSize - Height))))};
}
void drawScene(Canvas &c, const Project &p, const Scene &s, Vec2 camera,
               Vec2 player, Rect playerSprite,
               const std::vector<std::string> &removed, bool debug) {
  c.clear({25, 45, 39, 255});
  int left = int(camera.x) / TileSize, top = int(camera.y) / TileSize;
  for (int y = top; y < std::min(s.height, top + Height / TileSize + 2); ++y)
    for (int x = left; x < std::min(s.width, left + Width / TileSize + 2); ++x)
      c.sprite(p.tiles.at(s.at(x, y)).source, {float(x * TileSize) - camera.x,
                                               float(y * TileSize) - camera.y});
  std::vector<const Object *> visible;
  for (const auto &o : s.objects)
    if (std::find(removed.begin(), removed.end(), o.id) == removed.end())
      visible.push_back(&o);
  auto bottom = [&](const Object *o) {
    return o->position.y + p.sprites.at(o->sprite).source.h;
  };
  std::stable_sort(
      visible.begin(), visible.end(),
      [&](const Object *a, const Object *b) { return bottom(a) < bottom(b); });
  bool drawn = false;
  auto drawPlayer = [&] {
    c.sprite(p.sprites.at("shadow").source,
             {player.x - camera.x - 8, player.y - camera.y - 3});
    c.sprite(playerSprite, {player.x - camera.x - 8, player.y - camera.y - 23});
  };
  for (auto o : visible) {
    if (!drawn && bottom(o) > player.y) {
      drawPlayer();
      drawn = true;
    }
    c.sprite(p.sprites.at(o->sprite).source,
             {o->position.x - camera.x, o->position.y - camera.y});
    if (debug) {
      auto b = p.sprites.at(o->sprite).collider;
      if (b.w > 0)
        c.rect({o->position.x + b.x - camera.x, o->position.y + b.y - camera.y,
                b.w, b.h},
               {255, 70, 90, 100});
    }
    if (debug)
      for (auto b : p.sprites.at(o->sprite).extraColliders)
        c.rect({o->position.x + b.x - camera.x, o->position.y + b.y - camera.y,
                b.w, b.h},
               {255, 70, 90, 100});
  }
  if (!drawn)
    drawPlayer();
  if (debug) {
    auto b = body(player);
    c.rect({b.x - camera.x, b.y - camera.y, b.w, b.h}, {70, 255, 220, 130});
  }
}
bool saveState(const std::string &path, const SavedState &s,
               std::string &error) {
  std::string temporary = path + ".tmp";
  std::ofstream out(temporary, std::ios::binary);
  if (!out) {
    error = "Cannot create save file";
    return false;
  }
  out << "LOTA_SAVE 1\n"
      << std::quoted(s.project) << " " << std::quoted(s.scene) << "\n"
      << std::setprecision(9) << s.player.x << " " << s.player.y << " "
      << s.score << "\n"
      << s.removed.size() << "\n";
  for (const auto &id : s.removed)
    out << std::quoted(id) << "\n";
  out.flush();
  bool okay = bool(out);
  out.close();
  if (!okay || out.fail()) {
    error = "Save write failed";
    std::remove(temporary.c_str());
    return false;
  }
#ifdef _WIN32
  bool committed =
      MoveFileExA(temporary.c_str(), path.c_str(),
                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
  bool committed = std::rename(temporary.c_str(), path.c_str()) == 0;
#endif
  if (!committed) {
    error = "Cannot commit save";
    std::remove(temporary.c_str());
    return false;
  }
  error.clear();
  return true;
}
bool loadState(const std::string &path, const Project &p, SavedState &state,
               std::string &error) {
  std::ifstream input(path, std::ios::binary);
  SavedState next;
  std::string magic;
  int version = 0, count = 0;
  input >> magic >> version >> std::quoted(next.project) >>
      std::quoted(next.scene) >> next.player.x >> next.player.y >> next.score >>
      count;
  if (!input || magic != "LOTA_SAVE" || version != 1 || next.project != p.id ||
      !p.scenes.count(next.scene) || count < 0 || count > 4096 ||
      next.score < 0 || !validPoint(next.player)) {
    error = "Save is missing or incompatible";
    return false;
  }
  for (int i = 0; i < count; ++i) {
    std::string id;
    input >> std::quoted(id);
    if (!input || id.size() > 128) {
      error = "Corrupt save";
      return false;
    }
    next.removed.push_back(id);
  }
  if (blocked(p, p.scenes.at(next.scene), body(next.player))) {
    error = "Saved position is blocked";
    return false;
  }
  state = std::move(next);
  error.clear();
  return true;
}
} // namespace lota
