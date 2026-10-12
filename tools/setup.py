#!/usr/bin/env python3
"""rac1-native's guided setup (run it through ./setup.sh; on Windows, setup.cmd).

From a fresh clone and your own disc image (Ratchet & Clank, NTSC-U, SCUS-97199) to the game:
  1. checks the system and says what to install;
  2. checks your disc image;
  3. generates the part of the game's code that is made from the game's own code on the disc
     (Python only, before the build): the boot executable and the overlay maps it needs, then
     tools/codegen/generate_all.py into assets/generated/src/;
  4. builds the game (build/rac1-native);
  5. generates the game's data into assets/ (`rac1-native --setup`, one step at a time), and makes the
     game find the disc (assets/iso/rac1-ntsc-u.iso);
  6. offers to start the game.

Re-runnable: the build only recompiles what changed, and each step is redone only when the sources
that make its output (or the disc) changed since it last ran, or when it is forced (--force).
The record of what was generated with what is assets/setup-state.json. docs/setup.md is the guide.

`tools/setup.py --code-inputs` (for CMake) prints the fingerprint of the generated code's sources, then
those sources, one per line.
"""

import argparse
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WINDOWS = os.name == "nt"  # (a native Windows Python: MSYS2's CLANG64 / UCRT64 one, or python.org's)
SETUP_CMD = "setup.cmd" if WINDOWS else "./setup.sh"  # (what the messages tell the player to run)
BUILD_DIR = ROOT / "build"
EXE = BUILD_DIR / ("rac1-native.exe" if WINDOWS else "rac1-native")
ASSETS_DIR = ROOT / "assets"
GAME_ISO = ASSETS_DIR / "iso/rac1-ntsc-u.iso"  # where the game reads the disc (runtime/kernel/disc.cpp)
# where links cannot be made (Windows without the privilege), the disc image's path instead (runtime/kernel/disc.cpp)
GAME_ISO_PATH_FILE = ASSETS_DIR / "iso/disc-path.txt"
STATE_FILE = ASSETS_DIR / "setup-state.json"
SECTOR = 2048

# The generated code: tools/codegen/generate_all.py writes CODE_ROOT/src/... (the same tree as src/), and
# this tool adds GEN_LIST, the list of the generated files for CMake (src/game/CMakeLists.txt).
CODE_ROOT = ASSETS_DIR / "generated"
GEN_SRC = CODE_ROOT / "src"
GEN_LIST = GEN_SRC / "generated_sources.cmake"
GENERATE_ALL = ROOT / "tools/codegen/generate_all.py"
ELF = ASSETS_DIR / "dumps/SCUS_971.99"
OVERLAY_MAPS = ASSETS_DIR / "generated/overlay_map"

# The generated code's sources: the generators and their inputs, and the headers (with variants_l<n>.cpp, the
# levels' variant tables) whose `// orig:` declarations the generators read. (The boot executable and the overlay
# maps they also read are steps of their own, below.)
CODE_SOURCES = ["tools/codegen/**/*", "tools/overlay_match/overlay_match.py", "tools/overlay_match/front_match.py",
                "src/game/**/*.h", "src/game/ovl/variants*.cpp"]
# What the generated VU programs come from, for the data steps that run them
VU_CODEGEN = ["tools/codegen/vu2cpp*.py", "tools/codegen/vu_micro.py"]

# The steps before the build, done by this tool (Python only), with the sources that make each one's output.
CODE_STEPS = [
    ("elf", "the boot executable", []),
    ("overlay", "the overlay address maps", ["tools/overlay_match/*.py"]),
    ("code", "the generated code", CODE_SOURCES),
]
# The data steps of `rac1-native --setup --step <name>`, in order, with the sources that make each
# one's output: a change to any of them (after a git pull) makes the step run again.
STEPS = [
    ("wads", "the levels' data", ["src/setup/iso_extract.*"]),
    ("moby", "the moby models and textures",
     ["src/setup/mrp_export.*", "src/setup/kick_trace.*", "src/formats/**/*", "src/game/ovl/moby_vu1*",
      "tools/codegen/gen_moby_vu_fast.py", *VU_CODEGEN, "runtime/vu/**/*"]),
    ("world", "the world geometry and textures",
     ["src/setup/mrp_export.*", "src/setup/world_trace.*", "src/formats/**/*", "src/game/ovl/world_vu1*",
      "tools/codegen/gen_world_vu.py", *VU_CODEGEN, "src/game/render/mrp_world_assets.*", "runtime/vu/**/*"]),
    ("flight", "the flight between planets (the loading screen)",
     ["src/setup/scene_export.*", "src/setup/kick_trace.*", "src/formats/**/*", "src/game/ovl/moby_vu1*",
      "tools/codegen/gen_moby_vu_fast.py", *VU_CODEGEN, "src/game/render/mrp_world_assets.*", "runtime/vu/**/*"]),
]
STEP_NAMES = [s[0] for s in CODE_STEPS] + [s[0] for s in STEPS]

