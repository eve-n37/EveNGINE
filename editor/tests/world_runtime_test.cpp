// EveNGINE 1.2 world features on the Lantern Isles sample: time of day, weather, cutscenes,
// leveling, pushable blocks and pressure plates, vehicles, and saving all of them.
#include "evengine/studio.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace evengine;
int main(int argc, char **argv) {
  try {
    if (argc != 4)
      throw std::runtime_error("Expected Lantern Isles, Frontier Watch and an output folder");
    std::string sample = argv[1];
    std::filesystem::path out = argv[3];
    int checks = 0;
    auto check = [&](bool ok, const std::string &text) {
      if (!ok)
        throw std::runtime_error("FAILED: " + text);
      ++checks;
      std::cout << "OK " << text << '\n';
    };
    StudioGame g(sample);
    auto tick = [&](Input in, int frames = 1) {
      for (int i = 0; i < frames; i++) {
        g.update(in, Tick);
        in.consume();
      }
    };
    auto press = [&](Action a) {
      Input in;
      in.set(a, true);
      tick(in);
    };
    auto hold = [&](Action a, int frames) {
      Input in;
      in.set(a, true);
      in.consume();
      in.held[size_t(a)] = true;
      tick(in, frames);
    };
    auto fullScreen = [](const Canvas &c, Color colour) {
      for (auto &d : c.draws)
        if (!d.sprite && d.destination.x == 0 && d.destination.y == 0 && d.destination.w == Width &&
            d.destination.h == Height && d.color.r == colour.r && d.color.g == colour.g &&
            d.color.b == colour.b)
          return int(d.color.a);
      return -1;
    };

    // Time of day and weather.
    check(g.hasWorld() && g.timeSettings.enabled && g.hour == 17.5f, "the day starts at 17:30");
    check(g.weatherAt("grove") == "rain" && g.weatherAt("isle") == "fog" &&
              g.weatherAt("meadow") == "none",
          "maps have their own weather");
    g.hour = 12;
    check(g.darkness() == 0, "midday is fully lit");
    g.hour = 23;
    check(g.darkness() == 1 && fullScreen(g.draw(), {26, 36, 72}) == int(.6f * 255),
          "night tints the screen with the night colour");
    g.hour = 19.5f;
    check(g.darkness() > 0 && g.darkness() < 1, "dusk is part-way dark");
    g.hour = 17.5f;
    g.updateClock(20);
    check(std::abs(g.hour - 18.5f) < .001f, "an 8-minute day moves an hour every 20 seconds");
    check(g.timeBetween(19, 6) == false && (g.hour = 2, g.timeBetween(19, 6)),
          "time ranges wrap past midnight");
    g.hour = 17.5f;

    // The opening cutscene starts when the courtyard is entered.
    auto feet = g.feet;
    tick(Input{});
    check(g.cutscene == "keeper-intro", "entering the courtyard starts the cutscene");
    tick(Input{});
    check(g.fade == 1 && g.cameraOverride && g.cameraNow.x == 128,
          "the cutscene opens on black, looking at the keeper");
    auto letterbox = g.draw();
    check(std::any_of(letterbox.draws.begin(), letterbox.draws.end(),
                      [](auto &d) {
                        return !d.sprite && d.destination.y == 0 && d.destination.h == 14 &&
                               d.color.a == 255;
                      }),
          "cutscenes draw letterbox bars");
    hold(Action::Right, 80);
    check(g.feet.x == feet.x && g.fade == 0, "the screen fades in and the player can't walk");
    auto &steps = g.cutscenes.at("keeper-intro").steps;
    check(steps[g.cutsceneStep].type == "say", "the keeper speaks and waits");
    press(Action::Interact);
    tick(Input{}, 60);
    check(g.actorStates.at("meadow:keeper").position.x == 136,
          "the keeper (a patrolling NPC) walks over");
    check(steps[g.cutsceneStep].type == "say", "and speaks again");
    press(Action::Quit);
    check(g.cutscene.empty() && !g.cameraOverride && g.fade == 0 &&
              g.actorStates.at("meadow:keeper").position.x == 120 && !g.quit,
          "Escape skips to the end without quitting");
    tick(Input{});
    check(g.cutscene.empty(), "the cutscene plays once");

    // Leveling.
    auto damage = g.playerAttackDamage(), top = g.playerMaxHealth();
    check(g.leveling.enabled && g.level == 1 && g.xpForLevel(2) == 10 && g.xpForLevel(3) == 25,
          "levels need 10, then 15 more XP");
    g.gainXp(12);
    check(g.level == 2 && g.playerMaxHealth() == top + 2 && g.health == top + 2 &&
              g.playerAttackDamage() == damage + 1,
          "levelling up raises health and attack and heals");
    g.damageActor("meadow:thorn-raider", 999, true);
    check(g.xp == 18, "defeating an enemy gives its XP");
    check(g.questXp.at("keepers-errand") == 15, "quests give XP when completed");
    g.gainXp(99999);
    check(g.level == g.leveling.maxLevel && g.xp == g.xpForLevel(g.leveling.maxLevel),
          "XP stops at the top level");
    g.xp = 18;
    g.level = 2;
    g.health = g.playerMaxHealth();

    // Pushable blocks and pressure plates on the isle.
    g.scene = "isle";
    g.feet = {290, 190};
    tick(Input{});
    hold(Action::Right, 25);
    check(g.movedObjects.count("isle:stone") && g.movedObjects.at("isle:stone").x == 320,
          "walking into the stone pushes it one tile");
    g.feet = {290, 230};
    check(g.pushBlock("isle:stone", {1, 0}) && g.pushBlock("isle:stone", {1, 0}),
          "the stone slides onto the plate");
    tick(Input{});
    check(g.pressedPlates.count("isle:plate") && g.activated.count("isle:vault-door") &&
              g.plateDoors.count("isle:vault-door"),
          "the plate opens the vault door");
    auto &isle = g.project.scenes.at("isle");
    auto plate = std::find_if(isle.objects.begin(), isle.objects.end(),
                              [](auto &o) { return o.id == "plate"; });
    g.scene = "isle";
    check(g.visibleSprite(*plate) == "eve-plate-down", "the plate shows its pressed sprite");
    check(g.pushBlock("isle:stone", {1, 0}) && !g.pushBlock("isle:stone", {1, 0}),
          "the vault wall stops the stone");
    tick(Input{});
    check(!g.pressedPlates.count("isle:plate") && !g.activated.count("isle:vault-door"),
          "the door closes when the stone leaves the plate");
    g.resetPuzzle("isle");
    check(!g.movedObjects.count("isle:stone"), "resetting puts the stone back");

    // Vehicles: row across the lake to the island.
    g.feet = {264, 150};
    tick(Input{});
    check(g.board("isle:boat") && g.riding == "isle:boat", "the player gets in the boat");
    auto collision = g.collisionScene("isle", g.removed, g.activated);
    check(g.rideAllowed(collision, g.feet, g.vehicles.at("isle:boat")),
          "the boat sits on open water");
    check(!g.rideAllowed(collision, {280, 200}, g.vehicles.at("isle:boat")),
          "boats can't go on land");
    float before = g.feet.x;
    hold(Action::Left, 120);
    check(g.feet.x < before - 50 && g.feet.x >= 160, "the boat rows west and stops at the island");
    check(g.message.find("GET OFF") != std::string::npos, "the prompt offers getting off");
    press(Action::Interact);
    check(g.riding.empty() && g.feet.x < 160 && g.movedObjects.count("isle:boat"),
          "the player steps onto the island, leaving the boat at its shore");
    hold(Action::Left, 5);
    check(g.feet.x >= 112, "walkers can't step into the lake");

    // Saves keep the world.
    g.weatherOverride["isle"] = "none";
    g.hour = 21.25f;
    auto save = (out / "world-progress.dat").string();
    std::string error;
    check(g.save(save, error), "a world save writes: " + error);
    std::ifstream header(save);
    std::string magic;
    int version = 0;
    header >> magic >> version;
    check(version == 7, "world projects write save format 7");
    StudioGame h(sample);
    check(h.load(save, error), "the world save loads: " + error);
    check(h.hour == g.hour && h.xp == 18 && h.level == 2 && h.health == g.health &&
              h.weatherAt("isle") == "none" &&
              std::equal(h.movedObjects.begin(), h.movedObjects.end(), g.movedObjects.begin(),
                         g.movedObjects.end(),
                         [](auto &a, auto &b) {
                           return a.first == b.first && a.second.x == b.second.x &&
                                  a.second.y == b.second.y;
                         }) &&
              h.riding.empty(),
          "time, XP, weather and moved objects come back");
    g.feet = {264, 150};
    g.movedObjects.erase("isle:boat");
    check(g.board("isle:boat") && g.save(save, error) && h.load(save, error) &&
              h.riding == "isle:boat" && h.feet.x == g.feet.x,
          "a save made in the boat loads in the boat");
    std::ofstream(save, std::ios::app) << "junk\n";
    check(!h.load(save, error), "trailing world data is rejected");

    // Land vehicles stay out of water and can't use exits.
    StudioGame cart(sample);
    cart.finishCutscene();
    auto courtyard = cart.collisionScene("meadow", cart.removed, cart.activated);
    check(cart.rideAllowed(courtyard, {188, 114}, cart.vehicles.at("meadow:cart")) &&
              !cart.rideAllowed(courtyard, {96, 72}, cart.vehicles.at("meadow:cart")),
          "carts roll on land but not into the pond");
    cart.feet = {196, 110};
    check(cart.board("meadow:cart"), "the player climbs onto the cart");
    cart.feet = {24, 120};
    Input idle;
    cart.update(idle, Tick);
    cart.update(idle, Tick);
    check(cart.scene == "meadow", "exits wait until the player gets off");

    // Projects without world features keep their old save format.
    StudioGame plain(argv[2]);
    check(!plain.hasWorld(), "Frontier Watch uses no world features");
    auto plainSave = (out / "plain-progress.dat").string();
    check(plain.save(plainSave, error), "a plain save writes");
    std::ifstream plainHeader(plainSave);
    plainHeader >> magic >> version;
    check(version == 6, "projects without world features still write format 6");

    std::cout << checks << " world checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
