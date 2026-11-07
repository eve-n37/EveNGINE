// EveNGINE 1.1 adventure expansion: equipment, ranged combat, factions, actor animations and
// timed events. Everything here is optional project data, so 1.0 projects load unchanged.
#include "lota/studio.hpp"
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
const char *directions[] = {"south", "north", "east", "west"};
float distance(Vec2 a, Vec2 b) {
  return std::hypot(a.x - b.x, a.y - b.y);
}
bool inside(Vec2 p, Rect r) {
  return p.x >= r.x && p.y >= r.y && p.x < r.x + r.w && p.y < r.y + r.h;
}
} // namespace

void StudioGame::loadExpansion(const Json &j) {
  if (j.has("factions")) {
    auto &list = j["factions"];
    require(list.type == Json::Array && list.array.size() <= 16, "Use at most 16 factions");
    for (auto &f : list.array) {
      require(f.type == Json::Object, "Invalid faction");
      auto name = ident(f["id"], "faction ID");
      StudioFaction faction;
      require(f["name"].type == Json::String && !f["name"].string.empty() &&
                  f["name"].string.size() <= 160,
              "Invalid faction name");
      faction.name = f["name"].string;
      if (f.has("player")) {
        auto attitude = f["player"].text();
        require(attitude == "hostile" || attitude == "neutral", "Invalid faction attitude");
        faction.hostileToPlayer = attitude == "hostile";
      }
      require(factions.emplace(name, faction).second, "Duplicate faction");
    }
    for (auto &f : list.array)
      if (f.has("hostile")) {
        require(f["hostile"].type == Json::Array && f["hostile"].array.size() <= 16,
                "Invalid faction hostility");
        auto self = f["id"].text();
        for (auto &other : f["hostile"].array) {
          auto name = ident(other, "hostile faction");
          require(factions.count(name) && name != self, "Unknown or self-hostile faction");
          factions.at(self).hostile.insert(name);
        }
      }
  }
  for (auto &item : j["items"].array)
    if (item.has("equip")) {
      auto &e = item["equip"];
      auto name = item["id"].text();
      require(e.type == Json::Object, "Invalid equipment in " + name);
      require(combatEnabled, "Equipment needs player combat settings");
      require(!item.has("heal"), "An item cannot be both healing and equipment");
      StudioEquipment gear;
      gear.slot = e["slot"].text();
      require(gear.slot == "weapon" || gear.slot == "armor" || gear.slot == "charm",
              "Unknown equipment slot");
      if (e.has("attack"))
        gear.attack = integer(e["attack"], 0, 99, "equipment attack");
      if (e.has("defense"))
        gear.defense = integer(e["defense"], 0, 99, "equipment defense");
      if (e.has("health"))
        gear.health = integer(e["health"], 0, 99, "equipment health");
      if (e.has("ranged")) {
        require(gear.slot == "weapon", "Only weapons can shoot");
        auto &r = e["ranged"];
        require(r.type == Json::Object, "Invalid ranged weapon");
        gear.ranged = true;
        gear.projectileSpeed = real(r["speed"], 40, 400, "projectile speed");
        gear.projectileRange = real(r["range"], 16, 320, "projectile range");
        if (r.has("ammo") && !r["ammo"].text().empty()) {
          gear.ammo = ident(r["ammo"], "ammunition");
          require(items.count(gear.ammo) && gear.ammo != name, "Unknown ammunition item");
        }
      }
      equipment[name] = gear;
    }
  for (auto &entry : equipment)
    require(!equipment.count(entry.second.ammo), "Ammunition cannot be equipment");
}

