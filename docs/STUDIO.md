# EveNGINE Studio guide

This guide covers the editor itself: making maps and artwork, adding audio, playtesting and
exporting. For gameplay content, see the [story guide](STORY.md), the
[adventure guide](ADVENTURE.md) and the [game setup guide](GAME-SETUP.md).

## Getting started

Open `EveNGINE-Studio.app` and choose **New**. Pick a template:

| Template                                 | What you get                                                    |
| ---------------------------------------- | --------------------------------------------------------------- |
| **Blank map · starter character**        | An empty map with a walking character. The default.            |
| **Exploration sample · two maps**        | Two connected maps to explore.                                  |
| **Interaction demo · keys and gates**    | Keys, a chest, a locked door, a lever and a gate.               |
| **Story demo · the keeper’s errand**     | A branching conversation and a complete quest.                  |
| **Appearance demo · icons and UI colors** | The keeper’s errand with a custom icon and colour theme.        |

To try every feature together, use **Open** on `Adventure-Candidate.eve` (in the download's
Templates folder, or `editor/examples/` in the source).

A project is a single `.eve` file. All artwork, maps, stamps, audio and settings live inside it, so
you can move or share it on its own. Older `.lota` files still open.

## Finding your way around

The editor has five sections. Each has its own tools and remembers the last one you used.

| Section     | Tools                                                    |
| ----------- | -------------------------------------------------------- |
| **Maps**    | Map editor, World overview, Terrain & groups             |
| **Artwork** | Project assets, Built-in library, Pixel editor, Animation |
| **Story**   | Dialogue, NPCs & trading, Quests, Items, Events & flags  |
| **Audio**   | Music, sound effects, event cues and the sound library   |
| **Game**    | Setup                                                    |

The sidebar shows tools for the current section. The inspector on the right edits whatever you have
selected.

## Maps

Select a map in the sidebar. Click **+** beside Maps to add one. Rename, resize or make it the
starting map in the inspector.

### Layers

- **Terrain** is the solid ground layer. Every cell has a tile.
- **Detail** sits on top. Cells can be empty, which makes it useful for flowers, rugs and so on.
- **Collision** marks cells the player can't walk through.
- **Water** marks cells that slow the player down.

Collision and water are separate from the artwork. Turn on **Tile flags** to have terrain painting
also apply each tile's default collision and water settings (off by default).

### Painting

Pick a tile, then use **Brush**, **Erase** or **Fill**. A whole brush stroke undoes as one step.
**Grid** and zoom only affect the editor view.

### Objects, spawn and exits

