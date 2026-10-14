# EveNGINE Studio 0.6.0 guide

Open **EveNGINE-Studio.app**. In the full SDK, it is in the `macOS` folder. The editor starts with a small two-map exploration sample. **New** offers this sample, a blank map with starter assets, or **Keys and Gates**, a two-map interaction demo. **Open** restores a `.eve` file. All artwork, map stamps, audio and definitions live in that one file.

## Maps

Select a map in the left panel. Use **+** beside Maps to add a map; use the inspector to rename it, resize it or make it the starting map.

- **Terrain** is an opaque tile layer. **Detail** is a second tile layer with transparent, empty cells.
- Pick a tile, then use **Brush**, **Erase** or **Fill**. Brush strokes are grouped into one undo action.
- **Collision** paints blocked cells; **Water** paints cells that slow the sample character. They are independent of the artwork. Erase removes these flags. With **Tile flags** enabled, terrain brush/fill also applies a tile’s solid/water defaults; the checkbox starts off.
- **Spawn** places the player's feet. It must leave the 10 × 7 foot collider clear of walls and solid objects.
- Pick a sprite, choose **Object**, then click to place it. **Select** picks placed objects or exits. Use the inspector for exact positions and properties.
- Object behaviours are decoration, collectible, sign, NPC, chest, door and switch. See Interactions below.
- **Exit** places a 16 × 16 trigger. Set its size, destination and arrival position in the inspector. Add a second exit on the destination map for a return route. Validation checks that the arrival is clear.
- **Grid** and zoom affect only the editor view. Maps can be scrolled.

Maps are limited to 256 × 256 tiles, with 16 × 16 tiles. There are two visual layers. A project can hold 64 maps and 64 tile assets.

## Tiles and sprites

Double-click an asset in the left panel, or choose **Tiles & sprites**. **+** creates a tile or sprite. Tiles are 16 × 16; sprites can be up to 64 × 64. The first palette colour is permanently transparent.

Pencil/eraser work while dragging; fill affects connected pixels; the eyedropper selects a colour. Drag a selection from its top left to its bottom right; Copy and Paste use that rectangle. Without a selection, Copy uses the whole image and Paste begins at the top left. Flip affects the full asset. Rotation is available for square assets. Palette colour edits affect every asset using that palette index.

**Import PNG** requires an image with exactly the selected asset's dimensions. It preserves alpha and adds colours up to the 256-colour palette limit. **PNG** saves the selected asset as a PNG. The two previews show the asset at game scale and repeated as a tile.

The inspector's collider rectangle is relative to the sprite's top left. Set width or height to zero for a decorative object without collision. Placed object coordinates are top-left coordinates. The player uses a bottom-centre foot anchor and a 10 × 7 body collider.

**Duplicate asset** makes an independent copy. **Save object template** writes a `.eve-object` file containing the sprite and collider. **Open** can import that template into another project with the same palette. For different palettes, use PNG import instead.

A project can hold 256 sprite frames. Undo also reverses new assets, map resizing, palette edits and object changes.

## Asset browser and sheet import

Choose **Asset browser** to see all tiles and sprites. Search by name, ID or group, and filter by artwork type or group. Select an asset to rename it or assign a group in the inspector. **Edit artwork** or double-click opens the pixel editor. Duplicate creates a separate asset. Deletion is available for unused artwork; map cells, stamps, placed objects, alternate-state sprites, item icons and animation frames all count as references. Deleting an unused tile adjusts later map indexes automatically.

**Import PNG sheet** opens a grid preview. Choose tiles (fixed 16 × 16 cells) or sprites (1–64 pixels per axis), then set cell size, outside margin, cell spacing and the shared name/group. The grid must fit the complete image exactly. Frames are read left to right, then down. Fully transparent pixels map to palette index zero; exact RGBA colours are reused or added. Import is atomic and is one undo action. The importer blocks mismatched grids, palette overflow and asset-limit overflow. Source PNGs are limited to 16 MiB and 4096 × 4096 pixels.

For a sprite sheet, you can replace an existing animation's frames during import. This uses all imported frames in order at 140 ms each; tune the timeline afterwards. Choose **Artwork only** to keep current clips. Clips have at most 64 frames. Imported pixels are embedded; the original PNG does not need to stay beside the project.

## Map regions and reusable stamps