def enable_windows_colors():
    """The console's escape sequences (Windows 10 and newer); False when they cannot be turned on."""
    try:
        import ctypes
        kernel32 = ctypes.windll.kernel32
        handle = kernel32.GetStdHandle(-11)  # STD_OUTPUT_HANDLE
        mode = ctypes.c_uint32()
        return bool(kernel32.GetConsoleMode(handle, ctypes.byref(mode))) and \
            bool(kernel32.SetConsoleMode(handle, mode.value | 0x0004))  # ENABLE_VIRTUAL_TERMINAL_PROCESSING
    except (AttributeError, OSError):
        return False


USE_COLOR = sys.stdout.isatty() and (not WINDOWS or enable_windows_colors())


def color(code, text):
    return f"\033[{code}m{text}\033[0m" if USE_COLOR else text


def step(n, total, msg):
    print("\n" + color("1;36", f"[{n}/{total}] {msg}"), flush=True)


def ok(msg):
    print(color("32", f"  OK  {msg}"), flush=True)


def warn(msg):
    print(color("33", f"  !   {msg}"), flush=True)


def fail(msg):
    print(color("1;31", f"  ERROR  {msg}"), flush=True)
    sys.exit(1)


def ask(question, default):
    """A yes / no question; False when there is no one to answer it."""
    if not sys.stdin.isatty():
        return False
    try:
        reply = input(f"  {question} {'[Y/n]' if default else '[y/N]'} ").strip().lower()
    except (EOFError, KeyboardInterrupt):
        print()
        return False
    return default if not reply else reply.startswith("y")


# --- 1. The system ------------------------------------------------------------------------------

def detect_distro():
    data = {}
    try:
        for line in Path("/etc/os-release").read_text().splitlines():
            if "=" in line:
                k, v = line.split("=", 1)
                data[k] = v.strip('"').lower()
    except OSError:
        return None
    ids = [data.get("ID", "")] + data.get("ID_LIKE", "").split()
    for family, names in (("debian", ("debian", "ubuntu")), ("fedora", ("fedora", "rhel", "centos")),
                          ("arch", ("arch",))):
        if any(i in names for i in ids):
            return family
    return None


def pkg_config(*modules, extra_path=None):
    if not shutil.which("pkg-config"):
        return False
    env = dict(os.environ)
    if extra_path and Path(extra_path).is_dir():
        env["PKG_CONFIG_PATH"] = extra_path + ":" + env.get("PKG_CONFIG_PATH", "")
    return subprocess.run(["pkg-config", "--exists", *modules], env=env, stdout=subprocess.DEVNULL,
                          stderr=subprocess.DEVNULL).returncode == 0


# (CMakeLists.txt also looks for FFmpeg in Homebrew's prefix)
HOMEBREW_PKGCONFIG = "/home/linuxbrew/.linuxbrew/lib/pkgconfig"


def has_header(*paths):
    return any(Path("/usr/include", p).exists() for p in paths)


def cmake_ok():
    if not shutil.which("cmake"):
        return False
    try:
        ver = subprocess.check_output(["cmake", "--version"], text=True).split()[2]
        return tuple(int(x) for x in ver.split(".")[:2]) >= (3, 20)
    except (subprocess.SubprocessError, ValueError, IndexError):
        return False


def compiler_ok():
    """A C++20 compiler: GCC 11+ or Clang 13+ (the one CMake would pick, c++, or either by name)."""
    for name, minimum in (("c++", 0), ("g++", 11), ("clang++", 13)):
        path = shutil.which(name)
        if not path:
            continue
        try:
            out = subprocess.check_output([path, "--version"], text=True)
            ver = int(subprocess.check_output([path, "-dumpversion"], text=True).strip().split(".")[0])
        except (subprocess.SubprocessError, ValueError):
            continue
        need = minimum or (13 if "clang" in out.lower() else 11)
        if ver >= need:
            return True
    return False


