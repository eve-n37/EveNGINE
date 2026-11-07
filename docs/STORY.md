# Story guide

This guide covers everything the player can interact with: items, objects, dialogue, quests, and
events. To try them together, choose **New → Story demo · the keeper’s errand** in Studio.

In the demo, talk to the keeper and accept the errand, open the gates, visit the grove, talk to the
trader, collect two trail breads and return to the keeper. Turning in the errand uses up the bread
and gives you five gold coins.

## Items

Choose **Story → Items** and add an item with a name and an icon sprite. You need at least one item
before you can make a reward chest or a locked door.

- A project can have up to 64 items.
- The player can carry up to 999 of each item. If a reward would go over that limit, it stays
  available rather than disappearing.
- An item's ID doesn't change when you rename it. Items that are in use can't be deleted.
- Items can heal the player; see the [adventure guide](ADVENTURE.md#combat-and-healing).

## Objects and interactions

Place an object on a map, then choose its **behaviour** in the inspector:

| Behaviour       | What it does                                                                               |
| --------------- | ------------------------------------------------------------------------------------------ |
| **Decoration**  | Nothing. Just scenery.                                                                     |
| **Collectible** | Adds to the score and can give an item. Disappears once collected.                         |
| **Sign**        | Shows its text when the player interacts with it.                                          |
| **NPC**         | Has a speaker name and dialogue. Simple dialogue is pages of text separated by blank lines (up to 16 pages of 180 characters). |
| **Chest**       | Gives an item (1–99 of it) once, and can switch to an "open" sprite.                        |
| **Door**        | Can require an item (a key), and optionally use it up. **Switch only** doors can only be opened by a switch. |
| **Switch**      | Opens one or more doors on the same map, ignoring their key requirements. Switches stay on once pulled. |

When a door opens, its collision is removed and its sprite can change. Leave the door's cell clear
in the painted collision layer, since opening a door can't remove a painted wall.

Built-in NPCs, doors, gates and chests come with sensible defaults when placed. Renaming an object's
ID updates any switches that link to it. A door that's linked to a switch can't be deleted until you
unlink it.

The player interacts with the closest object within 32 pixels. Try **New → Interaction demo · keys
and gates** for a working example.

## Dialogue

Simple NPC pages are fine for one-off lines. For conversations with choices, place an NPC and click
**Add dialogue choices** in its inspector, or open **Story → Dialogue** and pick the NPC. Any
existing pages are kept as connected nodes.

A conversation is a set of **nodes**. Each node has up to 180 characters of text and up to three
choices. Each choice has:

- **Text**, and either a node to go to next or **End conversation**.
- An optional quest action: **Accept quest** or **Turn in quest**.
- An optional quest condition: not accepted, in progress, ready or completed. "In progress" includes
  quests that are ready to turn in.
- An optional item requirement. The choice is hidden unless the player has enough of the item. It
  doesn't use the items up.

A typical quest-giver uses conditions to show an offer before the quest starts, a reminder during
it, a turn-in option when it's ready, and thanks afterwards.

Every node must be reachable from the starting node. Loops are fine. Validation checks the
structure, but it can't tell whether your conditions leave every branch reachable in play, so
playtest it.

### Portraits, colours and text reveal

In **Story → Dialogue**, give an NPC a portrait (any sprite) and an optional colour for their name.
Set how quickly text appears in **Game → Setup**; 0 shows it instantly. While text is appearing,
pressing confirm shows it all at once. Choices appear once the text is complete.

## Quests

Open **Story → Quests** and click **New quest**. Give it a name, a description, one to eight
objectives, and an optional reward item.

| Objective                       | Completed when…                                                                         |
| ------------------------------- | --------------------------------------------------------------------------------------- |
| **Collect and deliver items**   | The player has enough of the item. Turning in the quest takes them.                     |
| **Talk to an NPC**              | The player talks to that NPC after accepting the quest (or accepted it from that NPC).  |
| **Visit a map**                 | The player enters that map after accepting, or was already there.                       |
| **Activate a chest, door or switch** | The object has been activated, even before the quest was accepted.                  |

Objectives can be done in any order. Once they're all done the quest is **ready**, and it completes
when a dialogue choice turns it in. If the reward wouldn't fit in the bag, nothing is taken and the
quest stays ready. Completed quests can't be restarted.

Renaming an object updates the quests that refer to it. Remove quest references before deleting
items, objects or maps they use.

## Events and story flags

**Story → Events & flags** lets you make things happen without code.

**Flags** are named on/off values that remember story progress (for example, `met_the_keeper`).

An **event** has a trigger, optional conditions, and a list of actions.

**Triggers**

- Entering a map.
- Entering a rectangular area (measured in map pixels). Repeatable area events fire again only after
  the player leaves and comes back.
- Interacting with an object.

**Conditions** (all must match)

- A flag is on or off.
- The player has a certain number of an item.
- A quest is available (not yet accepted), active (including ready), ready or completed.

**Actions** (run in order)

- Show a message, or open an NPC conversation.
- Give an item, or start a quest.
- Open a door or chest, or activate a switch and the doors it links to.
- Move to another map at a given position.
- Set a flag, or play a sound.

Tick **Run only once** for events that should only ever happen once. That's saved with the player's
progress. Before running, the game checks that every item reward fits in the bag. If it doesn't, the
event doesn't run and can be tried again later.

If an event is triggered by an object, it replaces the object's normal behaviour whenever its
conditions match. When they don't match, the object behaves normally. Decorations and collectibles
can be used as event triggers too.

Limits: 128 events, 128 flags, 16 conditions per event and 32 actions per event. There's no
scripting language or timer triggers.

## Playing

| Action                       | Keyboard                 |
| ---------------------------- | ------------------------ |
| Interact / continue          | E or Enter               |
| Choose a dialogue option     | W/S or Up/Down           |
| Close a conversation         | Space or Escape          |
| Bag                          | Q                        |
| Quest journal                | J (A/D to change pages)  |

The player can't move while a conversation, the bag or the journal is open. See the
[game setup guide](GAME-SETUP.md#controls) for all controls and gamepad buttons.

## Saved progress and changing your story

Saves keep accepted quests, objectives, completed events, flags, inventory and activated objects.

Be careful about changing a quest's objectives (their type, target, quantity or order), or renaming
or deleting events and flags, once players have saves. Saves that no longer match the project are
rejected rather than loaded half-way. This mostly matters during development: delete your own test
saves after big story changes.
