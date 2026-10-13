# EveNGINE v0.7.0 development validation

October 14, 2026: a fresh universal Mac build succeeded with Apple Clang 21, CMake 4.4.4, and a macOS 12 deployment target. Apple Silicon execution passed all seven CTest suites in both desktop and headless configurations. Intel slices compiled; Intel execution remains untested.

- 91 editor model checks passed: 15 original, 22 sheet/stamp/audio, 23 built-in asset/interaction, and 31 story checks.
- 37 new native story runtime checks passed, covering acceptance, conditional branches, input, journal pause, all four objectives, item consumption, reward capacity, one-time rewards, progress persistence, corrupt-save atomicity, stale-objective protection, v0.6 saves, and native rendering.
- 13 native story-loader checks passed for malformed quests, nodes, conditions, references, and quantities.
- Browser checks exercised the story template, quest naming and undo, quest/node creation and undo, dialogue controls, validation, and file saving. The saved browser-authored project also passed native runtime validation.
- The packaged editor UI and runtime were byte-compared to the tested source and binary. Studio, the runtime, and the standalone demo contain arm64 and x86_64 slices. Native dialogue/journal previews were rendered and visually checked.

Run the model checks with `node editor/tests/run-model-tests.js`. Build with CMake and run `ctest --test-dir build --output-on-failure` for native checks. The story-loader suite is registered when Python is available.

This is a development test build. The new AppKit editor launch, native file-dialog workflow, and a freshly exported game's full manual playthrough still need hands-on testing. Apps are unsigned and not notarized. Existing projects and v0.6 progress load; new v0.7 progress cannot load in v0.6. See docs/STORY.md for save and objective-editing limits.

## Archived v0.6.0 evidence

# EveNGINE v0.6.0 validation

Built from the included sources. The editor, native runtime, starter and three SDK libraries were compiled and linked for Apple Silicon and Intel Mac and combined into universal binaries. Automated native Mac launch and audible playback were unavailable in the build environment. After delivery, the project owner reported that everything works on Mac; the owner’s Apple Silicon platform is confirmed. Intel execution remains untested.

| Check | Result |
| --- | --- |
| Original JavaScript model | 15 passed |
| Sheet/stamp/audio model | 22 passed |
| Built-in assets and interaction model | 23 passed |
| Original editor GUI | 19 passed, no page errors |
| Sheet/stamp/audio GUI | 32 passed, no page errors |
| Built-in assets and interaction GUI | 25 passed, no page errors |
| C++ exploration | 22 passed |
| C++ audio | 20 passed |
| C++ interactions/progress | 33 passed |
| Project loader compatibility and invalid interactions | 9 passed |
| Headless CTest | 5 suites passed |
| SDL desktop CTest, including dummy audio | 6 suites passed |
| Mac package/resource checks | 54 passed |
| Installed Mac SDK consumer | find_package(EveNGINE 0.6) compiled and linked Core, Platform, Studio and audio calls |
| Editor/native render comparison | 74,832 terrain/object pixels matched exactly |

The interaction checks cover paged NPC dialogue, movement pause, one-time chest rewards and pickups, consumed keys, opened-door collision and artwork, linked switch gates, repeat cue protection, bag controls, item capacity and progress round trips. Loading progress inside an opened doorway succeeds; invalid inventory rejects without partial mutation. Legacy progress loads. GUI checks use actual pointer, dialog, file input and download interactions and save/reopen authored interaction fields.

The loader accepts the new demo, original Mosslight .eve/.lota projects and both GUI-authored projects. Unknown reward items, unknown switch targets, empty NPC pages and incompatible switch-only/key door rules are rejected. Asset checks include exact RGBA remapping, imports with dependencies, edited asset reuse and capacity rejection.

Headless and SDL desktop suites also passed AddressSanitizer and UndefinedBehaviorSanitizer. LeakSanitizer was disabled because this environment restricts process/thread inspection; these results are not a leak-freedom claim.

SDL dummy audio checks cover queue refill, gain, stop/restart, overlapping effects and cleanup. They validate playback data paths, not audible quality. Render parity excludes editor overlays, spawn ghost, HUD and player region. Generic validation does not prove every map area is reachable.

Package checks verify ARM64/x86_64 slices, executable permissions, framework symlinks, Mac 12 deployment, relocatable runtime links, installed headers and version, exact tested UI resources and both sample projects. Delivery archives are integrity-checked after packaging.

Owner-reported Mac validation is successful. Individual manual scenarios and OS version were not recorded, so this is not a separate automated report. Remaining platform check: run the editor and a fresh standalone export on an Intel Mac. Development apps are unsigned and not notarized. Branching dialogue, quest tracking, combat damage, shops and 3DS export are outside this release. The separate Legend of the Ancients project is unchanged.

## Release recovery verification — October 10, 2026

The repository source was recovered from the uploaded v0.6.0 SDK. A fresh universal macOS build of the engine, runtime, starter, editor, and regression executables succeeded with Apple Clang 17 and CMake 4.4.4. All five native CTest suites passed on Apple Silicon. The three JavaScript model suites passed again: 15 original, 22 sheet/stamp/audio, and 23 built-in asset/interaction checks. The audio fixture was extracted from the included Mosslight project.

All three uploaded ZIP archives passed integrity checks. SHA-256 digests for all five downloads match GitHub metadata; SHA256SUMS.txt accompanies the release. The shipped Studio and sample executables contain arm64 and x86_64 slices and declare macOS 12.0 as their minimum version. This verification does not add an Intel execution claim or replace the owner-reported GUI testing above.