# (what, required, check, packages on Debian / Ubuntu, Fedora, Arch). The X11 and Wayland lists are
# what GLFW (built from source with both, cmake/dependencies.cmake) needs; xkbcommon is Wayland's keyboard.
DEPENDENCIES = [
    ("CMake 3.20 or newer", True, cmake_ok, "cmake", "cmake", "cmake"),
    ("a C++20 compiler (GCC 11+ or Clang 13+)", True, compiler_ok, "build-essential", "gcc-c++", "base-devel"),
    ("pkg-config", True, lambda: shutil.which("pkg-config") is not None, "pkg-config", "pkgconf-pkg-config", "pkgconf"),
    ("OpenGL development files", True, lambda: pkg_config("gl") or has_header("GL/gl.h"),
     "libgl-dev", "mesa-libGL-devel", "mesa"),
    ("X11 development files", True,
     lambda: pkg_config("x11", "xrandr", "xinerama", "xcursor", "xi", "xext"),
     "libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev",
     "libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel libXext-devel",
     "libx11 libxrandr libxinerama libxcursor libxi libxext"),
    ("Wayland development files", True,
     lambda: pkg_config("wayland-client", "wayland-cursor", "wayland-egl", "xkbcommon") and shutil.which("wayland-scanner"),
     "libwayland-dev libxkbcommon-dev", "wayland-devel libxkbcommon-devel", "wayland libxkbcommon"),
    ("FFmpeg's libavcodec (optional: plays the game's movies)", False,
     lambda: pkg_config("libavcodec", "libavutil") or pkg_config("libavcodec", "libavutil", extra_path=HOMEBREW_PKGCONFIG), "libavcodec-dev libavutil-dev", "ffmpeg-free-devel", "ffmpeg"),
]
INSTALL = {"debian": "sudo apt install", "fedora": "sudo dnf install", "arch": "sudo pacman -S --needed"}
DISTRO_NAME = {"debian": "Debian / Ubuntu", "fedora": "Fedora", "arch": "Arch"}


# Windows: the MSYS2 CLANG64 environment's packages (docs/setup.md); clang is the recommended compiler there
# (the same float code as on Linux, docs/platforms.md). The OpenGL, window and audio libraries are Windows' own.
WINDOWS_DEPENDENCIES = [
    ("CMake 3.20 or newer", True, cmake_ok, "mingw-w64-clang-x86_64-cmake"),
    ("a C++20 compiler (clang 13+, or GCC 11+ for MinGW-w64)", True, compiler_ok, "mingw-w64-clang-x86_64-clang"),
    ("Ninja (the build tool)", True, lambda: shutil.which("ninja") is not None, "mingw-w64-clang-x86_64-ninja"),
    ("pkg-config (finds FFmpeg)", False, lambda: shutil.which("pkg-config") is not None, "mingw-w64-clang-x86_64-pkgconf"),
    ("FFmpeg's libavcodec (optional: plays the game's movies)", False, lambda: pkg_config("libavcodec", "libavutil"),
     "mingw-w64-clang-x86_64-ffmpeg"),
]


def check_system_windows():
    missing_req, missing_opt = [], []
    for dep in WINDOWS_DEPENDENCIES:
        if not dep[2]():
            (missing_req if dep[1] else missing_opt).append(dep)
    for dep in missing_opt:
        warn(f"Not found: {dep[0]}. The game runs without it.")
    if not missing_req:
        ok("Everything needed to build the game is installed.")
    else:
        for dep in missing_req:
            print(color("1;31", f"  Missing: {dep[0]}"))
    wanted = missing_req + missing_opt
    if wanted:
        cmd = "pacman -S --needed " + " ".join(dep[3] for dep in wanted)
        print("\n  With MSYS2 (https://www.msys2.org), in its CLANG64 shell:\n    " + color("1", cmd) +
              "\n  then run the setup from that shell (or with MSYS2's clang64\\bin folder in your PATH).")
    if missing_req:
        print(f"\n  Then run {SETUP_CMD} again.")
        sys.exit(1)


