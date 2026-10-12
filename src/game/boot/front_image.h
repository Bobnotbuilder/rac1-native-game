// The boot module's own image: SCUS_971.99's sections .lit .. .text (0x15ef00.
// 0x23d360), the engine build the PS2 runs before the first level (the boot, the intro movie, the
// title and its main menu; the disc's globals/frontbin.elf is the same image). Its code shared with
// the levels runs as the ports (src/game/ovl), relocated onto its addresses (ADR 0008,
// ovl_reloc_set_level(OVL_FRONT)); its own code is ported at its own addresses (src/game/boot,
// `// orig: front:0x...`).
#ifndef RAC1_GAME_BOOT_FRONT_IMAGE_H
#define RAC1_GAME_BOOT_FRONT_IMAGE_H

#include <cstdint>

namespace rac1 {

// The boot module's sections copied from assets/dumps/SCUS_971.99 into EE memory (its .bss
// cleared): what the PS2 has in memory when the ELF starts, before any level overlay is loaded
// over it. False when the ELF cannot be read.
bool front_image_load();

}  // namespace rac1

#endif
