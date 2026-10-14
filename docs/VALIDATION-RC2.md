# RC2 navigation validation

RC2 reorganizes the editor's 14 workspaces into five main sections. Project data and the game runtime are unchanged from RC1.

Verified in the live editor:

- All 14 workspaces open through their main sections and tool navigation.
- Story returns to its last-used Events tool; Artwork returns to Animation.
- Opening a tile with its existing shortcut activates Artwork → Pixel editor.
- The complete Release Candidate sample can be created and validates successfully.
- Five main sections and the contextual tool row fit cleanly in the editor preview.
- The updated navigation script passes its syntax check, and the native Studio bundle builds with the current UI resources.

The packaged game runtime is byte-identical to RC1. Its existing validation baseline is 158 editor/model checks, 12 desktop suites, 10 headless suites, and 10 undefined-behavior sanitizer suites. See VALIDATION-RC1.md for their scope and remaining hardware/manual checks. Those runtime suites were not repeated for this navigation-only change.

A screenshot of the final RC2 navigation is included in the deliverables. The updated ZIP is checked for integrity, embedded navigation resources, current version label, and executable permissions.
