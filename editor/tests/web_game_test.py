"""Plays an exported web game in Chromium: it must load, start from Play, draw, take keyboard input
and keep saves in browser storage. Usage: web_game_test.py <exported folder> [screenshot folder]
"""

import functools, http.server, io, sys, threading
from pathlib import Path
from PIL import Image
from playwright.sync_api import sync_playwright

game = Path(sys.argv[1])
shots = Path(sys.argv[2]) if len(sys.argv) > 2 else None
checks = 0


def check(ok, name):
    global checks
    if not ok:
        raise AssertionError(name)
    checks += 1
    print("OK", name)


class Quiet(http.server.SimpleHTTPRequestHandler):
    extensions_map = {
        **http.server.SimpleHTTPRequestHandler.extensions_map,
        ".wasm": "application/wasm",
    }

    def log_message(self, *args):
        pass


server = http.server.ThreadingHTTPServer(
    ("127.0.0.1", 0), functools.partial(Quiet, directory=str(game))
)
threading.Thread(target=server.serve_forever, daemon=True).start()
url = "http://127.0.0.1:%d/" % server.server_address[1]


def frame(page, name):
    data = page.locator("#canvas").screenshot()
    if shots:
        shots.mkdir(parents=True, exist_ok=True)
        (shots / (name + ".png")).write_bytes(data)
    return Image.open(io.BytesIO(data)).convert("RGB")


def different(a, b):
    a, b = a.resize((200, 120)), b.resize((200, 120))
    return sum(1 for p, q in zip(a.getdata(), b.getdata()) if p != q)


errors = []
try:
    with sync_playwright() as p:
        browser = p.chromium.launch(args=["--autoplay-policy=no-user-gesture-required"])
        page = browser.new_page(viewport={"width": 1280, "height": 800})
        page.on("pageerror", lambda e: errors.append(str(e)))
        page.goto(url)
        page.wait_for_function(
            "!document.getElementById('play').disabled", timeout=60000
        )
        check(
            page.title() != "" and "{{" not in page.title(),
            "page title is the app name",
        )
        page.click("#play")
        page.wait_for_function("document.getElementById('overlay').hidden")
        page.wait_for_timeout(1500)
        title = frame(page, "1-title")
        check(len(set(title.resize((200, 120)).getdata())) > 4, "title screen draws")
        # Start a new game from the title screen, then walk right.
        page.keyboard.press("e")
        page.wait_for_timeout(800)
        playing = frame(page, "2-playing")
        check(different(title, playing) > 500, "keyboard starts the game")
        page.keyboard.down("d")
        page.wait_for_timeout(900)
        page.keyboard.up("d")
        page.wait_for_timeout(200)
        walked = frame(page, "3-walked")
        check(different(playing, walked) > 50, "the player moves")
        check(
            page.evaluate(
                "Module.FS.analyzePath('/saves').object.mount.type === Module.FS.filesystems.IDBFS"
            ),
            "saves live in browser storage",
        )
        page.wait_for_timeout(2500)
        check(
            page.evaluate(
                "async () => (await indexedDB.databases()).some(d => d.name === '/saves')"
            ),
            "the game syncs to IndexedDB",
        )
        check(not errors, "no page errors: " + "; ".join(errors))
        browser.close()
finally:
    server.shutdown()
print(checks, "web game checks passed")
