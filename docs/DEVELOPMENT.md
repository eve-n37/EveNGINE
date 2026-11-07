# Development guide

How to build EveNGINE from source, run the tests and keep the code formatted.

## Building

### Mac (full Studio app)

Install the Xcode Command Line Tools and [CMake](https://cmake.org/download/) 3.20 or later, then:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
```

```bash
cmake --build build --config Release
```

This produces `build/EveNGINE-Studio.app` using the bundled SDL3 framework.

### Linux (full Studio app)

Studio for Linux is a [GTK 3](https://www.gtk.org/) window hosting the editor in
[WebKitGTK](https://webkitgtk.org/). Install a compiler, CMake 3.20 or later, and the GTK, WebKitGTK
and SDL3 build dependencies. On Ubuntu 22.04 or later, or Debian 12 or later:

```bash
sudo apt install build-essential cmake pkg-config libgtk-3-dev libwebkit2gtk-4.1-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev libxkbcommon-dev libwayland-dev wayland-protocols libegl-dev libasound2-dev libpulse-dev libdbus-1-dev libudev-dev
```

Then build:

```bash
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release
```

```bash
cmake --build build-linux
```

This produces `build-linux/evengine-studio` with its `resources/` folder beside it. Run it from
there. If SDL3 3.2 or later isn't installed (most distributions don't package it yet), CMake
downloads and builds the official SDL 3.2.20 source release and checks its SHA-256 hash. Set
`-DEVE_FETCH_SDL=OFF` to require an installed SDL3 instead.

To make a portable download (a `.tar.gz` with Studio, the templates and an exported sample game):

```bash
python3 tools/package_linux.py --build build-linux --output deliverables
```

The package needs at least the glibc version of the machine it was built on, and the script prints
it. Build releases on the oldest distribution you want to support; Ubuntu 22.04 covers glibc 2.35
and later.

### Without a desktop window (any platform)

The engine, runtime and most tests build without a window or audio device. This is the quickest
setup for working on the engine, and it works on Linux:

```bash
cmake -S . -B build-headless -DLOTA_DESKTOP=OFF
```

```bash
cmake --build build-headless
```

### Build options

| Option          | Default | Effect                                                    |
| --------------- | ------- | --------------------------------------------------------- |
| `LOTA_DESKTOP`  | `ON`    | Build the SDL3 desktop backend. `OFF` uses the headless one. |
| `LOTA_SANITIZE` | `OFF`   | Build with address and undefined-behaviour sanitizers.    |
| `EVE_FETCH_SDL` | `ON`    | Download and build SDL 3.2.20 when SDL3 isn't installed.  |
| `EVE_LINUX_STUDIO` | `ON` | Build Studio for Linux when GTK 3 and WebKitGTK 4.1 are found. |

## Running tests

### Engine and runtime

```bash
ctest --test-dir build-headless --output-on-failure
```

Some loader tests need Python 3 and are skipped without it. A Linux desktop build also runs the
SDL platform tests with SDL's dummy drivers, and `linux-studio`, which starts Studio (under
`xvfb-run` when there's no display), drives every native action and plays the exported game.

### Editor model

The editor's data model is tested with [Node.js](https://nodejs.org/):

```bash
node editor/tests/run-model-tests.js
```

Run this rather than the individual `*.test.js` files; some of them need fixture arguments that it
sets up.

### Editor UI

`editor/tests/ui_test.py`, `features_ui_test.py`, `expansion_ui_test.py` and `render_parity.py`
drive the editor in a browser with the Python [Playwright](https://playwright.dev/python/) package.
Set `LOTA_CHROMIUM` to a Chromium executable if Playwright can't find one, and `LOTA_TEST_OUTPUT`
to choose where screenshots go.

```bash
python3 editor/tests/expansion_ui_test.py
```

## Continuous integration

`.github/workflows/linux.yml` runs on pushes to `main`, on pull requests, and on demand:

| Job                    | Runner       | What it does                                                         |
| ---------------------- | ------------ | -------------------------------------------------------------------- |
| Studio build, tests and package | Ubuntu 22.04 | Builds the Linux desktop app, runs `ctest` (including `linux-studio` under Xvfb), packages Studio, checks the unpacked package and uploads it as an artifact. |
| Headless engine with sanitizers | Ubuntu 24.04 | Runs `ctest` on a headless build with ASan and UBSan. |
| Editor model and UI tests | Ubuntu 24.04 | Runs the Node.js model tests and `expansion_ui_test.py` in Chromium. |
| Formatting             | Ubuntu 24.04 | Checks clang-format 18, Prettier 3.8 and Black.                      |

The Linux package is built on Ubuntu 22.04 so it runs on glibc 2.35 and later. Download it from the
run's **Artifacts** section. macOS builds aren't covered yet.

## Code style

Formatting is automatic. Format your changes before committing:

| Files                  | Tool                                                                 | Config              |
| ---------------------- | -------------------------------------------------------------------- | ------------------- |
| C++ and Objective-C    | [clang-format](https://clang.llvm.org/docs/ClangFormat.html)         | `.clang-format`     |
| JavaScript, CSS, HTML  | [Prettier](https://prettier.io/)                                     | `.prettierrc.json`  |
| Python                 | [Black](https://black.readthedocs.io/)                              | defaults            |

```bash
git ls-files '*.cpp' '*.hpp' '*.h' '*.m' | grep -v '^engine/vendor/' | xargs clang-format -i
```

```bash
prettier --write 'editor/ui/*.{js,css,html}' 'editor/tests/*.js'
```

```bash
black editor/tests/*.py editor/assets/build_assets.py tools/*.py
```

`.editorconfig` sets indentation and line endings for editors that support it.

Some files are deliberately left alone:

- `editor/ui/builtins-data.js` and `editor/ui/sample-audio.js` are generated data. Regenerate
  `builtins-data.js` with `python3 editor/assets/build_assets.py` (needs
  [Pillow](https://pypi.org/project/pillow/)) rather than editing it by hand.
- `engine/vendor/` is third-party code (SDL3).

## Documentation

User guides live in `docs/`. When you change how a feature works, update its guide:

| Change to…                                  | Update                 |
| ------------------------------------------- | ---------------------- |
| Editor tools, maps, artwork, audio, export  | `docs/STUDIO.md`       |
| Items, objects, dialogue, quests, events    | `docs/STORY.md`        |
| NPC movement, shops, combat                 | `docs/ADVENTURE.md`    |
| Menus, controls, icons, colours             | `docs/GAME-SETUP.md`   |
| Public C++ API                              | `docs/SDK.md`, `docs/AUDIO-API.md` |

Add user-visible changes to `CHANGELOG.md`. The `VALIDATION*.md` files are records of past test runs
and shouldn't be rewritten.
