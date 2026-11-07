#!/usr/bin/env python3
"""Package EveNGINE Studio for Linux as a portable tar.gz.

Build first with the desktop backend and Linux Studio enabled, for example:
  cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release
  cmake --build build-linux
  python3 tools/package_linux.py --build build-linux --output deliverables

For the widest compatibility, build on the oldest distribution you want to support: the package
needs at least the glibc version of the machine it was built on (the script reports it).
"""

import argparse, platform, re, shutil, subprocess, tarfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument("--build", required=True, type=Path, help="CMake build folder")
p.add_argument("--output", required=True, type=Path)
version = re.search(
    r"project\(EveNGINESDK VERSION ([0-9.]+)", (root / "CMakeLists.txt").read_text()
).group(1)
p.add_argument(
    "--version", default=version, help="defaults to the CMake project version"
)
p.add_argument("--arch", default=platform.machine())
a = p.parse_args()

studio, resources = a.build / "evengine-studio", a.build / "resources"
for needed in [
    studio,
    resources / "ui/index.html",
    resources / "runtime",
    resources / "lib/libSDL3.so.0",
]:
    if not needed.exists():
        raise SystemExit(
            "Missing %s. Build with -DLOTA_DESKTOP=ON and GTK/WebKitGTK installed."
            % needed
        )

name = "EveNGINE-Studio-v%s-linux-%s" % (a.version, a.arch)
stage = a.output / name
if stage.exists():
    shutil.rmtree(stage)
stage.mkdir(parents=True)
shutil.copy2(studio, stage / "evengine-studio")
shutil.copytree(resources, stage / "resources")
strip = shutil.which("strip")
if strip:
    for binary in [
        stage / "evengine-studio",
        stage / "resources/runtime",
        stage / "resources/lib/libSDL3.so.0",
    ]:
        subprocess.run([strip, "--strip-unneeded", str(binary)], check=True)

templates = stage / "Templates"
templates.mkdir()
for project in sorted((root / "editor/examples").glob("*.eve")):
    shutil.copy2(project, templates)
# A ready-to-play sample, exported by the packaged Studio itself.
subprocess.run(
    [
        str(stage / "evengine-studio"),
        "--export",
        str(root / "editor/examples/Frontier-Watch.eve"),
        str(stage / "Frontier Watch"),
    ],
    check=True,
    stdout=subprocess.DEVNULL,
)
licenses = stage / "Licenses"
licenses.mkdir()
shutil.copy2(root / "LICENSE", licenses / "EveNGINE-LICENSE.txt")
for f in (root / "third_party").glob("*"):
    shutil.copy2(f, licenses)


def glibc(binary):
    out = subprocess.run(
        ["objdump", "-T", str(binary)], capture_output=True, text=True
    ).stdout
    versions = [
        tuple(map(int, v.split(".")))
        for v in re.findall(r"GLIBC_(\d+\.\d+(?:\.\d+)?)", out)
    ]
    return ".".join(map(str, max(versions))) if versions else "unknown"


minimum = max(
    (glibc(b) for b in [stage / "evengine-studio", stage / "resources/runtime"]),
    key=lambda v: tuple(map(int, v.split("."))) if v != "unknown" else (0,),
)
(stage / "install-desktop-entry.sh").write_text("""#!/bin/sh
# Adds EveNGINE Studio to your application menu and opens .eve projects with it.
# Run it again after moving this folder.
set -e
here=$(cd "$(dirname "$0")" && pwd)
data="${XDG_DATA_HOME:-$HOME/.local/share}"
mkdir -p "$data/applications" "$data/mime/packages"
cat > "$data/mime/packages/evengine-studio.xml" <<'XML'
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
  <mime-type type="application/x-evengine-project">
    <comment>EveNGINE project</comment>
    <glob pattern="*.eve"/>
    <glob pattern="*.lota"/>
  </mime-type>
</mime-info>
XML
{
  printf '%s\\n' '[Desktop Entry]' 'Type=Application' 'Name=EveNGINE Studio' \\
    'Comment=Make top-down adventure games' 'Categories=Development;Game;' \\
    'MimeType=application/x-evengine-project;' 'Terminal=false'
  printf 'Exec="%s/evengine-studio" %%f\\n' "$here"
  printf 'Icon=%s/resources/studio.png\\n' "$here"
} > "$data/applications/evengine-studio.desktop"
command -v update-mime-database >/dev/null && update-mime-database "$data/mime" || true
command -v update-desktop-database >/dev/null && update-desktop-database "$data/applications" || true
echo "Added EveNGINE Studio to your applications menu."
""")
(stage / "install-desktop-entry.sh").chmod(0o755)
(stage / "README.txt").write_text("""EveNGINE Studio %(version)s for Linux (%(arch)s)

Run ./evengine-studio to start. Run ./install-desktop-entry.sh to add Studio to your
application menu and open .eve projects with it.

Studio needs GTK 3 and WebKitGTK 4.1, which most desktops already have:
  Ubuntu 22.04+ / Debian 12+:  sudo apt install libwebkit2gtk-4.1-0
  Fedora:                      sudo dnf install webkit2gtk4.1
  Arch:                        sudo pacman -S webkit2gtk-4.1
This build needs glibc %(glibc)s or newer.

On SteamOS and other read-only systems, the package manager can't install WebKitGTK. Run
Studio inside a Distrobox container instead; see docs/LINUX.md in the EveNGINE repository:
https://github.com/eve-n37/EveNGINE/blob/main/docs/LINUX.md

Templates/        editable example projects (open them with Open)
Frontier Watch/   a sample game exported with Export app; run ./frontier-watch

Export app writes a folder with the game, its project and its own copy of SDL3. Players
don't need Studio or WebKitGTK. Saves are kept in ~/.local/share/LotAEngine/<project id>/.
""" % {"version": a.version, "arch": a.arch, "glibc": minimum})
archive = a.output / (name + ".tar.gz")
with tarfile.open(archive, "w:gz") as tar:
    tar.add(stage, arcname=name)
print(archive)
print("Minimum glibc:", minimum)
