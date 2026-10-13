# Dialogue and quests in EveNGINE 0.7.0

Choose **New → Story demo · the keeper’s errand** to try the complete loop. Talk to the keeper and accept the errand, open the gates, visit the grove, talk to the trader, collect two trail breads, and return to the keeper. Turning in the errand consumes the bread and awards five gold coins once.

## Author a quest

Open **Quests** and choose **New quest**. Give it a name, description, and one to eight objectives. Choose an optional reward item and quantity.

- **Collect and deliver items:** the bag must hold the requested quantity. Turning in the quest consumes those items.
- **Talk to an NPC:** opening that NPC's conversation after accepting the quest marks the objective complete. Accepting a quest during that NPC's conversation also counts.
- **Visit a map:** entering the map after acceptance marks it complete. The current map counts when the quest is accepted.
- **Activate a chest, door or switch:** the object must have been activated. Objects already activated before acceptance also count.

Objectives can be completed in any order. The quest becomes ready when every objective is satisfied; it completes only when a dialogue choice turns it in. If the reward will exceed the bag's item limit, nothing is consumed and the quest remains ready. A completed quest cannot be restarted or rewarded again.

Item icons and object IDs remain reference-based. Renaming an object updates quest objectives. Remove quest references before deleting referenced items, objects, or maps.

## Author a conversation

Place an NPC and choose **Add dialogue choices** in its inspector, or open **Dialogue**, choose the NPC, and convert its simple pages. Existing page text is preserved as connected nodes.

Choose the starting node. Each node contains up to 180 characters and zero to three choices. Each choice has:

- Text and a destination node, or **End conversation**.
- An optional **Accept quest** or **Turn in quest** action.
- An optional quest-state condition: not accepted, in progress, ready, or completed. In progress includes quests ready to turn in.
- An optional item requirement and quantity. This hides the choice unless the bag has enough; the requirement itself does not consume items.

Use conditions to show an offer before acceptance, a reminder while working, a turn-in when ready, and thanks after completion. A failed quest action leaves the conversation on its current node and explains the problem. A node with no available choices can be closed with E.

Add nodes, then connect them through choices. Every node must be structurally reachable from the start. Loops are allowed; dialogue advances only on player input. The validator checks references and structure, but cannot guarantee that an author's conditions make every branch available during play.

## Play and save

**W/S or Up/Down** selects a choice; **E or Enter** confirms. **Space or Escape** closes the conversation. **J** opens the quest journal; **A/D** changes quest pages. Movement pauses while a conversation, bag, or journal is open. **F5** saves progress; **F9** restores it.

New progress saves use version 3 and preserve accepted quests, event objectives, completion, inventory, and activated objects. Version 2 and legacy progress still load. Version 3 saves cannot be read by v0.6.0. Changing the type, target, quantity, or order of a saved quest's objectives causes loading to reject the stale save without partially changing progress. Keep objective definitions stable for projects with existing player saves.

Quests and dialogue trees are optional. Existing `.eve` and `.lota` projects and their simple page conversations remain supported. New story features require the v0.7.0 runtime. This build includes no quest scripting, timers, failure states, custom variables, combat objectives, or graph-canvas editor.
