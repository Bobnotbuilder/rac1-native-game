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
After a successful generation, add `--verify-existing` to attest the completed files at the current
integration revision without rerunning extraction or codegen. It checks every upstream-listed
file and the original upstream fingerprint, records private per-file hashes, and retains the old
receipt. Missing/stale files fail; this option never rewrites the generated CMake manifest to pass.

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

## Existing audio backend: iOS compilation requirement

The pinned miniaudio 0.11.25 source requires Objective-C for its iOS CoreAudio implementation
(AVAudioSession route/interruption handling is already present). The device portability target now
compiles the unchanged upstream SPU2/IOP queue and mixer adapter as Objective-C++, fetching only
the same hash-pinned audio dependency, not GLFW/OpenGL. It explicitly requests Playback on iOS:
the dependency's default tries PlayAndRecord, while this game never needs microphone capture.
The desktop code path is unchanged. This is a real audio-adapter compilation gate, not a full
audio link or proof that sound, interruptions or foreground/background behavior work in a game.
The future UIKit host still must coordinate suspension with simulation and the IOP mirror.
Reference: https://github.com/mackron/miniaudio/blob/0.11.25/miniaudio.h .

The controller contract now follows Apple framework snapshots through the original upstream
`host_input.cpp` bindings and six-byte active-low PS2 packet encoder. It covers face/shoulder/menu
buttons, both sticks, trigger polarity and neutral state after disconnect. GLFW is used only for
its existing pinned numeric header constants (`GLFW_INCLUDE_NONE`); it is not linked and does not
poll devices or create a window. Real GameController sampling uses the same snapshot decoder.
Framework-backed synthetic inputs do not substitute for the user's physical Backbone test.

## Acceptance still required

1. Modern ARM64 runtime tests, then original oracle on private NTSC-U data.
2. Wire the backend into the existing MRP; render original world/mobys/GS 2D with paired snapshots.
3. Integrate UIKit, audio interruption/background handling, safe-area/fullscreen and game data import.
4. Private full generated-game device link; package executable plus dSYM, exact revision and checksum.
5. User physical iPhone tests: launch, original graphics, Backbone, sound, menus, first planet,
   transitions/saves and campaign progress. No device/gameplay or thermal improvement claim yet.

Classifications: PLATFORM_SPECIFIC / SHARED_ARM64 / SHARED_IOS / SHARED_METAL / SHARED_TESTING
as review candidates only; nothing is automatically adopted into GameStudio Shared Core.

## 2026-10-20 private preparation and compiler checkpoint

### UIKit / original GS stream integration continuation

`ios/app` now contains an actual UIKit scene/MetalKit host and original boot/frame/level-exit
adapter, not a stand-alone import-screen game substitute. These sources compile for iPhone
ARM64 but the complete executable is NOT linked yet. The source-only CI compiles the host;
it cannot link the disc-generated gameplay and must not package a misleading IPA.

The app-owned package root is Documents/RAC1NativeData, containing the existing assets/
layout (owned ISO, matching boot dump and converted MRP data). Paths escaping that root fail.
Before game initialization, the host checks the input and each converted asset against the
private build's input-identity resource and attested manifest. It never compiles a game from
an imported ISO on the phone. Files sharing is enabled; no game data is embedded in public CI.
The eventual private build must supply private-input-identity.json with disc_sha256 and
runtime_manifest_sha256 from verified preparation. A missing identity fails closed.

The new Metal scene consumer accepts the original gs::Primitive stream, preserving sprite
geometry/second-vertex colours, captured draw-time textures, scissor, source-derived alpha
tests and ordered programmable blending. It caches pipelines/samplers and immutable texture
snapshots, with no synchronous GPU wait in production. Unsupported render targets, mip
selection, split alpha/depth passes and extended fragment states are explicit errors before
frame publication, NOT dropped primitives or silently successful frames. This is NOT a
complete MRP renderer. Original-game images, widescreen reconstruction and gameplay remain
unvalidated. The initial faithful host keeps a geometry-correct 4:3 view rather than stretch
it and falsely claim native widescreen. The working ReRAC port is unchanged.

