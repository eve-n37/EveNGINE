"""Checks the shared Linux exporter (editor/export) as Linux and Mac Studio use it."""

import json, os, stat, struct, subprocess, sys, tarfile, tempfile, zlib
from pathlib import Path

cli, runtime, root = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3])
# Optional: a real libSDL3 for runtimes that link it, or --no-run where the runtime isn't a Linux
# program (the macOS build exercises the exporter with its own runtime as a stand-in).
mode = sys.argv[4] if len(sys.argv) > 4 else ""
run_games = mode != "--no-run"
real_sdl = Path(mode) if mode and run_games else None
examples = root / "editor/examples"
checks = 0


def check(ok, name):
    global checks
    if not ok:
        raise AssertionError(name)
    checks += 1
    print("OK", name)


def decode_png(data):
    """Returns (width, height, rgba rows) after checking every chunk's CRC."""
    assert data[:8] == b"\x89PNG\r\n\x1a\n", "PNG signature"
    at, idat, header = 8, b"", None
    while at < len(data):
        (size,) = struct.unpack(">I", data[at : at + 4])
        kind, body = data[at + 4 : at + 8], data[at + 8 : at + 8 + size]
        (crc,) = struct.unpack(">I", data[at + 8 + size : at + 12 + size])
        assert zlib.crc32(kind + body) == crc, "chunk CRC"
        if kind == b"IHDR":
            header = struct.unpack(">IIBBBBB", body)
        elif kind == b"IDAT":
            idat += body
        at += 12 + size
    width, height, depth, colour = header[:4]
    assert depth == 8 and colour == 6, "8-bit RGBA"
    raw = zlib.decompress(idat)
    stride = width * 4 + 1
    assert len(raw) == stride * height
    return (
        width,
        height,
        [raw[y * stride + 1 : (y + 1) * stride] for y in range(height)],
    )


def export(project, destination, archive=False, sdl=None):
    args = [
        cli,
        project,
        runtime,
        examples / "font.pam",
        sdl or fake_sdl,
        default_icon,
        destination,
    ]
    return subprocess.run(
        [str(a) for a in args] + (["--archive"] if archive else []),
        capture_output=True,
        text=True,
    )


with tempfile.TemporaryDirectory() as folder:
    folder = Path(folder)
    fake_sdl = folder / "libSDL3.so.0.2.20"
    if real_sdl:
        fake_sdl.write_bytes(real_sdl.read_bytes())
    else:
        fake_sdl.write_bytes(b"\x7fELF not really SDL")
    default_icon = root / "editor/native/Studio.png"
    # A project with a two-colour icon and export settings.
    project = json.loads((examples / "Frontier-Watch.eve").read_text())
    size = 16
    pixels = "".join(
        (
            ("ff0000ff" if y < 8 else "0000ffff")
            if 0 < x < 15 and 0 < y < 15
            else "00000000"
        )
        for y in range(size)
        for x in range(size)
    )
    project["appIcon"] = {"size": size, "pixels": pixels}
    project["export"] = {"appName": "Frontier: Watch"}
    source = folder / "frontier.eve"
    source.write_text(json.dumps(project))

    out = folder / "Plain"
    r = export(source, out)
    check(r.returncode == 0, "folder export: " + r.stderr)
    exe = out / project["id"]
    check(exe.stat().st_mode & stat.S_IXUSR, "runtime is executable")
    check(
        (out / "lib/libSDL3.so.0").read_bytes() == fake_sdl.read_bytes(),
        "SDL3 is bundled",
    )
    check(
        os.access(out / "install-desktop-entry.sh", os.X_OK),
        "desktop entry installer is executable",
    )
    check(
        "Frontier: Watch" in (out / "README.txt").read_text(),
        "README uses the app name",
    )
    check(
        "Frontier: Watch" in (out / "install-desktop-entry.sh").read_text(),
        "menu entry uses the app name",
    )
    width, height, rows = decode_png((out / "icon.png").read_bytes())
    check((width, height) == (256, 256), "icon is 256 × 256")
    check(rows[0][:4] == b"\0\0\0\0", "icon keeps transparency")
    check(rows[64][64 * 4 : 64 * 4 + 4] == b"\xff\0\0\xff", "icon top is red")
    check(rows[192][64 * 4 : 64 * 4 + 4] == b"\0\0\xff\xff", "icon bottom is blue")
    if run_games:
        # Pass the project explicitly: headless runtimes don't look beside themselves for it.
        r = subprocess.run(
            [str(exe), "--project", str(out / "project.lota"), "--validate"],
            capture_output=True,
            text=True,
        )
        check(r.returncode == 0, "exported game validates its project: " + r.stderr)

    tarball = folder / "Frontier.tar.gz"
    r = export(source, tarball, archive=True)
    check(r.returncode == 0, "archive export: " + r.stderr)
    with tarfile.open(tarball) as t:
        members = {m.name: m for m in t.getmembers()}
        base = "Frontier- Watch"
        check(
            {
                base + "/" + n
                for n in [
                    project["id"],
                    "project.lota",
                    "font.pam",
                    "icon.png",
                    "README.txt",
                    "install-desktop-entry.sh",
                    "lib/libSDL3.so.0",
                ]
            }
            <= set(members),
            "archive holds one folder named after the game",
        )
        check(
            not any("._" in n for n in members), "archive has no macOS metadata files"
        )
        check(
            members[base + "/" + project["id"]].mode & 0o111 == 0o111,
            "archive keeps the runtime executable",
        )
        check(
            members[base + "/install-desktop-entry.sh"].mode & 0o100,
            "archive keeps the installer executable",
        )
        t.extractall(folder / "unpacked", filter="tar")
    if run_games:
        unpacked = folder / "unpacked" / base
        r = subprocess.run(
            [
                str(unpacked / project["id"]),
                "--project",
                str(unpacked / "project.lota"),
                "--validate",
            ],
            capture_output=True,
            text=True,
        )
        check(r.returncode == 0, "game from the archive validates: " + r.stderr)
    check(
        not any(p.name.startswith(".eve-") for p in folder.iterdir()),
        "no staging folders are left",
    )

    # Re-exporting replaces the old archive; a failed export leaves it alone.
    before = tarball.read_bytes()
    r = export(source, tarball, archive=True, sdl=folder / "missing.so")
    check(
        r.returncode != 0 and "Missing export resource" in r.stderr,
        "missing SDL3 is reported",
    )
    check(tarball.read_bytes() == before, "failed export keeps the earlier archive")

print(checks, "Linux export checks passed")
