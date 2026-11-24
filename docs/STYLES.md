# Gameplay styles

Every map plays in one of three styles:

- **Top-down**, the default: the player walks in eight directions over the map.
- **Sidescroller**: a platformer seen from the side, with gravity, jumping, ledges, ladders and
  hazards.
- **First person (raycast)**: the map's walls rise up around the player, who turns and walks
  through it like a classic 90s dungeon.

One game can mix all three. Exits work the same way between maps of any style. To try them, open
`Lantern-Isles.eve` (in `editor/examples/` in the source). On Lantern Isle, the stairs on the west
shore lead down to a sidescrolling cave, and the stairs inside the vault lead to a first-person
crypt.

Choose a map's style in the map inspector under **Gameplay style → Plays as**.

## Movement

Every map, whatever its style, can tune how the player moves. Tick **Movement → Custom movement on
this map** to set:

| Setting                              | What it does                                                          |
| ------------------------------------ | --------------------------------------------------------------------- |
| **Walking speed (pixels a second)**  | From 8 to 512. Starts at the game's speed from **Game → Setup**.      |
| **Seconds to top speed**             | How long it takes to get going, from 0 (instantly) to 10.             |
| **Seconds to stop**                  | How long the player slides after letting go, from 0 (instantly) to 10. |
| **Turning speed (degrees a second)** | First-person maps only, from 30 to 720. The default is 140.           |

A little acceleration (0.1 to 0.2 seconds) makes a platformer feel weighty, and a long stop makes
an icy floor. Running into a wall stops the player in that direction. The run key still multiplies
the walking speed, and vehicles' top speeds are multiples of the map's walking speed. Maps without
custom movement use the game's speed and start and stop instantly, as before.

## What works where

| Feature                                     | Top-down | Sidescroller | First person |
| ------------------------------------------- | :------: | :----------: | :----------: |
| Signs, NPCs, dialogue, shops                |   Yes    |     Yes      |     Yes      |
| Chests, collectibles, doors, keys, switches |   Yes    |     Yes      |     Yes      |
| Quests, events and story flags              |   Yes    |     Yes      |     Yes      |
| NPC patrols                                 |   Yes    |      No      |     Yes      |
| Combat and enemies                          |   Yes    |      No      |      No      |
| Push blocks, pressure plates and vehicles   |   Yes    |      No      |      No      |
| Day and night, weather                      |   Yes    |     Yes      |     Yes      |
| Cutscenes                                   |   Yes    |     Yes      |  Yes, without camera moves |

Studio reports anything a map's style can't use. Player health still applies on sidescroller maps
when the game has combat, since hazards hurt.

## Sidescroller

The player runs left and right, and jumps with the attack key (Space by default) or Up. Holding
jump jumps higher, and tapping it makes a short hop. Jumps are forgiving: the player can still jump
for a moment after running off a ledge, and a jump pressed just before landing happens when they
land.

| Setting                 | What it does                                                         |
| ----------------------- | -------------------------------------------------------------------- |
| **Gravity**             | How quickly the player falls, from 100 to 4000. 900 feels natural.   |
| **Jump height (tiles)** | How high a full jump goes, from a quarter of a tile to 10 tiles.     |

The player walks at the map's walking speed (see [Movement](#movement)), and the run key still works.

### Painting collision

On a sidescroller map, the **Collision** layer has four kinds:

| Kind       | What it does                                                                      |
| ---------- | --------------------------------------------------------------------------------- |
| **Solid**  | Walls, floors and ceilings.                                                       |
| **Ledge**  | Solid only from above. Players jump up through it and land on top; holding Down drops through. |
| **Ladder** | Up and Down climb it. The top of a ladder can be stood on, and jumping lets go.  |
| **Hazard** | Hurts the player (if the game has combat) and puts them back on the last safe ground they stood on. |

The edges of the map are solid, so a pit needs hazard cells at the bottom. Objects with a collider,
such as chests, are solid boxes that the player can stand on.

The **Platforms & caves** built-in pack has sky, cave, grassy ground and rock tiles, plus a ladder,
a wooden ledge and spikes. The ladder, ledge and spikes are see-through, so paint them on the
**Detail** layer over sky or cave tiles, then paint the matching collision kind.

Artwork faces east and west only on these maps, and maps are drawn as they are in the editor:
whatever you paint is the side view.

## First person (raycast)

Up and Down walk forward and back, and Left and Right turn at the map's turning speed (see
[Movement](#movement)). The player can talk to people and use
things that are in front of them and close by.

Every **Solid** collision cell becomes a wall one tile high, covered with the cell's tile (its
detail tile if it has one, otherwise its terrain tile). Objects and characters stand in the world as
flat pictures that always face the player, and solid objects still block the way.

| Setting                   | What it does                                                         |
| ------------------------- | -------------------------------------------------------------------- |
| **Floor colour**          | The colour of the floor.                                             |
| **Ceiling colour**        | The colour of the ceiling.                                           |
| **View distance (tiles)** | How far away walls and objects fade into darkness, from 2 to 64 tiles. |

The floor and ceiling darken towards the horizon. Exits are invisible in first person, so mark them
with a sign, a lamp or a different wall tile. The player arrives facing the direction they were
walking in, so an exit walked into heading north arrives facing north.

Tile artwork for walls works best when it reads as a wall from the front: the **Ancient stone**
pack's dungeon, moss and torch walls are good starting points.

## Saves

Saves work the same on every style. A first-person save remembers which way the player faces,
rounded to north, south, east or west.
