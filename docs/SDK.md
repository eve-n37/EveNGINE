# Using the EveNGINE SDK

Studio covers top-down adventure games without code. When you want different gameplay (a
platformer, a puzzle game, custom rules), you can use the engine's C++ libraries directly.

## Libraries

| CMake target         | Header(s)                                           | What it provides                                                     |
| -------------------- | --------------------------------------------------- | -------------------------------------------------------------------- |
| `EveNGINE::Core`     | `<evengine/core.hpp>`, `<evengine/json.hpp>`, `<evengine/audio.hpp>` | Maths, collision, atlases, the `Canvas` draw list, input, projects and saves, JSON, WAV decoding |
| `EveNGINE::Platform` | `<evengine/platform.hpp>`                           | A window, input, audio and timing backend: SDL3 on desktop, or a headless backend |
| `EveNGINE::Studio`   | `<evengine/studio.hpp>`                             | Optional. The data-driven `StudioGame` runtime that plays `.eve` projects |

Everything lives in the `evengine` namespace. Link only Core and Platform if you're writing your own
game rules. Add Studio only if you want to load and play Studio projects.

## Install the SDK

Build and install from source:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

```bash
cmake --build build --config Release
```

```bash
cmake --install build --prefix sdk
```

Then point your project at it with `-DCMAKE_PREFIX_PATH=/path/to/sdk`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(my-game LANGUAGES CXX)

find_package(EveNGINE 1.1 CONFIG REQUIRED)

add_executable(my-game main.cpp)
target_link_libraries(my-game PRIVATE EveNGINE::Core EveNGINE::Platform)
target_compile_features(my-game PRIVATE cxx_std_17)
```

The package uses same-minor-version compatibility. Ask for the version you installed (`1.1` for
this source tree, `1.0` for the v1.0.0 download).

The published v0.6.0 download includes a prebuilt universal SDK in its `macOS` folder. For that
one, request `find_package(EveNGINE 0.6 CONFIG REQUIRED)`.

## A minimal game loop

```cpp
#include <evengine/core.hpp>
#include <evengine/platform.hpp>

int main() {
  auto atlas = evengine::Atlas::load("content/atlas.ppm");
  auto platform = evengine::createPlatform(atlas, "My Game");
  evengine::Input input;

  double last = platform->seconds(), accumulator = 0;
  while (platform->poll(input)) {
    double now = platform->seconds();
    accumulator += now - last;
    last = now;
    while (accumulator >= evengine::Tick) {
      // Update your game state here using `input`.
      input.consume();
      accumulator -= evengine::Tick;
    }
    evengine::Canvas canvas;
    // Draw sprites, tiles and text into `canvas`.
    platform->present(canvas);
    platform->idle();
  }
}
```

The engine runs at a fixed 60 updates per second (`evengine::Tick`) and renders to a
400 × 240 virtual screen (`evengine::Width`, `evengine::Height`) with 16 × 16 tiles.

For complete examples, see:

- `starter/main.cpp`: a small standalone game that uses Core and Platform.
- `editor/runtime/main.cpp`: the Studio runtime, which drives `StudioGame` with a platform.

## Playing Studio projects from code

`StudioGame` loads a `.eve` file and handles everything Studio authors: maps, interactions,
dialogue, quests, events, shops and combat.

```cpp
evengine::StudioGame game("MyGame.eve");
auto platform = evengine::createPlatform(game.atlas, game.project.title);
// Each tick:   game.update(input, evengine::Tick);
// Each frame:  platform->present(game.draw());
```

Audio from a Studio project is covered in the [audio API guide](AUDIO-API.md).

## Atlases

`Atlas::load` reads binary P6 PPM or P7 RGBA PAM files, so it doesn't need an image library. In PPM
files, magenta (`255,0,255`) is transparent. PAM files support partial transparency. Atlases can be
up to 4096 pixels on each side. `Canvas::text` expects a 6 × 8 ASCII font strip in the atlas; the
starter's atlas includes one. `tools/pack_atlas.py` can pack PNG sprites into an atlas.

## Older names

The engine was originally called LotA. These names still work, so existing code keeps compiling:

| Current                    | Still supported              |
| -------------------------- | ---------------------------- |
| `find_package(EveNGINE)`   | `find_package(LotAEngine)`   |
| `EveNGINE::Core` etc.      | `LotA::Core` etc.            |
| `<evengine/core.hpp>` etc. | `<lota/core.hpp>` etc.       |
| `namespace evengine`       | `namespace lota`             |

`evengine` is an alias of `lota`, so you can mix the two freely.
