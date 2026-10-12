// Aridia's class hooks: see aridia_hooks.cpp.
#ifndef RAC1_GAME_OVL_ARIDIA_HOOKS_H
#define RAC1_GAME_OVL_ARIDIA_HOOKS_H

#include <cstdint>

namespace rac1 {

uint32_t class_hook_10_l2(uint32_t m); // L2:0x002e6500 (Veldin's class_hook_10 0x2db890 with Aridia's hooks)
int32_t class_hook_4_l2(uint32_t m);  // L2:0x002e4e88 (Veldin's class_hook_4 0x2da218 with Aridia's hooks)

void shark_hook_moby_0(uint32_t m);   // L2:0x002d61c0
void shark_hook_moby_4(uint32_t m);   // L2:0x002d6210
void shark_hook_moby_8(uint32_t m);   // L2:0x002d6290
void shark_hook_moby_c(uint32_t m);   // L2:0x002d62e0
void shark_hook_moby_10(uint32_t m);  // L2:0x002d6310
void shark_hook_moby_14(uint32_t m);  // L2:0x002d6340

}  // namespace rac1

#endif
