// Gameplay styles: each map plays top-down (the default), as a sidescrolling platformer or as a
// first-person raycast view. Maps without a style load and play exactly as before.
#include "lota/studio.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace lota {
namespace {
void require(bool ok, const std::string &message) {
  if (!ok)
    throw std::runtime_error(message);
}
float real(const Json &j, float lo, float hi, const std::string &what) {
  require(j.type == Json::Number && std::isfinite(j.number) && j.number >= lo && j.number <= hi,
          "Invalid " + what);
  return float(j.number);
}
void onlyKeys(const Json &j, std::initializer_list<const char *> keys, const std::string &what) {
  require(j.type == Json::Object, "Invalid " + what + " settings");
  for (auto &entry : j.object)
    require(std::any_of(keys.begin(), keys.end(), [&](auto k) { return entry.first == k; }),
            "Unknown " + what + " setting: " + entry.first);
}
Color colour(const Json &j, const std::string &what) {
  auto &s = j.string;
  auto hex = [](char c) {
    return c >= '0' && c <= '9'   ? c - '0'
           : c >= 'a' && c <= 'f' ? c - 'a' + 10
           : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                  : -1;
  };
  require(j.type == Json::String && s.size() == 7 && s[0] == '#', "Invalid " + what);
  int v[6];
  for (int i = 0; i < 6; i++)
    require((v[i] = hex(s[size_t(i + 1)])) >= 0, "Invalid " + what);
  return {uint8_t(v[0] * 16 + v[1]), uint8_t(v[2] * 16 + v[3]), uint8_t(v[4] * 16 + v[5]), 255};
}
Color shaded(Color c, float light) {
  light = std::clamp(light, 0.f, 1.f);
  return {uint8_t(c.r * light), uint8_t(c.g * light), uint8_t(c.b * light), c.a};
}
constexpr float Pi = 3.14159265f;
constexpr float Plane = .66f; // half the field of view, as a tangent (about 66 degrees in all)
// A wall one tile away is this many pixels tall, so tiles look square on screen.
constexpr float Unit = Width / 2 / Plane;
// Sprites drawn for the top-down view are tall next to one-tile walls, so they're shrunk a little.
constexpr float BillboardScale = .6f;
float angleFor(const std::string &direction) {
  return direction == "east"    ? 0
         : direction == "west"  ? Pi
         : direction == "north" ? -Pi / 2
                                : Pi / 2;
}
std::string directionFor(float angle) {
  int quarter = int(std::lround(angle / (Pi / 2))) % 4;
  if (quarter < 0)
    quarter += 4;
  return quarter == 0 ? "east" : quarter == 1 ? "south" : quarter == 2 ? "west" : "north";
}
} // namespace

// ---- Loading --------------------------------------------------------------------------------

