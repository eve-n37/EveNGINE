# Third-party runtime components

This engine is original project code built on established platform libraries;
it is not a fork of Godot or SDL. No proprietary Nintendo game assets are used.

- Desktop: SDL3 3.2.20, unmodified official source/framework. Its zlib licence
  is reproduced in `SDL3-LICENSE.txt` and inside the supplied framework.
- 3DS: devkitPro libctru 2.7.0, Citro2D 1.7.0, Citro3D 1.7.1. Their notices
  are reproduced alongside this file. The devkitPro ARM toolchain supplies the
  C/C++ runtime used by the homebrew executable.
- The v0.6.0 editor/runtime and SDK libraries link the system libc++ and macOS
  frameworks. They do not embed a C++ runtime. The retained v0.3.0 notices are
  included for historical SDK files; new Mac builds use Clang/LLVM 18.1.3.

The authoring and packaging tools themselves are not included in the runtime.
Original sample and built-in artwork is included under `editor`; the built-in generator is `editor/assets/build_assets.py`. Original engine code and sample content are distributed under the MIT License in the repository root.
