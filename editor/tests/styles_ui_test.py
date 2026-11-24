# Drives the gameplay style editors (map style, sidescroller and raycast settings, and painting
# ledges, ladders and hazards) in a headless browser.
from pathlib import Path
import os
from playwright.sync_api import sync_playwright

root = Path(__file__).resolve().parents[1]
output = Path(
    os.environ.get("LOTA_TEST_OUTPUT", str(root.parent / "build-feature-tests"))
)
output.mkdir(exist_ok=True)
checks = []
with sync_playwright() as pw:
    browser = pw.chromium.launch(headless=True, args=["--no-sandbox"])
    page = browser.new_page(
        viewport={"width": 1440, "height": 960}, device_scale_factor=1
    )
    errors = []
    page.on("pageerror", lambda e: errors.append(str(e)))
    page.goto((root / "ui/index.html").as_uri())
    page.wait_for_selector("#mapCanvas")

    def check(name, expr):
        value = page.evaluate(expr)
        if not value:
            page.screenshot(path=str(output / "styles-ui-failure.png"))
        assert value, name
        checks.append(name)
        print("OK", name)

    def project(path):
        return "(() => { const p = studioTest.project(); return " + path + "; })()"

    def change(locator, value):
        locator.fill(str(value))
        locator.dispatch_event("change")
        page.wait_for_timeout(30)

    def field(label, tag="input"):
        return page.locator(
            "#inspector label:has(span:text-is('" + label + "')) " + tag
        )

    def shown(label, value, tag="input"):
        return (
            "[...document.querySelectorAll('#inspector label')].find(l => l.textContent.startsWith('"
            + label
            + "'))?.querySelector('"
            + tag
            + "').value === '"
            + value
            + "'"
        )

    cave = "p.scenes.find(s => s.id === 'cave')"
    crypt = "p.scenes.find(s => s.id === 'crypt')"
    page.evaluate(
        "studioTest.load(" + (root / "examples/Lantern-Isles.eve").read_text() + ")"
    )
    check(
        "Lantern Isles loads and validates",
        "LotaModel.validate(studioTest.project()).length === 0",
    )

    # The cave: a sidescroller with its gravity and jump height.
    page.evaluate("studioTest.selectMap('cave')")
    check("the cave plays as a sidescroller", shown("Plays as", "side", "select"))
    check("gravity is shown", shown("Gravity", "900"))
    check("jump height is shown in tiles", shown("Jump height", "3.25"))
    change(field("Jump height (tiles)"), 4)
    check(
        "jump height edits the project in pixels", project(cave + ".side.jump === 64")
    )
    change(field("Gravity"), 1200)
    check("gravity edits the project", project(cave + ".side.gravity === 1200"))

    # Painting ledges, ladders and hazards.
    page.locator("#layers .layerRow:has-text('Collision')").click()
    check(
        "sidescroller maps offer four kinds of collision",
        "document.querySelectorAll('#collisionKinds button').length === 4",
    )
    page.locator("#collisionKinds button:has-text('Ladder')").click()
    box = page.locator("#mapCanvas").bounding_box()
    scale = box["width"] / (40 * 16)
    page.mouse.click(box["x"] + (5 * 16 + 8) * scale, box["y"] + (8 * 16 + 8) * scale)
    check("ladders can be painted", project(cave + ".collision[8 * 40 + 5] === 3"))
    page.locator("#collisionKinds button:has-text('Hazard')").click()
    page.mouse.click(box["x"] + (6 * 16 + 8) * scale, box["y"] + (8 * 16 + 8) * scale)
    check("hazards can be painted", project(cave + ".collision[8 * 40 + 6] === 4"))
    check(
        "the painted cave still validates",
        "LotaModel.validate(studioTest.project()).length === 0",
    )
    page.screenshot(path=str(output / "styles-cave.png"))

    # Switching styles keeps the map valid.
    field("Plays as", "select").select_option("raycast")
    check(
        "a map can become first person",
        project(
            cave
            + ".style === 'raycast' && !('side' in "
            + cave
            + ") && "
            + cave
            + ".raycast.viewDistance === 192"
        ),
    )
    check(
        "ledges, ladders and hazards are cleared",
        project(cave + ".collision.every(v => v === 0 || v === 1)"),
    )
    check(
        "the collision kinds are hidden off sidescroller maps",
        "!document.querySelector('#collisionKinds')",
    )
    field("Plays as", "select").select_option("topdown")
    check(
        "top-down removes the style",
        project("!('style' in " + cave + ") && !('raycast' in " + cave + ")"),
    )
    check(
        "the switched map validates",
        "LotaModel.validate(studioTest.project()).length === 0",
    )

    # The crypt: first person, with floor and ceiling colours and a view distance.
    page.evaluate("studioTest.selectMap('crypt')")
    check("the crypt shows its view distance in tiles", shown("View distance", "11"))
    check(
        "floor and ceiling colours use colour pickers",
        "[...document.querySelectorAll('#inspector label')].filter(l => /colour/.test(l.textContent)).every(l => l.querySelector('input').type === 'color')",
    )
    change(field("Floor colour"), "#205030")
    check(
        "the floor colour edits the project",
        project(crypt + ".raycast.floor === '#205030'"),
    )
    check("the crypt shows its walking speed", shown("Walking speed", "56"))
    check("the crypt shows its turning speed", shown("Turning speed", "150"))
    change(field("Turning speed (degrees a second)"), 200)
    check(
        "turning speed edits the project",
        project(crypt + ".movement.turnSpeed === 200"),
    )
    change(field("Seconds to stop"), 0.5)
    check("braking edits the project", project(crypt + ".movement.braking === 0.5"))
    page.screenshot(path=str(output / "styles-crypt.png"))

    # Movement tuning works on top-down maps too, without a turning speed.
    page.evaluate("studioTest.selectMap('meadow')")
    meadow = "p.scenes.find(s => s.id === 'meadow')"
    page.locator(
        "#inspector label:has-text('Custom movement on this map') input"
    ).check()
    check(
        "custom movement starts from the game's speed",
        project(
            meadow
            + ".movement.speed === p.player.speed && "
            + meadow
            + ".movement.acceleration === 0 && !('turnSpeed' in "
            + meadow
            + ".movement)"
        ),
    )
    check(
        "top-down maps have no turning speed",
        "![...document.querySelectorAll('#inspector label')].some(l => l.textContent.startsWith('Turning speed'))",
    )
    change(field("Seconds to top speed"), 0.4)
    check(
        "acceleration edits the project",
        project(meadow + ".movement.acceleration === 0.4"),
    )
    check(
        "a tuned top-down map validates",
        "LotaModel.validate(studioTest.project()).length === 0",
    )
    page.locator(
        "#inspector label:has-text('Custom movement on this map') input"
    ).uncheck()
    check(
        "movement can go back to the game's", project("!('movement' in " + meadow + ")")
    )
    check(
        "the cave lost its turning speed when it stopped being first person",
        project(
            "!('turnSpeed' in (p.scenes.find(s => s.id === 'cave').movement || {}))"
        ),
    )

    # Top-down-only features are reported on other styles.
    check(
        "enemies on a first-person map are reported",
        "(() => { const p = structuredClone(studioTest.project()); const raider = p.scenes.find(s => s.id === 'meadow').objects.find(o => o.id === 'thorn-raider'); p.scenes.find(s => s.id === 'crypt').objects.push({ ...raider, id: 'crypt-raider', x: 40, y: 120 }); return LotaModel.validate(p).some(m => /only work on top-down maps/.test(m)); })()",
    )
    check("no page errors", "true")
    assert not errors, errors
    browser.close()
print(len(checks), "style UI checks passed")