StudioMapStyle StudioGame::loadMapStyle(const Json &m) const {
  StudioMapStyle style;
  if (m.has("style")) {
    style.kind = m["style"].text();
    require(style.kind == "topdown" || style.kind == "side" || style.kind == "raycast",
            "Maps play top-down, side-on or in first person (raycast)");
  }
  if (m.has("side")) {
    require(style.kind == "side", "Only sidescroller maps have sidescroller settings");
    auto &v = m["side"];
    onlyKeys(v, {"gravity", "jump"}, "sidescroller");
    if (v.has("gravity"))
      style.gravity = real(v["gravity"], 100, 4000, "gravity");
    if (v.has("jump"))
      style.jump = real(v["jump"], 4, 160, "jump height");
  }
  if (m.has("raycast")) {
    require(style.kind == "raycast", "Only raycast maps have raycast settings");
    auto &v = m["raycast"];
    onlyKeys(v, {"floor", "ceiling", "viewDistance"}, "raycast");
    if (v.has("floor"))
      style.floor = colour(v["floor"], "floor colour");
    if (v.has("ceiling"))
      style.ceiling = colour(v["ceiling"], "ceiling colour");
    if (v.has("viewDistance"))
      style.viewDistance = real(v["viewDistance"], 32, 1024, "view distance");
  }
  return style;
}
// Combat, puzzles and vehicles are top-down only for now, and patrols need the top-down view.
void StudioGame::checkStyles() const {
  for (auto &entry : mapStyles) {
    if (entry.second.kind == "topdown")
      continue;
    auto &s = project.scenes.at(entry.first);
    for (auto &o : s.objects) {
      require(o.kind != "enemy" && o.kind != "block" && o.kind != "plate" && o.kind != "vehicle",
              "Enemies, push blocks, pressure plates and vehicles only work on top-down maps (" +
                  o.id + " on " + s.title + ")");
      if (entry.second.kind == "side")
        require(!actors.count(s.id + ":" + o.id),
                "NPCs can't patrol on sidescroller maps (" + o.id + " on " + s.title + ")");
    }
  }
}
const StudioMapStyle &StudioGame::styleOf(const std::string &map) const {
  static const StudioMapStyle topdown;
  auto i = mapStyles.find(map);
  return i == mapStyles.end() ? topdown : i->second;
}
// Arriving on a map (or loading a save) starts its style afresh.
void StudioGame::enterStyle() {
  if (styleScene == scene)
    return;
  styleScene = scene;
  sideVelocity = {};
  grounded = climbing = jumpHeld = false;
  coyoteClock = jumpBuffer = 0;
  safeFeet = feet;
  viewAngle = angleFor(direction);
  if (styleOf(scene).kind == "side" && direction != "east" && direction != "west")
    direction = "east";
}

// ---- Sidescroller ---------------------------------------------------------------------------

