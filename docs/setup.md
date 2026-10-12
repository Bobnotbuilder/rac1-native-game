# Setup

From a fresh clone and your own disc image to the game running.

This repository contains none of the game's code or data. Everything the game needs is extracted
and generated from your own disc, on your machine: the game's data, and also part of the port's
source code, the files that are a mechanical translation of the game's own code (see
[Architecture](architecture.md#the-generated-code)).

## What you need

### Your game disc, as an ISO image

- **Ratchet & Clank**, North American release (NTSC-U), serial **`SCUS-97199`**.
- An unmodified ISO image of the disc. A good dump is 4,214,095,872 bytes (the size in the Redump
  database); a different size is accepted with a warning.
- The European release (PAL, `SCES-50916`) is not supported: its code sits at different addresses.

### A Linux system with

(Windows: see [On Windows](#on-windows-experimental) below.)

- **CMake** 3.20 or newer and a **C++20 compiler** (GCC 11 or newer, or Clang 13 or newer);
- **pkg-config**;
- **OpenGL**, **X11** and **Wayland** development files. The window library, GLFW, is built from
  source for both X11 and Wayland (it picks the one your desktop uses when the game starts), so it
  needs both: X11 with Xrandr, Xinerama, Xcursor, XInput and Xext, and Wayland with xkbcommon;
- **Python 3.8** or newer (the setup tool, the code generators and the overlay matcher are written
  in Python; no extra modules);
- optionally **FFmpeg** development files (`libavcodec`, `libavutil`), to play the game's movies.
  Without them the game runs and skips the movies;
- an internet connection for the first build (CMake downloads GLFW, GLM, Dear ImGui, TinyGLTF and
  miniaudio);
- about 1 GB of free disk for the game's data and generated code, besides the disc image, plus about
  2 GB for the build;
- a GPU and driver with **OpenGL 4.6**.

The setup tool checks all of this and tells you what is missing, with the command to install it on
Debian and Ubuntu (and their derivatives), Fedora, and Arch. It never installs anything without
asking. For reference, everything at once:

```bash
# Debian / Ubuntu
sudo apt install cmake build-essential pkg-config libgl-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev libwayland-dev libxkbcommon-dev python3 libavcodec-dev libavutil-dev

# Fedora
sudo dnf install cmake gcc-c++ pkgconf-pkg-config mesa-libGL-devel libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel libXext-devel wayland-devel libxkbcommon-devel python3 ffmpeg-free-devel

# Arch
sudo pacman -S --needed cmake base-devel pkgconf mesa libx11 libxrandr libxinerama libxcursor libxi libxext wayland libxkbcommon python ffmpeg
```

## Step by step

1. **Clone the repository:**

   ```bash
   git clone https://github.com/daino-selvatico/rac1-native-game.git
   cd rac1-native-game
   ```

2. **Run the setup, with the path to your disc image:**

   ```bash
   ./setup.sh --iso /path/to/your/ratchet.iso
   ```

   (Without `--iso` it asks for the path.) It goes through five steps:

   1. **Checks your system** and lists anything missing, with the install command for your
      distribution. It offers to run that command for you (it asks for your password); say no to
      run it yourself.
   2. **Checks your disc image**: that it is Ratchet & Clank's North American disc and the right
      size.
   3. **Generates the game's code** from your disc, with Python only: it copies the game's boot
      executable from the disc, matches each level's code against the first level's (the overlay
      address maps), then translates the parts of the game's code the port takes over mechanically
      into C++ under `assets/generated/src/` (about 140 files). This takes one to two minutes and
      uses the same number of parallel jobs as the build.
   4. **Builds the game** with CMake, in RelWithDebInfo, from the repository's sources and the
      generated ones. The result is `build/rac1-native`. It uses one job per CPU core, fewer when
      memory is short (`-j` changes that).
   5. **Generates the game's data** into `assets/` from your disc, step by step, showing each level
      as it goes. This takes a few minutes. First it checks the disc image fully (that every level
      is inside it), and, because the game reads the disc while it runs, makes
      `assets/iso/rac1-ntsc-u.iso` point to your image (a link: nothing is copied). Keep the image
      where it is; if you move it, run the setup again with its new path.

   At the end it offers to start the game.

3. **Play.** From the repository's root:

   ```bash
   ./build/rac1-native
   ```

   The game must be started from the repository's root: it finds `assets/` and its other files
   relative to the current directory.

## What the setup generates

All of it goes into `assets/`, which git ignores:

| Path | What |
|---|---|
| `assets/iso/rac1-ntsc-u.iso` | a link to your disc image, which the game reads while it runs |
| `assets/iso/disc-path.txt` | on Windows, when the link cannot be made: your disc image's path, which the game reads instead |
| `assets/dumps/SCUS_971.99` | the game's executable, copied from the disc |
| `assets/generated/src/` | the generated part of the game's source code, and `generated_sources.cmake`, its list for the build (only the build reads it) |
| `assets/extracted/level_wads/<n>_<name>/level.wad` | the data of each of the 19 levels |
| `assets/generated/mrp/moby/` | the moby models and textures for the renderer, per level |
| `assets/generated/mrp/world/` | the world's geometry and textures for the renderer, per level |
| `assets/generated/mrp/{moby,world}/flight/` | the loading screen's scene: the ship's flight between planets |
| `assets/generated/overlay_map/` | the per-level address maps (made by `tools/overlay_match/`) |
| `assets/setup-state.json` | what the setup generated, and from which code (so a re-run redoes only what changed) |

How these are made is described in [Rendering](rendering.md) and
[Architecture](architecture.md#per-level-overlays-and-the-ov-macros).

## Running it again

- **Run `./setup.sh` again at any time**, for example after a `git pull`. It rebuilds only what
  changed (CMake recompiles only what needs it), generates the code again only when the generators
  (`tools/codegen/`), the overlay matcher or the headers they read changed, and redoes a data step
  only when the code that makes it changed, or when that step did not finish. A step added by an update (such as the
  flight between planets) runs on its own, without redoing the others. It remembers the disc image's path.
  What was generated, and from which code, is recorded in `assets/setup-state.json`.
- **`./setup.sh --force`** regenerates the code and all the data; **`./setup.sh --force moby,world`**
  only those steps.
- If you build with CMake yourself (`cmake -S . -B build && cmake --build build`), run `./setup.sh`
  once first: configuring stops with a message when the generated code is missing, or out of date
  after a `git pull`.
- **`./build/rac1-native --setup --status`** shows which parts of the data are present.

### All the options

`./setup.sh` (which runs `tools/setup.py`):

| Option | Meaning |
|---|---|
| `--iso <path>` | your disc image (otherwise `$RAC1_ISO`, then `assets/iso/rac1-ntsc-u.iso`, then it asks) |
| `--force [steps]` | regenerate even if up to date: everything, or the steps given (`elf,overlay,code,wads,moby,world,flight`) |
| `-j <n>`, `--jobs <n>` | parallel jobs, for the code generation and the build |
| `--no-build` | skip the build |
| `--build-only` | only check the system and build (generating the code first if it is missing or out of date, which needs the disc) |
| `--no-play` | do not offer to start the game at the end |

`./build/rac1-native --setup` (the data generation on its own, run from the repository's root):

| Option | Meaning |
|---|---|
| `--iso <path>` | the disc image (default: `$RAC1_ISO`, then `assets/iso/rac1-ntsc-u.iso`) |
| `--assets <dir>` | where to write the data (default `assets`; the game itself only reads `assets/`) |
| `--force` | regenerate even if present |
| `--step elf\|wads\|moby\|world\|flight\|overlay\|all` | run one step only |
| `--check-iso` | only check the disc image, and make the game find it |
| `--status` | show what is present |

On its own, `--setup` skips a file that exists without checking whether it is up to date; that is
what `./setup.sh` adds. `./setup.sh` makes the boot executable and the overlay maps itself, before
the build, because the code generation needs them; the `elf` and `overlay` steps of `--setup` make
the same files.

The code generation on its own, from the repository's root (after `./setup.sh` has made the boot
executable and the overlay maps):

```bash
python3 tools/codegen/generate_all.py --out assets/generated --no-insn-comments
```

`--list` shows its units, `--files` the files it writes, `--only <unit>` runs some of them. Without
`--no-insn-comments`, each translated line keeps the original instruction as a comment.

## On Windows (experimental)

> **Windows support is experimental.** The game runs on Windows and most of it works without
> problems in the tests so far, but it has been tried on one PC only: bugs that only Windows shows
> can't be excluded. Please report them.

The Windows build is made with **clang** (or GCC) from **MSYS2**, not with Visual Studio's compiler
(see [Platforms](platforms.md)).

1. Get the repository (with git, or as a zip) and **double-click `setup.cmd`** in its folder (or drop
   your disc image on it, or run `setup.cmd --iso C:\Users\you\Games\ratchet.iso`).

   Nothing needs to be installed first. The first time, the setup downloads the tools that build the
   game (about 1 GB, a few minutes): a private copy of [MSYS2](https://www.msys2.org) in the
   repository's `.toolchain\` folder, with Python, clang, CMake, Ninja and FFmpeg
   (`tools\windows\toolchain.ps1`). Nothing is installed elsewhere: deleting `.toolchain\` removes it.
   Then `tools/setup.py` runs, with the same steps as on Linux, and the result is
   `build\rac1-native.exe`.

   With your own MSYS2, the setup can also run from its **CLANG64** shell (`./setup.sh`, or
   `setup.cmd`): it then uses that shell's tools and tells you which packages to install.

2. **Play**: double-click `build\rac1-native.exe`.

What differs from Linux:

- **The disc image.** Windows only lets programs make links with Developer Mode on (or as an
  administrator). Without it, the setup writes your image's path into `assets/iso/disc-path.txt`
  instead, and the game reads it from there. Keep the image where it is, as on Linux.
- **The settings and saves** are in `%APPDATA%\rac1-native\` (the settings files, `controls.txt`,
  and the memory card in `memcard\`), instead of `~/.config/rac1-native/` and
  `~/.local/share/rac1-native/`.
- **The DLLs.** Built without FFmpeg, the program needs only Windows' own DLLs (the C++ runtime is
  linked in). Built with FFmpeg (the movies), it uses MSYS2's DLLs: the build copies them into
  `build\`, beside the program.

## Troubleshooting

**"This is Ratchet & Clank's PAL disc"** (or NTSC-J): only the North American disc, `SCUS-97199`,
is supported for now. The other releases' code sits at different addresses.

**"not a PlayStation 2 disc image"** or **"This is not a disc image (ISO 9660)"**: the file is not a
plain ISO. Use an unmodified `.iso` dump, not a compressed format (`.chd`, `.cso`, `.gz`) or a
`.bin`/`.cue` pair.

**"This is another game"**: the image is a different PS2 game.

**"The disc image is damaged or incomplete"**: the image is cut short (an interrupted copy or
dump). Dump the disc again.

**A size warning**: the image is not the expected 4,214,095,872 bytes but looks complete. It may
still work; if the game misbehaves, check your dump against the Redump record.

**"your disc image was not found"** when starting the game: the image was moved or deleted since the
setup. Run `./setup.sh --iso <new path>`.

**"The game's generated sources are missing"** or **"... are out of date"** when building: the build
needs the code that `./setup.sh` generates from your disc. Run `./setup.sh` (it generates the code,
then builds). If the code generation itself fails, its full output is in `build/setup-codegen.log`.

**"the game's data is missing"** when starting the game: it was started from another directory, or
the setup did not finish. Run it from the repository's root, and run `./setup.sh` again.

**The build stops with "Failed to find wayland-scanner"**, or about missing X11 or XKB headers: the
Wayland or X11 development files are missing; install the packages above.

**The movies are skipped**: FFmpeg's development files were not found when the game was built.
Install them (the optional packages above), delete `build/CMakeCache.txt` so the build looks again,
and run `./setup.sh`.

**A gamepad is not recognised**: see [Input](input.md#how-the-host-builds-the-packet) to add its
mapping. Gamepads can be connected while the game runs.
