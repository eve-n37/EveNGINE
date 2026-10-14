# NPC movement, trading, and combat

Open the Adventure-Candidate.eve template. New projects default to Blank map. The exported Adventure-Candidate.app starts at the title screen. You begin with 15 coins: buy bread from the camp trader, explore the keeper’s quest, and fight the raider near camp.

## NPC movement

Story → NPCs & trading selects characters on any map. Choose Patrol to visit waypoints in order, or Wander to choose among the authored waypoints. Set speed and the wait at each stop. Pick waypoints on the map or enter their pixel coordinates; coordinates refer to the object’s top-left corner. Validation catches endpoints blocked by terrain or objects. Routing avoids collision; disconnected routes may leave an actor waiting until a route becomes available.

Actors move on the current map and pause while conversations, trading, inventory, journals, and menus are open. Their assigned sprite moves with them; separate NPC walking animation sets are not included in this candidate.

## Shops

Enable a shop on an NPC, select an inventory item as currency, and add merchandise. Buy is the player’s purchase price; Sell is the merchant’s purchase price. Sell 0 disables selling that item. Stock -1 means unlimited. Selling replenishes finite stock. Transactions check funds, inventory limits, and stock before committing any change.

Interact with a merchant using E or your configured confirm action. Use the shop’s displayed navigation controls to select merchandise and switch buying/selling. Back closes the shop. Shops take precedence over that NPC’s dialogue interaction.

## Combat and healing

Game → Setup → Player combat enables health and sets attack damage, reach, and cooldown. Story → NPCs & trading creates enemies and sets health, damage, speed, attack reach, pursuit distance, cooldown, and optional item rewards. Enemies pursue the player and respect map collision. NPC movement settings apply to friendly characters; enemies use pursuit behavior.

Space performs a directional melee attack by default. Enemies can hit within their attack reach; a short invulnerability period limits repeated damage. Defeated enemies award loot once. If inventory is full, approach the defeated enemy and interact to collect pending loot later.

Set an item’s healing amount under Story → Items. Q opens the bag; select an item with Up/Down and press E to use it. Healing items are consumed only when health is below maximum. On defeat, Return to camp restores health while preserving inventory, quests, and world progress. Load can restore a prior save and asks before discarding unsaved progress.

## Saving

Adventure projects use save format 5, preserving health, actor locations and route indices, defeated enemies, claimed loot, and merchant stock. Older save formats are accepted with default adventure state. Changed shop merchandise and incompatible actor records are rejected before modifying the active game. Projects without adventure systems retain format 4.

This is a foundation for melee adventure games. Ranged attacks, equipment statistics, factions, and combat animation sets are future extensions.