int StudioGame::sideCell(int x, int y) const {
  auto &s = project.scenes.at(scene);
  if (x < 0 || x >= s.width || y < 0 || y >= s.height)
    return 0;
  return layers.at(scene).collision[size_t(y * s.width + x)];
}
// The whole player stands in a sidescroller, not just their feet.
Rect StudioGame::sideBody(Vec2 at) const {
  return {at.x - 5, at.y - 14, 10, 14};
}
bool StudioGame::sideStanding(const Scene &collision, Vec2 at) const {
  auto r = sideBody(at);
  if (blocked(project, collision, {r.x, r.y + 1, r.w, r.h}))
    return true;
  // The top edge of a platform, or of a ladder with no ladder above it.
  if (std::fmod(at.y, 16.f) > .01f)
    return false;
  int y = int(at.y) / 16;
  for (int x = int(r.x) / 16; x <= int(r.x + r.w - .001f) / 16; x++) {
    int k = sideCell(x, y);
    if (k == 2 || (k == 3 && sideCell(x, y - 1) != 3))
      return true;
  }
  return false;
}
void StudioGame::moveSide(const Input &in, float dt) {
  auto &style = styleOf(scene);
  auto collision = collisionScene(scene, removed, activated);
  auto touches = [&](Rect r, int kind) {
    for (int y = int(std::floor(r.y / 16)); y <= int(std::floor((r.y + r.h - .001f) / 16)); y++)
      for (int x = int(std::floor(r.x / 16)); x <= int(std::floor((r.x + r.w - .001f) / 16)); x++)
        if (sideCell(x, y) == kind)
          return true;
    return false;
  };
  float run = in.down(Action::Run) ? runMultiplier : 1,
        across = float(in.down(Action::Right)) - float(in.down(Action::Left)),
        upDown = float(in.down(Action::Down)) - float(in.down(Action::Up));
  if (across != 0)
    direction = across > 0 ? "east" : "west";
  bool onLadder = touches(sideBody(feet), 3), ladderBelow = touches({feet.x - 2, feet.y, 4, 2}, 3);
  if (!climbing && ((onLadder && in.down(Action::Up)) || (ladderBelow && in.down(Action::Down))))
    climbing = true, sideVelocity = {};
  if (!onLadder && !ladderBelow)
    climbing = false;

  // Jumping: a short grace period after leaving a ledge, and presses just before landing count.
  coyoteClock = grounded ? .1f : std::max(0.f, coyoteClock - dt);
  bool jumpKey = in.hit(Action::Attack) || (in.hit(Action::Up) && !onLadder);
  jumpBuffer = jumpKey ? .12f : std::max(0.f, jumpBuffer - dt);
  if (jumpBuffer > 0 && (coyoteClock > 0 || climbing)) {
    sideVelocity.y = -std::sqrt(2 * style.gravity * style.jump);
    jumpBuffer = coyoteClock = 0;
    grounded = climbing = false;
    jumpHeld = true;
    cue("jump");
  }
  // Letting go early cuts the jump short.
  if (jumpHeld && !in.down(Action::Attack) && !in.down(Action::Up) && sideVelocity.y < 0)
    sideVelocity.y *= .45f, jumpHeld = false;
  if (sideVelocity.y >= 0)
    jumpHeld = false;
  if (climbing) {
    sideVelocity = {across * speed * .6f, upDown * speed * .9f};
  } else {
    sideVelocity.x = across * speed * run;
    sideVelocity.y = std::min(sideVelocity.y + style.gravity * dt, 420.f);
  }
  clock = across != 0 && (grounded || climbing) ? clock + dt * run : 0;

  // Across, then down or up in small steps, landing on solid ground and platform tops.
  Vec2 move{sideVelocity.x * dt, sideVelocity.y * dt};
  int steps = std::max(1, int(std::ceil(std::max(std::abs(move.x), std::abs(move.y)) / 3)));
  bool dropping = in.down(Action::Down) && !climbing;
  for (int i = 0; i < steps; i++) {
    Vec2 next{feet.x + move.x / steps, feet.y};
    if (!blocked(project, collision, sideBody(next)))
      feet = next;
    next = {feet.x, feet.y + move.y / steps};
    if (blocked(project, collision, sideBody(next))) {
      // Close the gap to whatever was hit, so the player stands exactly on the ground.
      float way = move.y > 0 ? 1.f : -1.f;
      for (int k = 0; k < 64; k++) {
        Vec2 closer{feet.x, feet.y + way / 16};
        if (blocked(project, collision, sideBody(closer)))
          break;
        feet = closer;
      }
      if (move.y > 0)
        feet.y = std::round(feet.y * 16) / 16;
      sideVelocity.y = 0;
      break;
    }
    if (move.y > 0 && !climbing && !dropping) {
      float top = std::floor(next.y / 16) * 16;
      auto r = sideBody(next);
      bool landed = false;
      if (feet.y <= top + .001f && next.y > top)
        for (int x = int(r.x) / 16; x <= int(r.x + r.w - .001f) / 16; x++) {
          int k = sideCell(x, int(top) / 16);
          landed |= k == 2 || (k == 3 && sideCell(x, int(top) / 16 - 1) != 3);
        }
      if (landed) {
        feet.y = top;
        sideVelocity.y = 0;
        break;
      }
    }
    feet = next;
  }
  grounded = !climbing && sideVelocity.y >= 0 && sideStanding(collision, feet);
  if (grounded)
    sideVelocity.y = 0;

  // Hazards send the player back to the last safe ground they stood on.
  auto r = sideBody(feet);
  if (touches({r.x + 2, r.y + 2, r.w - 4, r.h - 2}, 4)) {
    if (combatEnabled)
      hurtPlayer(1);
    else
      cue("hurt");
    feet = safeFeet;
    sideVelocity = {};
    climbing = false;
  } else if (grounded && !touches({r.x - 6, r.y, r.w + 12, r.h + 2}, 4))
    safeFeet = feet;
  if (across != 0 || move.y != 0)
    progressDirty = true;
}

// ---- Raycast --------------------------------------------------------------------------------