- **Object**: pick a sprite, choose Object, then click to place it. Set its behaviour (sign, NPC,
  chest, door and so on) in the inspector. See the [story guide](STORY.md#objects-and-interactions).
- **Spawn**: where the player starts. The player's 10 × 7 pixel foot area must be clear.
- **Exit**: a trigger that sends the player to another map. Set its size, destination and arrival
  position in the inspector. For a return trip, add an exit on the other map too.

### Moving placed things

Choose **Select / Move** and drag an object or exit. It keeps the point where you grabbed it.

- Turn on **Snap to grid** to snap to 16-pixel tiles, or hold **Shift** to snap temporarily.
- **Escape** cancels a drag. **Undo** reverses the whole move.
- Objects stay inside the map, and objects in locked groups can't be moved.

### Regions and stamps

Choose **Region** and drag across the map. With **All layers** checked, terrain, detail, collision
and water are all included. Otherwise only the current layer is. Regions can be up to 64 × 64
tiles. Objects, exits and spawns aren't included.

- **Copy**, **Cut** and **Clear** work like you'd expect. Cleared terrain resets to the first tile.
- **Paste** switches to the Stamp tool. Click to place it, on this map or another one. Anything
  outside the map edge is clipped.
- **Save stamp** keeps a named, reusable copy in the project. Click a saved stamp to place it;
  double-click to rename or delete it.
- **Rotate stamp** and **Flip stamp** change the copy you're placing, not the saved original.
- Cmd/Ctrl + C, X and V work on map selections when you're not typing in a text field.

### Autotiles

**Maps → Terrain & groups** lets you create autotile rules. A rule has 16 tile slots, one for each
combination of connected neighbours (N, E, S, W, or "Isolated" for none). Paint with the rule's
brush on the Terrain layer, and edges and corners update automatically as you paint.

- **Fill** follows connected cells that use the same rule.
- **Erase** resets the cell to the first tile and updates its neighbours.
- Painting with a normal brush or pasting a region over autotiled cells replaces them.

Autotiles are saved as ordinary terrain, so the exported game looks exactly like the editor.

### Object groups

Groups belong to one map. Assign objects to a group in the inspector. A group can be hidden or
locked in the editor, or moved one tile at a time. A move is cancelled if any object would leave the
map. Hidden and locked groups still appear normally in the game.

### World overview

**Maps → World overview** shows every map as a thumbnail, with lines for the exits between them.
Drag a map's heading to rearrange the overview, or click **Open map** to edit it. The layout only
affects this view; it doesn't change the game.

## Artwork

### Pixel editor

Double-click any artwork to open it. Click **+** to create a new tile (16 × 16) or sprite (up to
64 × 64).

- **Pencil** and **eraser** draw as you drag. **Fill** fills connected pixels. The **eyedropper**
  picks a colour.
- Drag a rectangle to select. **Copy** and **Paste** use the selection, or the whole image if
  nothing is selected.
- **Flip** and **Rotate** (square artwork only) affect the whole image.
- **Import PNG** loads an image of exactly the same size. **PNG** exports the current artwork.
- The two previews show the artwork at game scale and tiled.

Palette colour 0 is always transparent. Editing a palette colour changes it in every asset that uses
it. Palettes hold up to 256 colours.

A sprite's **collider** is the rectangle the player bumps into, measured from the sprite's top left.
Set the width or height to 0 for a decoration with no collision.

**Duplicate asset** makes an independent copy. **Save object template** writes an `.eve-object` file
with the sprite and collider, which you can **Open** in another project that uses the same palette.

### Project assets

**Artwork → Project assets** lists all your tiles and sprites. Search by name, ID or group and filter
by type or group. Select something to rename it or set its group.

You can delete artwork only when nothing uses it. Map cells, stamps, placed objects, open/closed
sprites, item icons and animation frames all count.

### Importing a sprite sheet

**Import PNG sheet** cuts a sheet into separate tiles or sprites. Set the cell size, outer margin
and spacing between cells. The grid must fit the image exactly. Cells are read left to right, then
top to bottom.

Fully transparent pixels become transparent. Other colours are matched to the palette, or added if
there's room. The import happens all at once and undoes as one step. It's refused if the grid doesn't
fit, the palette would overflow or the project would go over its limits. Sheets can be up to 16 MiB
and 4096 × 4096 pixels.

For sprite sheets, you can also replace an animation's frames during import. Frames are set to
140 ms each; adjust them on the timeline afterwards. The PNG is embedded in the project, so you don't
need to keep the original file.

### Built-in library

**Artwork → Built-in library** has 110 original assets (48 tiles and 62 sprites) in seven packs:
outdoors, interiors, dungeon, nature, village, characters and items. There are also eight map
stamps. Search, filter, and import a single item or everything shown. Imports bring along anything
they depend on (such as open-door sprites or a stamp's tiles) and undo as one step.

**Use explorer character** imports a four-direction walking character and replaces your walk
animations, after asking.

### Animation

Choose **Artwork → Animation** and select a clip. Every project has walk clips for south, north,
east and west. **New clip** adds an idle or attack clip for a direction.

Pick a sprite and click **Add selected sprite** to add it as a frame. Use the timeline buttons to
duplicate, remove and reorder frames, and set each frame's duration in milliseconds. **Onion skin**
shows the previous frame faintly behind the current one.

Walking animations speed up while running. If there's no idle clip, the first walk frame is used.
Attack clips play when the player attacks.

## Audio

Choose **Audio → Import WAV** to add a clip.

| Limit                  | Value                                              |
| ---------------------- | -------------------------------------------------- |
| Format                 | Uncompressed 16-bit PCM WAV, mono or stereo        |
| Sample rate            | 8–48 kHz                                           |
| Each clip              | Up to 60 seconds and 4 MiB                         |
| Per project            | Up to 32 clips and 12 MiB of audio                 |

Convert MP3s and other formats before importing; [Audacity](https://www.audacityteam.org/) works
well for this.

- **Preview** plays a clip once. **Stop preview** stops it.
- **Music** is set per map and loops. Maps that share a track keep playing it across exits.
- **Event cues** play a sound for collecting, attacking, using exits, talking or reading, chests,
  doors and switches.
- Music and sound effects have separate volume levels. Up to eight sound effects can play at once.

You can't delete a clip that's in use; remove its music or cue assignments first. If the player's
computer has no audio device, the game still runs silently.

## Playtest and export

**Validate** checks the project for problems: broken references, blocked spawns and arrivals,
invalid frame timing and so on. Click an issue to jump to it. Validation can't tell whether every
area is actually reachable, so playtest your routes.

**Playtest** runs the current map in the native game runtime, using exactly what's in the editor
(even unsaved changes). Closing Studio also closes its playtests. Press **F3** in a playtest to show
collision, water, the player's feet, exits and frame timing.

**Export app** builds a standalone Mac game (`.app`) after validating the project. The app includes
your project, the runtime, the font and SDL3; it doesn't need Studio, a browser or a compiler. If an
app already exists at that location, macOS asks before replacing it, and a failed export leaves the
old app untouched. Exported games run on macOS 12 or later.

## Saving, undo and recovery

- **Save** (Cmd+S) writes the `.eve` file. The header shows when you have unsaved changes.
- **Undo** (Cmd+Z) and **Redo** (Cmd+Shift+Z) keep up to 60 steps. History is cleared when you
  open a project.
- **Autosave** keeps a recovery copy as you work. **Recover autosave** restores it after asking. It
  only keeps the latest snapshot, so save your projects properly too.

Studio asks before closing if you have unsaved changes.

## Project limits

| Item              | Limit                |
| ----------------- | -------------------- |
| Maps              | 64, up to 256 × 256 tiles each |
| Tiles             | 64 (16 × 16 pixels)  |
| Sprite frames     | 256 (up to 64 × 64 pixels) |
| Stamps            | 64                   |
| Items             | 64                   |
| Animation frames  | 64 per clip          |
| Undo history      | 60 steps             |
| Project data      | 32 MiB               |
