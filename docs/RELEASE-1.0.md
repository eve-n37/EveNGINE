# EveNGINE 1.0.0 release notes

1.0 is the first stable release. It brings together everything built since v0.6: dialogue and
quests, app icons and UI themes, menus and controls, events, new map tools, moving NPCs, shops and
melee combat.

## Highlights

- **Story**: branching dialogue with conditions, quests with four objective types, events and story
  flags. See the [story guide](STORY.md).
- **Adventure**: patrolling and wandering NPCs, merchants, player health, healing and melee combat.
  See the [adventure guide](ADVENTURE.md).
- **Game setup**: title and pause menus, player settings, rebindable keyboard controls, gamepad
  support, custom app icons and UI colour themes. See the [game setup guide](GAME-SETUP.md).
- **Maps**: autotiles, object groups, a world overview and drag-to-move objects with grid snapping.
  See the [Studio guide](STUDIO.md).
- **Editor**: tools grouped into five sections, sidebars that follow the current task, all settings
  gathered in Game → Setup, and clickable validation issues.
- **Safer quitting**: games ask before discarding unsaved progress.

## Try it

Open `EveNGINE-Studio.app`. New projects start from **Blank map**. To try everything at once, open
`Adventure-Candidate.eve` from the Templates folder. `Adventure-Candidate.app` is the same project
exported as a standalone game.

## Compatibility

- Projects made with any earlier version open without changes. New features get sensible defaults:
  standard menus and controls, the default colours, instant text and no events.
- The project format is still `lota-studio` version 1. New fields are optional.
- Games that use adventure features write save format 5; others write format 4. Formats 2–5 all
  load.
- Saves that don't match the current project (for example, after changing quest objectives or
  deleting events) are rejected cleanly instead of loading partially.

## Platform support

- **Official:** Apple Silicon Macs on macOS 12 or later.
- **Unofficial:** Intel Macs. The download includes Intel binaries, but they aren't tested.
- **Not verified:** Windows and Linux desktop packages. The engine builds and tests on Linux
  without a desktop window. (Studio for Linux is coming in 1.1.)
- The apps aren't signed with a Developer ID or notarized.

## Testing

Automated model, runtime, loader, virtual-controller and export checks were run for this build, as
well as hands-on testing on Apple Silicon (launch, menus, fullscreen and the unsaved-progress prompt).
The owner tested a physical controller and older projects. See
[VALIDATION-1.0.md](VALIDATION-1.0.md) for details.
