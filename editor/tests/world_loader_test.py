"""Checks that the native runtime rejects invalid EveNGINE 1.2 world data with a clear message.

Usage: world_loader_test.py <runtime> <Lantern-Isles.eve>"""

import copy, json, subprocess, sys, tempfile
from pathlib import Path

binary, source = sys.argv[1:]
original = json.loads(Path(source).read_text())
checks = 0


def scene(p, name):
    return next(s for s in p["scenes"] if s["id"] == name)


def obj(p, name):
    return next(o for s in p["scenes"] for o in s["objects"] if o["id"] == name)


def event(p, name):
    return next(e for e in p["events"] if e["id"] == name)


def cut(p):
    return p["cutscenes"][0]


def run(p):
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / "project.eve"
        path.write_text(json.dumps(p))
        return subprocess.run(
            [binary, "--project", str(path), "--validate"],
            capture_output=True,
            text=True,
        )


def accept(name, change):
    global checks
    p = copy.deepcopy(original)
    change(p)
    result = run(p)
    assert result.returncode == 0, name + ": " + result.stderr
    checks += 1
    print("OK", name)


def reject(name, change, message):
    global checks
    p = copy.deepcopy(original)
    change(p)
    result = run(p)
    assert result.returncode != 0, name + " was accepted"
    assert message.lower() in result.stderr.lower(), name + ": " + result.stderr
    checks += 1
    print("OK", name)


accept("Lantern Isles validates", lambda p: None)

# Time of day and weather
reject("short days", lambda p: p["time"].update(dayLength=10), "day length")
reject("night too dark", lambda p: p["time"].update(strength=0.9), "night strength")
reject("bad night colour", lambda p: p["time"].update(night="navy"), "night colour")
reject(
    "unknown time setting", lambda p: p["time"].update(dawn=6), "unknown time of day"
)
reject("unknown weather", lambda p: scene(p, "isle").update(weather="hail"), "weather")


def without_time(p):
    p.pop("time")
    p["events"] = [e for e in p["events"] if e["id"] != "isle-night"]


accept("projects work without time of day", without_time)
reject("time conditions need time", lambda p: p.pop("time"), "time of day")
reject(
    "time conditions need two hours",
    lambda p: event(p, "isle-night")["conditions"][0].update(to=19),
    "two different hours",
)
reject(
    "weather actions name real weather",
    lambda p: event(p, "fog-lifts")["actions"][0].update(weather="hail"),
    "weather",
)

# Puzzles
reject(
    "blocks need a collider",
    lambda p: obj(p, "stone").update(sprite="eve-plate"),
    "collider",
)
reject(
    "plates open doors only",
    lambda p: obj(p, "plate")["plate"].update(targets=["stone"]),
    "only open doors",
)
reject(
    "plates use known flags",
    lambda p: obj(p, "plate")["plate"].update(flag="no-such-flag"),
    "plate flag",
)
reject(
    "plate settings only on plates",
    lambda p: obj(p, "stone").update(plate={}),
    "only pressure plates",
)
reject(
    "reset needs a map",
    lambda p: event(p, "reset-stone")["actions"][0].update(target="nowhere"),
    "puzzle map",
)

# Vehicles
reject(
    "vehicles need settings",
    lambda p: obj(p, "boat").pop("vehicle"),
    "vehicle settings",
)
reject(
    "vehicles go on land or water",
    lambda p: obj(p, "boat")["vehicle"].update(terrain="air"),
    "land or water",
)
reject(
    "vehicle speed",
    lambda p: obj(p, "cart")["vehicle"].update(speed=7),
    "vehicle speed",
)
reject(
    "vehicle acceleration",
    lambda p: obj(p, "cart")["vehicle"].update(acceleration=11),
    "vehicle acceleration",
)
reject(
    "vehicle braking",
    lambda p: obj(p, "boat")["vehicle"].update(braking=-1),
    "vehicle braking",
)

# Leveling
reject("leveling max", lambda p: p["leveling"].update(maxLevel=1), "maximum level")
reject("leveling growth", lambda p: p["leveling"].update(growth=4), "growth")


def without_leveling(p):
    p.pop("leveling")
    for s in p["scenes"]:
        for o in s["objects"]:
            if o["kind"] == "enemy":
                o["combat"].pop("xp")
    for q in p["quests"]:
        q.pop("rewardXp")


accept("projects work without leveling", without_leveling)


def enemy_xp_only(p):
    without_leveling(p)
    obj(p, "thorn-raider")["combat"]["xp"] = 5


reject("enemy XP needs leveling", enemy_xp_only, "needs leveling")


def give_xp_only(p):
    without_leveling(p)
    event(p, "intro")["actions"].append({"type": "giveXp", "quantity": 5})


