#!/usr/bin/env python3
"""Package EveNGINE Studio for Linux as a single AppImage that brings its own WebKitGTK.

The tar.gz package uses the WebKitGTK installed on the system. This AppImage bundles WebKitGTK,
GTK 3 and their libraries instead, so it also runs where they can't be installed, such as
SteamOS. Build Studio first (see package_linux.py), then:
  python3 tools/package_appimage.py --build build-linux --output deliverables \\
      --appimagetool appimagetool-x86_64.AppImage --runtime runtime-x86_64

How it works:
- Every library the Studio shell, WebKit's helper processes and GDK-PixBuf's image loaders need
  is copied to usr/lib, apart from the ones that must come from the host (glibc, the graphics
  drivers, X11/xcb and a few others listed below). RUNPATHs point binaries at usr/lib.
- WebKitGTK looks for its helper processes at a path fixed when it was compiled. The copy of
  libwebkit2gtk has that path changed from /usr/lib/... to the relative ././/lib/..., and AppRun
  starts Studio inside usr, so the bundled helpers are found.

Like the tar.gz, the AppImage needs at least the glibc of the machine it was built on.
"""

import argparse, os, platform, re, shutil, subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument("--build", required=True, type=Path, help="CMake build folder")
p.add_argument("--output", required=True, type=Path)
p.add_argument(
    "--appimagetool", type=Path, help="leave out to only assemble the AppDir folder"
)
p.add_argument("--runtime", type=Path, help="AppImage type 2 runtime file")
version = re.search(
    r"project\(EveNGINESDK VERSION ([0-9.]+)", (root / "CMakeLists.txt").read_text()
).group(1)
p.add_argument("--version", default=version)
a = p.parse_args()
arch = platform.machine()
triplet = "%s-linux-gnu" % arch
system_lib = Path("/usr/lib") / triplet
webkit_dir = system_lib / "webkit2gtk-4.1"

# Libraries that must come from the host: the C library, graphics drivers and the libraries
# they use, plus the X11 and font libraries every desktop has. Based on the AppImage
# project's excludelist (https://github.com/AppImageCommunity/pkg2appimage).
host_only = re.compile(
    r"^(ld-linux.*|libc|libm|libmvec|libdl|libpthread|librt|libresolv|libutil|libanl"
    r"|libthread_db|libBrokenLocale|libnss_.*|libgcc_s|libstdc\+\+"
    r"|libGL|libEGL|libGLX|libGLdispatch|libOpenGL|libGLESv2|libdrm.*|libgbm"
    r"|libX11|libX11-xcb|libxcb.*|libxshmfence|libICE|libSM"
    r"|libasound|libjack|libfontconfig|libfreetype|libharfbuzz|libfribidi|libexpat|libz"
    r"|libcom_err|libgpg-error|libp11-kit|libusb-1\.0|libwayland-.*)\.so(\.|$)"
)

appdir = a.output / "AppDir"
if appdir.exists():
    shutil.rmtree(appdir)
usr = appdir / "usr"
lib = usr / "lib"
for folder in [usr / "bin", lib, usr / "share"]:
    folder.mkdir(parents=True)

shutil.copy2(a.build / "evengine-studio", usr / "bin/evengine-studio")
shutil.copytree(a.build / "resources", usr / "bin/resources")
helpers = lib / triplet / "webkit2gtk-4.1"
helpers.mkdir(parents=True)
for name in ["WebKitWebProcess", "WebKitNetworkProcess", "WebKitGPUProcess"]:
    if (webkit_dir / name).exists():
        shutil.copy2(webkit_dir / name, helpers / name)
shutil.copytree(webkit_dir / "injected-bundle", helpers / "injected-bundle")
pixbuf = system_lib / "gdk-pixbuf-2.0/2.10.0/loaders"
loaders = lib / "gdk-pixbuf-2.0/loaders"
loaders.parent.mkdir(parents=True)
shutil.copytree(pixbuf, loaders)


def dependencies(binary):
    out = subprocess.run(["ldd", str(binary)], capture_output=True, text=True).stdout
    for line in out.splitlines():
        m = re.match(r"\s*(\S+) => (/\S+)", line)
        if m:
            yield m.group(1), Path(m.group(2))
        elif "not found" in line:
            raise SystemExit("%s: %s" % (binary, line.strip()))


# Binaries whose libraries are bundled, and the RUNPATH that finds usr/lib from each.
binaries = {usr / "bin/evengine-studio": "$ORIGIN/../lib"}
for f in helpers.iterdir():
    if f.is_file():
        binaries[f] = "$ORIGIN/../.."
