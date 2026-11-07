#include "evengine/studio.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
using namespace evengine;
// EveNGINE 1.1 adventure expansion: equipment, ranged combat, factions, actor animations, timers.
int main(int argc, char **argv) {
  try {
    if (argc < 3)
      throw std::runtime_error("usage: expansion-tests <Frontier-Watch.eve> <output folder>");
    StudioGame g(argv[1]);
    int checks = 0;
    auto check = [&](bool ok, const char *name) {
      if (!ok)
        throw std::runtime_error(name);
      ++checks;
      std::cout << "OK " << name << '\n';
    };
    Input idle;
    auto press = [&](Action action) {
      Input input;
      input.set(action, true);
      g.update(input, Tick);
    };
    auto run = [&](float seconds) {
      for (float t = 0; t < seconds; t += Tick) {
        g.update(idle, Tick);
        g.dialogueObject.clear();
      }
    };
    auto fight = [&](float seconds) {
      for (float t = 0; t < seconds; t += Tick) {
        g.hurtClock = std::max(0.f, g.hurtClock - Tick);
        g.updateActors(Tick);
        g.updateProjectiles(Tick);
      }
    };
    auto stones = [&]() {
      return g.inventory.count("sling-stone") ? g.inventory.at("sling-stone") : 0;
    };
    auto source = [&](const char *sprite) { return g.project.sprites.at(sprite).source; };
    auto same = [](Rect a, Rect b) { return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h; };

    check(g.equipment.size() == 3 && g.factions.size() == 2 &&
              g.actors.at("grove:scout").walkFrames.size() == 4 &&
              std::any_of(g.events.begin(), g.events.end(),
                          [](auto &e) { return e.trigger == "timer" && e.delay == 30; }),
          "1.1 definitions load");

    // Equipment
    g.inventory["iron-sword"] = 1;
    check(g.playerAttackDamage() == 2 && !g.weapon(), "base attack without equipment");
    check(g.equip("iron-sword") && g.playerAttackDamage() == 4 && g.weapon(),
          "equipping a sword raises attack");
    check(g.equip("iron-sword") && g.playerAttackDamage() == 2 && !g.weapon(),
          "equipping the same item again unequips it");
    check(!g.equip("trail-bread") && !g.equip("watch-shield"),
          "only owned equipment can be equipped");
    g.inventory["watch-shield"] = 1;
    g.equip("watch-shield");
    check(g.playerMaxHealth() == 14 && g.playerDefense() == 1,
          "armor raises maximum health and defense");
    g.health = 14;
    g.hurtClock = 0;
    g.hurtPlayer(3);
    check(g.health == 12, "defense reduces incoming damage");
    g.hurtClock = 0;
    g.hurtPlayer(1);
    check(g.health == 11, "hits always deal at least one damage");
    g.inventory.erase("watch-shield");
    g.reconcileEquipment();
    check(!g.equipped.count("armor") && g.health == 10,
          "losing equipped armor unequips it and clamps health");
    g.inventory.clear();
    g.inventory["iron-sword"] = 1;
    press(Action::Menu);
    press(Action::Interact);
    check(g.inventoryOpen && g.equipped.count("weapon") && g.equipped.at("weapon") == "iron-sword",
          "bag confirm equips the selected item");
    press(Action::Menu);
    g.shopObject = "meadow:camp-trader";
    g.inventory["gold-coin"] = 5;
    g.shops.at("meadow:camp-trader").offers.push_back({"iron-sword", 4, 2, -1});
    g.shopStock.at("meadow:camp-trader").push_back(-1);
    check(g.trade(g.shops.at("meadow:camp-trader").offers.size() - 1, true), "equipped item sells");
    g.update(idle, Tick);
    check(!g.equipped.count("weapon"), "selling equipped gear unequips it");
    g.shopObject.clear();
    g.shops.at("meadow:camp-trader").offers.pop_back();

    // Ranged combat
    g.startNewGame();
    check(g.equipped.empty() && g.projectiles.empty(), "New game clears equipment and projectiles");
    auto raider = [&]() -> StudioActorState & { return g.actorStates.at("meadow:thorn-raider"); };
    g.inventory["sling"] = 1;
    g.equip("sling");
    g.feet = {198, 264};
    g.direction = "east";
    g.combatClock = 0;
    g.playerAttack();
    check(g.projectiles.empty() && g.combatClock > 0, "sling without stones does not fire");
    g.inventory["sling-stone"] = 2;
    g.combatClock = 0;
    g.playerAttack();
    check(g.projectiles.size() == 1 && stones() == 1, "sling fires a projectile and uses a stone");
    g.updateProjectiles(.2f);
    check(raider().health == 4 && raider().provoked && g.projectiles.empty(),
          "projectile hits and damages an enemy");
    g.feet = {40, 40};
    g.direction = "north";
    g.combatClock = 0;
    g.playerAttack();
    g.updateProjectiles(.5f);
    check(g.projectiles.empty() && stones() == 0, "walls stop projectiles");
    g.inventory["sling-stone"] = 1;
    g.feet = {40, 56};
    g.direction = "east";
    g.combatClock = 0;
    g.playerAttack();
    g.updateProjectiles(.5f);
    check(g.projectiles.size() == 1, "projectile keeps flying within its range");
    g.updateProjectiles(.5f);
    check(g.projectiles.empty(), "projectile disappears at the end of its range");
    g.inventory["sling-stone"] = 1;
    g.combatClock = 0;
    g.playerAttack();
    g.scene = "grove";
    g.updateProjectiles(Tick);
    check(g.projectiles.empty(), "changing maps discards projectiles");

    // Factions
    g.startNewGame();
    g.scene = "grove";
    g.feet = {460, 290};
    auto guard = [&]() -> StudioActorState & { return g.actorStates.at("grove:watch-guard"); };
    auto slinger = [&]() -> StudioActorState & { return g.actorStates.at("grove:thorn-slinger"); };
    check(!g.hostileToPlayer("grove:watch-guard") && g.hostileToPlayer("grove:thorn-slinger") &&
              g.hostileActors("grove:watch-guard", "grove:thorn-slinger") &&
              !g.hostileActors("grove:thorn-slinger", "grove:thorn-slinger"),
          "faction relations resolve");
    fight(8);
    check(guard().health < 12 || slinger().health < 5, "rival factions fight each other");
    check(g.health == 10, "a distant player is not drawn into the faction fight");
    g.startNewGame();
    g.scene = "grove";
    slinger().health = 0;
    slinger().lootClaimed = true;
    g.feet = {150, 144};
    fight(2);
    check(g.health == 10, "neutral faction ignores the player");
    g.feet = {guard().position.x + 8 + 14, guard().position.y + 24};
    g.direction = "west";
    g.combatClock = 0;
    g.playerAttack();
    check(guard().health == 10 && guard().provoked && g.hostileToPlayer("grove:watch-guard"),
          "attacking a neutral enemy provokes it");
    fight(2);
    check(g.health < 10, "provoked neutral enemy fights back");
    g.startNewGame();
    g.scene = "grove";
    guard().health = 0;
    guard().lootClaimed = true;
    g.feet = {276, 128};
    fight(3);
    check(g.health < 10, "ranged enemy projectiles hurt the player");

    g.startNewGame();
    g.scene = "grove";
    guard().health = 0;
    guard().lootClaimed = true;
    g.health = 1;
    g.feet = {276, 128};
    fight(3);
    check(g.health == 0 && g.projectiles.empty(), "a fatal projectile ends the fight cleanly");

    // Actor animations
    g.startNewGame();
    g.scene = "grove";
    g.feet = {460, 290};
    Rect frame;
    auto &walk = g.actors.at("grove:scout").walkFrames;
    check(g.actorFrame("grove:scout", frame) && same(frame, walk.at("south")[0]),
          "idle animated NPC shows its first frame for its facing");
    std::set<float> seen;
    for (int i = 0; i < 240; i++) {
      g.updateActors(Tick);
      if (g.actorFrame("grove:scout", frame))
        seen.insert(frame.x);
    }
    check(g.actorStates.at("grove:scout").facing == "east" &&
              g.actorStates.at("grove:scout").animationClock > 0,
          "moving NPC faces its walking direction");
    check(seen.size() >= 2, "walking animation cycles through frames");
    check(!g.actorFrame("grove:watch-guard", frame), "actors without frames use their sprite");
    auto &slingerActor = g.actors.at("grove:thorn-slinger");
    slingerActor.attackFrames["east"] = {source("eve-raider"), source("eve-guard")};
    slinger().facing = "east";
    slinger().attackClock = .15f;
    check(g.actorFrame("grove:thorn-slinger", frame) && same(frame, source("eve-guard")),
          "attack animation plays its frames");
    slinger().attackClock = 0;
    slinger().timer = 5;
    g.feet = {276, 128};
    guard().health = 0;
    guard().lootClaimed = true;
    g.updateActors(1);
    check(slinger().attackClock < 0, "attack animation ends after its frames");
    SoftwareRenderer renderer;
    g.feet = {276, 128};
    fight(.4f);
    renderer.render(g.draw(), g.atlas);
    renderer.writePPM((std::filesystem::path(argv[2]) / "expansion-combat.ppm").string());
    check(true, "animated actors, projectiles and equipment HUD render");

    // Timed events
    g.startNewGame();
    run(29);
    check(!g.firedEvents.count("watch-horn"), "timed event waits for its delay");
    run(2);
    check(g.firedEvents.count("watch-horn"), "timed event fires after its delay");
    g.startNewGame();
    run(20);
    g.scene = "grove";
    g.feet = g.project.scenes.at("grove").spawn;
    run(Tick);
    g.scene = "meadow";
    g.feet = g.project.scenes.at("meadow").spawn;
    run(15);
    check(!g.firedEvents.count("watch-horn"), "leaving a map resets its timers");
    g.startNewGame();
    g.interact(*std::find_if(g.project.scenes.at("meadow").objects.begin(),
                             g.project.scenes.at("meadow").objects.end(),
                             [](auto &o) { return o.id == "keeper"; }));
    for (int i = 0; i < 40 * 60; i++)
      g.update(idle, Tick);
    check(!g.firedEvents.count("watch-horn"), "timers pause during conversations");
    g.dialogueObject.clear();
    StudioEvent repeat;
    repeat.id = "coin-drip";
    repeat.trigger = "timer";
    repeat.scene = "meadow";
    repeat.delay = 1;
    repeat.once = false;
    StudioEventAction give;
    give.type = "giveItem";
    give.target = "gold-coin";
    repeat.actions.push_back(give);
    g.events.push_back(repeat);
    g.startNewGame();
    run(.5f); // map-entry events (such as starting coins) run first
    int coins = g.inventory.count("gold-coin") ? g.inventory.at("gold-coin") : 0;
    run(3);
    check(g.inventory.at("gold-coin") == coins + 3, "repeating timer fires on every interval");
    g.events.pop_back();

    // Saves
    g.startNewGame();
    g.inventory["iron-sword"] = 1;
    g.inventory["watch-shield"] = 1;
    g.equip("iron-sword");
    g.equip("watch-shield");
    g.health = 13;
    guard().provoked = true;
    std::string error, path = (std::filesystem::path(argv[2]) / "expansion-save.dat").string();
    check(g.save(path, error), "expansion progress saves");
    std::ifstream saved(path);
    std::string data((std::istreambuf_iterator<char>(saved)), {});
    check(data.rfind("EVE_STUDIO_SAVE 6", 0) == 0, "adventure saves use format 6");
    StudioGame restored(argv[1]);
    check(restored.load(path, error) && restored.health == 13 &&
              restored.equipped.at("weapon") == "iron-sword" &&
              restored.equipped.at("armor") == "watch-shield" &&
              restored.actorStates.at("grove:watch-guard").provoked,
          "equipment, bonus health and provocation restore together");
    auto bad = [&](std::string from, std::string to) {
      auto copy = data;
      auto at = copy.find(from);
      if (at == std::string::npos)
        throw std::runtime_error("save fixture missing " + from);
      copy.replace(at, from.size(), to);
      std::ofstream(path + ".bad") << copy;
      return !restored.load(path + ".bad", error);
    };
    check(bad("\"weapon\" \"iron-sword\"", "\"armor\" \"iron-sword\""),
          "equipment in the wrong slot rejects");
    check(bad("\n13 ", "\n99 "), "health above the equipped maximum rejects");
    check(restored.health == 13 && restored.equipped.size() == 2,
          "rejected saves leave progress unchanged");
    // Build a 1.0-style (format 5) save from a game without 1.1 state.
    g.startNewGame();
    check(g.save(path, error), "plain adventure progress saves");
    std::ifstream plain(path);
    std::stringstream lines;
    lines << plain.rdbuf();
    std::string line, v5;
    std::vector<std::string> all;
    while (std::getline(lines, line))
      all.push_back(line);
    all.pop_back(); // equipment count
    for (auto &l : all) {
      std::istringstream words(l);
      std::vector<std::string> tokens;
      std::string w;
      while (words >> std::quoted(w))
        tokens.push_back(w);
      if (l.rfind("EVE_STUDIO_SAVE", 0) == 0)
        v5 += "EVE_STUDIO_SAVE 5\n";
      else if (tokens.size() == 7 && l[0] == '"')
        v5 += l.substr(0, l.rfind(' ')) + "\n";
      else
        v5 += l + "\n";
    }
    std::ofstream(path + ".v5") << v5;
    check(restored.load(path + ".v5", error) && restored.equipped.empty() && restored.health == 10,
          "1.0 adventure saves still load");
    std::cout << checks << " expansion runtime checks passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
