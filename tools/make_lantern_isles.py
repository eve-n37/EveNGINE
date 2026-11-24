#!/usr/bin/env python3
"""Build editor/examples/Lantern-Isles.eve, the 1.2 sample, from Frontier-Watch.eve.

It adds a third map with a lake to cross by boat, a block-and-plate puzzle and a fog bank, plus
day and night with a clock, rain in the grove, leveling, a handcart and an opening cutscene.
Run it again after changing Frontier-Watch.eve or the built-in artwork."""

import json
from pathlib import Path

root = Path(__file__).resolve().parent.parent
examples = root / "editor/examples"
library = json.loads((root / "editor/assets/library.json").read_text())
p = json.loads((examples / "Frontier-Watch.eve").read_text())

p["id"] = "lantern-isles"
p["title"] = "Lantern Isles"
p["export"] = {"appName": "Lantern Isles", "version": "1.0.0"}
p["menus"]["title"] = "Lantern Isles"


def import_sprite(asset_id, into="sprites"):
    """Copies a built-in sprite or tile, remapping its colours onto the project palette."""
    if any(s["id"] == asset_id for s in p[into]):
        return
    asset = next(a for a in library["assets"] if a["id"] == asset_id)
    pixels = ""
    for i in range(0, len(asset["pixels"]), 2):
        colour = library["palette"][int(asset["pixels"][i : i + 2], 16)]
        if colour not in p["palette"]:
            p["palette"].append(colour)
        pixels += "%02x" % p["palette"].index(colour)
    sprite = {
        k: asset[k] for k in ["id", "name", "group", "width", "height", "collider"]
    }
    sprite["pixels"] = pixels
    if into == "tiles":
        del sprite["group"]
    p[into].append(sprite)


for name in ["push-block", "plate", "plate-down", "boat", "cart"]:
    import_sprite("eve-" + name)
for name in [
    "sky",
    "cave-back",
    "ground-top",
    "rock-block",
    "ladder",
    "ledge",
    "spikes",
]:
    import_sprite("eve-" + name, "tiles")

tile = {t["id"]: i for i, t in enumerate(p["tiles"])}
W, H = 32, 20
base = [tile["eve-meadow"]] * (W * H)
collision = [0] * (W * H)
water = [0] * (W * H)


def cell(x, y, t, solid=0, wet=0):
    base[y * W + x] = tile[t]
    collision[y * W + x] = solid
    water[y * W + x] = wet


for x in range(W):
    for y in range(H):
        if x in (0, W - 1) or y in (0, H - 1):
            cell(x, y, "eve-dark-grass", 1)
# The lake: walls to walkers, open water to the boat. The island in the middle holds a chest.
for x in range(3, 16):
    for y in range(3, 16):
        island = 7 <= x <= 9 and 7 <= y <= 9
        deep = 5 <= x <= 13 and 5 <= y <= 13
        if island:
            cell(x, y, "eve-sand")
        else:
            cell(x, y, "eve-deep-water" if deep else "eve-water", 1, 1)
# The vault wall, with one doorway closed by a door that the pressure plate opens.
for y in range(1, H - 1):
    if y == 9:
        cell(24, y, "eve-cobble")
    else:
        cell(24, y, "eve-brick-wall", 1)
for x in range(25, W - 1):
    for y in range(1, H - 1):
        cell(x, y, "eve-cobble")
for y in range(1, H - 1):
    cell(16, y, "eve-sand")


def obj(id, kind, sprite, x, y, **extra):
    o = {"id": id, "kind": kind, "sprite": sprite, "x": x, "y": y, "text": ""}
    o.update(extra)
    return o


