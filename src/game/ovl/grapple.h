#ifndef RAC1_GAME_OVL_GRAPPLE_H
#define RAC1_GAME_OVL_GRAPPLE_H
// grapple, transliterated (grapple_asm.cpp). Addresses are Veldin's overlay addresses (the shared engine).
#include <cstdint>

namespace rac1 {

void swingshot_grapple_update(uint32_t m);  // orig: 0x002c7070
void grapple_fns_register();  // orig: (none)

}  // namespace rac1
#endif