def check_system():
    if WINDOWS:
        check_system_windows()
        return
    distro = detect_distro()
    column = {"debian": 3, "fedora": 4, "arch": 5}.get(distro)
    missing_req, missing_opt = [], []
    for dep in DEPENDENCIES:
        if not dep[2]():
            (missing_req if dep[1] else missing_opt).append(dep)
    for dep in missing_opt:
        warn(f"Not found: {dep[0]}. The game runs without it.")
    if not missing_req:
        ok("Everything needed to build the game is installed.")
    else:
        for dep in missing_req:
            print(color("1;31", f"  Missing: {dep[0]}"))
    wanted = missing_req + missing_opt
    if not wanted:
        return
    if column is None:
        print(f"\n  Install them with your distribution's package manager, then run {SETUP_CMD} again.")
    else:
        cmd = f"{INSTALL[distro]} " + " ".join(dep[column] for dep in wanted)
        print(f"\n  To install {'them' if missing_req else 'it'} on {DISTRO_NAME[distro]}:\n    {color('1', cmd)}")
        if missing_req and ask("Run this command now (it asks for your password)?", False):
            if subprocess.run(cmd, shell=True).returncode == 0:
                missing_req = [d for d in missing_req if not d[2]()]
                if not missing_req:
                    ok("Installed.")
                    return
            fail(f"Something is still missing. Install it and run {SETUP_CMD} again.")
    if missing_req:
        print(f"\n  Then run {SETUP_CMD} again.")
        sys.exit(1)


# --- 2. The disc image --------------------------------------------------------------------------