for f in loaders.iterdir():
    binaries[f] = "$ORIGIN/../.."
for f in (helpers / "injected-bundle").iterdir():
    binaries[f] = "$ORIGIN/../../.."

bundled = {}
pending = list(binaries)
while pending:
    for soname, path in dependencies(pending.pop()):
        if soname in bundled or host_only.match(soname):
            continue
        target = lib / soname
        shutil.copy2(path.resolve(), target)
        target.chmod(0o755)
        bundled[soname] = target
        pending.append(target)
for target in bundled.values():
    binaries[target] = "$ORIGIN"
for binary, runpath in binaries.items():
    subprocess.run(["patchelf", "--set-rpath", runpath, str(binary)], check=True)

# Point WebKit at the bundled helper processes (same-length change, see above).
webkit = lib / "libwebkit2gtk-4.1.so.0"
data = webkit.read_bytes()
old = ("/usr/lib/%s/webkit2gtk-4.1" % triplet).encode()
new = b"././" + old[4:]
if old not in data:
    raise SystemExit("WebKit's helper path was not found in " + str(webkit))
webkit.write_bytes(data.replace(old, new))
print("Patched %d WebKit helper paths" % data.count(old))

# Image loaders, listed relative to usr (AppRun's working folder).
cache = subprocess.run(
    ["/usr/lib/%s/gdk-pixbuf-2.0/gdk-pixbuf-query-loaders" % triplet]
    + [str(f) for f in sorted(loaders.glob("*.so"))],
    capture_output=True,
    text=True,
    check=True,
).stdout
(lib / "gdk-pixbuf-2.0/loaders.cache").write_text(cache.replace(str(usr) + "/", ""))

# GTK's settings schemas (the file chooser needs them).
schemas = usr / "share/glib-2.0/schemas"
schemas.mkdir(parents=True)
for xml in Path("/usr/share/glib-2.0/schemas").glob("org.gtk.*.xml"):
    shutil.copy2(xml, schemas)
subprocess.run(["glib-compile-schemas", str(schemas)], check=True)
(lib / "gio/modules").mkdir(parents=True)

strip = shutil.which("strip")
if strip:
    for binary in binaries:
        subprocess.run([strip, "--strip-unneeded", str(binary)], check=True)

(appdir / "AppRun").write_text(r"""#!/bin/sh
# EveNGINE Studio AppImage launcher. See tools/package_appimage.py.
here="$(dirname "$(readlink -f "$0")")"
export APPDIR="${APPDIR:-$here}"
export EVE_STUDIO_WORKDIR="${OWD:-$PWD}"
export GDK_PIXBUF_MODULE_FILE="$APPDIR/usr/lib/gdk-pixbuf-2.0/loaders.cache"
export GSETTINGS_SCHEMA_DIR="$APPDIR/usr/share/glib-2.0/schemas"
export GIO_MODULE_DIR="$APPDIR/usr/lib/gio/modules"
export XDG_DATA_DIRS="$APPDIR/usr/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"
cd "$APPDIR/usr" || exit 1
exec "$APPDIR/usr/bin/evengine-studio" "$@"
""")
(appdir / "AppRun").chmod(0o755)
(appdir / "evengine-studio.desktop").write_text(
    "[Desktop Entry]\nType=Application\nName=EveNGINE Studio\n"
    "Comment=Make 2D adventure games\nExec=evengine-studio %f\nIcon=evengine-studio\n"
    "Categories=Development;Game;\nMimeType=application/x-evengine-project;\nTerminal=false\n"
)
shutil.copy2(a.build / "resources/studio.png", appdir / "evengine-studio.png")
os.symlink("evengine-studio.png", appdir / ".DirIcon")

if not a.appimagetool:
    raise SystemExit(print(appdir, "(%d bundled libraries)" % len(bundled)))
image = a.output / ("EveNGINE-Studio-v%s-linux-%s.AppImage" % (a.version, arch))
tool = [str(a.appimagetool.resolve()), "--appimage-extract-and-run"]
command = tool + ["--no-appstream", "--comp", "xz", str(appdir), str(image)]
if a.runtime:
    command[2:2] = ["--runtime-file", str(a.runtime.resolve())]
subprocess.run(command, check=True, env=dict(os.environ, ARCH=arch))
print(image, "(%d bundled libraries)" % len(bundled))
