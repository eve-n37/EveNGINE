import copy, json, subprocess, sys, tempfile
from pathlib import Path

# Checks that the native runtime rejects invalid EveNGINE 1.1 expansion data.
binary, source = sys.argv[1:]
original = json.loads(Path(source).read_text())
checks = 0


def item(p, name):
    return next(i for i in p["items"] if i["id"] == name)


def obj(p, name):
    return next(o for s in p["scenes"] for o in s["objects"] if o["id"] == name)


def event(p, name):
    return next(e for e in p["events"] if e["id"] == name)


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


def reject(name, change):
    global checks
    p = copy.deepcopy(original)
    change(p)
    result = run(p)
    assert result.returncode != 0 and result.stderr.strip(), name
    checks += 1
    print("OK", name)


accept("sample validates", lambda p: None)
# Equipment
reject(
    "unknown equipment slot",
    lambda p: item(p, "iron-sword")["equip"].update(slot="ring"),
)
reject(
    "equipment attack bounds",
    lambda p: item(p, "iron-sword")["equip"].update(attack=100),
)
reject(
    "negative defense", lambda p: item(p, "watch-shield")["equip"].update(defense=-1)
)
reject(
    "fractional health bonus",
    lambda p: item(p, "watch-shield")["equip"].update(health=1.5),
)
reject("healing equipment", lambda p: item(p, "iron-sword").update(heal=2))


def without_combat(p):
    p.pop("combat")
    for s in p["scenes"]:
        s["objects"] = [o for o in s["objects"] if o["kind"] != "enemy"]


reject("equipment without combat", without_combat)
reject(
    "ranged armor",
    lambda p: item(p, "watch-shield")["equip"].update(
        ranged={"speed": 100, "range": 100}
    ),
)
reject(
    "projectile speed bounds",
    lambda p: item(p, "sling")["equip"]["ranged"].update(speed=1000),
)
reject(
    "projectile range bounds",
    lambda p: item(p, "sling")["equip"]["ranged"].update(range=5),
)
reject(
    "unknown ammunition",
    lambda p: item(p, "sling")["equip"]["ranged"].update(ammo="missing"),
)
reject(
    "weapon as its own ammunition",
    lambda p: item(p, "sling")["equip"]["ranged"].update(ammo="sling"),
)
reject(
    "equipment as ammunition",
    lambda p: item(p, "sling")["equip"]["ranged"].update(ammo="iron-sword"),
)
accept(
    "ranged weapon without ammunition",
    lambda p: item(p, "sling")["equip"]["ranged"].pop("ammo"),
)
# Factions
reject(
    "duplicate faction", lambda p: p["factions"].append(copy.deepcopy(p["factions"][0]))
)
reject("unknown faction attitude", lambda p: p["factions"][0].update(player="friendly"))
reject(
    "unknown hostile faction", lambda p: p["factions"][0].update(hostile=["missing"])
)
reject("self-hostile faction", lambda p: p["factions"][0].update(hostile=["raiders"]))
reject(
    "unknown enemy faction",
    lambda p: obj(p, "watch-guard")["combat"].update(faction="missing"),
)
reject(
    "too many factions",
    lambda p: p.update(factions=[{"id": "f%d" % i, "name": "F"} for i in range(17)]),
)
# Ranged enemies
reject(
    "unknown attack style",
    lambda p: obj(p, "thorn-slinger")["combat"].update(style="magic"),
)
reject(
    "ranged enemy needs projectile speed",
    lambda p: obj(p, "thorn-slinger")["combat"].pop("projectileSpeed"),
)
reject(
    "ranged enemy reach bounds",
    lambda p: obj(p, "thorn-slinger")["combat"].update(range=300),
)
reject(
    "melee enemy keeps short reach",
    lambda p: obj(p, "watch-guard")["combat"].update(range=96),
)
# Animations
reject("stationary NPC cannot animate", lambda p: obj(p, "scout").pop("movement"))
reject(
    "NPC attack animation",
    lambda p: obj(p, "scout")["animations"].update(attack={"south": ["eve-raider"]}),
)
reject(
    "unknown animation sprite",
    lambda p: obj(p, "scout")["animations"]["walk"].update(south=["missing"]),
)
reject(
    "unknown animation direction",
    lambda p: obj(p, "scout")["animations"]["walk"].update(up=["eve-raider"]),
)
reject(
    "empty animation direction",
    lambda p: obj(p, "scout")["animations"]["walk"].update(south=[]),
)
reject(
    "too many animation frames",
    lambda p: obj(p, "scout")["animations"]["walk"].update(south=["eve-raider"] * 17),
)
reject("animation frame duration", lambda p: obj(p, "scout")["animations"].update(ms=5))
reject(
    "animations without frames",
    lambda p: obj(p, "scout").update(animations={"ms": 140}),
)
accept(
    "enemy attack animation",
    lambda p: obj(p, "thorn-slinger").update(
        animations={"attack": {"east": ["eve-raider", "eve-guard"]}}
    ),
)
# Timed events
reject("timer delay too short", lambda p: event(p, "watch-horn").update(delay=0.1))
reject("timer delay too long", lambda p: event(p, "watch-horn").update(delay=4000))
reject("timer needs a delay", lambda p: event(p, "watch-horn").pop("delay"))
accept(
    "repeating timer", lambda p: event(p, "watch-horn").update(once=False, delay=0.5)
)
print(checks, "expansion loader checks passed")
