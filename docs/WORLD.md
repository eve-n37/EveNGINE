# World guide

This guide covers day and night, weather, push-block puzzles, leveling, cutscenes and vehicles. To
try them all, open `Lantern-Isles.eve` in Studio (in `editor/examples/` in the source). A cutscene
greets you in the courtyard, the grove is rainy, and the west edge of the courtyard leads to Lantern
Isle: row the boat to the island chest, and push the stone onto the plate to open the vault.

Everything here is optional. Projects that don't use these features work and save exactly as
before.

## Day and night

Turn on **Game → Setup → Time & weather → Day and night**.

| Setting               | What it does                                                         |
| --------------------- | -------------------------------------------------------------------- |
| **Length of a day**   | Real seconds for a full 24 hours, from 60 to 7200. 600 is ten minutes. |
| **Starting hour**     | The time when a new game starts, from 0 to 23.99 (17.5 is 17:30).    |
| **Night colour**      | The colour laid over the map at night.                               |
| **Night strength**    | How strongly that colour shows at midnight, from 0 to 0.85.          |
| **Show a clock**      | Adds the time to the top of the screen.                              |

It's light from 7:00 to 18:00, gets darker until 21:00, stays dark until 5:00 and brightens again
by 7:00. Time pauses during menus, conversations and cutscenes, and it's saved with progress.

In the map inspector, tick **Indoors** for maps that shouldn't get dark, such as houses and caves.

Events can check the time with a **Time of day** condition: it matches from one hour to another,
and wraps past midnight (19 to 6 means evenings and nights). The **Set the time** action jumps the
clock, for example to morning after the player sleeps.

## Weather

Choose **Clear**, **Rain**, **Snow** or **Fog** for each map in the map inspector. The **Change
weather** event action switches a map's weather during play, and the change is saved. In Lantern
Isles, the fog lifts 45 seconds after you arrive on the isle.

## Push blocks and pressure plates

Set an object's behaviour to **block** and the player can push it one tile (16 pixels) at a time
by walking into it for a moment. The block's artwork needs a collider. Blocks stop at walls, other
objects and water.

Set an object's behaviour to **plate** for a pressure plate. While the player or a block is on it,
the plate:

- shows its **Pressed sprite**,
- turns on its **Story flag while pressed** (and off again when it's released),
- opens the **Doors it opens** on the same map, which close again when nothing is on the plate,
  unless the player is standing in the doorway.

Tick **Stays pressed once triggered** for a plate that latches. The **Reset push blocks** event
action puts every block on a map back where it started and releases latched plates, so players
can't get stuck. In Lantern Isles, reading the sign next to the stone resets it.

The **Puzzles & travel** built-in pack has a push block and a pressure plate with its pressed state.

## Leveling

Leveling needs player combat. Turn on **Game → Setup → Leveling**:

| Setting                 | What it does                                             |
| ----------------------- | -------------------------------------------------------- |
| **Top level**           | The highest level, from 2 to 99.                         |
| **XP to reach level 2** | How much XP the first level-up costs.                    |
| **XP growth per level** | Each level costs this much more than the one before.     |
| **Health/Attack/Defence per level** | Added to the player's stats for every level above 1. |

The setup panel shows how much XP a few levels need. Players get XP from:

- defeating an enemy: set **XP for defeating it** in the enemy's inspector,
- completing a quest: set **Reward XP** on the quest,
- events: the **Give XP** action.

Levelling up refills health. The HUD shows the level and the XP needed for the next one.

## Cutscenes

Open **Story → Cutscenes** and click **New cutscene**. A cutscene plays on one map and runs its
steps in order:

| Step                      | What it does                                                      |
| ------------------------- | ----------------------------------------------------------------- |
| **Show a line of dialogue** | Shows a speaker and a line, and waits for the player to press E. |
| **Move a character**      | Walks the player (to a position of their feet) or an object on the map (to a top-left position) in a straight line. |
| **Turn a character**      | Faces the player or a moving NPC or enemy north, south, east or west. |
| **Move the camera**       | Pans to a point, or back to the player, over a number of seconds. |
| **Fade the screen**       | Fades to black or back to clear.                                  |
| **Wait**                  | Pauses for a number of seconds.                                   |
| **Play a sound**          | Plays a sound effect.                                             |
| **Set a story flag**      | Turns a flag on or off.                                           |

Start a cutscene from an event with the **Play cutscene** action. The event must be on the same
map as the cutscene. While it plays, the player can't move, the HUD hides behind black bars, and
NPCs and enemies wait. Players can press the pause key to skip to the end: every move, turn and
flag still happens, but lines and sounds are skipped. The screen and camera always return to normal
when a cutscene ends.

Objects that a cutscene moves stay where it leaves them, and that's saved. Patrolling NPCs carry
on with their routes afterwards.

## Vehicles

Set an object's behaviour to **vehicle**, then choose what it **Travels on**:

- **Land**: carts, horses and so on. They can't go into water.
- **Water**: boats. They can only move on water, including water that's painted as a wall to keep
  walkers out. That's the usual way to make a lake: paint water, and mark it as collision.

| Setting                  | What it does                                                                  |
| ------------------------ | ----------------------------------------------------------------------------- |
| **Top speed**            | A multiple of the map's walking speed, from 0.25 to 6. The run key still speeds it up. |
| **Seconds to top speed** | How long it takes to get going, from 0 (instantly) to 10.                     |
| **Seconds to stop**      | How long it coasts after the player lets go, from 0 (instantly) to 10.        |

A heavy boat might take a second or two each way, and a horse half a second. Running into a wall
or a shore stops a vehicle in that direction.

The player presses E next to a vehicle to get in, and E again to get out; a boat has to be next to
dry land. Vehicles stay where the player leaves them, and exits only work once the player has got
off. Vehicle artwork should face east; it's mirrored when travelling west.

The **Puzzles & travel** built-in pack has a rowing boat and a handcart.

## Saves

Projects that use any of these features write save format 7, which adds the time, weather changes,
block and vehicle positions, latched plates, the vehicle being ridden and XP. Saves from earlier
versions still load: the world starts as it would in a new game.
