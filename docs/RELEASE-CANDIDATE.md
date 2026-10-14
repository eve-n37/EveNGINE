# EveNGINE 1.0.0 RC1

This candidate combines the planned v0.7 and v0.8 additions into one build. It is ready for testing as a release candidate; the public GitHub release remains v0.6.0.

## Try it

Open EveNGINE-Studio.app. Choose **New → Release candidate demo · menus, events & map tools**, or open the included `Release-Candidate.eve` project. Playtest starts directly on your selected map. The included `Release-Candidate.app` opens at its title screen and runs independently of Studio.

The sample includes the keeper's branching quest, a custom gem icon, Midnight blue UI, portraits and text reveal, a welcome event, a one-time grove coin reward, a repeatable camp area event, a path autotile rule, scenery groups, and a saved world layout.

## Menus and player settings

**Menus & controls** lets you set the game title, subtitle, title artwork from a project sprite, and button labels. The title screen offers New, Continue when a compatible save exists, Settings, and Quit. Every exported Studio game has these menus, including older projects.

Escape opens Pause. Resume, Save, Load, Settings, Return to title, and Quit are available there. Escape first closes an open conversation, bag, or journal. Menu exits and loading ask before discarding unsaved progress. Save and load success or failure appears on screen. Closing the operating system window exits directly; use the pause menu to save or request the unsaved-progress check.

Settings provides music and effects volume, fullscreen, and Controls. Left/Right adjusts volume in 10% steps. Enter or the confirm action selects. The game stores player preferences separately from story progress, so New game keeps the player's settings and key assignments. The runtime multiplies player volume by the project's audio mix.

## Controls

Defaults: WASD or arrows move, Shift runs, E/Enter interacts, Space attacks or closes dialogue, Q opens the bag, J opens the journal, Escape pauses, F5 saves, F9 loads, and F3 shows collision/debug information. Studio can set default keyboard and gamepad assignments in **Menus & controls**. Choosing a used assignment swaps the two actions.

Players can rebind keyboard actions through Settings → Controls. Select an action and press a new key; Escape cancels. Duplicate keys are rejected. Arrows, Enter, and Escape remain available as menu fallbacks. Escape's pause binding stays reserved.

Gamepads use D-pad/left stick for movement and menu navigation. Defaults use Xbox notation: A confirms, B attacks/back, X opens the bag, Y opens the journal, RB runs, Start pauses, and Back saves. Prompts change with the most recently used input device and follow the project assignments. Gamepad buttons are configured in Studio; the player's in-game rebinding screen changes keyboard keys.

## Events and story flags

Use **Events** to add named boolean flags and event sequences. Trigger an event when entering a map, entering a rectangular area, or interacting with an object. Areas use map pixels. Repeatable area events run again after the player leaves and reenters; they do not fire every frame.

Conditions all need to match: a story flag's value, an inventory quantity, or a quest state. Available means not accepted; Active includes ready to turn in; Ready and Completed match those specific states.

Actions run in their listed order:

- Show a message or open an NPC conversation.
- Give an item or start a quest.
- Open a door/chest or activate a switch and its linked doors.
- Change maps using a valid arrival position.
- Set a story flag or play an embedded sound.

**Run only once** saves the event's completion with game progress. The runtime checks the whole sequence's item rewards against the 999-per-item limit before changing any flags or awarding items. A full bag leaves the sequence available to retry. Multiple message actions display the final message; multiple dialogue actions leave the final conversation open.

An authored interaction replaces the object's built-in interaction when its conditions match. A completed one-time interaction remains handled, preventing fallback chest rewards. When conditions do not match, the ordinary object interaction is available. Decoration and collectible objects can also become explicit interaction triggers.

The current limits are 128 events, 128 flags, 16 conditions per event, and 32 actions per event. There is no arbitrary scripting or timer trigger in this candidate.

## Dialogue presentation

Choose an NPC in **Dialogue** to assign a portrait and an optional speaker color. Set the project's text reveal speed in **Menus & controls**; 0 shows text immediately. Confirm first finishes revealing text, then the next confirm follows the selected choice or advances the page. Choices appear after the text is revealed. The portrait comes from the sprite library, so PNG artwork can use the existing sprite import workflow.

## Map tools

**Autotiles & groups** defines four-neighbor terrain rules. Each of the 16 patterns has a tile assignment. N/E/S/W indicate connected neighbors; Isolated has none. Use the rule's brush on the Terrain layer. Painting updates neighboring tiles automatically. Fill follows connected cells with the same rule. Erase detaches the cell, resets its terrain to the first tile, and updates its neighbors. A normal brush or a pasted region replaces autotile annotations where it paints. **Tile flags** can apply the resulting tiles' collision and water defaults.

Autotiles bake into ordinary terrain, so exported games use the exact map shown in Studio. The sample path rule initially uses the same pebble artwork for all patterns; assign different edge and corner tiles to customize it.

Object groups belong to one map. Assign an object using its inspector. Group controls can hide scenery in the editor, lock editing, and move every member by one tile at a time. Moves are rejected as a whole if any object would leave the map. Hidden and locked groups still appear and behave normally in exported games.

**World** shows thumbnails and connections for map exits. Drag each map's heading to arrange the overview; use Open map to edit it. The arrangement saves with the project and does not move the game maps or change their exits.

## Compatibility and verification

Project format remains `lota-studio` version 1. New authoring fields are optional. Existing projects receive default menus, controls, UI colors, instant text, and no events. Current saves use format 4 and load Studio save formats 2 and 3. New progress includes quests, flags, and completed one-time events. Changed quest objectives or missing saved event/flag definitions reject incompatible progress without partially modifying the current session. Renaming or deleting story definitions during development can invalidate existing saves.

RC1 is built for macOS 12 or later, with arm64 and x86_64 binaries. Automated model, runtime, loader, virtual-controller, and production export checks accompany the build. Native Apple Silicon checks covered app launch, title and pause navigation, fullscreen toggling, and the unsaved-progress confirmation. Physical-controller testing, execution on Intel hardware, and Windows/Linux desktop packaging remain unverified. The Mac candidate is not Developer ID signed or notarized.

See `VALIDATION-RC1.md` for the final check results. Before declaring a stable v1.0, play through the included game and try an existing project through edit → playtest → export → save → relaunch → continue.
