// EveNGINE 1.2 world features: time of day and weather, pushable blocks and pressure plates,
// leveling, cutscenes and vehicles. Everything here is optional project data, so older projects
// load unchanged and their saves keep their old format.
#include "lota/studio.hpp"
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>
namespace lota {
namespace {
void require(bool ok, const std::string &message) {
  if (!ok)
    throw std::runtime_error(message);
}
int integer(const Json &j, int lo, int hi, const std::string &what) {
  require(j.type == Json::Number && std::floor(j.number) == j.number && j.number >= lo &&
              j.number <= hi,
          "Invalid " + what);
  return int(j.number);
}
float real(const Json &j, float lo, float hi, const std::string &what) {
  require(j.type == Json::Number && std::isfinite(j.number) && j.number >= lo && j.number <= hi,
          "Invalid " + what);
  return float(j.number);
}
std::string ident(const Json &j, const std::string &what) {
  require(j.type == Json::String && !j.string.empty() && j.string.size() <= 64 &&
              j.string.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                         "0123456789_-") == std::string::npos,
          "Invalid " + what);
  return j.string;
}
bool toggle(const Json &j, const std::string &what) {
  require(j.type == Json::Bool, "Invalid " + what);
  return j.boolean();
}
std::string words(const Json &j, size_t max, bool empty, const std::string &what) {
  require(j.type == Json::String && textLength(j.string) <= max && (empty || !j.string.empty()),
          "Invalid " + what);
  return j.string;
}
int hexDigit(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}
Color colour(const Json &j, const std::string &what) {
  auto &s = j.string;
  require(j.type == Json::String && s.size() == 7 && s[0] == '#', "Invalid " + what);
  int v[6];
  for (int i = 0; i < 6; i++)
    require((v[i] = hexDigit(s[size_t(i + 1)])) >= 0, "Invalid " + what);
  return {uint8_t(v[0] * 16 + v[1]), uint8_t(v[2] * 16 + v[3]), uint8_t(v[4] * 16 + v[5]), 255};
}
void onlyKeys(const Json &j, std::initializer_list<const char *> keys, const std::string &what) {
  for (auto &entry : j.object)
    require(std::any_of(keys.begin(), keys.end(), [&](auto k) { return entry.first == k; }),
            "Unknown " + what + " setting " + entry.first);
}
bool validWeather(const std::string &w) {
  return w == "none" || w == "rain" || w == "snow" || w == "fog";
}
const char *directionNames[] = {"south", "north", "east", "west"};
// Deterministic noise for weather particles, so nothing random needs saving.
float noise(int i, int salt) {
  uint32_t h = uint32_t(i) * 374761393u + uint32_t(salt) * 668265263u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return float((h ^ (h >> 16)) & 0xffffff) / float(0x1000000);
}
float smooth(float t) {
  t = std::clamp(t, 0.f, 1.f);
  return t * t * (3 - 2 * t);
}
float wrap(float v, float span) {
  v = std::fmod(v, span);
  return v < 0 ? v + span : v;
}
std::string mapOf(const std::string &key) {
  return key.substr(0, key.find(':'));
}
std::string idOf(const std::string &key) {
  return key.substr(key.find(':') + 1);
}
const Object *findObject(const Scene &s, const std::string &id) {
  for (auto &o : s.objects)
    if (o.id == id)
      return &o;
  return nullptr;
}
} // namespace

// ---- Loading ------------------------------------------------------------------------------

