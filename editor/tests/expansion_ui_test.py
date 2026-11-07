# Drives the EveNGINE 1.1 editors (factions, animations, ranged enemies, equipment, timed events)
# in a headless browser through the current five-section navigation.
from pathlib import Path
import json, os
from playwright.sync_api import sync_playwright

root = Path(__file__).resolve().parents[1]
output = Path(
    os.environ.get("LOTA_TEST_OUTPUT", str(root.parent / "build-feature-tests"))
)
output.mkdir(exist_ok=True)
checks = []
with sync_playwright() as pw:
    browser = pw.chromium.launch(
        executable_path=os.environ.get("LOTA_CHROMIUM"),
        headless=True,
        args=["--no-sandbox", "--disable-dev-shm-usage"],
    )
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
            page.screenshot(path=str(output / "expansion-ui-failure.png"))
            (output / "expansion-ui-failure.json").write_text(
                json.dumps(page.evaluate("studioTest.project()"), indent=1)
            )
        assert value, name
        checks.append(name)
        print("OK", name)

    def project(path):
        return "(() => { const p = studioTest.project(); return " + path + "; })()"

    def actor(key):
        page.locator(
            "#actorWorkspace label:has-text('Actor') select"
        ).first.select_option(key)
        page.wait_for_timeout(50)

    page.evaluate(
        "studioTest.load(" + (root / "examples/Frontier-Watch.eve").read_text() + ")"
    )
    page.locator("#primaryNavigation [data-workspace=story]").click()
    page.locator("[data-section=actors]").click()
    check("NPCs & trading opens", "!document.getElementById('actorWorkspace').hidden")
    check(
        "factions card lists project factions",
        "document.querySelectorAll('#actorWorkspace [data-faction]').length === 2",
    )
    actor("grove:scout")
    row = "#actorWorkspace [data-animation=walk-east]"
    check(
        "animated NPC shows its walk frames",
        "document.querySelectorAll('" + row + " .frameChip').length === 4",
    )
    check(
        "NPCs have no attack animation rows",
        "!document.querySelector('#actorWorkspace [data-animation^=attack]')",
    )
    page.locator(row + " .frameChip").first.click()
    check(
        "removing a frame updates the project",
        project(
            "p.scenes[1].objects.find(o => o.id === 'scout').animations.walk.east.length"
        )
        + " === 3",
    )
    page.locator(row + " select").select_option("eve-hero-east0")
    check(
        "adding a frame appends the sprite",
        project(
            "p.scenes[1].objects.find(o => o.id === 'scout').animations.walk.east.at(-1)"
        )
        + " === 'eve-hero-east0'",
    )
    actor("grove:thorn-slinger")
    check(
        "ranged enemy shows its attack style",
        "[...document.querySelectorAll('#actorWorkspace label')].find(l => l.textContent.startsWith('Attack style')).querySelector('select').value === 'ranged'",
    )
    page.locator("#actorWorkspace [data-animation=attack-east] select").select_option(
        "eve-raider"
    )
    check(
        "enemy attack frames can be added",
        project(
            "p.scenes[1].objects.find(o => o.id === 'thorn-slinger').animations.attack.east[0]"
        )
        + " === 'eve-raider'",
    )
    page.locator("#actorWorkspace label:has-text('Attack style') select").select_option(
        "melee"
    )
    check(
        "switching to melee clamps reach and drops projectile speed",
        project(
            "(o => !o.combat.style && !o.combat.projectileSpeed && o.combat.range <= 64)(p.scenes[1].objects.find(o => o.id === 'thorn-slinger'))"
        ),
    )
    page.locator("#actorWorkspace label:has-text('Faction') select").last.select_option(
        ""
    )
    check(
        "faction can be cleared",
        project(
            "p.scenes[1].objects.find(o => o.id === 'thorn-slinger').combat.faction"
        )
        + " === undefined",
    )
    check(
        "hostility shows on both factions",
        "[...document.querySelectorAll('#actorWorkspace [data-faction=watch] input[type=checkbox]')][0].checked",
    )
    page.locator(
        "#actorWorkspace [data-faction=watch] input[type=checkbox]"
    ).first.uncheck()
    check(
        "clearing hostility from either side ends it",
        project("p.factions.every(f => !(f.hostile || []).length)"),
    )
    page.locator(
        "#actorWorkspace [data-faction=watch] input[type=checkbox]"
    ).first.check()
    check(
        "hostility can be set from either side",
        project("p.factions.find(f => f.id === 'watch').hostile[0]") + " === 'raiders'",
    )
    page.locator("#actorWorkspace button:has-text('Remove faction')").first.click()
    check("factions in use cannot be removed", project("p.factions.length") + " === 2")
    page.locator("#actorWorkspace button:has-text('New faction')").click()
    check("new faction is added", project("p.factions.length") + " === 3")
    page.locator("#actorWorkspace").screenshot(
        path=str(output / "expansion-actors.png")
    )

    page.locator("[data-section=items]").click()
    page.wait_for_timeout(50)
    items = page.evaluate("studioTest.project().items.map(i => i.id)")

    def item_select(item_id, label):
        return (
            page.locator("#itemList")
            .locator(".itemRow")
            .nth(items.index(item_id))
            .locator("label:has-text('" + label + "') select")
        )

    item_select("sling-stone", "Equipment slot").select_option("charm")
    check(
        "ammunition cannot become equipment",
        project("p.items.find(i => i.id === 'sling-stone').equip") + " === undefined",
    )
    item_select("trail-bread", "Equipment slot").select_option("armor")
    check(
        "equipping a healing item removes its healing",
        project(
            "(i => i.equip.slot === 'armor' && i.heal === undefined)(p.items.find(i => i.id === 'trail-bread'))"
        ),
    )
    check(
        "ranged weapon shows its ammunition",
        "[...document.querySelectorAll('#itemList .itemRow')][%d].textContent.includes('Ammunition')"
        % items.index("sling"),
    )

    page.locator("[data-section=events]").click()
    page.wait_for_timeout(50)
    delay = page.locator("[data-event=watch-horn] label:has-text('Delay') input")
    check(
        "timed event shows its delay",
        "document.querySelector('[data-event=watch-horn]') !== null",
    )
    assert delay.input_value() == "30"
    delay.fill("12.5")
    delay.dispatch_event("change")
    check(
        "timer delay accepts half seconds",
        project("p.events.find(e => e.id === 'watch-horn').delay") + " === 12.5",
    )
    page.locator(
        "[data-event=watch-horn] label:has-text('Trigger') select"
    ).first.select_option("enter")
    check(
        "switching away from a timer removes its delay",
        project("p.events.find(e => e.id === 'watch-horn').delay") + " === undefined",
    )
    page.locator("#validate").click()
    check(
        "edited project still validates",
        "document.getElementById('health').textContent.includes('Project checks passed')",
    )
    assert not errors, errors
    checks.append("no page errors")
    browser.close()
print(len(checks), "expansion UI checks passed")