void StudioGame::loadActorExtras(StudioActor &actor, const Json &o) {
  if (actor.mode == "enemy") {
    auto &c = o["combat"];
    if (c.has("style")) {
      auto style = c["style"].text();
      require(style == "melee" || style == "ranged", "Unknown enemy attack style");
      actor.ranged = style == "ranged";
    }
    if (actor.ranged) {
      actor.range = real(c["range"], 8, 256, "ranged enemy reach");
      actor.projectileSpeed = real(c["projectileSpeed"], 40, 400, "enemy projectile speed");
    }
    if (c.has("faction") && !c["faction"].text().empty()) {
      actor.faction = ident(c["faction"], "enemy faction");
      require(factions.count(actor.faction), "Unknown enemy faction");
    }
  }
  if (!o.has("animations"))
    return;
  auto &a = o["animations"];
  require(a.type == Json::Object, "Invalid actor animations");
  for (auto &entry : a.object)
    require(entry.first == "ms" || entry.first == "walk" || entry.first == "attack",
            "Unknown actor animation field");
  if (a.has("ms"))
    actor.animationMs = integer(a["ms"], 16, 5000, "actor frame duration");
  bool any = false;
  for (auto state : {"walk", "attack"}) {
    if (!a.has(state))
      continue;
    require(std::string(state) == "walk" || actor.mode == "enemy",
            "Only enemies have attack animations");
    auto &set = a[state];
    require(set.type == Json::Object, "Invalid actor animation set");
    for (auto &entry : set.object)
      require(std::find(std::begin(directions), std::end(directions), entry.first) !=
                  std::end(directions),
              "Unknown animation direction");
    for (auto d : directions)
      if (set.has(d)) {
        auto &frames = set[d];
        require(frames.type == Json::Array && !frames.array.empty() && frames.array.size() <= 16,
                "Actor animations need 1-16 frames");
        auto &out = std::string(state) == "walk" ? actor.walkFrames[d] : actor.attackFrames[d];
        for (auto &f : frames.array) {
          auto sprite = ident(f, "animation sprite");
          require(project.sprites.count(sprite), "Unknown animation sprite");
          out.push_back(project.sprites.at(sprite).source);
        }
        any = true;
      }
  }
  require(any, "Actor animations need frames");
}