void StudioGame::loadWorld(const Json &j) {
  if (j.has("time")) {
    auto &t = j["time"];
    require(t.type == Json::Object, "Invalid time of day settings");
    onlyKeys(t, {"dayLength", "start", "night", "strength", "clock"}, "time of day");
    timeSettings.enabled = true;
    if (t.has("dayLength"))
      timeSettings.dayLength = real(t["dayLength"], 60, 7200, "day length");
    if (t.has("start"))
      timeSettings.startHour = real(t["start"], 0, 23.99f, "starting hour");
    if (t.has("night"))
      timeSettings.night = colour(t["night"], "night colour");
    if (t.has("strength"))
      timeSettings.strength = real(t["strength"], 0, .85f, "night strength");
    if (t.has("clock"))
      timeSettings.showClock = toggle(t["clock"], "clock setting");
  }
  for (auto &m : j["scenes"].array) {
    auto map = m["id"].text();
    auto &scene = project.scenes.at(map);
    if (m.has("weather")) {
      auto w = words(m["weather"], 8, false, "weather");
      require(validWeather(w), "Unknown weather " + w);
      if (w != "none")
        mapWeather[map] = w;
    }
    if (m.has("indoors") && toggle(m["indoors"], "indoors setting"))
      indoorMaps.insert(map);
    for (auto &o : m["objects"].array) {
      auto key = map + ":" + o["id"].text();
      auto kind = o["kind"].text();
      auto &sprite = project.sprites.at(o["sprite"].text());
      if (kind == "block") {
        require(sprite.collider.w > 0 && sprite.collider.h > 0,
                "Pushable blocks need artwork with a collider");
        blocks.insert(key);
      }
      if (kind == "plate") {
        StudioPlate plate;
        if (o.has("plate")) {
          auto &p = o["plate"];
          require(p.type == Json::Object, "Invalid pressure plate");
          onlyKeys(p, {"flag", "targets", "sticky"}, "pressure plate");
          if (p.has("flag") && !p["flag"].text().empty())
            plate.flag = ident(p["flag"], "pressure plate flag");
          if (p.has("targets")) {
            require(p["targets"].type == Json::Array && p["targets"].array.size() <= 32,
                    "Invalid pressure plate doors");
            for (auto &t : p["targets"].array) {
              auto door = ident(t, "pressure plate door");
              auto target = findObject(scene, door);
              require(target && target->kind == "door",
                      "Pressure plates can only open doors on their own map");
              require(std::find(plate.targets.begin(), plate.targets.end(), door) ==
                          plate.targets.end(),
                      "Duplicate pressure plate door");
              plate.targets.push_back(door);
            }
          }
          if (p.has("sticky"))
            plate.sticky = toggle(p["sticky"], "pressure plate latch");
        }
        plates[key] = plate;
      } else
        require(!o.has("plate"), "Only pressure plates have plate settings");
      if (kind == "vehicle") {
        auto &v = o["vehicle"];
        require(v.type == Json::Object, "Vehicles need vehicle settings");
        onlyKeys(v, {"speed", "terrain", "acceleration", "braking"}, "vehicle");
        StudioVehicle vehicle;
        if (v.has("speed"))
          vehicle.speed = real(v["speed"], .25f, 6, "vehicle speed");
        if (v.has("acceleration"))
          vehicle.acceleration = real(v["acceleration"], 0, 10, "vehicle acceleration");
        if (v.has("braking"))
          vehicle.braking = real(v["braking"], 0, 10, "vehicle braking");
        auto terrain = v.has("terrain") ? v["terrain"].text() : "land";
        require(terrain == "land" || terrain == "water", "Vehicles travel on land or water");
        vehicle.water = terrain == "water";
        vehicles[key] = vehicle;
      } else
        require(!o.has("vehicle"), "Only vehicles have vehicle settings");
    }
  }
  if (j.has("cutscenes")) {
    auto &list = j["cutscenes"];
    require(list.type == Json::Array && list.array.size() <= 64, "Use at most 64 cutscenes");
    for (auto &c : list.array) {
      require(c.type == Json::Object, "Invalid cutscene");
      StudioCutscene cut;
      cut.id = ident(c["id"], "cutscene ID");
      cut.name = words(c["name"], 160, false, "cutscene name");
      cut.scene = ident(c["scene"], "cutscene map");
      require(project.scenes.count(cut.scene), "Unknown cutscene map");
      auto &scene = project.scenes.at(cut.scene);
      auto &steps = c["steps"];
      require(steps.type == Json::Array && !steps.array.empty() && steps.array.size() <= 64,
              "Cutscenes need 1-64 steps");
      for (auto &s : steps.array) {
        require(s.type == Json::Object, "Invalid cutscene step");
        StudioCutsceneStep step;
        step.type = words(s["type"], 16, false, "cutscene step");
        auto point = [&]() {
          return Vec2{real(s["x"], 0, scene.width * 16 - .001f, "cutscene x"),
                      real(s["y"], 0, scene.height * 16 - .001f, "cutscene y")};
        };
        auto actor = [&]() {
          auto name = ident(s["actor"], "cutscene actor");
          require(name == "player" || findObject(scene, name),
                  "Cutscene actor isn't on the cutscene's map");
          return name;
        };
        if (step.type == "wait")
          step.seconds = real(s["seconds"], .05f, 60, "cutscene wait");
        else if (step.type == "fade") {
          auto to = s["to"].text();
          require(to == "black" || to == "clear", "Fades go to black or clear");
          step.dark = to == "black";
          step.seconds = real(s["seconds"], 0, 10, "fade length");
        } else if (step.type == "say") {
          if (s.has("speaker"))
            step.speaker = words(s["speaker"], 160, true, "cutscene speaker");
          step.text = words(s["text"], 720, false, "cutscene line");
        } else if (step.type == "move") {
          step.actor = actor();
          step.point = point();
          if (s.has("speed"))
            step.speed = real(s["speed"], 8, 256, "cutscene move speed");
        } else if (step.type == "face") {
          step.actor = actor();
          step.direction = s["direction"].text();
          require(std::find(std::begin(directionNames), std::end(directionNames), step.direction) !=
                      std::end(directionNames),
                  "Unknown cutscene direction");
        } else if (step.type == "camera") {
          if (s.has("reset"))
            step.reset = toggle(s["reset"], "camera reset");
          if (!step.reset)
            step.point = point();
          step.seconds = real(s["seconds"], 0, 10, "camera move length");
        } else if (step.type == "sound")
          step.target = ident(s["target"], "cutscene sound");
        else if (step.type == "flag") {
          step.target = ident(s["target"], "cutscene flag");
          step.value = toggle(s["value"], "cutscene flag value");
        } else
          require(false, "Unknown cutscene step " + step.type);
        cut.steps.push_back(step);
      }
      require(cutscenes.emplace(cut.id, cut).second, "Duplicate cutscene");
    }
  }
}