isle = {
    "id": "isle",
    "title": "Lantern Isle",
    "width": W,
    "height": H,
    "base": base,
    "detail": [-1] * (W * H),
    "collision": collision,
    "water": water,
    "spawn": {"x": 304, "y": 256},
    "weather": "fog",
    "objects": [
        obj(
            "boat",
            "vehicle",
            "eve-boat",
            224,
            136,
            vehicle={
                "speed": 1.6,
                "terrain": "water",
                "acceleration": 1.2,
                "braking": 1.5,
            },
        ),
        obj(
            "island-chest",
            "chest",
            "eve-chest",
            128,
            120,
            item="gold-coin",
            quantity=20,
            openSprite="eve-chest-open",
            text="Twenty gold coins, kept dry.",
        ),
        obj("stone", "block", "eve-push-block", 304, 176),
        obj(
            "plate",
            "plate",
            "eve-plate",
            352,
            176,
            openSprite="eve-plate-down",
            plate={"targets": ["vault-door"]},
        ),
        obj(
            "vault-door",
            "door",
            "eve-door",
            384,
            128,
            openSprite="eve-door-open",
            switchOnly=True,
        ),
        obj(
            "vault-chest",
            "chest",
            "eve-chest",
            440,
            136,
            item="trail-bread",
            quantity=3,
            openSprite="eve-chest-open",
            text="Three loaves of trail bread.",
        ),
        obj(
            "reset-sign",
            "sign",
            "eve-sign",
            272,
            216,
            text="Push the stone onto the plate. Read me to reset the stone.",
        ),
        obj("dock-lamp", "decoration", "eve-lamp", 264, 112),
        obj("island-lamp", "decoration", "eve-lamp", 144, 112),
    ],
    "portals": [
        {
            "id": "back-to-courtyard",
            "x": 288,
            "y": 288,
            "w": 32,
            "h": 16,
            "target": "meadow",
            "arrival": {"x": 40, "y": 128},
        },
    ],
}
p["scenes"].append(isle)
p["worldLayout"]["isle"] = {"x": 0, "y": 360}

meadow = next(s for s in p["scenes"] if s["id"] == "meadow")
meadow["portals"].append(
    {
        "id": "isle-exit",
        "x": 16,
        "y": 96,
        "w": 16,
        "h": 32,
        "target": "isle",
        "arrival": {"x": 304, "y": 256},
    }
)
meadow["objects"].append(
    obj(
        "cart",
        "vehicle",
        "eve-cart",
        176,
        96,
        vehicle={
            "speed": 1.4,
            "terrain": "land",
            "acceleration": 0.6,
            "braking": 0.3,
        },
    )
)
grove = next(s for s in p["scenes"] if s["id"] == "grove")
grove["weather"] = "rain"


# Gameplay styles. A sidescrolling cave off the isle's west shore, and a first-person crypt below
# the vault.
def grid_map(rows, legend):
    """Builds map layers from rows of characters; legend maps a character to
    (base tile, detail tile or None, collision value)."""
    h, w = len(rows), len(rows[0])
    layers = {"base": [], "detail": [], "collision": [], "water": [0] * (w * h)}
    for row in rows:
        assert len(row) == w, row
        for ch in row:
            b, d, c = legend[ch]
            layers["base"].append(tile[b])
            layers["detail"].append(tile[d] if d else -1)
            layers["collision"].append(c)
    return w, h, layers