def looks_like_ps2_disc(path):
    """A quick look before the build (the full check is `rac1-native --setup --check-iso`):
    an ISO 9660 image with a PS2 boot executable ("XXXX_NNN.NN") in its root. Returns its name, or None."""
    try:
        with open(path, "rb") as f:
            f.seek(16 * SECTOR)
            pvd = f.read(SECTOR)
            if len(pvd) < SECTOR or pvd[1:6] != b"CD001":
                return None
            lba, size = struct.unpack_from("<I", pvd, 158)[0], struct.unpack_from("<I", pvd, 166)[0]
            f.seek(lba * SECTOR)
            data = f.read(min(size, 64 * SECTOR))
    except OSError:
        return None
    off = 0
    while off < len(data):
        ln = data[off]
        if ln == 0:
            off = (off // SECTOR + 1) * SECTOR
            continue
        name = data[off + 33:off + 33 + data[off + 32]].split(b";")[0].decode(errors="replace")
        if len(name) == 11 and name[4] == "_" and name[8] == ".":
            return name
        off += ln
    return None


# Ratchet & Clank's releases, by their boot executable: (serial, region, supported, a good dump's size). The same table as
# KNOWN_DISCS in src/setup/iso_extract.cpp (the full check, after the build); this copy only lets the
# setup stop before the build when the disc is the wrong one. Add a release to both.
KNOWN_DISCS = {
    "SCUS_971.99": ("SCUS-97199", "NTSC-U", True, 4214095872),
    "SCES_509.16": ("SCES-50916", "PAL", False, 0),
    "SCPS_150.55": ("SCPS-15055", "NTSC-J", False, 0),
}


def find_iso(arg):
    if arg:
        return Path(arg).expanduser().resolve()
    if os.environ.get("RAC1_ISO"):
        return Path(os.environ["RAC1_ISO"]).expanduser().resolve()
    if GAME_ISO.exists():
        return GAME_ISO
    if GAME_ISO_PATH_FILE.is_file():  # (where the setup could not link the image: its path)
        lines = GAME_ISO_PATH_FILE.read_text(encoding="utf-8").splitlines()
        if lines and Path(lines[0].strip()).is_file():
            return Path(lines[0].strip())
    if not sys.stdin.isatty():
        fail(f"No disc image given. Run {SETUP_CMD} --iso <path-to-your-iso>")
    print("  Where is your Ratchet & Clank disc image (NTSC-U, SCUS-97199)?")
    while True:
        try:
            val = input("  Path to the .iso file: ").strip().strip("'\"")
        except (EOFError, KeyboardInterrupt):
            print()
            sys.exit(1)
        if val:
            p = Path(val).expanduser().resolve()
            if p.is_file():
                return p
            print(f"  File not found: {p}")


def disc_fingerprint(iso):
    st = iso.stat()
    return f"{looks_like_ps2_disc(iso)}:{st.st_size}"


def iso_root_file(path, name):
    """(start, size) in bytes of a file in the disc image's root directory, or None."""
    with open(path, "rb") as f:
        f.seek(16 * SECTOR)
        pvd = f.read(SECTOR)
        lba, size = struct.unpack_from("<I", pvd, 158)[0], struct.unpack_from("<I", pvd, 166)[0]
        f.seek(lba * SECTOR)
        data = f.read(size)
    off = 0
    while off < len(data):
        ln = data[off]
        if ln == 0:  # (records do not cross sector boundaries)
            off = (off // SECTOR + 1) * SECTOR
            continue
        rec = data[off:off + ln]
        if rec[33:33 + rec[32]].split(b";")[0].decode(errors="replace") == name:
            return struct.unpack_from("<I", rec, 2)[0] * SECTOR, struct.unpack_from("<I", rec, 10)[0]
        off += ln
    return None


# --- 3. The generated code ----------------------------------------------------------------------
# Part of the game's code is generated from the game's own MIPS and VU code on the disc, by tools/codegen/
# (docs/architecture.md, "The generated code"). It needs the boot executable and the overlay address maps, and
# comes before the build: the build compiles it from assets/generated/src/.

def sources_files(patterns):
    return sorted({p for pat in patterns for p in ROOT.glob(pat) if p.is_file() and "__pycache__" not in p.parts})


def code_sources_fingerprint():
    return sources_fingerprint(CODE_SOURCES)


def extract_elf(iso):
    """The boot executable (SCUS_971.99), copied from the disc's root directory: the generators read the
    VU programs and the boot module from it, and the game reads it while it runs."""
    found = iso_root_file(iso, ELF.name)
    if found is None:
        fail(f"{ELF.name} is not on the disc image: it is damaged, or another disc.")
    start, size = found
    if start + size > iso.stat().st_size:
        fail("The disc image is damaged or incomplete (it ends before the boot executable does). Dump the disc again.")
    with open(iso, "rb") as f:
        f.seek(start)
        data = f.read(size)
    ELF.parent.mkdir(parents=True, exist_ok=True)
    tmp = ELF.with_name(ELF.name + ".part")
    tmp.write_bytes(data)
    tmp.replace(ELF)


def run_quiet(cmd, what):
    r = subprocess.run([str(c) for c in cmd], cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    if r.returncode:
        print(r.stderr[-3000:])
        fail(f"{what} failed (see above). Run {SETUP_CMD} again to retry.")


def overlay_maps(iso):
    """Each level's code matched against Veldin's (level 0's), and the boot module's (tools/overlay_match/)."""
    OVERLAY_MAPS.mkdir(parents=True, exist_ok=True)
    tools = ROOT / "tools/overlay_match"
    print("   title", end="", flush=True)
    run_quiet([sys.executable, tools / "front_match.py", "--elf", ELF, "--iso", iso, "--out", OVERLAY_MAPS],
              "Matching the boot module's code")
    for lv in range(1, 19):
        print(f" {lv}", end="", flush=True)
        run_quiet([sys.executable, tools / "overlay_match.py", "--iso", iso, "--out", OVERLAY_MAPS, "--levels", str(lv)],
                  f"Matching level {lv}'s code")
    print()


CODE_LOG = BUILD_DIR / "setup-codegen.log"


def generated_files():
    out = subprocess.run([sys.executable, GENERATE_ALL, "--files"], cwd=ROOT, capture_output=True, text=True, check=True).stdout
    return [l.strip() for l in out.splitlines() if l.strip()]


def generate_code(iso, jobs):
    """tools/codegen/generate_all.py into assets/generated/src/ (without the comments that carry each original
    instruction: the same C++), then the list of its files for CMake."""
    GEN_LIST.unlink(missing_ok=True)  # (until it is complete, the build asks for ./setup.sh)
    files = generated_files()
    units = subprocess.run([sys.executable, GENERATE_ALL, "--list"], cwd=ROOT, capture_output=True, text=True,
                           check=True).stdout.count("\n")
    BUILD_DIR.mkdir(exist_ok=True)
    env = dict(os.environ, RAC1_ASSETS=str(ASSETS_DIR), RAC1_ISO=str(iso))
    cmd = [sys.executable, str(GENERATE_ALL), "--out", str(CODE_ROOT), "--no-insn-comments", "--jobs", str(jobs)]
    done = 0
    with open(CODE_LOG, "w") as log:
        log.write("$ " + " ".join(cmd) + "\n")
        proc = subprocess.Popen(cmd, cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                                errors="replace")
        for line in proc.stdout:
            log.write(line)
            if re.match(r"\S+:\S+\s+(ok|FAIL)\s", line):
                done += 1
                if USE_COLOR:
                    print(f"\r   {done}/{units}", end="", flush=True)
        rc = proc.wait()
    if USE_COLOR and done:
        print()
    if rc:
        print("".join(open(CODE_LOG, errors="replace").readlines()[-30:]))
        fail(f"Generating the code failed (see above; the full output is in {CODE_LOG.relative_to(ROOT)}).")
    missing = [f for f in files if not (CODE_ROOT / f).is_file()]
    if missing:
        fail(f"The code generation left out {len(missing)} file(s), e.g. {missing[0]} (see {CODE_LOG.relative_to(ROOT)}).")
    # files of an earlier generation that the generators no longer make
    keep = {(CODE_ROOT / f).resolve() for f in files}
    for p in GEN_SRC.rglob("*"):
        if p.is_file() and p.suffix in (".cpp", ".h") and p.resolve() not in keep:
            p.unlink()
    rel = [f[len("src/"):] for f in files]
    GEN_LIST.write_text("# Written by ./setup.sh (tools/setup.py) with the generated code: do not edit.\n"
                        f"set(RAC1_CODEGEN_FINGERPRINT \"{code_sources_fingerprint()}\")\n"
                        "set(RAC1_GENERATED_SOURCES\n" + "".join(f"    {f}\n" for f in rel) + ")\n")
    ok(f"Generated {len(files)} source files into {GEN_SRC.relative_to(ROOT)}/")


def code_current():
    """True when the generated code is there and was made from the current sources (what CMake checks)."""
    try:
        m = re.search(r'RAC1_CODEGEN_FINGERPRINT "(\w+)"', GEN_LIST.read_text())
    except OSError:
        return False
    return bool(m) and m.group(1) == code_sources_fingerprint()


def prepare_code(iso, forced, jobs):
    """The steps before the build, each redone only when the disc or its sources changed (or forced)."""
    state = load_state()
    disc = disc_fingerprint(iso)
    for name, what, patterns in CODE_STEPS:
        fingerprint = f"{disc}:{sources_fingerprint(patterns)}"
        if name == "code":  # (made from the two steps before it, too)
            fingerprint += ":" + hashlib.sha256((state.get("elf", "") + state.get("overlay", "")).encode()).hexdigest()
        present = {"elf": ELF.is_file(), "overlay": (OVERLAY_MAPS / "front_reloc.txt").is_file()
                   and all((OVERLAY_MAPS / f"level{lv}.txt").is_file() for lv in range(1, 19)),
                   "code": code_current()}[name]
        if name not in forced and state.get(name) == fingerprint and present:
            ok(f"{what[0].upper() + what[1:]}: up to date")
            continue
        print(f"  {what[0].upper() + what[1:]}...", flush=True)
        state.pop(name, None)
        if name == "elf":
            extract_elf(iso)
            ok(f"Copied {ELF.relative_to(ROOT)}")
        elif name == "overlay":
            overlay_maps(iso)
            ok(f"Wrote {OVERLAY_MAPS.relative_to(ROOT)}/")
        else:
            generate_code(iso, jobs)
        state[name] = fingerprint
        save_state(state)


# --- 4. The build -------------------------------------------------------------------------------

def available_memory_kb_windows():
    import ctypes

    class MemoryStatus(ctypes.Structure):
        _fields_ = [("dwLength", ctypes.c_uint32), ("dwMemoryLoad", ctypes.c_uint32), ("ullTotalPhys", ctypes.c_uint64),
                    ("ullAvailPhys", ctypes.c_uint64), ("ullTotalPageFile", ctypes.c_uint64),
                    ("ullAvailPageFile", ctypes.c_uint64), ("ullTotalVirtual", ctypes.c_uint64),
                    ("ullAvailVirtual", ctypes.c_uint64), ("ullAvailExtendedVirtual", ctypes.c_uint64)]
    st = MemoryStatus()
    st.dwLength = ctypes.sizeof(MemoryStatus)
    if not ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(st)):
        raise OSError("GlobalMemoryStatusEx")
    return st.ullAvailPhys // 1024


def default_jobs():
    """One compile job per core, but no more than one per 2 GB of available memory."""
    jobs = os.cpu_count() or 2
    if WINDOWS:
        try:
            jobs = min(jobs, max(1, available_memory_kb_windows() // (2 * 1024 * 1024)))
        except (OSError, AttributeError):
            pass
        return max(1, jobs)
    try:
        for line in Path("/proc/meminfo").read_text().splitlines():
            if line.startswith("MemAvailable:"):
                jobs = min(jobs, max(1, int(line.split()[1]) // (2 * 1024 * 1024)))
    except (OSError, ValueError):
        pass
    return max(1, jobs)


BUILD_LOG = BUILD_DIR / "setup-build.log"


def run_logged(cmd, log):
    """Runs a build command with its output in `log`, showing only its progress; on failure, the errors."""
    with open(log, "a") as f:
        f.write("$ " + " ".join(cmd) + "\n")
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace")
        shown = ""
        for line in proc.stdout:
            f.write(line)
            m = re.match(r"\[\s*(\d+)%\]", line)
            if not m and WINDOWS:  # (Ninja's progress: [done/total])
                n = re.match(r"\[(\d+)/(\d+)\]", line)
                if n:
                    m = re.match(r"(\d+)", str(int(n.group(1)) * 100 // max(1, int(n.group(2)))))
            if m and m.group(1) != shown and USE_COLOR:
                shown = m.group(1)
                print(f"\r  {shown:>3}%", end="", flush=True)
        if USE_COLOR and shown:
            print()
        if proc.wait() == 0:
            return True
    errors = [l.rstrip() for l in open(log, errors="replace") if " error" in l or "Error" in l or "CMake Error" in l]
    print("\n".join("    " + l for l in errors[-25:]))
    print(f"  The full output is in {log.relative_to(ROOT)}.")
    return False


def build(jobs):
    BUILD_DIR.mkdir(exist_ok=True)
    BUILD_LOG.write_text("")
    if not (BUILD_DIR / "CMakeCache.txt").exists() or not any((BUILD_DIR / f).exists() for f in ("Makefile", "build.ninja")):
        print("  Configuring (this downloads a few libraries the first time)...", flush=True)
        cmd = ["cmake", "-S", str(ROOT), "-B", str(BUILD_DIR), "-DCMAKE_BUILD_TYPE=RelWithDebInfo"]
        if WINDOWS:  # (Ninja and clang, not Visual Studio's generator and compiler: docs/platforms.md)
            cmd += ["-G", "Ninja"]
            if shutil.which("clang++"):
                cmd += ["-DCMAKE_C_COMPILER=clang", "-DCMAKE_CXX_COMPILER=clang++"]
            else:
                cmd += ["-DCMAKE_C_COMPILER=gcc", "-DCMAKE_CXX_COMPILER=g++"]
        if not run_logged(cmd, BUILD_LOG):
            fail(f"CMake could not configure the build (see above). If a library is missing, install it and run {SETUP_CMD} again.")
    print(f"  Compiling with {jobs} parallel jobs (the first build takes several minutes)...", flush=True)
    if not run_logged(["cmake", "--build", str(BUILD_DIR), "-j", str(jobs)], BUILD_LOG):
        fail("The build failed (see above).")
    if not EXE.exists():
        fail(f"{EXE} is missing after the build.")
    ok(f"Built {EXE.relative_to(ROOT)}")


# --- 5. The game's data -------------------------------------------------------------------------

def sources_fingerprint(patterns):
    h = hashlib.sha256()
    for p in sources_files(patterns):
        h.update(str(p.relative_to(ROOT)).encode() + b"\0" + p.read_bytes() + b"\0")
    return h.hexdigest()


def load_state():
    try:
        return json.loads(STATE_FILE.read_text())
    except (OSError, ValueError):
        return {}


def save_state(state):
    STATE_FILE.parent.mkdir(parents=True, exist_ok=True)
    STATE_FILE.write_text(json.dumps(state, indent=2) + "\n")


def generate(iso, forced):
    state = load_state()
    disc = disc_fingerprint(iso)
    for name, what, patterns in STEPS:
        fingerprint = f"{disc}:{sources_fingerprint(patterns)}"
        if name not in forced and state.get(name) == fingerprint:
            ok(f"{what[0].upper() + what[1:]}: up to date")
            continue
        # a step that is forced, whose sources changed, or that never finished, is redone from scratch
        cmd = [str(EXE), "--setup", "--iso", str(iso), "--step", name, "--force"]
        if subprocess.run(cmd, cwd=ROOT, env=game_env()).returncode:
            fail(f"Generating {what} failed (see the messages above). Run {SETUP_CMD} again to retry.")
        state[name] = fingerprint
        save_state(state)


def game_env():
    """The environment the game's setup mode runs in: on Windows, the Python it runs the overlay matcher with
    (src/platform/platform_win32.cpp) is this one."""
    return dict(os.environ, RAC1_PYTHON=sys.executable) if WINDOWS else None


# --- main ---------------------------------------------------------------------------------------

def parse_force(value):
    groups = STEP_NAMES if value == "all" else [g.strip() for g in value.split(",") if g.strip()]
    bad = [g for g in groups if g not in STEP_NAMES]
    if bad:
        raise argparse.ArgumentTypeError(f"unknown step {', '.join(bad)} (steps: {', '.join(STEP_NAMES)})")
    return set(groups)


def check_disc(arg):
    iso = find_iso(arg)
    if not iso.is_file():
        fail(f"File not found: {iso}")
    boot = looks_like_ps2_disc(iso)
    if boot is None:
        fail(f"{iso} is not a PlayStation 2 disc image (an ISO 9660 image).\n"
             "         Use a plain .iso dump of your disc, not a compressed one (.chd, .cso, .gz) or a .bin/.cue pair.")
    if boot not in KNOWN_DISCS:
        fail(f"{iso} is another game ({boot}), not Ratchet & Clank.")
    serial, region, supported, good_size = KNOWN_DISCS[boot]
    if not supported:
        fail(f"This is Ratchet & Clank's {region} disc ({serial}). "
             "Only the North American disc, SCUS-97199, is supported for now.")
    if iso.stat().st_size < good_size:  # (cut short, or trimmed: checked fully after the build)
        warn(f"The image is smaller than a good dump ({iso.stat().st_size} bytes, not {good_size}): "
             "it may be incomplete.")
    ok(f"Ratchet & Clank, {region} ({serial}): {iso}")
    return iso


def print_code_inputs():
    """For CMake (src/game/CMakeLists.txt): the fingerprint of the generated code's sources, then the sources."""
    print(code_sources_fingerprint())
    for p in sources_files(CODE_SOURCES):
        print(p)


def main():
    if sys.argv[1:] == ["--code-inputs"]:
        print_code_inputs()
        return
    ap = argparse.ArgumentParser(description="Sets up rac1-native: checks the system and your disc image, generates "
                                             "the game's code from it, builds the game and generates the game's data.")
    ap.add_argument("--iso", help="your disc image (Ratchet & Clank, NTSC-U, SCUS-97199)")
    ap.add_argument("--force", nargs="?", const="all", default=set(), type=parse_force, metavar="STEPS",
                    help=f"regenerate even if up to date: everything, or some steps ({','.join(STEP_NAMES)})")
    ap.add_argument("-j", "--jobs", type=int, default=None, help="parallel jobs (the code generation and the build)")
    ap.add_argument("--no-build", action="store_true", help="skip the build")
    ap.add_argument("--build-only", action="store_true",
                    help="only build the game (and generate its code first if that is missing or out of date)")
    ap.add_argument("--no-play", action="store_true", help="do not offer to start the game at the end")
    args = ap.parse_args()
    if args.iso:
        args.iso = str(Path(args.iso).expanduser().resolve())  # (relative to where the user ran the setup)
    os.chdir(ROOT)
    jobs = args.jobs or default_jobs()

    print(color("1", "\nRatchet & Clank (rac1-native) setup"))
    # --build-only needs the disc only when the generated code must be made again
    need_disc = not args.build_only or not code_current() or bool(args.force & {"elf", "overlay", "code"})
    total = 5 if not args.build_only else (4 if need_disc else 2)
    n = 0

    def next_step(msg):
        nonlocal n
        n += 1
        step(n, total, msg)

    next_step("Checking your system")
    check_system()

    iso = None
    if need_disc:
        next_step("Checking your disc image")
        iso = check_disc(args.iso)
        next_step("Generating the game's code from your disc")
        prepare_code(iso, args.force, jobs)

    next_step("Building the game")
    if args.no_build:
        if not EXE.exists():
            fail(f"--no-build, but {EXE.relative_to(ROOT)} does not exist yet.")
        ok("Skipped (--no-build)")
    else:
        build(jobs)
    if args.build_only:
        return

    next_step("Generating the game's data from your disc")
    r = subprocess.run([str(EXE), "--setup", "--iso", str(iso), "--check-iso"], cwd=ROOT, env=game_env())
    if r.returncode:
        sys.exit(1)
    generate(iso, args.force)

    print(color("1;32", "\nAll done: Ratchet & Clank is ready."))
    play = "build\\rac1-native.exe" if WINDOWS else "./build/rac1-native"
    print(f"To play, run this from {ROOT}:\n    {play}\n")
    if not args.no_play and ask("Start the game now?", True):
        sys.exit(subprocess.run([str(EXE)], cwd=ROOT).returncode)


if __name__ == "__main__":
    main()