reject("XP actions need leveling", give_xp_only, "leveling")
reject(
    "enemy XP bounds",
    lambda p: obj(p, "thorn-raider")["combat"].update(xp=10000),
    "enemy XP",
)

# Cutscenes
reject(
    "cutscene actors are on its map",
    lambda p: cut(p)["steps"][4].update(actor="watch-guard"),
    "isn't on the cutscene's map",
)
reject(
    "cutscene moves stay on the map",
    lambda p: cut(p)["steps"][4].update(x=9999),
    "cutscene x",
)
reject(
    "unknown step",
    lambda p: cut(p)["steps"].append({"type": "dance"}),
    "unknown cutscene",
)
reject(
    "cutscene sounds exist",
    lambda p: cut(p)["steps"].append({"type": "sound", "target": "no-such-sound"}),
    "cutscene sound",
)
reject(
    "cutscenes play on the event's map",
    lambda p: event(p, "isle-night")["actions"].append(
        {"type": "cutscene", "target": "keeper-intro"}
    ),
    "map of the event",
)
reject("cutscenes need steps", lambda p: cut(p).update(steps=[]), "1-64 steps")
reject(
    "duplicate cutscenes",
    lambda p: p["cutscenes"].append(copy.deepcopy(cut(p))),
    "duplicate cutscene",
)

# Text limits count characters, so accented and symbol text gets the full length.
accept(
    "a full-length line of accented text",
    lambda p: cut(p)["steps"][3].update(text="é" * 719 + "♥"),
)
reject(
    "one character too many",
    lambda p: cut(p)["steps"][3].update(text="é" * 721),
    "cutscene",
)


# Gameplay styles
def moved(p, name, to):
    """Moves an object from the courtyard onto another map."""
    o = obj(p, name)
    scene(p, "meadow")["objects"].remove(o)
    scene(p, to)["objects"].append(o)


reject("unknown style", lambda p: scene(p, "cave").update(style="iso"), "top-down")
reject(
    "sidescroller settings on a top-down map",
    lambda p: scene(p, "meadow").update(side={"jump": 40}),
    "only sidescroller maps",
)
reject(
    "raycast settings on a sidescroller map",
    lambda p: scene(p, "cave").update(raycast={}),
    "only raycast maps",
)
reject("weak gravity", lambda p: scene(p, "cave")["side"].update(gravity=10), "gravity")
reject("huge jumps", lambda p: scene(p, "cave")["side"].update(jump=500), "jump height")
reject(
    "unknown sidescroller settings",
    lambda p: scene(p, "cave")["side"].update(wallJump=True),
    "unknown sidescroller setting",
)
reject(
    "floor colours",
    lambda p: scene(p, "crypt")["raycast"].update(floor="red"),
    "floor colour",
)
reject(
    "short view distance",
    lambda p: scene(p, "crypt")["raycast"].update(viewDistance=2),
    "view distance",
)
reject(
    "ladders on a top-down map",
    lambda p: scene(p, "meadow")["collision"].__setitem__(40, 3),
    "layer cell",
)
reject(
    "unknown collision kinds",
    lambda p: scene(p, "cave")["collision"].__setitem__(40, 5),
    "layer cell",
)
reject(
    "enemies on a sidescroller map",
    lambda p: moved(p, "thorn-raider", "cave"),
    "only work on top-down maps",
)
reject(
    "vehicles in first person",
    lambda p: moved(p, "cart", "crypt"),
    "only work on top-down maps",
)
reject(
    "patrols on a sidescroller map",
    lambda p: scene(p, "cave")["objects"].append(
        {
            k: v
            for k, v in {**copy.deepcopy(obj(p, "keeper")), "id": "cave-keeper"}.items()
            if k != "dialogue"
        }
    ),
    "can't patrol",
)
accept(
    "a map can be switched back to top-down",
    lambda p: [scene(p, "cave").pop(k) for k in ["style", "side"]]
    and scene(p, "cave").update(
        collision=[min(c, 1) for c in scene(p, "cave")["collision"]]
    ),
)

# Movement tuning
reject(
    "slow walking",
    lambda p: scene(p, "cave")["movement"].update(speed=2),
    "walking speed",
)
reject(
    "slow acceleration",
    lambda p: scene(p, "crypt")["movement"].update(acceleration=11),
    "walking acceleration",
)
reject(
    "turning off first-person maps",
    lambda p: scene(p, "cave")["movement"].update(turnSpeed=90),
    "turning speed",
)
reject(
    "unknown movement settings",
    lambda p: scene(p, "cave")["movement"].update(jumpy=True),
    "unknown movement setting",
)
accept(
    "movement on a top-down map",
    lambda p: scene(p, "meadow").update(
        movement={"speed": 90, "acceleration": 0.3, "braking": 0.2}
    ),
)

print(checks, "world loader checks passed")
