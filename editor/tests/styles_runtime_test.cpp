// Gameplay styles on the Lantern Isles sample: the sidescrolling cave and the first-person crypt.
#include "evengine/studio.hpp"
#include <cmath>
#include <iostream>
using namespace evengine;
int main(int argc, char **argv) {
  try {
    if (argc != 3)
      throw std::runtime_error("Expected Lantern Isles and an output folder");
    std::string out = argv[2];
    int checks = 0;
    auto check = [&](bool ok, const std::string &text) {
      if (!ok)
        throw std::runtime_error("FAILED: " + text);
      ++checks;
      std::cout << "OK " << text << '\n';
    };
    StudioGame g(argv[1]);
    auto tick = [&](Input in, int frames = 1) {
      for (int i = 0; i < frames; i++) {
        g.update(in, Tick);
        in.consume();
      }
    };
    auto holding = [](std::initializer_list<Action> actions) {
      Input in;
      for (auto a : actions)
        in.held[size_t(a)] = true;
      return in;
    };
    auto pressing = [](std::initializer_list<Action> actions) {
      Input in;
      for (auto a : actions)
        in.set(a, true);
      return in;
    };
    Input idle;
    // Skip the opening cutscene in the courtyard.
    tick(idle);
    tick(pressing({Action::Quit}));
    check(g.cutscene.empty(), "the opening cutscene is skipped");

    // ---- Sidescroller -------------------------------------------------------------------------
    check(g.styleOf("cave").kind == "side" && g.styleOf("cave").jump == 52 &&
              g.styleOf("meadow").kind == "topdown",
          "the cave is a sidescroller and the courtyard stays top-down");
    g.scene = "cave";
    g.feet = {40, 230};
    tick(idle, 20);
    check(g.grounded && g.feet.y == 240, "the player falls onto the cave floor and stands on it");
    auto &caveStyle = g.styleOf("cave");
    check(caveStyle.speed == 72 && caveStyle.acceleration == .15f && caveStyle.braking == .1f,
          "the cave has its own walking speed, acceleration and braking");
    float x = g.feet.x;
    tick(holding({Action::Right}), 3);
    check(g.sideVelocity.x > 0 && g.sideVelocity.x < 72, "the player speeds up from a standstill");
    tick(holding({Action::Right}), 27);
    check(g.feet.y == 240 && g.sideVelocity.x == 72 &&
              std::abs(g.feet.x - x - 72 * (.5f - .15f / 2)) < 1.5f,
          "walking right reaches the map's walking speed");
    tick(idle);
    check(g.sideVelocity.x > 0, "letting go, the player slows down");
    tick(idle, 6);
    check(g.sideVelocity.x == 0, "and stops within the braking time");
    check(g.direction == "east", "the player faces the way they walk");

    // A full jump reaches the jump height; letting go early makes a short hop.
    auto peak = [&](int held) {
      float top = g.feet.y, ground = g.feet.y;
      tick(pressing({Action::Attack}));
      for (int i = 0; i < 90; i++) {
        tick(i < held ? holding({Action::Attack}) : idle);
        top = std::min(top, g.feet.y);
      }
      return ground - top;
    };
    float full = peak(90), hop = peak(2);
    check(full > 48 && full <= 53, "holding jump reaches the jump height");
    check(hop > 4 && hop < full * .6f, "tapping jump makes a short hop");
    check(g.grounded && g.feet.y == 240, "the player lands again");

    // Jump buffering: a press just before landing still jumps.
    tick(pressing({Action::Attack}));
    for (int i = 0; i < 90 && !(g.sideVelocity.y > 0 && g.feet.y > 236); i++)
      tick(holding({Action::Attack}));
    tick(pressing({Action::Attack}));
    tick(idle, 8);
    check(g.feet.y < 240, "a jump pressed just before landing happens on landing");
    tick(idle, 90);

    // Coyote time: jumping just after walking off a ledge still works.
    g.feet = {292.5f, 192};
    tick(idle, 2);
    check(g.grounded && g.feet.y == 192, "ledges hold the player up");
    tick(holding({Action::Right}), 6);
    bool fell = !g.grounded;
    tick(pressing({Action::Attack}));
    tick(holding({Action::Attack}), 6);
    check(fell && g.sideVelocity.y < 0, "a jump just after walking off a ledge still counts");
    tick(idle, 120);

    // One-way ledges: jump up through them from below, drop down through them with Down.
    g.feet = {264, 240};
    tick(idle, 2);
    tick(pressing({Action::Attack}));
    tick(holding({Action::Attack}), 60);
    tick(idle, 30);
    check(g.grounded && g.feet.y == 192, "jumping up through a ledge lands on top of it");
    check(g.score >= 1 &&
              std::find(g.removed.begin(), g.removed.end(), "cave:cave-coin-1") != g.removed.end(),
          "the coin above the ledge is collected");
    tick(holding({Action::Down}), 40);
    check(g.grounded && g.feet.y == 240, "holding down drops through the ledge");

    // Spikes hurt and send the player back to safe ground.
    g.feet = {140, 240};
    tick(idle, 2);
    int health = g.health;
    for (int i = 0; i < 90 && g.health == health; i++)
      tick(holding({Action::Right}));
    tick(idle, 30);
    check(g.health == health - 1 && g.feet.x < 160 && g.feet.y == 240,
          "spikes hurt and put the player back on safe ground");
    g.hurtClock = 0;

    // Ladders: climb up without gravity, then step off onto the high shelf.
    g.feet = {488, 240};
    tick(idle, 2);
    tick(holding({Action::Up}), 10);
    check(g.climbing && g.feet.y < 240, "holding up on a ladder climbs it");
    float climbed = g.feet.y;
    tick(idle, 30);
    check(g.climbing && g.feet.y == climbed, "the player hangs on the ladder without falling");
    tick(holding({Action::Up}), 240);
    check(!g.climbing && g.grounded && g.feet.y == 80, "the top of the ladder can be stood on");
    tick(holding({Action::Right}), 40);
    check(g.grounded && g.feet.y == 96 && g.feet.x > 500, "the player steps down onto the shelf");
    tick(holding({Action::Right}), 60);
    check(g.message.find("OPEN CHEST") != std::string::npos, "the chest on the shelf is in reach");
    tick(pressing({Action::Interact}));
    check(g.activated.count("cave:cave-chest") || g.inventory.count("gold-coin"),
          "the chest opens");
    tick(idle, 2);
    float before = g.combatClock;
    tick(pressing({Action::Attack}));
    check(g.combatClock == before, "space jumps instead of attacking in the cave");
    tick(idle, 60);

    // Saves keep the cave and the player's place in it.
    std::string error, save = out + "/styles-progress.dat";
    g.feet = {104, 240};
    tick(idle, 2);
    check(g.save(save, error), "saving in the cave works: " + error);
    StudioGame h(argv[1]);
    check(h.load(save, error) && h.scene == "cave" && h.feet.x == 104 && h.feet.y == 240,
          "loading returns to the cave");

    // The exit on the left leads back to the isle, which is top-down again.
    g.feet = {40, 240};
    tick(idle, 40); // the exit waits a moment after arriving
    tick(holding({Action::Left}), 40);
    check(g.scene == "isle", "the cave's exit leads back to the isle");

    // ---- Top-down movement tuning --------------------------------------------------------------
    StudioMapStyle tuned;
    tuned.speed = 100;
    tuned.acceleration = .5f;
    tuned.braking = .25f;
    g.mapStyles["grove"] = tuned;
    g.scene = "grove";
    g.feet = g.project.scenes.at("grove").spawn;
    tick(idle);
    tick(holding({Action::Right}), 6);
    check(g.walkVelocity.x > 0 && g.walkVelocity.x < 100 * .25f,
          "top-down walking speeds up over the acceleration time");
    tick(holding({Action::Right}), 30);
    check(std::abs(g.walkVelocity.x - 100) < .01f, "and reaches the map's walking speed");
    x = g.feet.x;
    tick(idle, 3);
    check(g.feet.x > x && g.walkVelocity.x > 0 && g.walkVelocity.x < 100,
          "letting go, the player slides to a stop");
    tick(idle, 15);
    check(g.walkVelocity.x == 0, "and stops within the braking time");
    g.mapStyles.erase("grove");
    g.scene = "isle";
    g.feet = {304, 256};
    tick(idle);

    // ---- Raycast ------------------------------------------------------------------------------
    check(g.styleOf("crypt").kind == "raycast" && g.styleOf("crypt").viewDistance == 176,
          "the crypt is a first-person map");
    g.scene = "crypt";
    g.feet = {40, 160};
    g.direction = "east";
    tick(idle);
    check(std::abs(g.viewAngle) < .001f, "arriving facing east looks east");
    check(std::abs(g.styleOf("crypt").turn - 150 * 3.14159265f / 180) < .001f,
          "the crypt has its own turning speed");
    tick(holding({Action::Right}), 39);
    check(std::abs(g.viewAngle - g.styleOf("crypt").turn * 39 * Tick) < .01f &&
              g.direction == "south",
          "left and right turn the view at the turning speed");
    tick(holding({Action::Left}), 39);
    x = g.feet.x;
    tick(holding({Action::Up}), 3);
    check(g.forwardSpeed > 0 && g.forwardSpeed < 56, "walking forward speeds up");
    tick(holding({Action::Up}), 12);
    check(g.feet.x > x + 8 && std::abs(g.feet.y - 160) < .5f && g.forwardSpeed == 56,
          "up walks forward at the map's walking speed");
    tick(holding({Action::Up}), 120);
    check(g.feet.x < 80 - 6 && g.forwardSpeed == 0,
          "walls stop the player before their nose touches them");
    x = g.feet.x;
    tick(holding({Action::Down}), 10);
    check(g.feet.x < x, "down walks backward");

    // Things are only in reach when they're in front of the player.
    g.feet = {140, 166};
    g.viewAngle = 0.15f;
    tick(idle);
    check(g.message.find("TALK") != std::string::npos, "the lamplighter is in reach when faced");
    g.viewAngle = 3.1f;
    tick(idle);
    check(g.message.find("TALK") == std::string::npos, "turning away puts them out of reach");
    g.viewAngle = 0.15f;
    tick(idle);
    tick(pressing({Action::Interact}));
    check(g.dialogueObject == "crypt:crypt-ghost", "talking works in first person");
    tick(pressing({Action::Quit}));

    // The view: shaded floor and ceiling bands, wall slices and billboards.
    g.feet = {104, 168};
    g.viewAngle = 0.15f;
    tick(idle);
    auto canvas = g.draw();
    int slices = 0, bands = 0;
    for (auto &d : canvas.draws) {
      if (d.sprite && d.source.w == 1)
        ++slices;
      if (!d.sprite && d.destination.w == Width && d.destination.h == 4)
        ++bands;
    }
    check(bands == 60, "the floor and ceiling are drawn in shaded bands");
    check(slices > 200, "walls and characters are drawn in one-pixel slices");
    SoftwareRenderer renderer;
    renderer.render(canvas, g.atlas);
    renderer.writePPM(out + "/styles-crypt.ppm");
    before = g.combatClock;
    tick(pressing({Action::Attack}));
    check(g.combatClock == before && g.attackClock < 0, "there's no attacking in first person");

    std::cout << checks << " style checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
