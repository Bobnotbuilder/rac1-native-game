# Native iOS port: first implementation checkpoint

Baseline: upstream 0.4.0, `3fd14c12e4dff2fd4b6661a6e54187faea3f76c8`.
Development: `Bobnotbuilder/rac1-native-game`, branch `ios/arm64-metal-port`.
This is NOT ReRAC. No changes to ReRAC or GameStudio engines are required.

## Implemented in this checkpoint

- The existing platform coroutine interface on Apple ARM64 without `ucontext`, Linux mmap
  flags, extra threads, or executable memory. Guarded, demand-paged stack, Apple reserved x18
  untouched, callee-saved GPR/FP registers and FPCR/FPSR retained. Plain arm64 only, not arm64e.
- ARM64 FPCR flush-to-zero in the existing PS2 float setup, retaining toward-zero rounding.
  Strict compiler flags and runtime tests are mandatory; an old compiler warning about unsupported
  rounding must not be ignored as proof of fidelity.
- A narrow, independently written native NEON adapter for the SSE operations used by
  `vu0_regs.h`, enabled only through the Apple ARM64 include path. The original game helper
  header is unchanged. Differential fixtures exercise all 16 write masks, operand clamping,
  MAC/status flags, arithmetic, signed zero and shuffles against upstream scalar helpers;
  the same test can run against the untouched SSE implementation on x86.
- The existing platform file interface on Apple, app-owned Documents saves and Application
  Support settings, offset reads, maps, thread CPU diagnostics, explicit failure for subprocesses.
- Apple GameController extended-gamepad snapshots into the existing `HostState`, including sticks,
  triggers, face buttons, D-pad, Menu/Options and thumbstick presses. The existing remapping and
  `to_packet` path remains authoritative. Physical Backbone behaviour is NOT yet tested.
- A first Metal shader building block translated from the existing MRP: texture functions,
  alpha comparisons and two-pass alpha-fail decisions. Synthetic GPU tests compare with a CPU
  expression of the upstream rules. This is NOT yet a complete renderer or an image comparison.
- Native Metal GS blend-state mapping, derived from the untouched OpenGL `blend_state` reference.
  RGB add/subtract/reverse-subtract and factors are mapped to real pipeline state; alpha is
  source-write-only as upstream. FIX changes use encoder state, not new pipeline compilation.
  Known upstream approximations (`1+C`, FIX above 128) are explicitly flagged. A strict policy
  rejects those states without changing the attachment; reference-compatible policy is explicit.
  A disc-free CPU contract covers all register selectors and FIX boundaries. A GPU raster test
  renders synthetic per-state pixels and checks readback against the upstream coefficient policy.
  These tests are renderer-component evidence, not original-game screenshots or GS hardware proof.
- Separate public, disc-free Apple runtime/Metal CI. It builds real iPhone runtime objects, not
  an import-screen app, and must never label these as a game IPA or an oracle pass.

## Reproduce public tests

On an Apple ARM64 Mac with modern Xcode and CMake (or Actions `iOS native port contracts`):

```sh
cmake -S ios/portability -B build-apple -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build-apple --parallel 2
ctest --test-dir build-apple --output-on-failure
cmake -S ios/portability -B build-device -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
cmake --build build-device --parallel 2
```

The coroutine test uses the REAL `runtime/ee/ee_thread.cpp`, 10,000 yields with live stack state,
per-context float rounding, exception propagation and reuse. No gameplay simulation or interpreter
is substituted. Public CI preserves the original Linux/Windows workflow unchanged.

## First unresolved integration work

There is no playable iOS target yet. The desktop root build still requires OpenGL 4.6 and GLFW.
The complete Metal MRP, GS 2D/target ordering, UIKit game host, miniaudio/CoreAudio lifecycle and
game-data import are not yet integrated. Do not produce a misleading placeholder IPA.

One important renderer constraint: `mrp_tfrag_gpu.cpp` uses GLSL `double` storage and arithmetic
for world reconstruction. Do not narrow this to float silently. Initially use/verify the existing
CPU reference for that stage, then test a Metal representation against captured upstream output.
The translated VU/game code and original renderer remain unchanged as the reference.

## Private game-code build gate