void StudioGame::moveRaycast(const Input &in, float dt) {
  float run = in.down(Action::Run) ? runMultiplier : 1,
        turn = float(in.down(Action::Right)) - float(in.down(Action::Left)),
        forward = float(in.down(Action::Up)) - float(in.down(Action::Down));
  viewAngle = std::remainder(viewAngle + turn * 2.4f * dt, 2 * Pi);
  direction = directionFor(viewAngle);
  float step = forward * speed * run * dt;
  if (step != 0) {
    // A wider body than top-down keeps the camera from pressing its nose against walls.
    auto collision = collisionScene(scene, removed, activated);
    auto room = [&](Vec2 at) {
      return !blocked(project, collision, {at.x - 7, at.y - 10.5f, 14, 14}) &&
             !blocked(project, collision, body(at));
    };
    Vec2 move{std::cos(viewAngle) * step, std::sin(viewAngle) * step};
    int steps = std::max(1, int(std::ceil(std::abs(step) / 2)));
    for (int i = 0; i < steps; i++) {
      Vec2 next{feet.x + move.x / steps, feet.y};
      if (room(next))
        feet = next;
      next = {feet.x, feet.y + move.y / steps};
      if (room(next))
        feet = next;
    }
    progressDirty = true;
  }
  clock = forward != 0 ? clock + dt * run : 0;
}
// Things the player can reach in first person must be roughly in front of them.
bool StudioGame::inView(Vec2 point, float range) const {
  Vec2 eye{feet.x, feet.y - 3.5f}, to{point.x - eye.x, point.y - eye.y};
  float distance = std::hypot(to.x, to.y);
  if (distance > range)
    return false;
  if (distance < 6)
    return true;
  return (to.x * std::cos(viewAngle) + to.y * std::sin(viewAngle)) / distance > .7f;
}
void StudioGame::drawRaycast(Canvas &c) const {
  auto &style = styleOf(scene);
  auto s = collisionScene(scene, removed, activated);
  auto &l = layers.at(scene);
  float px = feet.x / 16, py = (feet.y - 3.5f) / 16, dx = std::cos(viewAngle),
        dy = std::sin(viewAngle), planeX = -dy * Plane, planeY = dx * Plane,
        fade = style.viewDistance / 16;
  // Ceiling and floor darken towards the horizon.
  const int band = 4, half = Height / 2;
  for (int y = 0; y < half; y += band) {
    float distance = .5f * Unit / float(half - y);
    float light = 1 - distance / fade;
    c.rect({0, float(y), float(Width), float(band)}, shaded(style.ceiling, std::max(light, .12f)));
    c.rect({0, float(Height - y - band), float(Width), float(band)},
           shaded(style.floor, std::max(light, .12f)));
  }
  // Walls: one ray every two columns, each drawing a one-pixel slice of the wall's tile.
  const int column = 2;
  std::vector<float> depth(size_t(Width / column), 1e9f);
  for (int x = 0; x < Width; x += column) {
    float camera = 2.f * (x + column / 2.f) / Width - 1, rx = dx + planeX * camera,
          ry = dy + planeY * camera;
    int mx = int(std::floor(px)), my = int(std::floor(py)), stepX = rx < 0 ? -1 : 1,
        stepY = ry < 0 ? -1 : 1, side = 0;
    float deltaX = rx == 0 ? 1e30f : std::abs(1 / rx), deltaY = ry == 0 ? 1e30f : std::abs(1 / ry),
          sideX = (rx < 0 ? px - mx : mx + 1 - px) * deltaX,
          sideY = (ry < 0 ? py - my : my + 1 - py) * deltaY;
    bool hit = false;
    for (int i = 0; i < 256 && !hit; i++) {
      if (sideX < sideY)
        sideX += deltaX, mx += stepX, side = 0;
      else
        sideY += deltaY, my += stepY, side = 1;
      if (mx < 0 || my < 0 || mx >= s.width || my >= s.height)
        break;
      hit = s.at(mx, my) == '#';
    }
    if (!hit)
      continue;
    float distance = side == 0 ? sideX - deltaX : sideY - deltaY;
    distance = std::max(distance, .02f);
    depth[size_t(x / column)] = distance;
    float along = side == 0 ? py + distance * ry : px + distance * rx;
    along -= std::floor(along);
    int textureX = std::clamp(int(along * 16), 0, 15);
    if ((side == 0 && rx < 0) || (side == 1 && ry > 0))
      textureX = 15 - textureX;
    int cell = my * s.width + mx;
    auto &tile = tileSources[size_t(l.detail[size_t(cell)] >= 0 ? l.detail[size_t(cell)]
                                                                : l.base[size_t(cell)])];
    float tall = Unit / distance,
          light = std::max(1 - distance / fade, .1f) * (side == 1 ? .82f : 1.f);
    c.image({tile.x + float(textureX), tile.y, 1, 16},
            {float(x), half - tall / 2, float(column), tall}, false,
            shaded({255, 255, 255, 255}, light));
  }
  // Objects and characters face the camera, sorted far to near and hidden behind walls.
  struct Billboard {
    float depth, screenX;
    Rect source;
  };
  std::vector<Billboard> boards;
  float inverse = 1 / (planeX * dy - dx * planeY);
  for (auto &o : s.objects) {
    auto key = scene + ":" + o.id;
    if (std::find(removed.begin(), removed.end(), key) != removed.end())
      continue;
    auto &sprite = project.sprites.at(visibleSprite(o));
    Rect source = sprite.source;
    actorFrame(key, source);
    auto &box = project.sprites.at(o.sprite).collider;
    Vec2 at = box.w > 0
                  ? Vec2{o.position.x + box.x + box.w / 2, o.position.y + box.y + box.h / 2}
                  : Vec2{o.position.x + sprite.source.w / 2, o.position.y + sprite.source.h - 4};
    float rx = at.x / 16 - px, ry = at.y / 16 - py, sx = inverse * (dy * rx - dx * ry),
          sz = inverse * (-planeY * rx + planeX * ry);
    if (sz > .15f)
      boards.push_back({sz, Width / 2.f * (1 + sx / sz), source});
  }
  // Doors that are shut still block, but opened ones (and everything else) are just pictures.
  for (auto &o : project.scenes.at(scene).objects) {
    auto key = scene + ":" + o.id;
    if (o.kind != "door" || !activated.count(key))
      continue;
    auto &sprite = project.sprites.at(visibleSprite(o));
    float rx = (o.position.x + sprite.source.w / 2) / 16 - px,
          ry = (o.position.y + sprite.source.h - 4) / 16 - py, sx = inverse * (dy * rx - dx * ry),
          sz = inverse * (-planeY * rx + planeX * ry);
    if (sz > .15f)
      boards.push_back({sz, Width / 2.f * (1 + sx / sz), sprite.source});
  }
  std::sort(boards.begin(), boards.end(), [](auto &a, auto &b) { return a.depth > b.depth; });
  for (auto &b : boards) {
    float scale = Unit / 16 / b.depth * BillboardScale, w = b.source.w * scale,
          h = b.source.h * scale, floor = half + Unit / 2 / b.depth, left = b.screenX - w / 2,
          light = std::max(1 - b.depth / fade, .1f);
    for (int x = std::max(0, int(left) / column * column); x < std::min(Width, int(left + w) + 1);
         x += column) {
      if (b.depth >= depth[size_t(x / column)])
        continue;
      int texture =
          std::clamp(int((x + column / 2.f - left) / w * b.source.w), 0, int(b.source.w) - 1);
      c.image({b.source.x + float(texture), b.source.y, 1, b.source.h},
              {float(x), floor - h, float(column), h}, false, shaded({255, 255, 255, 255}, light));
    }
  }
}
} // namespace lota
