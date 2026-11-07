# End-to-end test for EveNGINE Studio on Linux. Starts the GTK/WebKitGTK shell (under Xvfb when
# there is no display), drives every native bridge action with linux_studio_test.js, then checks
# the files it wrote and runs the exported game.
# Usage: linux_studio_test.py <evengine-studio> <Frontier-Watch.eve>
import json, os, shutil, subprocess, sys, tempfile
from pathlib import Path

studio, sample = map(Path, sys.argv[1:])
here = Path(__file__).resolve().parent
checks = 0


def ok(name, value):
    global checks
    assert value, name
    checks += 1
    print("OK", name)


with tempfile.TemporaryDirectory() as tmp:
    folder = Path(tmp)
    script = folder / "test.js"
    script.write_text(
        "window.EVE_TEST_FOLDER = %s;\n" % json.dumps(str(folder))
        + (here / "linux_studio_test.js").read_text()
    )
    choices = [
        sample,
        folder / "chosen",
        folder / "scout",
        folder / "pixel",
        folder / "game",
    ]
    env = dict(
        os.environ,
        EVE_STUDIO_TEST_SCRIPT=str(script),
        EVE_STUDIO_TEST_CHOOSE=":".join(map(str, choices)),
        XDG_DATA_HOME=str(folder / "data"),
        SDL_AUDIODRIVER="dummy",
    )
    command = [str(studio)]
    if not os.environ.get("DISPLAY") and not os.environ.get("WAYLAND_DISPLAY"):
        command = ["xvfb-run", "-a", "-s", "-screen 0 1600x1000x24"] + command
    result = subprocess.run(
        command, env=env, capture_output=True, text=True, timeout=180
    )
    # The playtest child shares stdout, so keep only the script's report lines.
    report = [
        l for l in result.stdout.splitlines() if l.startswith(("OK ", "PASS", "FAIL"))
    ]
    for line in report:
        print("  studio:", line)
    passed = "PASS" in report and not any(l.startswith("FAIL") for l in report)
    if not passed:
        print(result.stdout[-3000:], result.stderr[-3000:], file=sys.stderr)
    ok("bridge script passes inside Studio", passed)
    ok("Studio exits cleanly", result.returncode == 0)

    project = json.loads((folder / "direct.eve").read_text())
    ok("saved project is the opened sample", project["id"] == "frontier-watch")
    ok("save as wrote chosen.eve", (folder / "chosen.eve").is_file())
    ok(
        "autosave went to the recovery file",
        (folder / "data/EveNGINE Studio/recovery.lota").is_file(),
    )
    ok("template has its extension", (folder / "scout.eve-object").is_file())
    ok(
        "PNG bytes were decoded",
        (folder / "pixel.png").read_bytes().startswith(b"\x89PNG"),
    )

    game = folder / "game"
    for name in [
        "frontier-watch",
        "project.lota",
        "font.pam",
        "lib/libSDL3.so.0",
        "icon.png",
        "install-desktop-entry.sh",
        "README.txt",
    ]:
        ok("export contains " + name, (game / name).is_file())
    ok("exported runtime is executable", os.access(game / "frontier-watch", os.X_OK))
    ldd = subprocess.run(
        ["ldd", str(game / "frontier-watch")], capture_output=True, text=True
    )
    sdl = next(line for line in ldd.stdout.splitlines() if "libSDL3" in line)
    ok("exported game uses its bundled SDL3", str(game / "lib") in sdl)
    # Move the game to prove it is relocatable, then let it find its own project.
    moved = folder / "moved game"
    shutil.move(str(game), str(moved))
    exe = str(moved / "frontier-watch")
    check = subprocess.run([exe, "--validate"], capture_output=True, text=True)
    ok(
        "relocated exported game loads its own project",
        check.returncode == 0 and "2 maps" in check.stdout,
    )
    capture = folder / "frame.ppm"
    shot = subprocess.run(
        [exe, "--capture", str(capture), "--ticks", "30"],
        capture_output=True,
        text=True,
    )
    ok(
        "exported game renders a frame",
        shot.returncode == 0 and capture.stat().st_size > 1000,
    )
    entry = subprocess.run(
        [str(moved / "install-desktop-entry.sh")],
        env=dict(os.environ, XDG_DATA_HOME=str(folder / "menu")),
        capture_output=True,
        text=True,
    )
    desktop = (folder / "menu/applications/evengine-frontier-watch.desktop").read_text()
    ok(
        "desktop entry installs",
        entry.returncode == 0 and "Name=Frontier Watch" in desktop,
    )
    ok("desktop entry points at the game", 'Exec="%s"' % exe in desktop)

    # Command-line export, for build scripts.
    def export(source, target):
        return subprocess.run(
            [str(studio), "--export", str(source), str(target)],
            capture_output=True,
            text=True,
        )

    cli = folder / "cli game"
    ok(
        "command-line export works",
        export(sample, cli).returncode == 0 and (cli / "frontier-watch").is_file(),
    )
    bad = folder / "bad.eve"
    bad.write_text(json.dumps(dict(project, start="missing")))
    ok(
        "command-line export validates first",
        export(bad, folder / "bad game").returncode != 0
        and not (folder / "bad game").exists(),
    )
    ok(
        "re-exporting replaces the folder",
        export(sample, cli).returncode == 0 and not list(folder.glob(".eve-*")),
    )
print(checks, "Linux Studio checks passed")