Choose **Region** and drag in either direction. **All layers** copies terrain, detail, collision and water together; uncheck it to copy the selected layer. Regions are limited to 64 × 64 tiles. Objects, exits and spawns are authored separately.

- **Copy** captures the region. **Cut** captures it and clears its copied layers. **Clear** only clears those layers. Terrain clears to the first tile, detail to empty, and flags to zero.
- **Paste** switches to the Stamp tool. Click a map to place the clipboard; choose another map to paste there. Each placement is one undo action. Cells outside the destination map are clipped and reported.
- **Save stamp** stores a named, reusable copy in the project. Click a saved stamp in the sidebar and place it with the Stamp tool. Double-click a saved stamp to rename or delete it.
- **Rotate stamp** rotates the active copy clockwise, including rectangular areas. **Flip stamp** flips it horizontally. Transforming the active copy does not change the stored original.
- Command/Control-C, X and V work on map selections when text fields are not focused.

Stamps retain tile IDs rather than palette positions. A project can hold 64 stamps. Save and reopen preserves them.

## Audio

Choose **Audio**, then **Import WAV**. Supported clips are uncompressed PCM 16-bit WAV, mono or stereo, 8–48 kHz, at most 60 seconds and 4 MiB each. The project can hold 32 clips with 12 MiB of combined WAV data. MP3, compressed/float WAV and other formats must be converted before import. The whole project has a 32 MiB runtime JSON limit.

**Preview** plays a clip once at the effects volume; **Stop preview** ends playback. Rename clips in the list. Remove music and event assignments before deleting a referenced clip.

The music selector applies to the map selected in the sidebar. Each track loops end to start; choose **None** for silence. Seamless source clips give the best loop. Maps sharing a track continue it; switching tracks starts the new one. Seven project-wide event cues can be assigned: collect, attack, exit, talk/read, chest, door and switch. Interaction and attack sounds use button press edges rather than repeating each held frame. Music and effects have independent 0–100% volume controls. The native backend supports eight overlapping effects, replacing the oldest if necessary.

WAV bytes are embedded in the `.eve` file and travel with unsaved playtests and exported apps. The sample contains an original soft meadow loop and a short collection chime. The blank template starts without audio. The game continues silently if a playback device cannot be opened. This milestone uses per-map music and seven fixed cues; event graphs, streaming long music, spatial audio and custom buses are future extensions.

## Built-in assets

Choose **Built-in assets** to browse 110 original assets: 48 tiles and 62 sprites across outdoors, interiors, dungeon, nature, village, characters and items. Eight four-layer map stamps are included. Search and filter by pack or type, then import one asset/stamp or the filtered set. Imports reuse exact palette colours, include alternate-state sprites and stamp tile dependencies, and form one undo action. Imports that exceed project limits are rejected as a whole. Reimporting preserves your edits to existing compatible assets.

**Use explorer character** imports sixteen directional walk frames and replaces the four walking clips after confirmation. It does not replace other animation states. Imported artwork is ordinary editable project content. Source PNGs, pack contact sheets, palette metadata and the artwork generator are in `editor/assets` in the full SDK.

## Items and interactions

Choose **Items** and add a named item with a sprite icon before making a reward chest or keyed door. Projects allow 64 item definitions. Their IDs stay stable when renamed. Referenced items cannot be deleted. Inventory holds up to 999 of each defined item; a reward exceeding that limit stays available rather than silently discarding items.

Place an object and use its inspector to choose a behaviour:

| Behaviour | Authoring and play |
| --- | --- |
| Collectible | Awards the existing score counter; optionally also gives an item and quantity. Disappears after collection. |
| Sign | Displays its interaction text while E is held nearby. |
| NPC | Set speaker and dialogue pages; separate pages with a blank line. E opens/advances, Space or Escape closes. Up to 16 pages, 180 characters each. Dialogue pauses movement. |
| Chest | Choose item and quantity (1–99), plus an optional open sprite. E gives the reward once. |
| Door | Select an optional required item and whether to consume one. **Switch only** gates cannot be opened manually. Activation removes the object collider and may replace its artwork. |
| Switch | Select one or more doors on the same map. E latches the switch and opens all linked doors. It can bypass their key requirements. |

Built-in NPCs, doors, gates and reward props provide useful placement defaults. Chests use your first item; levers link to the first existing door. Without those prerequisites they start as decorations; choose the behaviour after creating an item or door. Editing an object ID updates switch links; deletion of a linked door is blocked until it is unlinked.

