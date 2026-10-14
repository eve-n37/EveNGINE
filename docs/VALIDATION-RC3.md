# RC3 validation

RC3 includes NPC routes, shops, melee combat, healing, defeat recovery, persistent adventure state, contextual sidebars, unified Game Setup, clickable validation, and custom-event sound protection.

## Automated checks

- 192 editor/model checks, including 34 adventure/schema and validation-location checks.
- 14 desktop suites: 13 runtime/model/loader/platform suites plus the production Mac export suite.
- 12 headless suites and 12 undefined-behavior sanitizer suites.
- 46 new adventure runtime assertions covering routes, paused movement, collision, transaction limits, healing, directional combat, pursuit, damage, one-time and pending loot, defeat confirmation, save restoration, and rejected corrupt state.
- 24 malformed adventure project cases rejected by the native loader.
- 36 production export assertions, including the standalone adventure sample, exact project preservation, icons, and embedded runtime validation.

Detailed outputs are in validation-results/rc3-*.txt.

## Editor and package checks

Verified in the live editor preview: creation of the complete adventure sample; character selection and merchant controls; unified Game Setup; contextual sidebars; clicking a blocked-waypoint validation issue to open its character; undoing the invalid edits and successful validation. Screenshots document the trading and setup workspaces.

The macOS Studio, embedded runtime, exported adventure runtime, and SDL framework include arm64 and x86_64 slices. Bundle resources are compared with current source. ZIP integrity, executable permissions, exported project equality, icon metadata, and SHA-256 checksums are checked during packaging.

## Remaining manual checks

The native screen-capture service failed when inspecting the exported sample, so a complete hands-on native RC3 playthrough is not claimed. Engine-rendered previews and automated gameplay checks are separate evidence. Physical Intel execution, physical gamepad testing, and Windows/Linux desktop runs remain unverified. The candidate requires macOS 12 or later.

Actors update on the current map; their assigned sprites currently have no separate NPC walking animation sets. Combat is directional melee. Closing the operating system window exits directly; the pause menu supplies save and unsaved-progress confirmation. This is a local release candidate, pending manual acceptance; it has not been published to GitHub.
