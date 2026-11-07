# EveNGINE 1.1.0 release notes

1.1 brings EveNGINE Studio to **Linux** and adds equipment, ranged combat, factions, character
animations and timed events. Projects and saves from 1.0 open unchanged.

## Downloads

| File                                          | What's inside                                                      |
| --------------------------------------------- | ------------------------------------------------------------------ |
| `EveNGINE-Studio-v1.1.0-macOS.zip`            | Studio for macOS 12 or later (Apple Silicon and Intel) and editable templates |
| `EveNGINE-Studio-v1.1.0-linux-x86_64.tar.gz`  | Studio for Linux, editable templates and the Frontier Watch sample game |
| `SHA256SUMS.txt`                              | Checksums for the two downloads                                    |

The source code is attached by GitHub as **Source code (zip)** and **Source code (tar.gz)**.

## Highlights

### Studio for Linux

- The same editor as on a Mac, in a GTK 3 window using WebKitGTK. Open, save, autosave recovery,
  templates, PNG export, playtests and **Export app** all work.
- **Export app** on Linux makes a game folder with its own copy of SDL3. Players don't need Studio
  or WebKitGTK, and a script in the folder adds the game to their application menu.
- Needs x86_64 Linux with glibc 2.35 or later (Ubuntu 22.04, Debian 12, Fedora 36 and newer) and
  WebKitGTK 4.1: `sudo apt install libwebkit2gtk-4.1-0` or `sudo dnf install webkit2gtk4.1`.
- Run `./install-desktop-entry.sh` in the unpacked folder to add Studio to your application menu
  and open `.eve` projects with it.

### Adventure features

- **Equipment**: weapons, armor and charms with attack, defense and maximum-health bonuses, equipped
  from the bag.
- **Ranged attacks**: ranged weapons fire projectiles and can use up ammunition. Enemies can shoot
  too.
- **Factions**: decide who fights whom. Neutral enemies leave the player alone until attacked, and
  rival factions fight each other.
- **Animations**: walking animations for moving NPCs and enemies, attack animations for enemies,
  and hit flashes.
- **Timed events**: run an event after time spent on a map, once or repeatedly.

See the [adventure guide](https://github.com/eve-n37/EveNGINE/blob/v1.1.0/docs/ADVENTURE.md) and
the [story guide](https://github.com/eve-n37/EveNGINE/blob/v1.1.0/docs/STORY.md#events-and-story-flags).

## Try it

Open `Frontier-Watch.eve` from the Templates folder. Buy a sling and stones from the camp trader,
then head to the grove, where the watch is fighting a stone-slinging raider. On Linux, the
`Frontier Watch` folder is the same project exported as a standalone game.

## Compatibility

- Projects made with 1.0 or earlier open without changes. The project format is still `lota-studio`
  version 1, and every new field is optional.
- Games with adventure features now write **save format 6**, which adds equipped items and
  provoked enemies. Formats 2–6 load in 1.1, but 1.0 can't read format 6.
- C++ SDK: request `find_package(EveNGINE 1.1 CONFIG REQUIRED)`. The package uses same-minor-version
  compatibility. See the [SDK guide](https://github.com/eve-n37/EveNGINE/blob/v1.1.0/docs/SDK.md).

## Platform support

- **Official:** Apple Silicon Macs on macOS 12 or later.
- **New:** Linux x86_64 (see above). It's tested automatically on every change, including under a
  virtual X11 display, but not yet on a wide range of desktops, Wayland sessions or gamepads.
- **Unofficial:** Intel Macs. The macOS download includes Intel binaries, but they aren't tested.
- **Not supported:** Windows.
- The apps aren't signed with a Developer ID or notarized. On a Mac, Control-click the app and
  choose **Open** the first time.

## Testing

Both downloads are built by GitHub Actions from the tagged commit, after the full test suite passes
on Linux and macOS: engine and runtime tests (also with address and undefined-behaviour
sanitizers), loader validation, editor model and browser UI tests, an end-to-end Linux Studio test
that exports and runs a game, and macOS bundle checks for both architectures and the macOS 12
minimum.

The full list of changes is in the
[changelog](https://github.com/eve-n37/EveNGINE/blob/v1.1.0/CHANGELOG.md). Notes for 1.0 are in
[RELEASE-1.0.md](https://github.com/eve-n37/EveNGINE/blob/v1.1.0/docs/RELEASE-1.0.md).
