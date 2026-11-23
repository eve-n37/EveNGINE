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
    lambda p: obj(p, "cart")["vehicle"].update(speed=5),
    "vehicle speed",
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

print(checks, "world loader checks passed")
