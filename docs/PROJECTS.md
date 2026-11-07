# Legacy starter project format

> This is the original text-based project format. It's used by the C++ starter in `starter/`. For
> new games, use [EveNGINE Studio](STUDIO.md) and `.eve` projects instead.

A starter project has a `.project` file that lists tiles, sprites and scenes, and a `.scene` file
for each map. Maps and artwork can change without recompiling. Gameplay is up to your C++ code; the
engine core doesn't interpret the `mode` field.

`starter/content/` is a complete example.

## General rules

- Files are UTF-8 and line-based.
- Lines starting with `#` are comments.
- Quoted text uses C++ `std::quoted` escaping (`\"` and `\\`).
- Paths are relative to the project file and can't contain spaces.
- Coordinates are in world pixels.

## The `.project` file

```text
id my-game
title "My Game"
mode explore
atlas atlas.ppm
start clearing
tile . 0 0 0
tile ~ 128 0 1
sprite shadow 80 32 16 8 0 0 0 0
sprite tree 256 0 36 48 13 36 8 9
collider tree 8 34 16 4
scene clearing scenes/clearing.scene
```

| Line                                         | Meaning                                                                                    |
| -------------------------------------------- | ------------------------------------------------------------------------------------------ |
| `id NAME`                                    | A unique ID. Save files are kept separately per ID.                                        |
| `title "TEXT"`                               | The window title.                                                                          |
| `atlas FILE`                                 | The image atlas. See [atlases](SDK.md#atlases).                                            |
| `start SCENE`                                | The scene the game starts in.                                                              |
| `tile CHAR X Y SOLID`                        | A 16 × 16 tile at atlas position X, Y. SOLID is 0 or 1.                                    |
| `sprite ID X Y W H BX BY BW BH`              | A sprite's atlas rectangle, plus a collision box relative to its top left. A 0 × 0 box means no collision. |
| `collider ID X Y W H`                        | Adds another collision box to a sprite, for shapes like an L-shaped building.             |
| `scene ID FILE`                              | A map.                                                                                     |

## The `.scene` file

```text
title "A Clearing"
size 4 4
spawn 32 32
tiles
....
....
....
....
object prop tree tree-1 0 0 ""
object sign sign sign-1 48 20 "A readable sign."
portal 0 20 10 20 other-scene 100 100
```

| Line                                     | Meaning                                                                                 |
| ---------------------------------------- | --------------------------------------------------------------------------------------- |
| `size W H`                               | Map size in tiles, up to 256 × 256.                                                     |
| `spawn X Y`                              | Where the player's feet start.                                                          |
| `tiles`                                  | Followed by exactly H rows of exactly W tile characters.                               |
| `object KIND SPRITE ID X Y "TEXT"`       | A placed object. IDs must be unique across the project for saving to work.              |
| `portal X Y W H SCENE X Y`               | A trigger rectangle that moves the player to another scene and position.              |

The loader rejects unknown tiles, missing sprites or scenes, and blocked spawn or arrival points.
Put arrival points outside the destination's own portals.

## Using it in code

Link `EveNGINE::Core`, load the project with `evengine::Project::load`, and use helpers such as
`moveBody`, `blocked`, `cameraFor` and `drawScene`. `drawScene` is a convenience for top-down games;
other genres can draw their own way. See `starter/main.cpp` and the [SDK guide](SDK.md).
