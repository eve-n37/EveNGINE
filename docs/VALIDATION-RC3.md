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

## User verification and remaining checks

The owner reports that the current build works and has verified compatibility with older projects. This is user-reported verification; the earlier assistant native screen-capture failure does not invalidate that acceptance. No additional specific playthrough steps are inferred from the report.

Physical gamepad testing remains pending. Official Mac support is Apple Silicon on macOS 12 or later. Intel compatibility is unofficial and Intel hardware testing is not a release requirement. Windows/Linux desktop runs remain unverified.

Actors update on the current map; their assigned sprites currently have no separate NPC walking animation sets. Combat is directional melee. Closing the operating system window exits directly; the pause menu supplies save and unsaved-progress confirmation. This is a local release candidate, with user acceptance of the current functionality and older-project compatibility recorded; it has not been published to GitHub.
