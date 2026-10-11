# Other platforms: the plan

rac1-native builds and runs on Linux x86-64 only for now. This page is the plan for Windows, and a
note on macOS. Nothing here is started yet.

The goal is one codebase that builds on Linux and Windows and behaves identically on both: same
game state, same oracle results, same reference images.

## What is already portable

- **The libraries.**
  - GLFW: window, input, gamepads.
  - glad and OpenGL 4.6.
  - Dear ImGui, the menus' widgets.
  - miniaudio: audio output.
  - OpenGOAL's 989snd.

  All of them support Windows.
- **The game's code** (`src/game/`) is standard C++20 with no operating-system calls.
- **OpenGL 4.6**, compute shaders included, is supported on Windows by the NVIDIA, AMD and Intel drivers.

## What needs work

The Linux-specific parts are few and in known places:

| Area | Where | Linux | Windows |
|---|---|---|---|
| The game frame's coroutine ([Decisions](decisions.md), 12) | `runtime/ee/ee_thread.{h,cpp}` | `ucontext` | Fibers (`CreateFiber` / `SwitchToFiber`) behind a small interface |
| Memory-mapped files | `src/io/mapped_file.h` and one other file | `mmap` | `CreateFileMapping` / `MapViewOfFile` |
| Float rounding toward zero ([Decisions](decisions.md), 11) | `runtime/ee/ps2_fpu.h` | `fesetround` / MXCSR | MXCSR (`_mm_setcsr`), the same mode |
| Thread priority | `src/host/host_priority.cpp` | `sched`, `setrlimit` | `SetThreadPriority`, MMCSS |
| Settings and saves folder | the host code (`HOME`, `XDG_*`) | `~/.config/rac1-native/` | `%APPDATA%\rac1-native\` |
| Crash backtraces | `execinfo` | glibc | `DbgHelp` (or none) |
| `/proc` reads, `dlfcn` | a few host helpers | Linux | Win32 equivalents, or skipped |
| The setup | `setup.sh`, `tools/setup.py` | bash; package checks per distro | a Python (or PowerShell) entry point; instructions for Visual Studio / MSYS2 |
| Movies (optional) | FFmpeg through pkg-config | the distro's packages | vcpkg or MSYS2 |

All operating-system calls go behind one small host interface, so `src/game/` and the rest of
`runtime/` stay unchanged.

## The compiler: the important choice

The port must stay faithful: the PS2's float rounding, and the same results as the original in
every oracle check. That depends on the compiler generating the same float operations on both
platforms (no contraction into FMA, no reordering: today `-frounding-math`, with
`-ffp-contract=off` checked).

- **Recommended: clang** (clang-cl, or MinGW-w64 GCC) on Windows too. It has the same language
  extensions and the same float behaviour as the Linux build, so the reference images and oracle
  results should match bit for bit, which proves the Windows build behaves the same.
- **MSVC** is possible later. It means more work (a few GCC extensions; the very large
  generated sources) and more risk to float exactness, so it is a second step if it is ever
  needed.

## Verification

- **Build:** a CI job builds the game on a Windows runner (GitHub Actions) with clang.
- **Behaviour:** the headless reference runs on Windows (`RAC1_HEADLESS`) must give the same
  image hashes as on Linux, and the oracle checks must pass. These need the player's own disc, so
  they run on a developer's machine, not in public CI.
- **A human playtest** on a Windows PC.

## Steps

1. Put the host interface in place on Linux (coroutine, mapped files, rounding, priority, paths,
   backtraces), with no change in behaviour (same reference hashes).
2. Add the Windows implementations and a clang build in CMake.
3. Port the setup: one Python entry point for both systems, and Windows install instructions.
4. Windows CI build; reference runs and oracle on a Windows machine; playtest.
5. A release with a Windows build. The player still runs the setup with their own disc.

## macOS

macOS stops at OpenGL 4.1 (deprecated, no compute shaders), so the current renderer can't run
there. macOS comes after a Vulkan renderer, through MoltenVK, or a Metal one (see
[Roadmap](roadmap.md)). The host-interface work above would carry over.

## Other devices

The code is portable C++, but a device needs more than a renderer:
- a graphics API it supports (OpenGL ES or a console's own);
- a little-endian processor, because the game's memory is little-endian like the PS2's;
- enough memory and speed.

Each such port is its own project.
