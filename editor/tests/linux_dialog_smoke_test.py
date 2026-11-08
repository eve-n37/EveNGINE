# Opens a real GTK file dialog in EveNGINE Studio for Linux (Export app) and checks that Studio
# is still running a few seconds later with no GTK or GLib errors. linux_studio_test.py answers
# dialogs without showing them, so it can't catch problems drawing one, such as missing icon
# loaders in the AppImage.
# Usage: linux_dialog_smoke_test.py <evengine-studio or AppImage>
import os, signal, subprocess, sys, tempfile
from pathlib import Path

studio = Path(sys.argv[1])
with tempfile.TemporaryDirectory() as tmp:
    script = Path(tmp) / "dialog.js"
    script.write_text(
        "setTimeout(() => native('export', { project: P() }).catch(() => {}), 1000);\n"
    )
    env = dict(
        os.environ,
        EVE_STUDIO_TEST_SCRIPT=str(script),
        XDG_DATA_HOME=str(Path(tmp) / "data"),
    )
    command = [str(studio)]
    if not os.environ.get("DISPLAY") and not os.environ.get("WAYLAND_DISPLAY"):
        command = ["xvfb-run", "-a", "-s", "-screen 0 1600x1000x24"] + command
    child = subprocess.Popen(
        command,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        start_new_session=True,
    )
    try:
        out, _ = child.communicate(timeout=10)
        still_running = False
    except subprocess.TimeoutExpired:
        still_running = True
        os.killpg(child.pid, signal.SIGTERM)  # xvfb-run, Studio and its helpers
        out, _ = child.communicate(timeout=20)
    problems = [
        line
        for line in out.splitlines()
        if "Gtk:ERROR" in line
        or "GLib:ERROR" in line
        or "Bail out!" in line
        or "Could not load a pixbuf" in line
    ]
    if not still_running or problems:
        print(out[-4000:], file=sys.stderr)
        raise SystemExit(
            "FAIL: Studio crashed or logged errors with a file dialog open"
        )
print("OK Studio shows a real file dialog without errors")