void StudioGame::loadWorldLate(const Json &j) {
  if (j.has("leveling")) {
    auto &l = j["leveling"];
    require(l.type == Json::Object, "Invalid leveling settings");
    require(combatEnabled, "Leveling needs player combat settings");
    onlyKeys(l, {"maxLevel", "firstLevel", "growth", "health", "attack", "defense"}, "leveling");
    leveling.enabled = true;
    if (l.has("maxLevel"))
      leveling.maxLevel = integer(l["maxLevel"], 2, 99, "maximum level");
    if (l.has("firstLevel"))
      leveling.firstLevel = integer(l["firstLevel"], 1, 9999, "XP for level 2");
    if (l.has("growth"))
      leveling.growth = real(l["growth"], 1, 3, "XP growth");
    if (l.has("health"))
      leveling.health = integer(l["health"], 0, 99, "health per level");
    if (l.has("attack"))
      leveling.attack = integer(l["attack"], 0, 99, "attack per level");
    if (l.has("defense"))
      leveling.defense = integer(l["defense"], 0, 99, "defense per level");
  }
  for (auto &m : j["scenes"].array)
    for (auto &o : m["objects"].array)
      if (o["kind"].text() == "enemy" && o["combat"].has("xp")) {
        require(leveling.enabled, "Enemy XP needs leveling settings");
        enemyXp[m["id"].text() + ":" + o["id"].text()] =
            integer(o["combat"]["xp"], 0, 9999, "enemy XP");
      }
  for (auto &q : j["quests"].array)
    if (q.has("rewardXp")) {
      require(leveling.enabled, "Quest XP needs leveling settings");
      questXp[q["id"].text()] = integer(q["rewardXp"], 0, 99999, "quest XP");
    }
  for (auto &e : events)
    for (auto &a : e.actions)
      require(a.type != "giveXp" || leveling.enabled, "XP rewards need leveling settings");
  for (auto &p : plates)
    require(p.second.flag.empty() || flags.count(p.second.flag), "Unknown pressure plate flag");
  for (auto &c : cutscenes)
    for (auto &s : c.second.steps) {
      require(s.type != "sound" || audioAssets.count(s.target), "Unknown cutscene sound");
      require(s.type != "flag" || flags.count(s.target), "Unknown cutscene flag");
    }
  resetWorld();
}

bool StudioGame::hasWorld() const {
  return timeSettings.enabled || !mapWeather.empty() || !blocks.empty() || !plates.empty() ||
         !vehicles.empty() || leveling.enabled || !cutscenes.empty();
}

void StudioGame::resetWorld() {
  hour = timeSettings.startHour;
  ambientClock = 0;
  weatherOverride.clear();
  movedObjects.clear();
  pressedPlates.clear();
  latchedPlates.clear();
  plateDoors.clear();
  riding.clear();
  rideVelocity = {};
  pushTarget.clear();
  pushClock = 0;
  xp = 0;
  level = 1;
  cutscene.clear();
  cutsceneStep = 0;
  cutsceneClock = fade = fadeFrom = 0;
  cameraOverride = false;
}

// ---- Time of day and weather ----------------------------------------------------------------

