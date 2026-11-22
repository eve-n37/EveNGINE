"""Checks the shared web exporter (editor/export) with stand-in runtime files.

CI's web job runs a real exported game in a browser (editor/tests/web_game_test.py)."""

import json, shutil, subprocess, sys, tempfile, zipfile
from pathlib import Path

cli, root = Path(sys.argv[1]), Path(sys.argv[2])
examples = root / "editor/examples"
checks = 0


def check(ok, name):
    global checks
    if not ok:
        raise AssertionError(name)
    checks += 1
    print("OK", name)


with tempfile.TemporaryDirectory() as folder:
    folder = Path(folder)
    runtime = folder / "web"
    runtime.mkdir()
    (runtime / "evengine.js").write_text("// stand-in runtime\n")
    (runtime / "evengine.wasm").write_bytes(b"\0asm\1\0\0\0")
    shutil.copy(root / "editor/web/index.html", runtime / "index.html")

    def export(project, destination, zip=False):
        args = [
            cli,
            project,
            runtime,
            examples / "font.pam",
            root / "editor/native/Studio.png",
        ]
        return subprocess.run(
            [str(a) for a in args + [destination]] + (["--zip"] if zip else []),
            capture_output=True,
            text=True,
        )

    project = json.loads((examples / "Frontier-Watch.eve").read_text())
    project["export"] = {"appName": 'Watch <"Frontier"> & Co', "windowScale": 2}
    source = folder / "frontier.eve"
    source.write_text(json.dumps(project))

    out = folder / "site"
    r = export(source, out)
    check(r.returncode == 0, "folder export: " + r.stderr)
    names = {
        "index.html",
        "evengine.js",
        "evengine.wasm",
        "project.lota",
        "font.pam",
        "icon.png",
    }
    check(
        {p.name for p in out.iterdir()} == names,
        "folder holds the page, runtime and project",
    )
    page = (out / "index.html").read_text()
    check(
        "<title>Watch &lt;&quot;Frontier&quot;&gt; &amp; Co</title>" in page,
        "page title is the escaped app name",
    )
    check("{{" not in page and "800px" in page, "page placeholders are filled in")
    check(
        json.loads((out / "project.lota").read_text())["id"] == project["id"],
        "project is included",
    )
    icon = (out / "icon.png").read_bytes()
    check(
        icon[:8] == b"\x89PNG\r\n\x1a\n"
        and icon[16:24] == bytes.fromhex("0000010000000100"),
        "the project's own icon becomes a 256 × 256 PNG",
    )
    plain = dict(project)
    plain.pop("appIcon")
    (folder / "plain.eve").write_text(json.dumps(plain))
    r = export(folder / "plain.eve", folder / "plain")
    check(
        r.returncode == 0
        and (folder / "plain/icon.png").read_bytes()
        == (root / "editor/native/Studio.png").read_bytes(),
        "projects without an icon get the default one",
    )

    archive = folder / "frontier.zip"
    r = export(source, archive, zip=True)
    check(r.returncode == 0, "zip export: " + r.stderr)
    with zipfile.ZipFile(archive) as z:
        check(z.testzip() is None, "zip checksums are valid")
        check(set(z.namelist()) == names, "zip has index.html at the top level")
        check(
            z.read("index.html").decode() == page, "zip page matches the folder export"
        )
        check(
            z.read("evengine.wasm") == b"\0asm\1\0\0\0", "zip keeps binary files intact"
        )
        unix = {i.filename: i.external_attr >> 16 for i in z.infolist()}
        check(
            all(mode & 0o644 == 0o644 for mode in unix.values()),
            "zip entries are readable",
        )

    before = archive.read_bytes()
    (runtime / "evengine.wasm").unlink()
    r = export(source, archive, zip=True)
    check(
        r.returncode != 0 and "Missing export resource" in r.stderr,
        "missing runtime is reported",
    )
    check(archive.read_bytes() == before, "failed export keeps the earlier zip")
    check(
        not any(p.name.startswith(".eve-") for p in folder.iterdir()),
        "no staging folders are left",
    )

print(checks, "web export checks passed")
