# RC1 validation

Validated on the local Apple Silicon Mac on October 14, 2026. These results apply to the combined v1.0.0 RC1 source and its packaged Mac runtime.

| Check | Result |
| --- | --- |
| Editor/model regression checks | 158 passed, including 45 RC-specific checks |
| Universal desktop CTest suites | 12 of 12 passed |
| Headless CTest suites | 10 of 10 passed |
| Undefined-behavior sanitizer suites | 10 of 10 passed |
| RC player-flow assertions | 54 passed |
| Actual SDL keyboard/virtual-controller assertions | 20 passed |
| Malformed RC project loader cases | 34 rejected as expected |
| Production export and app-icon assertions | 32 passed |
| Existing project and progress compatibility | Original samples pass; save formats 2 and 3 load |
| Editor script syntax and whitespace checks | Passed |

The desktop tests include the production Mac exporter and independent validation by the runtime inside the exported RC app. Icon checks cover all ten representations, transparency, orientation, a default icon for older/blank projects, and preservation of an existing export after failure.

The SDL tests exercise rebinding, raw key capture, reserved menu keys, gamepad button assignments, device-dependent prompt mode, analog movement, and controller removal. They use a virtual gamepad with SDL's actual input backend. They caught a real button-name mismatch in the bundled SDL framework; explicit button-enum mapping fixed it.

The new runtime checks cover title/Continue behavior, world pausing, unsaved exit/load confirmation, successful and failed save/load feedback, persisted settings, text reveal before choice selection, one-time events across map reentry and saves, atomic full-bag rejection, switch activation, map changes, NPC dialogue actions, repeatable area entry, interaction overrides, and New game resets.

Native/manual checks on Apple Silicon: exported game launch; title → New game → Pause; settings navigation; fullscreen on/off; unsaved-progress confirmation. Browser checks: create the complete RC sample, validate it, inspect event authoring, and view the autotile and world workspaces. A screenshot of the final world workspace is included in the deliverables.

Not established by these checks: physical-controller behavior, Intel hardware execution, Windows/Linux desktop builds, a complete manual playthrough, every editor pointer interaction, or final user acceptance. The candidate is not Developer ID signed or notarized. Closing the operating system window exits directly; the pause-menu exits provide the unsaved-progress confirmation.

The universal package contains arm64 and x86_64 slices for Studio, the embedded runtime, the exported sample runtime, and SDL. Universal binary inspection verifies those slices; it does not substitute for an Intel hardware run. ZIP contents, executable permissions, embedded resource equality, icon registration, and checksum verification are checked during final packaging.

Detailed logs are in `validation-results/rc1-model-checks.txt`, `rc1-desktop-tests.txt`, `rc1-headless-tests.txt`, `rc1-ubsan-tests.txt`, and `rc1-detail-tests.txt` in the source archive.