void StudioGame::updateClock(float dt) {
  ambientClock += dt;
  if (timeSettings.enabled)
    hour = std::fmod(hour + dt * 24 / timeSettings.dayLength, 24.f);
}
std::string StudioGame::weatherAt(const std::string &map) const {
  auto o = weatherOverride.find(map);
  if (o != weatherOverride.end())
    return o->second;
  auto w = mapWeather.find(map);
  return w == mapWeather.end() ? "none" : w->second;
}
float StudioGame::darkness() const {
  if (!timeSettings.enabled || indoorMaps.count(scene))
    return 0;
  if (hour >= 7 && hour < 18)
    return 0;
  if (hour >= 18 && hour < 21)
    return smooth((hour - 18) / 3);
  if (hour >= 5 && hour < 7)
    return 1 - smooth((hour - 5) / 2);
  return 1;
}
bool StudioGame::timeBetween(float from, float to) const {
  return from <= to ? hour >= from && hour < to : hour >= from || hour < to;
}
Vec2 StudioGame::cameraPosition() const {
  auto &s = project.scenes.at(scene);
  return cameraFor(s, cameraOverride ? cameraNow : feet);
}
void StudioGame::drawWorld(Canvas &c, Vec2 camera) const {
  auto weather = weatherAt(scene);
  float t = ambientClock;
  if (weather == "rain") {
    c.rect({0, 0, float(Width), float(Height)}, {40, 60, 90, 36});
    for (int i = 0; i < 110; i++) {
      float x = wrap(noise(i, 1) * 440 - camera.x - t * 50, 440) - 20,
            y = wrap(noise(i, 2) * 270 - camera.y + t * (230 + noise(i, 3) * 90), 270) - 15;
      c.rect({x, y, 1, 6}, {170, 195, 230, 150});
    }
  } else if (weather == "snow") {
    for (int i = 0; i < 80; i++) {
      float x = wrap(noise(i, 1) * 420 - camera.x + std::sin(t * 1.3f + float(i)) * 6 + t * 5,
                     420) -
                10,
            y = wrap(noise(i, 2) * 260 - camera.y + t * (16 + noise(i, 3) * 14), 260) - 10;
      float size = noise(i, 4) < .3f ? 2 : 1;
      c.rect({x, y, size, size}, {245, 248, 255, 215});
    }
  } else if (weather == "fog") {
    c.rect({0, 0, float(Width), float(Height)}, {205, 214, 218, 48});
    for (int b = 0; b < 4; b++) {
      float x = wrap(t * (5 + float(b) * 3) + float(b) * 97 - camera.x * .3f, 520) - 260;
      for (float copy : {x, x + 520})
        c.rect({copy, float(26 + b * 52), 260, 30}, {215, 222, 226, 34});
    }
  }
  float d = darkness() * timeSettings.strength;
  if (d > 0) {
    auto n = timeSettings.night;
    c.rect({0, 0, float(Width), float(Height)}, {n.r, n.g, n.b, uint8_t(d * 255)});
  }
}
void StudioGame::drawWorldHud(Canvas &c) const {
  if (!timeSettings.enabled || !timeSettings.showClock)
    return;
  int minutes = int(hour * 60) % (24 * 60);
  char text[8];
  std::snprintf(text, sizeof text, "%02d:%02d", minutes / 60, minutes % 60);
  c.rect({290, 6, 36, 21}, uiPanel);
  c.text(text, 293, 12, uiAccent);
}

// ---- Puzzles --------------------------------------------------------------------------------

Rect StudioGame::objectBounds(const std::string &, const Object &o) const {
  auto c = project.sprites.at(o.sprite).collider;
  return {o.position.x + c.x, o.position.y + c.y, c.w, c.h};
}
bool StudioGame::pushBlock(const std::string &key, Vec2 direction) {
  if (mapOf(key) != scene)
    return false;
  auto s = collisionScene(scene, removed, activated);
  auto o =
      std::find_if(s.objects.begin(), s.objects.end(), [&](auto &o) { return o.id == idOf(key); });
  if (o == s.objects.end())
    return false;
  Vec2 to{o->position.x + direction.x * 16, o->position.y + direction.y * 16};
  auto moved = *o;
  moved.position = to;
  auto next = objectBounds(scene, moved);
  s.objects.erase(o);
  if (blocked(project, s, next) || overlaps(next, body(feet)))
    return false;
  auto &water = layers.at(scene).water;
  for (int y = int(next.y) / 16; y <= int(next.y + next.h - .001f) / 16; y++)
    for (int x = int(next.x) / 16; x <= int(next.x + next.w - .001f) / 16; x++)
      if (water[size_t(y * s.width + x)])
        return false;
  movedObjects[key] = to;
  progressDirty = true;
  return true;
}
bool StudioGame::updatePush(Vec2 attempted, Vec2 before, float dt) {
  (void)before;
  if (blocks.empty() || !riding.empty() || (attempted.x == 0 && attempted.y == 0)) {
    pushTarget.clear();
    pushClock = 0;
    return false;
  }
  Vec2 dir = std::abs(attempted.x) >= std::abs(attempted.y) ? Vec2{attempted.x > 0 ? 1.f : -1.f, 0}
                                                            : Vec2{0, attempted.y > 0 ? 1.f : -1.f};
  auto probe = body(feet);
  probe.x += dir.x * 2;
  probe.y += dir.y * 2;
  std::string hit;
  for (auto &o : liveScene(scene).objects) {
    auto key = scene + ":" + o.id;
    if (blocks.count(key) && overlaps(probe, objectBounds(scene, o)))
      hit = key;
  }
  if (hit.empty()) {
    pushTarget.clear();
    pushClock = 0;
    return false;
  }
  if (hit != pushTarget) {
    pushTarget = hit;
    pushClock = 0;
  }
  pushClock += dt;
  if (pushClock < .25f)
    return false;
  pushClock = 0;
  return pushBlock(hit, dir);
}
void StudioGame::updatePlates() {
  if (plates.empty())
    return;
  std::set<std::string> now;
  for (auto &p : plates) {
    auto map = mapOf(p.first);
    auto s = liveScene(map);
    auto plate = findObject(s, idOf(p.first));
    if (!plate)
      continue;
    auto source = project.sprites.at(plate->sprite).source;
    Rect centre{plate->position.x + source.w / 2 - 2, plate->position.y + source.h / 2 - 2, 4, 4};
    bool pressed = latchedPlates.count(p.first) || (map == scene && overlaps(body(feet), centre));
    for (auto &o : s.objects)
      if (!pressed && blocks.count(map + ":" + o.id) && overlaps(objectBounds(map, o), centre))
        pressed = true;
    if (pressed)
      now.insert(p.first);
  }
  for (auto &p : plates) {
    bool on = now.count(p.first), was = pressedPlates.count(p.first);
    if (on == was)
      continue;
    if (!p.second.flag.empty())
      flags[p.second.flag] = on;
    if (on) {
      auto map = mapOf(p.first);
      for (auto &door : p.second.targets) {
        auto key = map + ":" + door;
        if (!activated.count(key)) {
          activated.insert(key);
          plateDoors.insert(key);
          questEvent("activate", key);
        }
      }
      if (p.second.sticky)
        latchedPlates.insert(p.first);
      cue("chest");
    }
    progressDirty = true;
  }
  pressedPlates = std::move(now);
  // Doors a plate opened close again once no plate holds them, unless the player is in the way.
  for (auto i = plateDoors.begin(); i != plateDoors.end();) {
    auto map = mapOf(*i), door = idOf(*i);
    bool held = std::any_of(plates.begin(), plates.end(), [&](auto &p) {
      return mapOf(p.first) == map && pressedPlates.count(p.first) &&
             std::find(p.second.targets.begin(), p.second.targets.end(), door) !=
                 p.second.targets.end();
    });
    auto object = findObject(project.scenes.at(map), door);
    if (!held && map == scene && object && overlaps(body(feet), objectBounds(map, *object)))
      held = true;
    if (held) {
      ++i;
      continue;
    }
    activated.erase(*i);
    i = plateDoors.erase(i);
    progressDirty = true;
  }
}
void StudioGame::resetPuzzle(const std::string &map) {
  for (auto i = movedObjects.begin(); i != movedObjects.end();) {
    if (mapOf(i->first) != map || !blocks.count(i->first)) {
      ++i;
      continue;
    }
    auto original = findObject(project.scenes.at(map), idOf(i->first));
    if (map == scene && original && overlaps(objectBounds(map, *original), body(feet))) {
      ++i; // leave a block that would land on the player
      continue;
    }
    i = movedObjects.erase(i);
  }
  for (auto i = latchedPlates.begin(); i != latchedPlates.end();)
    i = mapOf(*i) == map ? latchedPlates.erase(i) : std::next(i);
  pushTarget.clear();
  pushClock = 0;
  progressDirty = true;
}