tile = {t["id"]: i for i, t in enumerate(p["tiles"])}
# Two rows of rock above and three below keep the play area clear of the HUD bars.
cave_rows = [
    "########################################",
    "########################################",
    "########################################",
    "#......................................#",
    "#......................................#",
    "#.............................H........#",
    "#.............................H=======.#",
    "#.............................H........#",
    "#.............................H........#",
    "#..................====.......H........#",
    "#.............................H........#",
    "#.............................H........#",
    "#.............====............H........#",
    "#.............................H........#",
    "#.............................H........#",
    "gggggggggg^^gggggggggggg^^gggggggggggggg",
    "########################################",
    "########################################",
    "########################################",
]
cw, ch, cave_layers = grid_map(
    cave_rows,
    {
        "#": ("eve-rock-block", None, 1),
        "g": ("eve-ground-top", None, 1),
        ".": ("eve-cave-back", None, 0),
        "=": ("eve-cave-back", "eve-ledge", 2),
        "H": ("eve-cave-back", "eve-ladder", 3),
        "^": ("eve-cave-back", "eve-spikes", 4),
    },
)
p["scenes"].append(
    {
        "id": "cave",
        "title": "Lantern Cave",
        "width": cw,
        "height": ch,
        **cave_layers,
        "spawn": {"x": 40, "y": 240},
        "style": "side",
        "side": {"gravity": 900, "jump": 52},
        "indoors": True,
        "objects": [
            obj(
                "cave-sign",
                "sign",
                "eve-sign",
                56,
                216,
                text="Jump over the spikes, and climb the ladder to the high shelf.",
            ),
            obj(
                "cave-coin-1",
                "collectible",
                "eve-coin",
                248,
                176,
                item="gold-coin",
                quantity=1,
            ),
            obj(
                "cave-coin-2",
                "collectible",
                "eve-coin",
                328,
                128,
                item="gold-coin",
                quantity=1,
            ),
            obj(
                "cave-coin-3",
                "collectible",
                "eve-coin",
                392,
                224,
                item="gold-coin",
                quantity=1,
            ),
            obj(
                "cave-chest",
                "chest",
                "eve-chest",
                576,
                72,
                item="gold-coin",
                quantity=15,
                openSprite="eve-chest-open",
                text="Fifteen gold coins, hidden on the high shelf.",
            ),
        ],
        "portals": [
            {
                "id": "cave-out",
                "x": 16,
                "y": 208,
                "w": 8,
                "h": 32,
                "target": "isle",
                "arrival": {"x": 264, "y": 256},
            }
        ],
    }
)
crypt_rows = [
    "################",
    "#....#.........#",
    "#.##.#.#######.#",
    "#.#..#.......#.#",
    "#.#.####.###.#.#",
    "#.#......#...#.#",
    "#.######.#.###.#",
    "#........#.....#",
    "####.#####.###.#",
    "#....#.....#...#",
    "#....#.........#",
    "################",
]
rw, rh, crypt_layers = grid_map(
    crypt_rows,
    {
        "#": ("eve-dungeon-wall", None, 1),
        ".": ("eve-flagstone", None, 0),
    },
)
# Vary the walls: moss low down and torches here and there.
for i, c in enumerate(crypt_layers["collision"]):
    if c and (i * 7) % 11 == 0:
        crypt_layers["base"][i] = tile["eve-moss-wall"]
    elif c and (i * 5) % 13 == 0:
        crypt_layers["base"][i] = tile["eve-torch-wall"]
p["scenes"].append(
    {
        "id": "crypt",
        "title": "Lantern Crypt",
        "width": rw,
        "height": rh,
        **crypt_layers,
        "spawn": {"x": 40, "y": 160},
        "style": "raycast",
        "raycast": {"floor": "#4a4038", "ceiling": "#1c1e2a", "viewDistance": 176},
        "indoors": True,
        "objects": [
            obj(
                "crypt-sign",
                "sign",
                "eve-sign",
                16,
                136,
                text="The stairs back up to the vault are behind you.",
            ),
            obj("crypt-lamp", "decoration", "eve-lamp", 20, 84),
            obj(
                "crypt-ghost",
                "npc",
                "eve-traveller",
                152,
                136,
                speaker="Old lamplighter",
                pages=[
                    "You found my crypt! I used to light every lantern on the isle.",
                    "My lamp oil is in the chest at the far end. Take what you need.",
                ],
            ),
            obj(
                "crypt-chest",
                "chest",
                "eve-chest",
                216,
                144,
                item="trail-bread",
                quantity=2,
                openSprite="eve-chest-open",
                text="Two loaves of trail bread, still fresh somehow.",
            ),
        ],
        "portals": [
            {
                "id": "crypt-out",
                "x": 16,
                "y": 160,
                "w": 16,
                "h": 16,
                "target": "isle",
                "arrival": {"x": 456, "y": 224},
            }
        ],
    }
)
p["worldLayout"]["cave"] = {"x": 560, "y": 360}
p["worldLayout"]["crypt"] = {"x": 560, "y": 0}
isle["portals"] += [
    {
        "id": "to-cave",
        "x": 256,
        "y": 272,
        "w": 16,
        "h": 16,
        "target": "cave",
        "arrival": {"x": 40, "y": 240},
    },
    {
        "id": "to-crypt",
        "x": 464,
        "y": 256,
        "w": 16,
        "h": 16,
        "target": "crypt",
        "arrival": {"x": 40, "y": 160},
    },
]
for x, y in [(16, 17), (29, 16)]:
    isle["detail"][y * W + x] = tile["eve-stairs-down"]