A local-only dependency audit compiled 297 actual handwritten native game/runtime/format/
sound/CPU-scene units. Combined with all 227 existing generated units it resolved 723 of
the original 784 undefined dependency names. This remains an MH_OBJECT relocatable link,
NOT a game executable or a measure of engine/game behaviour coverage. The subsequent real
UIKit executable-link attempt fails on remaining renderer capture/readiness and audio-output
dependencies; no dynamic_lookup, success stubs or undefined-symbol bypass was used.

The VU0 driver's optional profiling clock contained an x86-only rdtsc intrinsic. It now
retains the original x86 counter and uses monotonic nanoseconds on ARM64, correctly labelled;
simulation timing/arithmetic are unchanged. A header dependency and clock contract guard it.
Public dependencies are pinned to the original versions and verified by exact Git blob hashes
in an ignored build cache, not installed or vendored into the game source.

The earlier temporary materialization has been preserved in a durable, hash-checked worktree.
The owned Downloads disc was verified from ISO9660, SYSTEM.CNF's `SCUS_971.99` BOOT2,
the MIPS boot ELF and all nineteen level ranges. Its complete SHA-256 matches the prior
generation receipt. No game input was uploaded, and no matching generated file was regenerated.
Exact paths and private hashes belong in the local progress/receipts, not public CI artifacts.

The 227 generated translation units are complete and retain their original recorded hashes.
`tools/ios/preparation` builds the original upstream setup controller, exporters and generated VU
programs without the desktop OpenGL host. It completed the missing moby, world and flight stages.
Original typed readers parsed all 3,493 outputs: 3,384 classes, 88 texture files and 21 worlds
(levels 0–18, title and flight). This proves structural preparation, not game pixels or gameplay.
`finalize_private_assets.py` records source/binary/content hashes and updates only genuinely
completed setup-stage fingerprints; it does not forge the guided game's build-complete state.

Reproduce or resume LOCAL preparation, never in public CI:

```sh
cmake -S tools/ios/preparation -B build-private-preparation -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-private-preparation --target rac1-private-asset-prep --parallel 1
build-private-preparation/rac1-private-asset-prep --setup --iso '<verified owned ISO>' --check-iso
# Run only a missing/stale data stage, never indiscriminately force already verified outputs:
build-private-preparation/rac1-private-asset-prep --setup --iso '<verified owned ISO>' --step world
build-private-preparation/rac1-private-asset-prep --verify-assets
python3 tools/ios/audit_private_setup.py --iso '<verified owned ISO>'
```

The SDK's older libc++ lacks `std::bit_cast`. A narrow constexpr compiler-builtin fallback now
preserves representations exactly; vecmath changes only those representation conversions.
One million bit-pattern round trips and edge representations passed locally. A selected oracle
loads the real owned-disc Veldin overlay into the upstream reference interpreter and compares
four original functions with native vecmath: 1,024 bitwise comparisons passed on Intel. This is
not full-game, ARM64, physical PS2 or device proof. The interpreter is a test reference only.

The Apple portability configure now rejects a compiler without strict ARM64 FP support. Clang 13
has a separately opt-in experimental constrained-FP path that emits `round.dynamic` and real
ARM64 objects. Its generated-code compile audit is **investigation only**, not a release default
or runtime-fidelity pass. Resume cached objects with:

```sh
python3 tools/ios/compile_generated_audit.py --experimental-clang13-investigation --max-seconds 900
```

Its local receipt explicitly records `full_game_linked=false` and
`production_fidelity_verified=false`. Do not package these objects as a working IPA.

