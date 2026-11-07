#include "lota/studio.hpp"
#include <stdexcept>
#include <queue>
#include <cstdlib>
#include <iomanip>
#include <istream>
#include <ostream>
namespace lota {
namespace {
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
int integer(const Json &j, int lo, int hi) {
  require(j.type == Json::Number && std::floor(j.number) == j.number && j.number >= lo &&
              j.number <= hi,
          "Invalid adventure number");
  return int(j.number);
}
float real(const Json &j, float lo, float hi) {
  require(j.type == Json::Number && std::isfinite(j.number) && j.number >= lo && j.number <= hi,
          "Invalid adventure value");
  return float(j.number);
}
void array(const Json &j, size_t max) {
  require(j.type == Json::Array && j.array.size() <= max, "Invalid adventure list");
}
float distance(Vec2 a, Vec2 b) {
  return std::hypot(a.x - b.x, a.y - b.y);
}
bool clearLine(const Project &p, const Scene &s, Vec2 from, Vec2 to) {
  int steps = std::max(1, int(distance(from, to) / 4));
  for (int i = 1; i < steps; i++) {
    float f = float(i) / steps;
    Vec2 at{from.x + (to.x - from.x) * f, from.y + (to.y - from.y) * f};
    if (blocked(p, s, {at.x - 1, at.y - 1, 2, 2}))
      return false;
  }
  return true;
}
std::vector<Vec2> route(const Project &p, Scene s, Vec2 from, Vec2 to) {
  int sx = int(from.x / 16), sy = int(from.y / 16), tx = int(to.x / 16), ty = int(to.y / 16);
  if (sx < 0 || sy < 0 || tx < 0 || ty < 0 || sx >= s.width || tx >= s.width || sy >= s.height ||
      ty >= s.height)
    return {};
  int start = sy * s.width + sx, target = ty * s.width + tx;
  std::vector<int> previous(size_t(s.width * s.height), -1);
  std::queue<int> queue;
  queue.push(start);
  previous[start] = start;
  while (!queue.empty() && previous[target] < 0) {
    int at = queue.front();
    queue.pop();
    for (auto step : {Vec2{1, 0}, Vec2{-1, 0}, Vec2{0, 1}, Vec2{0, -1}}) {
      int x = at % s.width + int(step.x), y = at / s.width + int(step.y);
      if (x < 0 || y < 0 || x >= s.width || y >= s.height)
        continue;
      int next = y * s.width + x;
      if (previous[next] >= 0 ||
          blocked(p, s, body(next == target ? to : Vec2{float(x * 16 + 8), float(y * 16 + 12)})))
        continue;
      previous[next] = at;
      queue.push(next);
    }
  }
  if (previous[target] < 0)
    return {};
  std::vector<Vec2> out{to};
  for (int at = target; at != start; at = previous[at])
    if (at != target)
      out.push_back({float((at % s.width) * 16 + 8), float((at / s.width) * 16 + 12)});
  std::reverse(out.begin(), out.end());
  return out;
}
} // namespace
bool StudioGame::hasAdventure() const {
  return combatEnabled || !actors.empty() || !shops.empty() || !healing.empty() ||
         !equipment.empty();
}
void StudioGame::loadAdventure(const Json &j) {
  if (j.has("combat")) {
    auto &c = j["combat"];
    require(c.type == Json::Object, "Invalid player combat settings");
    combatEnabled = true;
    maxHealth = integer(c["health"], 1, 999);
    attackDamage = integer(c["damage"], 1, 999);
    attackRange = real(c["range"], 8, 96);
    attackDelay = real(c["cooldown"], .1f, 5);
  }
  for (auto &item : j["items"].array)
    if (item.has("heal"))
      healing[item["id"].text()] = integer(item["heal"], 1, 999);
  loadExpansion(j);
  for (auto &map : j["scenes"].array)
    for (auto &object : map["objects"].array) {
      std::string key = map["id"].text() + ":" + object["id"].text();
      auto &source = project.sprites.at(object["sprite"].text()).source;
      auto &scene = project.scenes.at(map["id"].text());
      if (object.has("movement") || object["kind"].text() == "enemy") {
        StudioActor actor;
        actor.map = map["id"].text();
        actor.id = object["id"].text();
        actor.mode = object["kind"].text() == "enemy" ? "enemy" : "patrol";
        if (actor.mode == "enemy") {
          require(!object.has("movement"), "Enemies use combat pursuit");
          require(combatEnabled, "Enemies need player combat settings");
          auto &c = object["combat"];
          require(c.type == Json::Object, "Enemy needs combat settings");
          actor.maxHealth = integer(c["health"], 1, 999);
          actor.damage = integer(c["damage"], 1, 999);
          actor.speed = real(c["speed"], 1, 128);
          actor.range = real(c["range"], 8, c["style"].text() == "ranged" ? 256 : 64);
          actor.aggro = real(c["aggro"], 16, 512);
          actor.cooldown = real(c["cooldown"], .1f, 10);
          if (c.has("rewardItem")) {
            require(c["rewardItem"].type == Json::String && c["rewardItem"].string.size() <= 64,
                    "Invalid enemy reward");
            actor.rewardItem = c["rewardItem"].text();
            require(actor.rewardItem.empty() || items.count(actor.rewardItem),
                    "Missing enemy reward item");
          }
          if (c.has("rewardQuantity"))
            actor.rewardQuantity = integer(c["rewardQuantity"], 1, 99);
        } else {
          require(object["kind"].text() == "npc", "Only NPCs can patrol");
          auto &m = object["movement"];
          require(m.type == Json::Object, "Invalid NPC movement");
          actor.mode = m["mode"].text();
          require(actor.mode == "patrol" || actor.mode == "wander", "Invalid NPC movement mode");
          actor.speed = real(m["speed"], 1, 128);
          actor.wait = real(m["wait"], 0, 30);
          array(m["points"], 32);
          require(!m["points"].array.empty(), "NPC movement needs waypoints");
          for (auto &point : m["points"].array) {
            Vec2 top{real(point["x"], 0, scene.width * 16 - source.w),
                     real(point["y"], 0, scene.height * 16 - source.h)};
            Scene collision = scene;
            collision.objects.erase(std::remove_if(collision.objects.begin(),
                                                   collision.objects.end(),
                                                   [&](auto &o) { return o.id == actor.id; }),
                                    collision.objects.end());
            require(!blocked(project, collision, body({top.x + source.w / 2, top.y + source.h})),
                    "NPC waypoint is blocked");
            actor.points.push_back(top);
          }
        }
        loadActorExtras(actor, object);
        require(actors.size() < 128, "Use at most 128 moving NPCs and enemies");
        actors[key] = actor;
      } else
        require(!object.has("animations"), "Only moving NPCs and enemies can animate");
      if (object.has("shop")) {
        require(object["kind"].text() == "npc", "Only NPCs can trade");
        auto &s = object["shop"];
        StudioShop shop;
        require(s.type == Json::Object, "Invalid merchant");
        shop.currency = s["currency"].text();
        require(items.count(shop.currency), "Missing shop currency");
        array(s["offers"], 32);
        require(!s["offers"].array.empty(), "Merchant needs offers");
        std::set<std::string> used;
        for (auto &offer : s["offers"].array) {
          StudioOffer o;
          o.item = offer["item"].text();
          require(items.count(o.item) && o.item != shop.currency && used.insert(o.item).second,
                  "Invalid or duplicate shop item");
          o.buy = integer(offer["buy"], 0, 9999);
          o.sell = integer(offer["sell"], 0, 9999);
          require(o.sell <= o.buy, "Sell price exceeds buy price");
          o.stock = integer(offer["stock"], -1, 999);
          shop.offers.push_back(o);
        }
        shops[key] = shop;
      }
    }
  resetAdventure();
}
void StudioGame::resetAdventure() {
  equipped.clear();
  projectiles.clear();
  health = maxHealth;
  combatClock = hurtClock = 0;
  shopObject.clear();
  bagRow = 0;
  actorStates.clear();
  shopStock.clear();
  for (auto &a : actors) {
    auto &objects = project.scenes.at(a.second.map).objects;
    auto o =
        std::find_if(objects.begin(), objects.end(), [&](auto &o) { return o.id == a.second.id; });
    actorStates[a.first] = {o->position, a.second.maxHealth, 0, false};
  }
  for (auto &s : shops)
    for (auto &o : s.second.offers)
      shopStock[s.first].push_back(o.stock);
}
Scene StudioGame::liveScene(const std::string &map) const {
  Scene s = project.scenes.at(map);
  for (auto &o : s.objects) {
    auto a = actorStates.find(map + ":" + o.id);
    if (a != actorStates.end())
      o.position = a->second.position;
  }
  s.objects.erase(std::remove_if(s.objects.begin(), s.objects.end(),
                                 [&](auto &o) {
                                   auto a = actorStates.find(map + ":" + o.id);
                                   return a != actorStates.end() && a->second.health == 0 &&
                                          a->second.lootClaimed;
                                 }),
                  s.objects.end());
  return s;
}
Vec2 StudioGame::actorFeet(const std::string &key) const {
  auto &a = actors.at(key);
  auto &objects = project.scenes.at(a.map).objects;
  auto o = std::find_if(objects.begin(), objects.end(), [&](auto &o) { return o.id == a.id; });
  auto source = project.sprites.at(o->sprite).source;
  auto at = actorStates.at(key).position;
  return {at.x + source.w / 2, at.y + source.h};
}
bool StudioGame::claimLoot(const std::string &key) {
  auto &state = actorStates.at(key);
  auto &actor = actors.at(key);
  if (state.health > 0 || state.lootClaimed)
    return false;
  if (!actor.rewardItem.empty() && !giveItem(actor.rewardItem, actor.rewardQuantity))
    return false;
  state.lootClaimed = true;
  progressDirty = true;
  toast = actor.rewardItem.empty() ? "Enemy defeated." : "Found " + items.at(actor.rewardItem).name;
  toastTime = 3;
  return true;
}
void StudioGame::playerAttack() {
  if (!combatEnabled || combatClock > 0 || health == 0)
    return;
  Vec2 facing = direction == "east"    ? Vec2{1, 0}
                : direction == "west"  ? Vec2{-1, 0}
                : direction == "north" ? Vec2{0, -1}
                                       : Vec2{0, 1};
  if (auto w = weapon(); w && w->ranged) {
    if (!w->ammo.empty()) {
      auto ammo = inventory.find(w->ammo);
      if (ammo == inventory.end() || ammo->second < 1) {
        toast = "Out of " + items.at(w->ammo).name + ".";
        toastTime = 2;
        combatClock = .25f;
        return;
      }
      if (--ammo->second == 0)
        inventory.erase(ammo);
    }
    combatClock = attackDelay;
    fireProjectile({feet.x, feet.y - 8}, facing, w->projectileSpeed, w->projectileRange,
                   playerAttackDamage(), "player");
    progressDirty = true;
    return;
  }
  combatClock = attackDelay;
  auto sight = collisionScene(scene, removed, activated);
  sight.objects.erase(std::remove_if(sight.objects.begin(), sight.objects.end(),
                                     [&](auto &o) { return actors.count(scene + ":" + o.id); }),
                      sight.objects.end());
  for (auto &a : actors)
    if (a.second.map == scene && a.second.mode == "enemy" && actorStates.at(a.first).health > 0) {
      auto target = actorFeet(a.first);
      auto vector = normalized({target.x - feet.x, target.y - feet.y});
      if (distance(target, feet) <= attackRange &&
          vector.x * facing.x + vector.y * facing.y >= .25f &&
          clearLine(project, sight, feet, target))
        damageActor(a.first, playerAttackDamage(), true);
    }
}
void StudioGame::updateActors(float dt) {
  auto face = [](StudioActorState &state, Vec2 d) {
    if (std::abs(d.x) < .001f && std::abs(d.y) < .001f)
      return;
    state.facing =
        std::abs(d.x) > std::abs(d.y) ? (d.x > 0 ? "east" : "west") : (d.y > 0 ? "south" : "north");
  };
  for (auto &entry : actors) {
    auto &actor = entry.second;
    auto &state = actorStates.at(entry.first);
    if (actor.map != scene)
      continue;
    state.flash = std::max(0.f, state.flash - dt);
    if (state.attackClock >= 0) {
      state.attackClock += dt;
      auto frames = actor.attackFrames.find(state.facing);
      float total = frames == actor.attackFrames.end()
                        ? 0
                        : frames->second.size() * actor.animationMs / 1000.f;
      if (state.attackClock >= total)
        state.attackClock = -1;
    }
    auto before = state.position;
    // Returns false when the player was defeated and the update must stop.
    auto step = [&]() -> bool {
      if (state.health == 0) {
        if (!state.lootClaimed && distance(actorFeet(entry.first), feet) < 32 &&
            (actor.rewardItem.empty() ||
             (inventory.count(actor.rewardItem) ? inventory.at(actor.rewardItem) : 0) +
                     actor.rewardQuantity <=
                 999))
          claimLoot(entry.first);
        return true;
      }
      state.timer = std::max(0.f, state.timer - dt);
      state.routeTimer = std::max(0.f, state.routeTimer - dt);
      auto from = actorFeet(entry.first);
      Vec2 destination = from;
      auto map = collisionScene(scene, removed, activated);
      map.objects.erase(std::remove_if(map.objects.begin(), map.objects.end(),
                                       [&](auto &o) {
                                         return o.id == actor.id ||
                                                actorStates.count(scene + ":" + o.id) &&
                                                    actorStates.at(scene + ":" + o.id).health == 0;
                                       }),
                        map.objects.end());
      if (actor.mode == "enemy") {
        std::string targetKey;
        Vec2 target{};
        bool found = false;
        float best = actor.aggro;
        if (hostileToPlayer(entry.first) && health > 0 && distance(from, feet) <= best) {
          target = feet;
          best = distance(from, feet);
          found = true;
        }
        for (auto &other : actors)
          if (other.second.map == scene && other.second.mode == "enemy" &&
              actorStates.at(other.first).health > 0 && hostileActors(entry.first, other.first)) {
            auto at = actorFeet(other.first);
            if (distance(from, at) < best) {
              best = distance(from, at);
              target = at;
              targetKey = other.first;
              found = true;
            }
          }
        if (!found) {
          state.route.clear();
          return true;
        }
        auto sight = map;
        sight.objects.erase(
            std::remove_if(sight.objects.begin(), sight.objects.end(),
                           [&](auto &o) { return actors.count(scene + ":" + o.id); }),
            sight.objects.end());
        if (distance(from, target) <= actor.range && clearLine(project, sight, from, target)) {
          state.route.clear();
          face(state, {target.x - from.x, target.y - from.y});
          if (state.timer == 0 && (!targetKey.empty() || hurtClock == 0)) {
            state.timer = actor.cooldown;
            state.attackClock = 0;
            if (actor.ranged)
              fireProjectile({from.x, from.y - 8},
                             normalized({target.x - from.x, target.y - from.y}),
                             actor.projectileSpeed, actor.range + 24, actor.damage, entry.first);
            else if (targetKey.empty()) {
              hurtPlayer(actor.damage);
              if (health == 0)
                return false;
            } else
              damageActor(targetKey, actor.damage, false);
          }
          return true;
        }
        destination = target;
      } else {
        if (state.timer > 0)
          return true;
        auto point = actor.points.at(size_t(state.waypoint));
        auto &objects = project.scenes.at(scene).objects;
        auto o =
            std::find_if(objects.begin(), objects.end(), [&](auto &o) { return o.id == actor.id; });
        auto source = project.sprites.at(o->sprite).source;
        destination = {point.x + source.w / 2, point.y + source.h};
        if (distance(from, destination) < 1) {
          state.waypoint = (state.waypoint + 1 +
                            (actor.mode == "wander" && actor.points.size() > 1
                                 ? std::rand() % int(actor.points.size() - 1)
                                 : 0)) %
                           int(actor.points.size());
          state.timer = actor.wait;
          state.route.clear();
          progressDirty = true;
          return true;
        }
      }
      if (state.route.empty() || state.routeTimer == 0) {
        state.route = route(project, map, from, destination);
        state.routeTimer = actor.mode == "enemy" ? .4f : 1.f;
      }
      if (state.route.empty())
        return true;
      auto next = state.route.front();
      float remaining = distance(from, next);
      if (remaining < 1) {
        state.route.erase(state.route.begin());
        return true;
      }
      auto d = normalized({next.x - from.x, next.y - from.y});
      float stride = std::min(remaining, actor.speed * dt);
      auto moved = moveBody(project, map, from, {d.x * stride, d.y * stride});
      if (distance(moved, feet) < 12 && actor.mode != "enemy")
        return true;
      if (distance(moved, from) > .001f) {
        state.position.x += moved.x - from.x;
        state.position.y += moved.y - from.y;
        progressDirty = true;
      } else {
        state.route.clear();
        state.routeTimer = .5f;
      }
      return true;
    };
    bool alive = step();
    Vec2 delta{state.position.x - before.x, state.position.y - before.y};
    if (std::abs(delta.x) > .001f || std::abs(delta.y) > .001f) {
      face(state, delta);
      state.animationClock += dt;
    } else
      state.animationClock = 0;
    if (!alive)
      return;
  }
}
bool StudioGame::trade(size_t row, bool sell, int quantity) {
  if (shopObject.empty() || !shops.count(shopObject) || quantity < 1 || quantity > 99)
    return false;
  auto &shop = shops.at(shopObject);
  if (row >= shop.offers.size())
    return false;
  auto &offer = shop.offers[row];
  auto next = inventory;
  int &stock = shopStock.at(shopObject).at(row);
  auto say = [&](const char *text) {
    toast = text;
    toastTime = 3;
    return false;
  };
  if (sell) {
    if (offer.sell <= 0)
      return say("This merchant does not buy that item.");
    if (next[offer.item] < quantity)
      return say("You do not have enough to sell.");
    if (next[shop.currency] + offer.sell * quantity > 999 || stock >= 0 && stock + quantity > 999)
      return say("The trade would exceed capacity.");
    next[offer.item] -= quantity;
    next[shop.currency] += offer.sell * quantity;
  } else {
    if (stock >= 0 && stock < quantity)
      return say("Sold out.");
    if (next[shop.currency] < offer.buy * quantity)
      return say("Not enough currency.");
    if (next[offer.item] + quantity > 999)
      return say("Make room in your bag first.");
    next[shop.currency] -= offer.buy * quantity;
    next[offer.item] += quantity;
  }
  for (auto i = next.begin(); i != next.end();)
    if (i->second == 0)
      i = next.erase(i);
    else
      ++i;
  inventory = std::move(next);
  if (stock >= 0)
    stock += sell ? quantity : -quantity;
  progressDirty = true;
  toast = sell ? "Item sold." : "Item purchased.";
  toastTime = 2;
  return true;
}
bool StudioGame::useItem(const std::string &item) {
  if (!combatEnabled || !healing.count(item) || !inventory.count(item) || inventory.at(item) < 1)
    return false;
  if (health >= playerMaxHealth()) {
    toast = "You are already at full health.";
    toastTime = 2;
    return false;
  }
  health = std::min(playerMaxHealth(), health + healing.at(item));
  if (--inventory[item] == 0)
    inventory.erase(item);
  bagRow = std::clamp(bagRow, 0, std::max(0, int(inventory.size()) - 1));
  progressDirty = true;
  toast = "Recovered health.";
  toastTime = 2;
  return true;
}
bool StudioGame::updateShop(const Input &in) {
  if (shopObject.empty())
    return false;
  if (in.hit(Action::Attack) || in.hit(Action::Quit) || in.hit(Action::Pause)) {
    shopObject.clear();
    return true;
  }
  int count = int(shops.at(shopObject).offers.size());
  if (in.hit(Action::Up))
    shopRow = (shopRow + count - 1) % count;
  if (in.hit(Action::Down))
    shopRow = (shopRow + 1) % count;
  if (in.hit(Action::Left) || in.hit(Action::Right))
    selling = !selling;
  if (in.hit(Action::Interact))
    trade(size_t(shopRow), selling);
  clock = 0;
  return true;
}
void StudioGame::drawAdventure(Canvas &c) const {
  if (combatEnabled) {
    c.rect({6, 31, 145, 16}, uiPanel);
    c.text("HEALTH " + std::to_string(health) + "/" + std::to_string(playerMaxHealth()), 12, 36,
           hurtClock > 0 ? Color{255, 125, 110} : uiAccent);
    if (!equipment.empty()) {
      c.rect({155, 31, 110, 16}, uiPanel);
      c.text("ATK " + std::to_string(playerAttackDamage()) + "  DEF " +
                 std::to_string(playerDefense()),
             161, 36, uiMuted);
    }
    auto camera = cameraFor(project.scenes.at(scene), feet);
    for (auto &a : actors)
      if (a.second.map == scene && a.second.mode == "enemy") {
        auto &state = actorStates.at(a.first);
        if (state.health <= 0)
          continue;
        auto p = state.position;
        c.rect({p.x - camera.x, p.y - camera.y - 5, 16, 3}, {40, 30, 30, 255});
        c.rect({p.x - camera.x, p.y - camera.y - 5, 16.f * state.health / a.second.maxHealth, 3},
               {220, 90, 90, 255});
      }
    for (auto &p : projectiles)
      if (p.map == scene)
        c.rect({p.position.x - camera.x - 2, p.position.y - camera.y - 2, 4, 4},
               p.owner == "player" ? uiAccent : Color{255, 110, 90, 255});
    auto w = weapon();
    if (combatClock > attackDelay - .12f && !(w && w->ranged)) {
      float x = feet.x - camera.x, y = feet.y - camera.y;
      c.rect(direction == "east"    ? Rect{x + 7, y - 8, 20, 3}
             : direction == "west"  ? Rect{x - 27, y - 8, 20, 3}
             : direction == "north" ? Rect{x - 2, y - 32, 3, 20}
                                    : Rect{x - 2, y + 2, 3, 20},
             uiAccent);
    }
    if (inventoryOpen)
      c.text(controlLabel(Action::Interact) +
                 (equipment.empty() ? " USE SELECTED HEALING ITEM" : " USE OR EQUIP SELECTED ITEM"),
             52, 188, uiAccent);
  }
  if (shopObject.empty())
    return;
  auto &shop = shops.at(shopObject);
  c.rect({12, 36, 376, 178}, uiModal);
  c.text((selling ? "SELL - " : "BUY - ") + interactions.at(shopObject).speaker.substr(0, 35), 22,
         44, uiAccent);
  auto money = inventory.find(shop.currency);
  c.text(items.at(shop.currency).name + ": " +
             std::to_string(money == inventory.end() ? 0 : money->second),
         22, 59, uiText);
  int start = std::max(0, shopRow - 6);
  for (int i = start; i < std::min(int(shop.offers.size()), start + 7); i++) {
    auto &o = shop.offers[size_t(i)];
    int y = 78 + (i - start) * 15;
    if (i == shopRow)
      c.rect({18, float(y - 3), 364, 14}, uiSelection);
    int stock = shopStock.at(shopObject).at(size_t(i));
    std::string line =
        items.at(o.item).name.substr(0, 25) + "  " + std::to_string(selling ? o.sell : o.buy) +
        "  " +
        (selling     ? std::to_string(inventory.count(o.item) ? inventory.at(o.item) : 0) + " OWNED"
         : stock < 0 ? "IN STOCK"
                     : std::to_string(stock) + " LEFT");
    c.text(line.substr(0, 57), 22, y, i == shopRow ? uiAccent : uiText);
  }
  if (toastTime > 0)
    c.text(toast.substr(0, 55), 22, 186, uiAccent);
  c.text("LEFT/RIGHT BUY/SELL  " + controlLabel(Action::Interact) + " TRADE  " +
             controlLabel(Action::Attack) + " CLOSE",
         22, 201, uiMuted);
}
void StudioGame::respawnPlayer() {
  scene = project.start;
  feet = project.scenes.at(scene).spawn;
  auto collision = collisionScene(scene, removed, activated);
  if (blocked(project, collision, body(feet)))
    for (int y = 1; y < collision.height - 1; y++)
      for (int x = 1; x < collision.width - 1; x++) {
        Vec2 next{float(x * 16 + 8), float(y * 16 + 12)};
        if (!blocked(project, collision, body(next))) {
          feet = next;
          goto found;
        }
      }
found:
  health = playerMaxHealth();
  projectiles.clear();
  hurtClock = 2;
  combatClock = 0;
  frontend = "playing";
  progressDirty = true;
  eventScene.clear();
  insideTriggers.clear();
  shopObject.clear();
  dialogueObject.clear();
  inventoryOpen = journalOpen = false;
  toast = "Recovered at camp.";
  toastTime = 3;
}
void StudioGame::writeAdventure(std::ostream &out) const {
  out << health << " " << actorStates.size() << "\n";
  for (auto &a : actorStates)
    out << std::quoted(a.first) << " " << a.second.position.x << " " << a.second.position.y << " "
        << a.second.health << " " << a.second.waypoint << " " << int(a.second.lootClaimed) << " "
        << int(a.second.provoked) << "\n";
  out << shopStock.size() << "\n";
  for (auto &shop : shopStock) {
    out << std::quoted(shop.first) << " " << shop.second.size();
    for (size_t i = 0; i < shop.second.size(); i++)
      out << " " << std::quoted(shops.at(shop.first).offers.at(i).item) << " " << shop.second[i];
    out << "\n";
  }
  out << equipped.size() << "\n";
  for (auto &e : equipped)
    out << std::quoted(e.first) << " " << std::quoted(e.second) << "\n";
}
StudioAdventureState StudioGame::readAdventure(std::istream &in, int version) const {
  StudioAdventureState result;
  int count = -1;
  in >> result.health >> count;
  require(in && result.health >= 0 && count == int(actors.size()),
          "Invalid saved actors or health");
  for (int i = 0; i < count; i++) {
    std::string key;
    StudioActorState state;
    int loot = -1, provoked = 0;
    in >> std::quoted(key) >> state.position.x >> state.position.y >> state.health >>
        state.waypoint >> loot;
    if (version >= 6)
      in >> provoked;
    require(in && actors.count(key) && !result.actors.count(key) &&
                std::isfinite(state.position.x) && std::isfinite(state.position.y) &&
                (loot == 0 || loot == 1) && (provoked == 0 || provoked == 1),
            "Invalid actor save record");
    state.provoked = provoked;
    auto &a = actors.at(key);
    auto &s = project.scenes.at(a.map);
    require(state.health >= 0 && state.health <= a.maxHealth &&
                (a.mode == "enemy" || state.health == a.maxHealth) && state.waypoint >= 0 &&
                state.waypoint < std::max(1, int(a.points.size())) && state.position.x >= 0 &&
                state.position.y >= 0 && state.position.x < s.width * 16 &&
                state.position.y < s.height * 16 && (loot == 0 || state.health == 0),
            "Invalid saved actor state");
    auto o =
        std::find_if(s.objects.begin(), s.objects.end(), [&](auto &o) { return o.id == a.id; });
    auto source = project.sprites.at(o->sprite).source;
    auto terrain = s;
    terrain.objects.clear();
    require(!blocked(project, terrain,
                     body({state.position.x + source.w / 2, state.position.y + source.h})),
            "Saved actor is outside terrain");
    state.lootClaimed = loot;
    result.actors[key] = state;
  }
  in >> count;
  require(in && count == int(shops.size()), "Saved merchants changed");
  for (int i = 0; i < count; i++) {
    std::string key;
    int offers = -1;
    in >> std::quoted(key) >> offers;
    require(in && shops.count(key) && !result.stock.count(key) &&
                offers == int(shops.at(key).offers.size()),
            "Invalid saved merchant");
    auto &shop = shops.at(key);
    auto &stock = result.stock[key];
    for (int k = 0; k < offers; k++) {
      std::string item;
      int number = -2;
      in >> std::quoted(item) >> number;
      require(in && item == shop.offers[size_t(k)].item && number >= -1 && number <= 999 &&
                  (shop.offers[size_t(k)].stock < 0 ? number == -1 : number >= 0),
              "Invalid merchant stock");
      stock.push_back(number);
    }
  }
  if (version >= 6) {
    in >> count;
    require(in && count >= 0 && count <= 3, "Invalid saved equipment");
    for (int i = 0; i < count; i++) {
      std::string slot, item;
      in >> std::quoted(slot) >> std::quoted(item);
      require(in && equipment.count(item) && equipment.at(item).slot == slot &&
                  !result.equipped.count(slot),
              "Saved equipment changed");
      result.equipped[slot] = item;
    }
  }
  require(result.health <= maxHealthWith(result.equipped), "Invalid saved health");
  return result;
}
void StudioGame::restoreAdventure(const StudioAdventureState &state) {
  health = state.health;
  equipped = state.equipped;
  projectiles.clear();
  actorStates = state.actors;
  shopStock = state.stock;
  combatClock = hurtClock = 0;
  shopObject.clear();
  bagRow = 0;
}
} // namespace lota
