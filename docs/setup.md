# Setup

From a fresh clone and your own disc image to the game running.

This repository contains none of the game's code or data. Everything the game needs is extracted
and generated from your own disc, on your machine.

## What you need

### Your game disc, as an ISO image

- **Ratchet & Clank**, North American release (NTSC-U), serial **`SCUS-97199`**.
- An unmodified ISO image of the disc. A good dump is 4,214,095,872 bytes (the size in the Redump
  database); a different size is accepted with a warning.
- The European release (PAL, `SCES-50916`) is not supported: its code sits at different addresses.

### A Linux system with

- **CMake** 3.20 or newer and a **C++20 compiler** (GCC 11 or newer, or Clang 13 or newer);
- **pkg-config**;
- **OpenGL**, **X11** and **Wayland** development files. The window library, GLFW, is built from
  source for both X11 and Wayland (it picks the one your desktop uses when the game starts), so it
  needs both: X11 with Xrandr, Xinerama, Xcursor, XInput and Xext, and Wayland with xkbcommon;
- **Python 3.8** or newer (the setup tool, and one of its data steps, are written in Python; no extra
  modules);
- optionally **FFmpeg** development files (`libavcodec`, `libavutil`), to play the game's movies.
  Without them the game runs and skips the movies;
- an internet connection for the first build (CMake downloads GLFW, GLM, Dear ImGui, TinyGLTF and
  miniaudio);
- about 1 GB of free disk for the game's data, besides the disc image;
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

   (Without `--iso` it asks for the path.) It goes through four steps:

   1. **Checks your system** and lists anything missing, with the install command for your
      distribution. It offers to run that command for you (it asks for your password); say no to
      run it yourself.
   2. **Checks your disc image**: that it is Ratchet & Clank's North American disc, complete, and
      the right size. The game reads the disc while it runs, so the setup makes
      `assets/iso/rac1-ntsc-u.iso` point to your image (a link: nothing is copied). Keep the image
      where it is; if you move it, run the setup again with its new path.
   3. **Builds the game** with CMake, in RelWithDebInfo. The result is `build/rac1-native`. It uses
      one job per CPU core, fewer when memory is short (`-j` changes that).
   4. **Generates the game's data** into `assets/` from your disc, step by step, showing each level
      as it goes. This takes a few minutes.

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
| `assets/dumps/SCUS_971.99` | the game's executable, copied from the disc |
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
  changed (CMake recompiles only what needs it) and redoes a data step only when the code that
  makes it changed, or when that step did not finish. A step added by an update (such as the
  flight between planets) runs on its own, without redoing the others. It remembers the disc image's path.
  What was generated, and from which code, is recorded in `assets/setup-state.json`.
- **`./setup.sh --force`** regenerates all the data; **`./setup.sh --force moby,world`** only those
  steps.
- **`./build/rac1-native --setup --status`** shows which parts of the data are present.

### All the options

`./setup.sh` (which runs `tools/setup.py`):

| Option | Meaning |
|---|---|
| `--iso <path>` | your disc image (otherwise `$RAC1_ISO`, then `assets/iso/rac1-ntsc-u.iso`, then it asks) |
| `--force [steps]` | regenerate the data even if it is up to date: all of it, or the steps given (`elf,wads,moby,world,flight,overlay`) |
| `-j <n>`, `--jobs <n>` | parallel build jobs |
| `--no-build` | skip the build |
| `--build-only` | only check the system and build |
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
what `./setup.sh` adds.

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

**"the game's data is missing"** when starting the game: it was started from another directory, or
the setup did not finish. Run it from the repository's root, and run `./setup.sh` again.

**The build stops with "Failed to find wayland-scanner"**, or about missing X11 or XKB headers: the
Wayland or X11 development files are missing; install the packages above.

**The movies are skipped**: FFmpeg's development files were not found when the game was built.
Install them (the optional packages above), delete `build/CMakeCache.txt` so the build looks again,
and run `./setup.sh`.

**A gamepad is not recognised**: see [Input](input.md#how-the-host-builds-the-packet) to add its
mapping. Gamepads can be connected while the game runs.