// ---- Vehicles -------------------------------------------------------------------------------

bool StudioGame::rideAllowed(const Scene &collision, Vec2 at, const StudioVehicle &v) const {
  auto r = body(at);
  if (r.x < 0 || r.y < 0 || r.x + r.w > collision.width * 16 || r.y + r.h > collision.height * 16)
    return false;
  auto &water = layers.at(collision.id).water;
  for (int y = int(r.y) / 16; y <= int(r.y + r.h - .001f) / 16; y++)
    for (int x = int(r.x) / 16; x <= int(r.x + r.w - .001f) / 16; x++) {
      bool wet = water[size_t(y * collision.width + x)];
      if (wet != v.water)
        return false;
      // Boats float over water even where the shore marks it as a wall.
      if (!v.water) {
        auto tile = project.tiles.find(collision.at(x, y));
        if (tile == project.tiles.end() || tile->second.solid)
          return false;
      }
    }
  auto objectsOnly = collision;
  objectsOnly.cells.assign(collision.cells.size(), '.');
  return !blocked(project, objectsOnly, r);
}
Vec2 StudioGame::ride(Vec2 from, Vec2 delta) {
  auto collision = collisionScene(scene, removed, activated);
  auto &v = vehicles.at(riding);
  int steps = std::max(1, int(std::ceil(std::max(std::abs(delta.x), std::abs(delta.y)) / 3)));
  Vec2 at = from;
  for (int i = 0; i < steps; i++) {
    Vec2 next{at.x + delta.x / steps, at.y};
    if (rideAllowed(collision, next, v))
      at = next;
    next = {at.x, at.y + delta.y / steps};
    if (rideAllowed(collision, next, v))
      at = next;
  }
  return at;
}
void StudioGame::drive(Vec2 direction, float top, float dt) {
  auto &v = vehicles.at(riding);
  bool going = direction.x != 0 || direction.y != 0;
  Vec2 target{direction.x * top, direction.y * top};
  Vec2 gap{target.x - rideVelocity.x, target.y - rideVelocity.y};
  float seconds = going ? v.acceleration : v.braking, distance = std::hypot(gap.x, gap.y),
        change = seconds > 0 ? top / seconds * dt : distance;
  if (distance <= change)
    rideVelocity = target;
  else
    rideVelocity = {rideVelocity.x + gap.x / distance * change,
                    rideVelocity.y + gap.y / distance * change};
  Vec2 step{rideVelocity.x * dt, rideVelocity.y * dt};
  auto at = ride(feet, step);
  // Running into something stops the vehicle in that direction.
  if (std::abs(at.x - feet.x) < std::abs(step.x) * .5f)
    rideVelocity.x = 0;
  if (std::abs(at.y - feet.y) < std::abs(step.y) * .5f)
    rideVelocity.y = 0;
  feet = at;
}
bool StudioGame::board(const std::string &key) {
  if (!riding.empty() || !vehicles.count(key) || mapOf(key) != scene)
    return false;
  auto s = liveScene(scene);
  auto o = findObject(s, idOf(key));
  if (!o)
    return false;
  auto source = project.sprites.at(o->sprite).source;
  auto collision = collisionScene(scene, removed, activated);
  auto &v = vehicles.at(key);
  for (Vec2 seat : {Vec2{o->position.x + source.w / 2, o->position.y + source.h - 2},
                    Vec2{o->position.x + source.w / 2, o->position.y + source.h / 2 + 4}})
    if (rideAllowed(collision, seat, v)) {
      riding = key;
      rideVelocity = {};
      feet = seat;
      movedObjects.erase(key);
      clock = 0;
      pushTarget.clear();
      toast = controlLabel(Action::Interact) + " TO GET OFF";
      toastTime = 2;
      progressDirty = true;
      return true;
    }
  toast = "There's no room to get on.";
  toastTime = 2;
  return false;
}
bool StudioGame::leaveVehicle() {
  if (riding.empty())
    return false;
  auto collision = collisionScene(scene, removed, activated);
  auto &water = layers.at(scene).water;
  auto dry = [&](Vec2 at) {
    auto r = body(at);
    if (blocked(project, collision, r))
      return false;
    for (int y = int(r.y) / 16; y <= int(r.y + r.h - .001f) / 16; y++)
      for (int x = int(r.x) / 16; x <= int(r.x + r.w - .001f) / 16; x++)
        if (water[size_t(y * collision.width + x)])
          return false;
    return true;
  };
  Vec2 facing = direction == "north"  ? Vec2{0, -1}
                : direction == "east" ? Vec2{1, 0}
                : direction == "west" ? Vec2{-1, 0}
                                      : Vec2{0, 1};
  std::vector<Vec2> ways{facing, {0, 1}, {0, -1}, {1, 0}, {-1, 0}};
  for (float distance : {14.f, 20.f, 26.f})
    for (auto way : ways) {
      Vec2 spot{feet.x + way.x * distance, feet.y + way.y * distance};
      if (!dry(spot))
        continue;
      auto o = findObject(project.scenes.at(scene), idOf(riding));
      auto source = project.sprites.at(o->sprite).source;
      auto &s = project.scenes.at(scene);
      movedObjects[riding] = {
          std::clamp(feet.x - source.w / 2, 0.f, float(s.width * 16) - source.w),
          std::clamp(feet.y - source.h + 2, 0.f, float(s.height * 16) - source.h)};
      riding.clear();
      rideVelocity = {};
      feet = spot;
      clock = 0;
      progressDirty = true;
      return true;
    }
  toast = vehicles.at(riding).water ? "Steer next to the shore to get off."
                                    : "There's no room to get off here.";
  toastTime = 2;
  return false;
}