p["time"] = {
    "dayLength": 480,
    "start": 17.5,
    "night": "#1a2448",
    "strength": 0.6,
    "clock": True,
}
p["leveling"] = {
    "maxLevel": 10,
    "firstLevel": 10,
    "growth": 1.5,
    "health": 2,
    "attack": 1,
    "defense": 0,
}
for s in p["scenes"]:
    for o in s["objects"]:
        if o["kind"] == "enemy":
            o["combat"]["xp"] = 6 if o["id"] == "thorn-raider" else 8
for q in p["quests"]:
    q["rewardXp"] = 15

p["cutscenes"] = [
    {
        "id": "keeper-intro",
        "name": "The keeper says hello",
        "scene": "meadow",
        "steps": [
            {"type": "fade", "to": "black", "seconds": 0},
            {"type": "camera", "x": 128, "y": 196, "seconds": 0},
            {"type": "fade", "to": "clear", "seconds": 1},
            {
                "type": "say",
                "speaker": "Keeper",
                "text": "There you are! The lanterns on the isle went dark last night.",
            },
            {"type": "move", "actor": "keeper", "x": 136, "y": 184, "speed": 32},
            {
                "type": "say",
                "speaker": "Keeper",
                "text": "Take the boat across the lake when you're ready. And mind the fog.",
            },
            {"type": "move", "actor": "keeper", "x": 120, "y": 184, "speed": 32},
            {"type": "camera", "reset": True, "seconds": 1},
        ],
    }
]
p["events"] += [
    {
        "id": "intro",
        "name": "Opening cutscene",
        "trigger": "enter",
        "scene": "meadow",
        "once": True,
        "conditions": [],
        "actions": [{"type": "cutscene", "target": "keeper-intro"}],
    },
    {
        "id": "isle-night",
        "name": "Lanterns at night",
        "trigger": "enter",
        "scene": "isle",
        "once": False,
        "conditions": [{"kind": "time", "from": 19, "to": 6}],
        "actions": [
            {"type": "message", "text": "Lanterns flicker across the dark water."}
        ],
    },
    {
        "id": "fog-lifts",
        "name": "The fog lifts",
        "trigger": "timer",
        "scene": "isle",
        "delay": 45,
        "once": True,
        "conditions": [],
        "actions": [
            {"type": "setWeather", "target": "isle", "weather": "none"},
            {"type": "message", "text": "The fog lifts off the lake."},
        ],
    },
    {
        "id": "reset-stone",
        "name": "Reset the stone",
        "trigger": "interact",
        "scene": "isle",
        "object": "reset-sign",
        "once": False,
        "conditions": [],
        "actions": [
            {"type": "resetPuzzle", "target": "isle"},
            {"type": "message", "text": "The stone slides back to where it started."},
        ],
    },
]

(examples / "Lantern-Isles.eve").write_text(json.dumps(p, indent=2) + "\n")
print("Wrote", examples / "Lantern-Isles.eve")
