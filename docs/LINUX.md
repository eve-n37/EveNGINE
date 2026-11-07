# Installing EveNGINE Studio on Linux

Studio for Linux runs on x86_64 with glibc 2.35 or later. The `.tar.gz` needs GTK 3 and WebKitGTK 4.1. Most
desktops already have GTK; WebKitGTK may need installing.

There are two Linux downloads:

- **`.tar.gz`**: a folder with Studio, the editable templates and a sample game. It uses the
  WebKitGTK installed on your system, so it's small and gets WebKit's security updates from
  your distribution.
- **`.AppImage`**: one file with WebKitGTK and GTK 3 built in. It runs without installing
  anything, including on SteamOS. It's larger, and its WebKitGTK only updates with new EveNGINE
  releases. The templates aren't inside; get them from the `.tar.gz` or the repository's
  `editor/examples` folder.

## The AppImage

Make it executable and run it:

```bash
chmod +x EveNGINE-Studio-*.AppImage
./EveNGINE-Studio-*.AppImage
```

You can also right-click it, choose **Properties → Permissions** and allow it to run, then
double-click it. To add it to your application menu, use an AppImage manager such as
[Gear Lever](https://flathub.org/apps/it.mijorus.gearlever), which you can install from
Discover on SteamOS.

## Regular distributions

| Distribution               | Command                                    |
| -------------------------- | ------------------------------------------ |
| Ubuntu 22.04+, Debian 12+  | `sudo apt install libwebkit2gtk-4.1-0`     |
| Fedora 36+                 | `sudo dnf install webkit2gtk4.1`           |
| Arch, Manjaro, EndeavourOS | `sudo pacman -S webkit2gtk-4.1`            |

Then unpack the download and run `./evengine-studio`. Run `./install-desktop-entry.sh` to add
Studio to your application menu and open `.eve` projects with it.

## SteamOS, Fedora Silverblue and other immutable systems

On these systems the system folders are read-only, so the package manager fails. On SteamOS,
`pacman` reports `unable to lock database` and `Read-only file system`.

The easiest option is the AppImage (see above). To use the `.tar.gz` instead, run Studio inside a [Distrobox](https://distrobox.it/) container instead. It comes with SteamOS 3.5
and later and is available on most immutable distributions (Fedora Silverblue can also use
`toolbox`). The container shares your home folder and survives system updates.

On a Steam Deck, switch to Desktop Mode and open Konsole. Then:

```bash
distrobox create --name eve --image ubuntu:22.04
distrobox enter eve
sudo apt update && sudo apt install -y libwebkit2gtk-4.1-0
```

Inside the container, go to the unpacked folder and run `./evengine-studio`. Your files are in the
same place as outside the container. To start Studio later, run `distrobox enter eve` first.
Running `./install-desktop-entry.sh` inside the container adds Studio to your application menu.

Avoid `sudo steamos-readonly disable` followed by `pacman`: the next SteamOS update erases what you
installed.

## Exported games

Games made with **Export app** don't need WebKitGTK or Distrobox. They include their own SDL3 and
run directly on SteamOS and other immutable systems.