// ---- Leveling -------------------------------------------------------------------------------

int StudioGame::xpForLevel(int target) const {
  double total = 0;
  for (int k = 1; k < target; k++)
    total += std::round(leveling.firstLevel * std::pow(double(leveling.growth), k - 1));
  return int(std::min(total, 9999999.0));
}
int StudioGame::levelFor(int experience) const {
  int l = 1;
  while (l < leveling.maxLevel && experience >= xpForLevel(l + 1))
    l++;
  return l;
}
int StudioGame::levelBonus(int perLevel, int atLevel) const {
  if (!leveling.enabled)
    return 0;
  return perLevel * ((atLevel > 0 ? atLevel : level) - 1);
}
void StudioGame::gainXp(int amount) {
  if (!leveling.enabled || amount <= 0)
    return;
  xp = std::min(xpForLevel(leveling.maxLevel), xp + amount);
  int next = levelFor(xp);
  if (next > level) {
    level = next;
    health = playerMaxHealth();
    toast = "LEVEL " + std::to_string(level) + "!";
    toastTime = 3;
    cue("chest");
  } else {
    // Add to a message already showing, such as "Quest completed", rather than replace it.
    auto gained = "+" + std::to_string(amount) + " XP";
    toast = toastTime > 0 && !toast.empty() && toast.size() + gained.size() < 60
                ? toast + "  " + gained
                : gained;
    toastTime = std::max(toastTime, 1.5f);
  }
  progressDirty = true;
}

// ---- Cutscenes ------------------------------------------------------------------------------

