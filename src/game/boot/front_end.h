// The game's start before the first level: the boot module of SCUS_971.99 (its
// code 0x1e8d00..0x23d360, the same as the disc's globals/frontbin.elf; called the boot module
// below) checks the memory card (its pictures), plays the intro movie, shows the copyright, runs
// the title screen (its own scene, logo, "press start", music) and its main menu, then leaves
// through its copy of the level exit to the chosen level.
//
// The boot module is a build of the same engine as the levels' overlays, linked at its own
// addresses. It runs on its own image (front_image.h): the engine code it shares with
// the levels is the ports' (src/game/ovl) relocated onto it (ovl_reloc_set_level(OVL_FRONT), ADR
// 0008); the code only it has is ported at its own addresses (`// orig: front:0x...`).
#ifndef RAC1_GAME_BOOT_FRONT_END_H
#define RAC1_GAME_BOOT_FRONT_END_H

#include <cstdint>

namespace rac1 {

// The boot module's start (its image loaded, the translation set to it, the host's IOP up).
void boot_start();
// One game frame of the boot. False once the title loop is left (0x15f5b0, by New Game / Load
// Game / the debug menu): the next level then in 0x15f600, the current level -1 (as the boot
// module leaves it for its level exit 0x231ff0 = the overlays' 0x292e08).
bool boot_frame();
bool boot_active();
// Called by the host once the title's level is loaded (the host renderers' copy of its ties,
// shrubs and sky); null when the host has no renderer (oracle tests).
extern void (*g_boot_title_loaded_hook)();
// The boot in its title loop (not a movie, not its wait).
bool boot_in_title();
// The boot in its memory card check (its picture waiting for a button).
bool boot_in_card_check();
// A boot picture is in the frame buffer (the memory card's message, the copyright): the host
// shows the frame buffer.
bool boot_picture_shown();
// The debug menu's "skip": the title left for `level` (as Load Game does, with the level in
// 0x15ed84).
void boot_skip_to(int32_t level);

}  // namespace rac1

#endif
