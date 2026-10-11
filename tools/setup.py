#!/usr/bin/env python3
"""rac1-native's guided setup (run it through ./setup.sh).

From a fresh clone and your own disc image (Ratchet & Clank, NTSC-U, SCUS-97199) to the game:
  1. checks the system and says what to install;
  2. checks your disc image and makes the game find it (assets/iso/rac1-ntsc-u.iso);
  3. builds the game (build/rac1-native);
  4. generates the game's data into assets/ (`rac1-native --setup`, one step at a time);
  5. offers to start the game.

Re-runnable: the build only recompiles what changed, and each data step is redone only when its
generator's sources (or the disc) changed since it last ran, or when it is forced (--force).
The record of what was generated with what is assets/setup-state.json. docs/setup.md is the guide.
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
BUILD_DIR = ROOT / "build"
EXE = BUILD_DIR / "rac1-native"
ASSETS_DIR = ROOT / "assets"
GAME_ISO = ASSETS_DIR / "iso/rac1-ntsc-u.iso"  # where the game reads the disc (runtime/kernel/disc.cpp)
STATE_FILE = ASSETS_DIR / "setup-state.json"
SECTOR = 2048

# The data steps of `rac1-native --setup --step <name>`, in order, with the sources that make each
# one's output: a change to any of them (after a git pull) makes the step run again.
STEPS = [
    ("elf", "the boot executable", ["src/setup/iso_extract.*"]),
    ("wads", "the levels' data", ["src/setup/iso_extract.*"]),
    ("moby", "the moby models and textures",
     ["src/setup/mrp_export.*", "src/setup/kick_trace.*", "src/formats/**/*", "src/game/ovl/moby_vu1*",
      "runtime/vu/**/*"]),
    ("world", "the world geometry and textures",
     ["src/setup/mrp_export.*", "src/setup/world_trace.*", "src/formats/**/*", "src/game/ovl/world_vu1*",
      "src/game/render/mrp_world_assets.*", "runtime/vu/**/*"]),
    ("flight", "the flight between planets (the loading screen)",
     ["src/setup/scene_export.*", "src/setup/kick_trace.*", "src/formats/**/*", "src/game/ovl/moby_vu1*",
      "src/game/render/mrp_world_assets.*", "runtime/vu/**/*"]),
    ("overlay", "the overlay address maps", ["tools/overlay_match/*.py"]),
]
STEP_NAMES = [s[0] for s in STEPS]

USE_COLOR = sys.stdout.isatty()


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


def check_system():
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
        print("\n  Install them with your distribution's package manager, then run ./setup.sh again.")
    else:
        cmd = f"{INSTALL[distro]} " + " ".join(dep[column] for dep in wanted)
        print(f"\n  To install {'them' if missing_req else 'it'} on {DISTRO_NAME[distro]}:\n    {color('1', cmd)}")
        if missing_req and ask("Run this command now (it asks for your password)?", False):
            if subprocess.run(cmd, shell=True).returncode == 0:
                missing_req = [d for d in missing_req if not d[2]()]
                if not missing_req:
                    ok("Installed.")
                    return
            fail("Something is still missing. Install it and run ./setup.sh again.")
    if missing_req:
        print("\n  Then run ./setup.sh again.")
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
    if not sys.stdin.isatty():
        fail("No disc image given. Run ./setup.sh --iso <path-to-your-iso>")
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


# --- 3. The build -------------------------------------------------------------------------------

def default_jobs():
    """One compile job per core, but no more than one per 2 GB of available memory."""
    jobs = os.cpu_count() or 2
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
    if not (BUILD_DIR / "CMakeCache.txt").exists():
        print("  Configuring (this downloads a few libraries the first time)...", flush=True)
        if not run_logged(["cmake", "-S", str(ROOT), "-B", str(BUILD_DIR), "-DCMAKE_BUILD_TYPE=RelWithDebInfo"], BUILD_LOG):
            fail("CMake could not configure the build (see above). If a library is missing, install it and run ./setup.sh again.")
    print(f"  Compiling with {jobs} parallel jobs (the first build takes several minutes)...", flush=True)
    if not run_logged(["cmake", "--build", str(BUILD_DIR), "-j", str(jobs)], BUILD_LOG):
        fail("The build failed (see above).")
    if not EXE.exists():
        fail(f"{EXE} is missing after the build.")
    ok(f"Built {EXE.relative_to(ROOT)}")


# --- 4. The game's data -------------------------------------------------------------------------

def sources_fingerprint(patterns):
    h = hashlib.sha256()
    files = sorted({p for pat in patterns for p in ROOT.glob(pat) if p.is_file() and "__pycache__" not in p.parts})
    for p in files:
        h.update(str(p.relative_to(ROOT)).encode() + b"\0" + p.read_bytes() + b"\0")
    return h.hexdigest()


def load_state():
    try:
        return json.loads(STATE_FILE.read_text())
    except (OSError, ValueError):
        return {}


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
        if subprocess.run(cmd, cwd=ROOT).returncode:
            fail(f"Generating {what} failed (see the messages above). Run ./setup.sh again to retry.")
        state[name] = fingerprint
        STATE_FILE.parent.mkdir(parents=True, exist_ok=True)
        STATE_FILE.write_text(json.dumps(state, indent=2) + "\n")


# --- main ---------------------------------------------------------------------------------------

def parse_force(value):
    groups = STEP_NAMES if value == "all" else [g.strip() for g in value.split(",") if g.strip()]
    bad = [g for g in groups if g not in STEP_NAMES]
    if bad:
        raise argparse.ArgumentTypeError(f"unknown step {', '.join(bad)} (steps: {', '.join(STEP_NAMES)})")
    return set(groups)


def main():
    ap = argparse.ArgumentParser(description="Sets up rac1-native: checks the system, builds the game, "
                                             "checks your disc image and generates the game's data.")
    ap.add_argument("--iso", help="your disc image (Ratchet & Clank, NTSC-U, SCUS-97199)")
    ap.add_argument("--force", nargs="?", const="all", default=set(), type=parse_force, metavar="STEPS",
                    help=f"regenerate the data even if it is up to date: all of it, or some steps "
                         f"({','.join(STEP_NAMES)})")
    ap.add_argument("-j", "--jobs", type=int, default=None, help="parallel build jobs")
    ap.add_argument("--no-build", action="store_true", help="skip the build")
    ap.add_argument("--build-only", action="store_true", help="only build the game")
    ap.add_argument("--no-play", action="store_true", help="do not offer to start the game at the end")
    args = ap.parse_args()
    if args.iso:
        args.iso = str(Path(args.iso).expanduser().resolve())  # (relative to where the user ran ./setup.sh)
    os.chdir(ROOT)

    print(color("1", "\nRatchet & Clank (rac1-native) setup"))
    total = 2 if args.build_only else 4

    step(1, total, "Checking your system")
    check_system()

    iso = None
    if not args.build_only:
        step(2, total, "Checking your disc image")
        iso = find_iso(args.iso)
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

    step(2 if args.build_only else 3, total, "Building the game")
    if args.no_build:
        if not EXE.exists():
            fail(f"--no-build, but {EXE.relative_to(ROOT)} does not exist yet.")
        ok("Skipped (--no-build)")
    else:
        build(args.jobs or default_jobs())
    if args.build_only:
        return

    step(4, total, "Generating the game's data from your disc")
    r = subprocess.run([str(EXE), "--setup", "--iso", str(iso), "--check-iso"], cwd=ROOT)
    if r.returncode:
        sys.exit(1)
    generate(iso, args.force)

    print(color("1;32", "\nAll done: Ratchet & Clank is ready."))
    print(f"To play, run this from {ROOT}:\n    ./build/rac1-native\n")
    if not args.no_play and ask("Start the game now?", True):
        sys.exit(subprocess.run([str(EXE)], cwd=ROOT).returncode)


if __name__ == "__main__":
    main()