void StudioGame::startCutscene(const std::string &id) {
  auto c = cutscenes.find(id);
  if (!cutscene.empty() || c == cutscenes.end() || c->second.scene != scene)
    return;
  cutscene = id;
  cutsceneStep = 0;
  cutsceneClock = 0;
  fadeFrom = fade;
  cameraNow = cameraFrom = feet;
  cameraOverride = false;
  dialogueObject.clear();
  shopObject.clear();
  inventoryOpen = journalOpen = false;
  clock = 0;
  dialogueTime = 0;
}
bool StudioGame::applyCutsceneStep(const StudioCutsceneStep &step, float dt, bool instant) {
  cutsceneClock += dt;
  auto progress = [&](float seconds) {
    return instant || seconds <= 0 ? 1.f : std::min(1.f, cutsceneClock / seconds);
  };
  if (step.type == "wait")
    return instant || cutsceneClock >= step.seconds;
  if (step.type == "say")
    return instant;
  if (step.type == "fade") {
    float t = progress(step.seconds);
    fade = fadeFrom + ((step.dark ? 1.f : 0.f) - fadeFrom) * t;
    return t >= 1;
  }
  if (step.type == "camera") {
    float t = smooth(progress(step.seconds));
    Vec2 target = step.reset ? feet : step.point;
    cameraOverride = true;
    cameraNow = {cameraFrom.x + (target.x - cameraFrom.x) * t,
                 cameraFrom.y + (target.y - cameraFrom.y) * t};
    if (t >= 1 && step.reset)
      cameraOverride = false;
    return t >= 1;
  }
  if (step.type == "move") {
    auto face = [](Vec2 d) {
      return std::abs(d.x) > std::abs(d.y) ? (d.x > 0 ? "east" : "west")
                                           : (d.y > 0 ? "south" : "north");
    };
    auto approach = [&](Vec2 &at) {
      Vec2 d{step.point.x - at.x, step.point.y - at.y};
      float length = std::hypot(d.x, d.y), travel = step.speed * dt;
      if (instant || length <= travel) {
        at = step.point;
        return true;
      }
      at.x += d.x / length * travel;
      at.y += d.y / length * travel;
      return false;
    };
    if (step.actor == "player") {
      Vec2 d{step.point.x - feet.x, step.point.y - feet.y};
      if (d.x != 0 || d.y != 0)
        direction = face(d);
      bool done = approach(feet);
      clock = done ? 0 : clock + dt;
      return done;
    }
    auto key = scene + ":" + step.actor;
    auto state = actorStates.find(key);
    if (state != actorStates.end()) {
      Vec2 d{step.point.x - state->second.position.x, step.point.y - state->second.position.y};
      if (d.x != 0 || d.y != 0)
        state->second.facing = face(d);
      state->second.animationClock += dt;
      state->second.route.clear();
      return approach(state->second.position);
    }
    auto o = findObject(liveScene(scene), step.actor);
    Vec2 at = o ? o->position : step.point;
    bool done = approach(at);
    movedObjects[key] = at;
    return done;
  }
  if (step.type == "face") {
    if (step.actor == "player")
      direction = step.direction;
    else if (actorStates.count(scene + ":" + step.actor))
      actorStates.at(scene + ":" + step.actor).facing = step.direction;
    return true;
  }
  if (step.type == "sound") {
    if (!instant && soundEvents.size() < 32)
      soundEvents.push_back(step.target);
    return true;
  }
  if (step.type == "flag")
    flags[step.target] = step.value;
  return true;
}
bool StudioGame::updateCutscene(const Input &in, float dt) {
  if (cutscene.empty())
    return false;
  if (in.hit(Action::Quit)) {
    finishCutscene();
    return true;
  }
  auto &steps = cutscenes.at(cutscene).steps;
  for (int guard = 0; guard < 64 && cutsceneStep < steps.size(); guard++) {
    auto &step = steps[cutsceneStep];
    bool done = step.type == "say" ? (dialogueTime += dt, in.hit(Action::Interact))
                                   : applyCutsceneStep(step, dt, false);
    if (!done)
      break;
    cutsceneStep++;
    cutsceneClock = 0;
    dialogueTime = 0;
    fadeFrom = fade;
    cameraFrom = cameraOverride ? cameraNow : feet;
    dt = 0; // later steps start on this frame but don't run its time twice
    if (step.type == "say")
      break;
  }
  if (cutsceneStep >= steps.size()) {
    cutscene.clear();
    fade = 0;
    cameraOverride = false;
    clock = 0;
    progressDirty = true;
  }
  return true;
}
void StudioGame::finishCutscene() {
  if (cutscene.empty())
    return;
  auto &steps = cutscenes.at(cutscene).steps;
  for (; cutsceneStep < steps.size(); cutsceneStep++) {
    cutsceneClock = 0;
    fadeFrom = fade;
    cameraFrom = cameraOverride ? cameraNow : feet;
    applyCutsceneStep(steps[cutsceneStep], 0, true);
  }
  cutscene.clear();
  fade = 0;
  cameraOverride = false;
  clock = 0;
  progressDirty = true;
}
void StudioGame::drawCutscene(Canvas &c) const {
  if (fade > 0)
    c.rect({0, 0, float(Width), float(Height)},
           {0, 0, 0, uint8_t(std::clamp(fade, 0.f, 1.f) * 255)});
  if (cutscene.empty())
    return;
  c.rect({0, 0, float(Width), 14}, {0, 0, 0, 255});
  c.rect({0, float(Height) - 14, float(Width), 14}, {0, 0, 0, 255});
  auto &steps = cutscenes.at(cutscene).steps;
  if (cutsceneStep < steps.size() && steps[cutsceneStep].type == "say") {
    auto &step = steps[cutsceneStep];
    c.rect({6, 148, 388, 72}, uiModal);
    int y = 155;
    if (!step.speaker.empty()) {
      c.text(clipText(step.speaker, 50), 12, y, uiAccent);
      y += 12;
    }
    auto lines = wrapText(step.text, 60);
    for (size_t i = 0; i < std::min(size_t(step.speaker.empty() ? 5 : 4), lines.size()); i++)
      c.text(lines[i], 12, y + int(i) * 9, uiText);
    c.text(controlLabel(Action::Interact) + " NEXT  " + controlLabel(Action::Pause) + " SKIP", 12,
           209, uiMuted);
  } else
    c.text(controlLabel(Action::Pause) + " SKIP", 12, int(Height) - 10, uiMuted);
}

