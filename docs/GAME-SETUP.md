# Game setup guide

**Game → Setup** holds project-wide settings: the title screen, menus, controls, the app icon, UI
colours, movement speed and player combat. Each group is a collapsible section. Everything here is
saved in the `.eve` file, can be undone, and goes into exported games.

## Title screen and menus

Set the game's title, subtitle, title artwork (any project sprite) and button labels.

Every exported game has these menus, including games made with older versions:

- **Title screen**: New, Continue (when a save exists), Settings and Quit.
- **Pause menu** (Escape): Resume, Save, Load, Settings, Return to title and Quit. If a
  conversation, the bag or the journal is open, Escape closes that first.

The game asks before throwing away unsaved progress, whether from a menu, loading a save or closing
the window. Closing the window offers **Save and quit**, **Keep playing** or **Discard and quit**. If
saving fails, the game stays open.

## Player settings

The in-game **Settings** menu has music and sound-effect volume, fullscreen, and **Controls**. Use
Left/Right to change volume in 10% steps. These preferences are stored separately from saves, so
starting a new game keeps them.

## Controls

| Action              | Keyboard          | Gamepad (Xbox layout)  |
| ------------------- | ----------------- | ---------------------- |
| Move                | WASD or arrows    | D-pad or left stick    |
| Run                 | Shift             | RB                     |
| Interact / confirm  | E or Enter        | A                      |
| Attack / close      | Space             | B                      |
| Bag                 | Q                 | X                      |
| Quest journal       | J                 | Y                      |
| Pause               | Escape            | Start                  |
| Save                | F5                | Back                   |
| Load                | F9                | —                      |
| Debug overlay       | F3                | —                      |

You can change the default keyboard and gamepad assignments in **Game → Setup**. Assigning a key
that's already in use swaps the two actions.

Players can rebind keyboard keys in **Settings → Controls**: pick an action, then press the new
key (Escape cancels). Duplicate keys aren't allowed. The arrows, Enter and Escape always work in
menus, and Escape is always pause. Gamepad buttons can only be changed in Studio.

On-screen prompts follow the player's most recently used device.

## App icon

Exported Mac games use your icon in Finder and the Dock.

- **New blank icon**: a transparent 64 × 64 canvas to draw on with Pencil, Eraser or Fill.
- **Create from sprite**: copies project artwork into the icon. Later edits to the sprite don't
  affect the icon.
- **Import PNG**: keeps transparency, and pads non-square images to a square. Icons are 64–512
  pixels; bigger images are scaled down. Files can be up to 8 MiB and 4096 × 4096 pixels.
- **Export PNG**: saves the icon as a PNG.
- **Use default icon**: removes your icon. Games without one (or with a fully transparent one) use
  the EveNGINE icon.

The exporter builds every icon size macOS needs, up to 1024 × 1024, scaling with sharp edges to
keep the pixel-art look.

## UI colours

Set the colours of panel backgrounds, text, headings and highlights, hints, and the selected-choice
background, plus panel opacity. Use the colour pickers or type `#RRGGBB` values. **Presets**
include Mosslight (the default), Midnight blue and Parchment. **Reset UI colors** goes back to the
defaults.

The colours apply to the HUD, messages, dialogue, the bag and the journal. They don't change your
artwork or Studio itself. The preview gives a rough idea; **Playtest** shows the real thing.

For an example, choose **New → Appearance demo · icons and UI colors**.

## Movement

Set the player's walking speed and how much faster running is. Running also speeds up the walk
animation.

## Player combat

Turns on player health and melee attacks. See the
[adventure guide](ADVENTURE.md#combat-and-healing).

## Dialogue text speed

Sets how quickly dialogue text appears. 0 shows it instantly. See the
[story guide](STORY.md#portraits-colours-and-text-reveal).
