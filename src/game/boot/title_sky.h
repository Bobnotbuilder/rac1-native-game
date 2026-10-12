// The title's sky shells: the boot module's own sky routine (see title_sky.cpp).
#ifndef RAC1_GAME_BOOT_TITLE_SKY_H
#define RAC1_GAME_BOOT_TITLE_SKY_H

#include <cstdint>

#include <game/ovl/novalis_mobys2.h>  // TrRet

namespace rac1 {

TrRet front_sky_shells_tr(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14,
                          float f15);  // orig: front:0x0022ae70

}  // namespace rac1

#endif