// ---- Saves ----------------------------------------------------------------------------------

void StudioGame::writeWorld(std::ostream &out) const {
  out << std::setprecision(9) << hour << "\n" << weatherOverride.size() << "\n";
  for (auto &w : weatherOverride)
    out << std::quoted(w.first) << " " << std::quoted(w.second) << "\n";
  out << movedObjects.size() << "\n";
  for (auto &m : movedObjects)
    out << std::quoted(m.first) << " " << m.second.x << " " << m.second.y << "\n";
  out << latchedPlates.size() << "\n";
  for (auto &p : latchedPlates)
    out << std::quoted(p) << "\n";
  out << plateDoors.size() << "\n";
  for (auto &d : plateDoors)
    out << std::quoted(d) << "\n";
  out << std::quoted(riding) << " " << xp << "\n";
}
StudioWorldState StudioGame::readWorld(std::istream &in) const {
  StudioWorldState w;
  auto count = [&](int max) {
    int n = -1;
    in >> n;
    require(in && n >= 0 && n <= max, "Invalid saved world count");
    return n;
  };
  in >> w.hour;
  require(in && std::isfinite(w.hour) && w.hour >= 0 && w.hour < 24, "Invalid saved time of day");
  for (int i = 0, n = count(512); i < n; i++) {
    std::string map, weather;
    in >> std::quoted(map) >> std::quoted(weather);
    require(in && project.scenes.count(map) && validWeather(weather) &&
                w.weather.emplace(map, weather).second,
            "Invalid saved weather");
  }
  for (int i = 0, n = count(65536); i < n; i++) {
    std::string key;
    Vec2 at;
    in >> std::quoted(key) >> at.x >> at.y;
    require(in && key.find(':') != std::string::npos && project.scenes.count(mapOf(key)),
            "Invalid saved object position");
    auto &s = project.scenes.at(mapOf(key));
    require(findObject(s, idOf(key)) && !actors.count(key) && std::isfinite(at.x) &&
                std::isfinite(at.y) && at.x >= 0 && at.y >= 0 && at.x < s.width * 16 &&
                at.y < s.height * 16 && w.moved.emplace(key, at).second,
            "Invalid saved object position");
  }
  for (int i = 0, n = count(4096); i < n; i++) {
    std::string key;
    in >> std::quoted(key);
    require(in && plates.count(key) && plates.at(key).sticky && w.latched.insert(key).second,
            "Invalid saved pressure plate");
  }
  for (int i = 0, n = count(4096); i < n; i++) {
    std::string key;
    in >> std::quoted(key);
    bool targeted = std::any_of(plates.begin(), plates.end(), [&](auto &p) {
      return mapOf(p.first) == mapOf(key) &&
             std::find(p.second.targets.begin(), p.second.targets.end(), idOf(key)) !=
                 p.second.targets.end();
    });
    require(in && targeted && w.plateDoors.insert(key).second, "Invalid saved plate door");
  }
  in >> std::quoted(w.riding) >> w.xp;
  require(in && (w.riding.empty() || vehicles.count(w.riding)) && w.xp >= 0 &&
              w.xp <= (leveling.enabled ? xpForLevel(leveling.maxLevel) : 0),
          "Invalid saved vehicle or XP");
  return w;
}
void StudioGame::restoreWorld(const StudioWorldState &w) {
  hour = w.hour;
  weatherOverride = w.weather;
  movedObjects = w.moved;
  latchedPlates = w.latched;
  plateDoors = w.plateDoors;
  riding = w.riding;
  rideVelocity = {};
  xp = w.xp;
  level = leveling.enabled ? levelFor(xp) : 1;
  pressedPlates.clear();
  pushTarget.clear();
  pushClock = 0;
  cutscene.clear();
  fade = fadeFrom = 0;
  cameraOverride = false;
}
} // namespace lota
