# Version 1.0 validation

RC4 fixes the percent glyph, protects unsaved game progress on window/application close, defaults New projects to Blank map, and removes Release candidate demo from that menu. The editable adventure example remains in Templates.

## Checks

- 192 editor/model checks.
- 14 desktop suites, including production export; 12 headless and 12 undefined-behavior sanitizer suites.
- 63 release-menu/runtime assertions, including nine RC4 checks: close confirmation defaults to cancel, repeated close requests, restoring Settings, cancellation with the bag open, failed-save protection, successful save and quit, explicit discard, clean closing, and percent glyph/text-cell rendering.
- SDL platform checks exercise window-close and application-quit interception, consuming requests once, and legacy immediate-close behavior.
- Existing adventure and production export checks remain passing. Detailed output is in validation-results/rc4-*.txt.
- Package checks verify RC4 labels, blank-first template order, removal of the RC demo menu entry, updated font bytes, current embedded runtime, ZIP integrity, and checksums.

## User verification and support

The owner has verified current functionality, older-project compatibility, and physical-controller operation. These are user-reported results. The new RC4 close prompt and percent fix have automated coverage and rendered-preview inspection; the owner subsequently confirmed that the RC4 checks looked good and approved release.

Official Mac support is Apple Silicon on macOS 12 or later. Intel compatibility remains unofficial and is not a release requirement. Windows/Linux desktop packaging is unverified. The owner accepted RC4 for release after testing the new behavior. Version 1.0 uses the accepted runtime and font, with release labels and documentation updated. The Mac build is not Developer ID signed or notarized.

Studio already checks unsaved editor changes when closing. RC4 adds save/cancel/discard to the game runtime’s window and application quit requests. Failed game saves leave the prompt open.
