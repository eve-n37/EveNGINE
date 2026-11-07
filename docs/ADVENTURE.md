# Adventure guide

This guide covers moving NPCs, shops and combat. To try them, open `Adventure-Candidate.eve` in
Studio (it's in the download's Templates folder, or `editor/examples/` in the source). You start with
15 coins: buy bread from the camp trader, follow the keeper's quest, and fight the raider near camp.

## NPC movement

Open **Story → NPCs & trading** and pick a character. Choose a movement style:

- **Patrol**: visits the waypoints in order.
- **Wander**: picks waypoints at random.

Set the walking speed and how long they wait at each stop. Place waypoints by clicking the map or by
typing pixel coordinates (measured from the object's top-left corner).

NPCs find their way around walls and objects. Validation warns you if a waypoint is blocked. If
there's no route, the NPC waits until one opens up. NPCs pause whenever the player is in a
conversation, shop, bag, journal or menu. They use their single sprite while moving; separate
walking animations for NPCs aren't supported yet.

## Shops

Turn on **Shop** for an NPC, choose which item is the currency (for example, coins), and add
things to sell.

| Field     | Meaning                                                         |
| --------- | --------------------------------------------------------------- |
| **Buy**   | What the player pays.                                           |
| **Sell**  | What the merchant pays the player. 0 means they won't buy it.   |
| **Stock** | How many the merchant has. -1 means unlimited.                  |

Selling an item back restocks it. Every transaction checks money, bag space and stock before
anything changes, so a purchase never goes through half-way.

The player opens a shop by interacting with the merchant. The shop shows its own controls for
choosing items and switching between buying and selling. A merchant's shop replaces their dialogue.

## Combat and healing

### Setting it up

- **Game → Setup → Player combat** turns on player health and sets attack damage, reach and
  cooldown.
- **Story → NPCs & trading** turns characters into enemies. Set their health, damage, speed, attack
  reach, how far they'll chase the player, cooldown and an optional item drop.
- **Story → Items** sets how much an item heals.

### How it plays

- **Space** (by default) swings a melee attack in the direction the player faces.
- Enemies chase the player, respect collision, and attack when in range. After being hit, the player
  is briefly invulnerable.
- Defeated enemies drop their loot once. If the bag is full, come back and interact with the enemy
  later to collect it.
- To heal, open the bag (**Q**), select an item with Up/Down and press **E**. Healing items are only
  used if the player is hurt.
- If the player is defeated, **Return to camp** restores their health and keeps inventory, quests
  and world progress. **Load** restores an earlier save instead.

## Saving

Adventure saves keep the player's health, NPC positions and routes, defeated enemies, collected loot
and shop stock. Saves from older versions still load. If a shop's goods or NPCs change in a way that
doesn't match an old save, that save is rejected rather than loaded partly.

## Not included yet

Ranged attacks, equipment stats, factions and combat animations are possible future additions.
