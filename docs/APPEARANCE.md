# Game icons and UI colors

Open **Game appearance** in EveNGINE Studio. These settings are embedded in the `.eve` project, participate in undo/redo and autosave, and travel with native game exports.

## Create or import an icon

- **New blank icon** creates a transparent 64 × 64 canvas. Draw with Pencil, Eraser, or Fill and choose a paint color. One brush stroke is one undo step.
- **Create from sprite** turns selected project artwork into a separate icon. Editing or deleting that sprite afterward does not change the icon.
- **Import PNG** preserves transparency and proportions, padding non-square images to a square. Images are sized to a square canvas from 64 to 512 pixels; larger images are reduced. The input limit is 8 MiB and 4096 × 4096 pixels.
- **Export PNG** saves the current icon as ordinary artwork.
- **Use default icon** removes custom artwork. Games without a custom icon, or with entirely transparent artwork, use the default EveNGINE icon.

The exporter creates the complete Mac icon set, including Retina representations up to 1024 × 1024. Scaling uses nearest-neighbor sampling to preserve pixel-art edges. The app registers its icon for Finder and the Dock. Exporting again rebuilds the icon; a failed export leaves the existing app intact.

## Change the game's UI colors

Edit panel background, text, headings/highlights, hints/controls, selected-choice background, and panel opacity. Use color pickers or enter exact `#RRGGBB` values. Presets include Mosslight default, Midnight blue, and Parchment. **Reset UI colors** restores the original defaults.

The colors apply to the game's HUD, interaction messages, simple and branching dialogue, inventory bag, and quest journal. They do not alter sprite/tile palettes or Studio's own interface. The live preview illustrates the colors; **Playtest** shows your actual game.

Choose **New → Appearance demo · icons and UI colors** for the Keeper's Errand with a gem icon and a blue theme. The editable `Branded-Errand.eve` example contains those settings. Existing projects can use these features without a format migration.

Icons currently target native Mac app exports. Theme settings work in the Studio runtime on its supported platforms. This adds color controls; font selection and UI layout editing remain future features.