The local O1 audit completed 214/227 objects before one 1 MB fast-VU source hit its
600-second budget. The exact same source compiled at O0, separating source compatibility from
that old compiler's optimization cost (not identifying a specific LLVM pass defect).
`probe_remaining_generated.py --experimental-clang13-investigation` preserves those 214 objects
and separately probes only remaining units. All 227 now have ARM64 objects: 214 O1 and 13 O0.
They also passed a combined `ld -r` link into an ARM64 **MH_OBJECT**, retaining unresolved native,
game and standard-library dependencies. This is NOT an application link, release-fidelity test,
gameplay proof or IPA. All partial/probe artifacts remain private and excluded from Git.

### Metal texture upload component

The new native MRP texture-array uploader preserves the source's CSM1 palette indexing,
linear RGBA8 representation and raw GS alpha. It validates dimensions, every requested mip
and every layer before allocation; malformed inputs do not become zero-filled successful
uploads. Loading does not create command buffers, generate replacement mipmaps or introduce
per-frame texture copies. It is not yet integrated with scene rendering.

CPU tests cover all 256 palette indices, raw alpha, multiple mips/layers and malformed input.
The framework GPU contract reads both texture-array layers and three mips through Metal and
checks every channel. No Metal device is SKIP 77, not PASS. A private actual-asset test explicitly
counts entries excluded by the original GL bucket guard; those are never claimed decoded.
It also compares the original dimension buckets: 269 total, with 57 requiring the upstream's
explicit RGBA-zero initialization of absent upper mips (1,244 layer/mip images). The uploader's
default `Reject` policy still rejects these. Opt-in `UpstreamZeroFill` reproduces that original
initialization, with a count in the validated shape; it does not generate mips or accept a
malformed present mip or missing base image. All 269 real buckets pass this source-derived
policy. Neither the policy nor these CPU checks relax any strict GS blend rejection.
Actual command cost, image correctness, full renderer integration and physical-device behavior
remain unmeasured. Existing strict blend rejections are unchanged.

No Shared Core adoption, ReRAC changes or GameStudio engine changes were made.

### Full game / UIKit integration investigation

`compile_native_core.py` reuses generated objects and compiles the original game,
runtime, format readers and sound implementation. Mixed CPU/GL renderer files retain
their original capture/readiness functions; normal dead stripping removes the unused
GL graph. No GL replacement symbols or undefined-symbol bypass are permitted.
`attempt_game_application_link.py --experimental-local-investigation` performs a true
UIKit MH_EXECUTE link with all generated units, the Metal GS stream encoder, original
audio adapter, GameController and sandbox reader. A partial link is never accepted.

The Apple scheduling adapter explicitly reports Linux RealtimeKit elevation unavailable;
CoreAudio controls device scheduling. It does not claim QoS or realtime promotion.
The profiling clock no longer uses an x86 intrinsic on ARM64; game clocks are unchanged.

The UIKit host verifies the imported owned-disc identity and every converted asset hash,
keeps saves in app-owned storage, pauses on lifecycle/audio interruption, and feeds the
original game/VU GS primitive stream into Metal. Its retained framebuffer preserves
Held fades; the drawable must have `framebufferOnly=NO` because presentation uses a blit.
The current faithful presentation preserves 4:3 geometry; native widened projection is
not yet implemented. Unsupported render-target/mip/alpha-depth/extended GS paths fail
explicitly, rather than silently producing approximate graphics.

`package_private_investigation.py --experimental-local-investigation` is **local only**.
It checks full-link provenance, ARM64 MH_EXECUTE, system library dependencies, matching
dSYM UUID, required resources and ZIP integrity before producing a private unsigned IPA.
It bundles no ISO or assets, but its linked executable contains generated game code:
**never upload this IPA, executable, dSYM or private identity to public CI/artifacts**.
Signing and physical installation remain untested. Old local compiler experimental
strict-FP output is investigation evidence, not production fidelity or a playable claim.
Modern hosted public-source component tests do not validate the private full-game build.
