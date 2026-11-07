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

### Without a desktop window (any platform)

The engine, runtime and most tests build without a window or audio device. This is the quickest
setup for working on the engine, and it works on Linux:

```bash
cmake -S . -B build-headless -DLOTA_DESKTOP=OFF
```

```bash
cmake --build build-headless
```

A Linux desktop build needs SDL3 3.2 or later installed. Studio's native app is Mac-only.

### Build options

| Option          | Default | Effect                                                    |
| --------------- | ------- | --------------------------------------------------------- |
| `LOTA_DESKTOP`  | `ON`    | Build the SDL3 desktop backend. `OFF` uses the headless one. |
| `LOTA_SANITIZE` | `OFF`   | Build with address and undefined-behaviour sanitizers.    |

## Running tests

### Engine and runtime

```bash
ctest --test-dir build-headless --output-on-failure
```

Some loader tests need Python 3 and are skipped without it.

### Editor model

The editor's data model is tested with [Node.js](https://nodejs.org/):

```bash
node editor/tests/run-model-tests.js
```

Run this rather than the individual `*.test.js` files; some of them need fixture arguments that it
sets up.

### Editor UI

`editor/tests/ui_test.py`, `features_ui_test.py` and `render_parity.py` drive the editor in a
browser with the Python [Playwright](https://playwright.dev/python/) package. Set `LOTA_CHROMIUM`
to a Chromium executable if Playwright can't find one.

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
