# Adventure guide

This guide covers moving NPCs, shops, combat, equipment and factions. To try them, open
`Adventure-Candidate.eve` in Studio (it's in the download's Templates folder, or `editor/examples/`
in the source). You start with 15 coins: buy bread from the camp trader, follow the keeper's quest,
and fight the raider near camp.

`Frontier-Watch.eve` is the same adventure with the 1.1 additions: buy a sling and stones from the
camp trader, a shield or sword from the grove merchant, and watch the grove's guard fight a raider
who slings stones from a distance. A scout patrols the grove with a walking animation.

## NPC movement

Open **Story → NPCs & trading** and pick a character. Choose a movement style:

- **Patrol**: visits the waypoints in order.
- **Wander**: picks waypoints at random.

Set the walking speed and how long they wait at each stop. Place waypoints by clicking the map or by
typing pixel coordinates (measured from the object's top-left corner).

NPCs find their way around walls and objects. Validation warns you if a waypoint is blocked. If
there's no route, the NPC waits until one opens up. NPCs pause whenever the player is in a
conversation, shop, bag, journal or menu. They use their placed sprite unless you give them
[animations](#animations).

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

## Equipment

In **Story → Items**, choose an **Equipment slot** for an item: **Weapon**, **Armor** or **Charm**.
Player combat must be turned on first (**Game → Setup → Player combat**).

| Field                  | Effect                                                          |
| ---------------------- | --------------------------------------------------------------- |
| **Attack bonus**       | Added to the player's attack damage, for melee and ranged hits. |
| **Defense**            | Taken off every hit the player receives. Hits always do at least 1 damage. |
| **Maximum health bonus** | Raises the player's maximum health while equipped.            |

Each bonus goes from 0 to 99. The player can equip one item per slot. In the bag (**Q**), select an
item and press **E** to equip it, or press **E** again to take it off. The bag marks equipped items
and the HUD shows the current attack and defense. Equipment can't also be a healing item.

Selling, handing over or otherwise losing the last copy of an equipped item takes it off. If that
lowers maximum health, current health is reduced to match.

## Ranged attacks

### Player weapons

Tick **Ranged weapon** on a weapon to make it shoot instead of swing. Set the projectile's speed
(40–400 pixels per second) and how far it flies (16–320 pixels). Optionally choose an
**Ammunition** item: each shot uses one, and the weapon won't fire without it. Ammunition can't be
equipment itself.

Shots travel in the direction the player faces, stop at walls, closed doors and solid objects, and
hit the first enemy in their path. Ranged weapons use the player's attack cooldown.

### Ranged enemies

In **Story → NPCs & trading**, set an enemy's **Attack style** to **Ranged**. It shoots at its
target from up to its attack reach (up to 256 pixels instead of 64) whenever it has a clear line,
and moves closer when it doesn't. Set how fast its projectiles fly.

Projectiles are drawn as small squares: gold for the player's, red for enemies'. They disappear when
the player changes maps.

## Factions

Factions decide who fights whom. Create them in the **Factions** card at the bottom of
**Story → NPCs & trading**, then choose a faction for each enemy.

- **Toward the player**: a **hostile** faction attacks the player on sight. A **neutral** one leaves
  the player alone until the player hits one of its members. That member then stays hostile, and
  this is saved.
- **Hostile to…**: enemies attack members of the factions they're hostile to, using the same
  detection distance, reach and attack style they use against the player. Hostility always works
  both ways.
- Enemies without a faction are always hostile to the player and never fight other enemies.

Enemies only fight other enemies; ordinary NPCs aren't attacked. Loot from an enemy defeated by
another enemy can still be collected by walking up to it.

Up to 16 factions. A faction can't be removed while enemies or other factions refer to it.

## Animations

Moving NPCs and enemies can have their own animations, set up in their card in
**Story → NPCs & trading**:

- **Walk** frames play while the character moves, in the direction it's facing. While standing
  still, it shows the first walk frame for that direction.
- **Attack** frames (enemies only) play once each time the enemy attacks or shoots.
- **Frame duration** applies to every frame (16–5000 ms).
- Each direction can have 1–16 frames. Directions without frames use the placed sprite.
- **Copy player walk frames** fills in the walk frames from the player's walking animation.

Frames are drawn standing on the character's feet, so they can be a different size from the placed
sprite. Collision still uses the placed sprite's collider. Stationary NPCs can't be animated.

Combat also has some built-in feedback: enemies flash when hit, and the player blinks while briefly
invulnerable after taking damage.

## Saving

Adventure saves keep the player's health, NPC positions and routes, defeated enemies, collected loot,
shop stock, equipped items and which neutral enemies have been provoked. Projectiles, animation
timing and event timers aren't saved. Saves from older versions still load. If a shop's goods, NPCs
or equipment change in a way that doesn't match an old save, that save is rejected rather than
loaded partly.

Games with adventure features write save format 6. Older games keep writing format 4.

## Not included yet

Custom projectile sprites, fighting between ordinary NPCs, equipment that changes speed, and
faction-aware dialogue are possible future additions.