int StudioGame::maxHealthWith(const std::map<std::string, std::string> &gear) const {
  int total = maxHealth;
  for (auto &slot : gear)
    if (equipment.count(slot.second))
      total += equipment.at(slot.second).health;
  return total;
}
int StudioGame::playerMaxHealth() const {
  return maxHealthWith(equipped);
}
int StudioGame::playerAttackDamage() const {
  int total = attackDamage;
  for (auto &slot : equipped)
    total += equipment.at(slot.second).attack;
  return total;
}
int StudioGame::playerDefense() const {
  int total = 0;
  for (auto &slot : equipped)
    total += equipment.at(slot.second).defense;
  return total;
}
const StudioEquipment *StudioGame::weapon() const {
  auto w = equipped.find("weapon");
  return w == equipped.end() ? nullptr : &equipment.at(w->second);
}
bool StudioGame::equip(const std::string &item) {
  auto gear = equipment.find(item);
  if (gear == equipment.end() || !inventory.count(item) || inventory.at(item) < 1)
    return false;
  auto &slot = gear->second.slot;
  if (equipped.count(slot) && equipped.at(slot) == item) {
    equipped.erase(slot);
    toast = "Unequipped " + items.at(item).name;
  } else {
    equipped[slot] = item;
    toast = "Equipped " + items.at(item).name;
  }
  health = std::min(health, playerMaxHealth());
  toastTime = 2;
  progressDirty = true;
  return true;
}
void StudioGame::reconcileEquipment() {
  for (auto i = equipped.begin(); i != equipped.end();)
    if (!inventory.count(i->second) || inventory.at(i->second) < 1)
      i = equipped.erase(i);
    else
      ++i;
  health = std::min(health, playerMaxHealth());
}
void StudioGame::hurtPlayer(int damage) {
  if (health == 0 || hurtClock > 0)
    return;
  health = std::max(0, health - std::max(1, damage - playerDefense()));
  hurtClock = .6f;
  progressDirty = true;
  toast = "You took damage.";
  toastTime = 1;
  if (health == 0) {
    shopObject.clear();
    dialogueObject.clear();
    projectiles.clear();
    if (frontendStarted) {
      frontend = "defeat";
      menuIndex = 0;
    }
  }
}
bool StudioGame::hostileToPlayer(const std::string &key) const {
  auto &faction = actors.at(key).faction;
  if (faction.empty())
    return true;
  return factions.at(faction).hostileToPlayer || actorStates.at(key).provoked;
}
bool StudioGame::hostileActors(const std::string &a, const std::string &b) const {
  auto &fa = actors.at(a).faction, &fb = actors.at(b).faction;
  if (a == b || fa.empty() || fb.empty() || fa == fb)
    return false;
  return factions.at(fa).hostile.count(fb) || factions.at(fb).hostile.count(fa);
}
void StudioGame::damageActor(const std::string &key, int damage, bool byPlayer) {
  auto &state = actorStates.at(key);
  if (state.health == 0)
    return;
  state.health = std::max(0, state.health - damage);
  state.timer = .18f;
  state.flash = .3f;
  if (byPlayer)
    state.provoked = true;
  progressDirty = true;
  if (state.health == 0) {
    state.route.clear();
    state.attackClock = -1;
    if (byPlayer)
      claimLoot(key);
  } else if (byPlayer) {
    toast = "Hit!";
    toastTime = .5f;
  }
}
void StudioGame::fireProjectile(Vec2 from, Vec2 direction, float speed, float range, int damage,
                                const std::string &owner) {
  if (projectiles.size() >= 64)
    projectiles.erase(projectiles.begin());
  StudioProjectile p;
  p.position = from;
  p.velocity = {direction.x * speed, direction.y * speed};
  p.remaining = range;
  p.damage = damage;
  p.owner = owner;
  p.map = scene;
  projectiles.push_back(p);
}
void StudioGame::updateProjectiles(float dt) {
  auto walls = collisionScene(scene, removed, activated);
  walls.objects.erase(std::remove_if(walls.objects.begin(), walls.objects.end(),
                                     [&](auto &o) { return actors.count(scene + ":" + o.id); }),
                      walls.objects.end());
  auto bounds = [&](const std::string &key) {
    auto &a = actors.at(key);
    auto &objects = project.scenes.at(a.map).objects;
    auto o = std::find_if(objects.begin(), objects.end(), [&](auto &o) { return o.id == a.id; });
    auto source = project.sprites.at(o->sprite).source;
    auto at = actorStates.at(key).position;
    return Rect{at.x, at.y, source.w, source.h};
  };
  for (size_t i = 0; i < projectiles.size();) {
    auto &p = projectiles[i];
    bool spent = p.map != scene;
    float speed = std::hypot(p.velocity.x, p.velocity.y);
    float travel = std::min(p.remaining, speed * dt);
    int steps = std::max(1, int(std::ceil(travel / 4)));
    for (int s = 0; s < steps && !spent; s++) {
      float part = travel / steps;
      p.position.x += p.velocity.x / speed * part;
      p.position.y += p.velocity.y / speed * part;
      p.remaining -= part;
      if (blocked(project, walls, {p.position.x - 1, p.position.y - 1, 2, 2})) {
        spent = true;
        break;
      }
      if (p.owner != "player" && actors.count(p.owner) && hostileToPlayer(p.owner) &&
          inside(p.position, {feet.x - 6, feet.y - 20, 12, 20})) {
        hurtPlayer(actors.at(p.owner).damage);
        if (health == 0)
          return; // defeat clears the projectile list
        spent = true;
        break;
      }
      for (auto &a : actors) {
        if (a.second.map != scene || a.second.mode != "enemy" || a.first == p.owner ||
            actorStates.at(a.first).health == 0)
          continue;
        bool target =
            p.owner == "player" || (actors.count(p.owner) && hostileActors(p.owner, a.first));
        if (target && inside(p.position, bounds(a.first))) {
          damageActor(a.first, p.damage, p.owner == "player");
          spent = true;
          break;
        }
      }
    }
    if (spent || p.remaining <= .01f)
      projectiles.erase(projectiles.begin() + long(i));
    else
      ++i;
    if (combatEnabled && health == 0)
      return;
  }
}
void StudioGame::updateTimers(float dt) {
  for (auto &e : events) {
    if (e.trigger != "timer" || e.scene != scene)
      continue;
    auto &t = eventTimers[e.id];
    t += dt;
    if (t < e.delay || (e.once && firedEvents.count(e.id)))
      continue;
    auto previous = scene;
    bool fired = fireEvent(e);
    if (fired || !e.once)
      t = 0;
    if (scene != previous || !dialogueObject.empty())
      return;
  }
}
bool StudioGame::actorFrame(const std::string &key, Rect &frame) const {
  auto a = actors.find(key);
  if (a == actors.end())
    return false;
  auto &state = actorStates.at(key);
  auto pick = [&](const std::map<std::string, std::vector<Rect>> &set, float clock,
                  bool loop) -> bool {
    auto f = set.find(state.facing);
    if (f == set.end())
      return false;
    int ms = int(clock * 1000), index = ms / a->second.animationMs, count = int(f->second.size());
    frame = f->second[size_t(loop ? index % count : std::min(index, count - 1))];
    return true;
  };
  if (state.attackClock >= 0 && pick(a->second.attackFrames, state.attackClock, false))
    return true;
  return pick(a->second.walkFrames, state.animationClock, true);
}
} // namespace lota
