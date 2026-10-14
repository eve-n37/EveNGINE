# Changelog

## RC3 update — drag placed assets

- Select / Move drags map objects and exits, preserving grab offset with optional Shift tile snapping.
- One undo step per drag, Escape cancellation, map boundaries, and locked-group protection.

## 1.0.0 RC3 — adventure systems and focused editing

- Context-aware sidebars, unified Game Setup, clickable validation locations, and sound deletion protection for custom events.
- NPC patrol and wander routes with collision-aware movement and editable map waypoints.
- Merchants with item currencies, buy/sell prices, finite or unlimited stock, and atomic inventory transactions.
- Optional player health, healing items, directional melee attacks, pursuing enemies, one-time loot, and defeat recovery.
- Version 5 saves preserve health, actor positions, defeated enemies, claimed loot, and merchant stock; older saves remain readable.
- A playable Keeper’s Adventure sample combines dialogue, quests, movement, trading, and combat.

## 1.0.0 RC2 — editor navigation cleanup

- Reorganized the 14 workspace tabs into five main sections: Maps, Artwork, Story, Audio, and Game.
- Contextual tool navigation groups related workspaces beneath each section.
- Sections remember their last-used tool, and inspector/asset shortcuts update the active navigation.
- All existing editing tools remain available; project and runtime behavior are unchanged.

## 1.0.0 RC1 — combined v0.7/v0.8 candidate

- Customizable title screens and pause menus with New, Continue, Save, Load, Settings, Return to title, and Quit.
- Visible save/load feedback and unsaved-progress confirmation before menu exit or loading.
- Player music/effects volume, fullscreen, controls reference, and persisted keyboard rebinding.
- Named keyboard and gamepad actions, controller menu/dialogue navigation, and prompts that follow the active device and assignments.
- Events triggered by map entry, area entry, or object interaction; item/quest/flag conditions; ordered message, dialogue, item, quest, activation, map-change, flag, and sound actions.
- Saved one-time events and story flags; item reward preflight prevents partial reward sequences when a bag is full.
- Optional dialogue portraits, speaker colors, and typewriter reveal with confirm-to-reveal behavior.
- Four-neighbor autotile rules with 16 editable patterns and baked runtime terrain.
- Per-map object groups with editor visibility, locking, and atomic movement.
- Interactive world overview with map thumbnails, exit connections, draggable arrangement, and map navigation.
- Dedicated authoring workspaces and a complete Release Candidate demo.
- Save format 4 retains event state and reads existing formats 2 and 3.
- Virtual-controller regression coverage caught and fixed a button-name mismatch in the bundled SDL library.

## 0.7.0 — development build

- Game Appearance workspace with PNG icon import, sprite-to-icon creation, pixel painting, fill, erasing, previews, and PNG export.
- Mac exports generate and register complete app icons; older projects receive a default icon.
- Editable game UI colors and panel opacity for the HUD, dialogue, bag, and journal, with live preview and presets.
- Appearance demo with a custom gem icon and Midnight blue UI.

- Branching NPC conversations with up to three choices per node, quest-state and inventory conditions, and quest acceptance/turn-in actions.
- Optional quests with item delivery, NPC conversations, map visits, and object activation objectives, plus one-time item rewards.
- A paged quest journal opened with J, and persisted quest progress with compatibility for older saves.
- Dedicated Quests and Dialogue workspaces, conversion of existing simple pages, and reference-aware editing.
- The Keeper's Errand, a playable two-map story template.
- Native and editor validation for story data, transactional turn-in, and stale-save protection when objective definitions change.
- Editor resources refresh during incremental builds, preventing an updated runtime or UI from being omitted from the app bundle.

## 0.6.0 — October 10, 2026

First public release: EveNGINE Studio, universal Mac runtime and export, built-in artwork, audio, map and pixel tools, NPC dialogue pages, inventory, chests, doors, switches, and the Keys and Gates demo.