Door collision belongs to the placed object. Leave the door cell clear in the painted collision layer; an opened object cannot erase a painted wall. Switches are one-way latches in this release. E acts on the closest interaction within 32 pixels. Movement pauses while the bag or NPC dialogue is open.

Try **New → Keys and Gates**: talk to the keeper, take the bronze key from the chest, open the timber door, pull the lever to open the iron gate, and follow the exit to the second map. F5/F9 saves and restores progress. Older Studio progress files continue to load; they start with no new inventory or activated interactions.

## Animation

Choose **Animation**, then select a clip. Walk clips exist for south, north, east and west. **New clip** adds an optional idle or attack clip for a direction. Each state/direction pair has one clip.

Pick a sprite in the library and use **Add selected sprite**. Duplicate/remove/reorder frames with the timeline buttons; edit the selected frame duration in milliseconds. Onion skin shows the previous frame. Preview uses the same frame durations that the runtime reads.

Walk animation speeds up by the project's run multiplier. Idle falls back to the first walk frame if no idle clip is defined. Space plays an authored attack clip and its assigned sound; a sound can play without an animation. The generic sample does not implement combat damage.

Project settings edit the title, walking speed and running multiplier.

## Playtest and export

**Validate** checks data, asset references, frame timing, collider bounds, spawns and exits. It checks data validity, not whether every area can be reached. Build a route yourself and playtest it.

**Playtest** validates again in C++ and launches the selected map with its spawn. The playable window uses exactly the project currently in the workspace, even before saving. Unsaved playtest content is held in a temporary folder.

Controls: WASD or arrows to move, Shift to run, E to interact/advance dialogue, Q or I to open the bag (A/D changes bag pages), Space to play attack animations or dismiss dialogue, F3 to show collision, water, feet, exits, coordinates and frame interval, F5 to save progress, F9 to reload, Escape closes dialogue/bag first, then closes the game. Progress saves include inventory, collected pickups, opened chests/doors and latched switches, and are kept separately by project ID. Close the playtest when done; closing the editor also closes its active playtests.

**Export app** validates through the same C++ loader, asks where to save and produces a standalone universal Mac `.app`. The app contains project data, packed-at-load artwork, the native runtime, font and SDL3 framework. A compiler is not required. Export replaces an existing app only after the native save dialog's replacement confirmation; the new package is staged before committing. Exported games target macOS 12+ and can be launched without the editor.

The exported sample uses the generic exploration behaviours; C++ game-specific code needs a source build. Authoring/code tools are separate from the runtime.

## Save, undo and recovery

Use **Save** or Command-S to keep the portable `.eve` file. A filled Save indicator means the current workspace has been saved. Unsaved changes are marked in the header.

Undo/redo use the toolbar or Command-Z / Command-Shift-Z. History keeps up to 60 edits within a 64 MiB snapshot budget and is reset when opening a project.

Autosave writes recovery data after edits. **Recover autosave** restores the last snapshot; it asks before replacing your workspace. On Mac it retains the existing Application Support/LotA Studio location for compatibility. Save manually to keep multiple projects: recovery is one latest snapshot, not an archive. If a save or validation fails, the status line explains the error.

## Current boundary

This release adds original built-in artwork and basic inventory/interactions alongside reusable asset/map workflows and audio. Existing `.project` / `.scene` SDK content and the RPG's baked `world.json` are not editable `.eve` projects yet. The original C++ starter remains included; the existing Legend of the Ancients build is unchanged. Quest tracking, branching dialogue, shops, equipment, RPG data and UI layout editing are future modules. A full visual scripting system and 3DS export are outside this release.

Existing `.lota` and `.lota-object` files still open. New saves use `.eve` and `.eve-object`. The compatible data format and project IDs are preserved.


## v0.7.0 dialogue and quests

Use the new Quests and Dialogue tabs to author story content. Choose the Keeper’s Errand template for a complete playable example. See [STORY.md](STORY.md) for authoring, controls, objectives, rewards, and save compatibility.


## Game icons and UI colors

Use the Game appearance tab to draw or import an app icon and edit the game’s UI colors. Native Mac exports include the icon automatically. See [APPEARANCE.md](APPEARANCE.md) for controls, presets, import limits, and the appearance demo.
