# SUM PLAYER

A native desktop media player built from scratch in C++ — hardware-accelerated
decoding, full subtitle support (text and bitmap), multi-audio track switching,
playlists, and a custom Qt-based UI. Built as a from-scratch systems programming
project, comparable in engineering scope to VLC/mpv rather than a tutorial toy.



## Features

- **Hardware-accelerated video decode** (D3D11VA) with automatic software fallback
- **Multi-audio track switching**, gapless, with no interruption to playback
- **Full subtitle support**:
  - Text subtitles (SRT, ASS/SSA) rendered via [libass](https://github.com/libass/libass)
  - Bitmap subtitles (PGS/Blu-ray) with correct signal-driven timing
  - Load external subtitle files at runtime
  - Adjustable subtitle text size
- **Playlists** — create, rename, delete; add local video files; thumbnails
  generated automatically; persists across restarts (cleared with cache)
- **Playback controls** — seek (click-anywhere progress bar), loop, aspect
  ratio modes (Fit / Fill / 16:9 / 4:3 / 1:1 / 2.35:1), volume with mute
- **Fullscreen** with auto-hiding controls
- **Settings** — subtitle size, default volume, persisted between sessions

## Built with

- **C++17**, MSVC toolchain, CMake
- **Qt6** (Widgets) — application UI
- **FFmpeg** — demuxing, decoding, format handling (GPL build)
- **libass** — subtitle rendering
- **vcpkg** — dependency management for libass and its dependencies

## Building from source

### Prerequisites

- Visual Studio 2022 Build Tools (MSVC, C++ workload)
- CMake 3.20+
- Qt 6 (Widgets, Multimedia)
- FFmpeg (GPL shared build) — [gyan.dev builds](https://www.gyan.dev/ffmpeg/builds/) recommended
- [vcpkg](https://github.com/microsoft/vcpkg) with `libass` installed

### Steps

```powershell
# Install libass via vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg.exe install libass:x64-windows

# Clone and configure SumPlayer
git clone https://github.com/sumanth0021/SumPlayer.git
cd SumPlayer
```

Update the paths in `CMakeLists.txt` (`FFMPEG_DIR`, `VCPKG_DIR`) to match your local setup, then:

```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

The built executable and all required assets/DLLs will be in `build/Release/`.

## Download

Prebuilt Windows installers are available on the
[Releases page](https://github.com/sumanth0021/SumPlayer/releases).

## Project status

This is an active learning/portfolio project. See open items and architecture
notes in [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md) (if present).

## License

SUM PLAYER is licensed under the **GNU General Public License v3.0** — see
[LICENSE](LICENSE). This is required because SUM PLAYER links against the
GPL build of FFmpeg. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
for a full list of third-party components and their licenses.

## Acknowledgments

- [FFmpeg](https://ffmpeg.org) for media handling
- [libass](https://github.com/libass/libass) for subtitle rendering
- [Lucide](https://lucide.dev) for icons

SUM PLAYER - A native media player built with Qt, FFmpeg, and libass
Copyright (C) 2026 Sumanth

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.