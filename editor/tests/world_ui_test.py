# Drives the EveNGINE 1.2 world editors (time & weather, leveling, map weather, puzzles,
# vehicles, cutscenes and the new event options) in a headless browser.
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
            page.screenshot(path=str(output / "world-ui-failure.png"))
        assert value, name
        checks.append(name)
        print("OK", name)

    def project(path):
        return "(() => { const p = studioTest.project(); return " + path + "; })()"

    def change(locator, value):
        locator.fill(str(value))
        locator.dispatch_event("change")
        page.wait_for_timeout(30)

    page.evaluate(
        "studioTest.load(" + (root / "examples/Lantern-Isles.eve").read_text() + ")"
    )
    check(
        "Lantern Isles loads and validates",
        "LotaModel.validate(studioTest.project()).length === 0",
    )

    # Game → Setup: time & weather, leveling.
    page.locator("#primaryNavigation [data-workspace=game]").click()
    page.locator("summary:has-text('Time & weather')").click()
    day = page.locator("#setupTime label:has-text('Length of a day') input")
    check(
        "day length shows the project value",
        "document.querySelector('#setupTime input[type=number]').value === '480'",
    )
    change(day, 900)
    check("day length edits the project", project("p.time.dayLength === 900"))
    page.locator("#setupTime label:has-text('Show a clock') input").uncheck()
    check("the clock can be hidden", project("p.time.clock === false"))
    page.locator("summary:has-text('Leveling')").click()
    change(
        page.locator("#setupLeveling label:has-text('XP to reach level 2') input"), 12
    )
    check("XP for level 2 edits the project", project("p.leveling.firstLevel === 12"))
    check(
        "the XP table follows the settings",
        "document.querySelector('#setupLeveling').textContent.includes('Level 2: 12 XP')",
    )
    page.locator("#gameSetupWorkspace").screenshot(path=str(output / "world-setup.png"))

    # Map weather.
    page.evaluate("studioTest.selectMap('isle')")
    weather = page.locator("#inspector label:has-text('Weather') select")
    check(
        "the isle shows its fog",
        "[...document.querySelectorAll('#inspector label')].find(l => l.textContent.startsWith('Weather')).querySelector('select').value === 'fog'",
    )
    weather.select_option("snow")
    check(
        "weather is set per map",
        project("p.scenes.find(s => s.id === 'isle').weather === 'snow'"),
    )
    weather.select_option("none")
    check(
        "clear weather removes the setting",
        project("!('weather' in p.scenes.find(s => s.id === 'isle'))"),
    )

    # Pressure plate, push block and vehicle inspectors.
    page.evaluate("studioTest.inspectObject('plate')")
    check(
        "the plate lists its door",
        "[...document.querySelectorAll('#inspector label')].some(l => l.textContent.includes('vault-door') && l.querySelector('input').checked)",
    )
    page.locator(
        "#inspector label:has-text('Stays pressed once triggered') input"
    ).check()
    check(
        "plates can latch",
        project(
            "p.scenes.find(s => s.id === 'isle').objects.find(o => o.id === 'plate').plate.sticky === true"
        ),
    )
    page.locator("#inspector").screenshot(path=str(output / "world-plate.png"))
    page.evaluate("studioTest.inspectObject('stone')")
    check(
        "push blocks explain themselves",
        "document.querySelector('#inspector').textContent.includes('PUSH BLOCK')",
    )
    page.locator("#inspector [data-field=kind]").select_option("vehicle")
    check(
        "a new vehicle gets settings",
        project(
            "p.scenes.find(s => s.id === 'isle').objects.find(o => o.id === 'stone').vehicle.terrain === 'land'"
        ),
    )
    page.locator("#inspector label:has-text('Travels on') select").select_option(
        "water"
    )
    check(
        "vehicles can be boats",
        project(
            "p.scenes.find(s => s.id === 'isle').objects.find(o => o.id === 'stone').vehicle.terrain === 'water'"
        ),
    )
    page.locator("#inspector [data-field=kind]").select_option("block")
    check(
        "leaving vehicle behaviour removes its settings",
        project(
            "!('vehicle' in p.scenes.find(s => s.id === 'isle').objects.find(o => o.id === 'stone'))"
        ),
    )

    # Story → Cutscenes.
    page.locator("#primaryNavigation [data-workspace=story]").click()
    page.locator("[data-section=cutscenes]").click()
    check(
        "the cutscene editor opens",
        "!document.getElementById('cutscenesWorkspace').hidden",
    )
    check(
        "the keeper's cutscene is listed",
        "document.querySelectorAll('#cutscenesWorkspace [data-cutscene]').length === 1",
    )
    check(
        "it says which event plays it",
        "document.querySelector('#cutscenesWorkspace').textContent.includes('Played by: Opening cutscene')",
    )
    page.locator("#cutscenesWorkspace button:has-text('Add step')").click()
    check("steps can be added", project("p.cutscenes[0].steps.length === 9"))
    page.locator("#cutscenesWorkspace label:has-text('Step 9') select").select_option(
        "move"
    )
    page.locator(
        "#cutscenesWorkspace label:has-text('Character') select"
    ).last.select_option("keeper")
    check(
        "a step can move the keeper",
        project(
            "p.cutscenes[0].steps[8].type === 'move' && p.cutscenes[0].steps[8].actor === 'keeper'"
        ),
    )
    page.locator("#cutscenesWorkspace").screenshot(
        path=str(output / "world-cutscenes.png")
    )

    # Events: time conditions and the new actions.
    page.locator("[data-section=events]").click()
    night = "#eventsWorkspace [data-event=isle-night]"
    check(
        "time conditions show their hours",
        f"document.querySelector('{night}').textContent.includes('From hour')",
    )
    check(
        "event actions offer the new types",
        "['setTime','setWeather','resetPuzzle','giveXp','cutscene'].every(t => document.querySelector('#eventsWorkspace [data-event=intro] select option[value=' + t + ']'))",
    )
    check(
        "the edited project still validates",
        "LotaModel.validate(studioTest.project()).length === 0",
    )
    assert not errors, errors
    checks.append("no page errors")
    print("OK no page errors")
    browser.close()
print(len(checks), "world UI checks passed")
