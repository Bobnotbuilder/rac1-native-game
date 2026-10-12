// The boot module's start and boot pictures (boot_start.cpp): its own code at its
// own addresses (see front_image.h).
#ifndef RAC1_GAME_BOOT_BOOT_START_H
#define RAC1_GAME_BOOT_BOOT_START_H

#include <cstdint>

namespace rac1 {

void boot_tex_base();                       // orig: front:0x001e9128
void boot_clut_order();                     // orig: front:0x001f7a30
void boot_module_start();                   // orig: front:0x00201650
int32_t boot_card_check();                  // orig: front:0x00209168
void boot_picture(uint32_t src);            // orig: front:0x002012b8
int32_t boot_bank_load(uint32_t lsn);       // orig: front:0x0022d708

}  // namespace rac1

#endif