The public source alone cannot link the game. The pinned generators list 227 C++ files (103 units), generated from the
user's disc. `RAC1_NO_GENERATED=ON` compiles libraries only and is NEVER a playable build.
The inspected local `Game/Ratchet.iso` link points at a missing PAL file. A broader read-only
search found the user's NTSC-U disc in Trash; it has not been moved, restored or uploaded.
The new `tools/ios/prepare_private_sources.py` reuses upstream extraction/matching/codegen
without needing desktop OpenGL, CMake or a desktop gameplay build. It validates NTSC-U identity,
exact reference size and source revision, hashes the disc, uses one job by default, verifies
the upstream source fingerprint and writes its receipt under ignored `assets/` only.

```sh
python3 tools/ios/prepare_private_sources.py --iso '/path/to/user-owned/SCUS-97199.iso' \
  --source-revision '<exact checkout commit>' --jobs 1
```

This prepares PRIVATE code, not MRP runtime assets or an IPA. Oracle and full game linking remain unrun.

Use a readable user-owned SCUS-97199 input and upstream setup/codegen, privately. Do not put ISO,
generated copyrighted source, dumps or extracted assets in this public fork/Actions artifacts.
Before a hosted full game build, establish a specifically authorised private code-generation/build
pipeline. Merely adding a public Actions secret does not authorise distributing generated code.

Future bundle identity must be distinct from ReRAC (proposed `io.github.bobnotbuilder.rac1native`).
No app with that identity is packaged yet; existing ReRAC saves/data are untouched.

## Strict blend rejection investigation

The 1,280 enabled synthetic register combinations in the fixed-function GPU contract include
reserved selectors deliberately, to preserve the upstream GL reference behavior. Strict rejection
remains enforced: 188 states are not directly factorable without the reference's approximations.
The new reason bits and CPU audit distinguish 108 `1+C`-only cases, 68 FIX-clamp-only cases and
12 with both; 88 use defined selectors and 100 also use reserved selectors. These are synthetic
combinations, not evidence that the game exercises all of them.

`gs_programmable_blend.metal` evaluates `(A-B)*C+D` without those two approximations.
Its separate framebuffer-fetch contract uses three overlapping layers, destination readback,
defined selectors only and raw FIX/128 including values over one. The old fixed-function tests
and strict rejection policy are unchanged. This building block is not selected automatically by
the game: target formats, raw-GS vs blend-domain alpha, dithering, COLCLAMP and alpha/depth tests
must be integrated and compared against upstream images before claiming correct game pixels.
Apple-family/raster-order support is checked before running the new GPU contract. A missing
capability is SKIP (77), never PASS. No per-primitive render pass or texture copy is required.

API reference: Apple's Metal Shading Language specification, programmable blending and raster
order groups, https://developer.apple.com/metal/Metal-Shading-Language-Specification.pdf .
No external shader implementation was copied.

## Build warning audit

The hosted modern strict C++ build produced no native compiler warnings. The ignored
`CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED` option was removed from the static-library Makefiles
configure command: it did not disable signing for an application, and no app was built there.
GitHub action Node/punycode deprecation notices are infrastructure warnings, not shader or
gameplay correctness evidence. Existing desktop third-party GLFW/glad diagnostics remain separate.
Local Xcode 13 warns that ARM64 `-frounding-math` is unsupported: local generated-object success
is compile-only evidence, not acceptable production PS2 floating-point verification. Do not
suppress that warning and claim a full device build validated.

## Acceptance still required

1. Modern ARM64 runtime tests, then original oracle on private NTSC-U data.
2. Wire the backend into the existing MRP; render original world/mobys/GS 2D with paired snapshots.
3. Integrate UIKit, audio interruption/background handling, safe-area/fullscreen and game data import.
4. Private full generated-game device link; package executable plus dSYM, exact revision and checksum.
5. User physical iPhone tests: launch, original graphics, Backbone, sound, menus, first planet,
   transitions/saves and campaign progress. No device/gameplay or thermal improvement claim yet.

Classifications: PLATFORM_SPECIFIC / SHARED_ARM64 / SHARED_IOS / SHARED_METAL / SHARED_TESTING
as review candidates only; nothing is automatically adopted into GameStudio Shared Core.
