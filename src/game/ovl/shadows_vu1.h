// The VU1 shadow-mesh program (588 instructions at micro address 0, uploaded by MPG from the boot
// ELF's 0x10e810 by the shadows' set-up 0x1fc740; the render frame's program 7): at 0 its
// start-up (the two view matrices, the clipped fans' GIF tag, the screen constants), at 0xc one
// batch of a textured triangle strip projected and kicked to the GS, at 0xe the same with each
// triangle that crosses the view's sides or near plane disabled in the strip (ADC) and drawn
// instead as a fan clipped in clip space.
#ifndef RAC1_GAME_OVL_SHADOWS_VU1_H
#define RAC1_GAME_OVL_SHADOWS_VU1_H

#include <runtime/vu/vu_interpreter.h>

#include <cstdint>
#include <functional>

namespace rac1 {

// orig: 0x0010e818
void shadows_vu1(VUState& vu, uint32_t start, const std::function<void(uint32_t)>& kick);
// Registers it with the VU1 backend (by the hash of its code: runtime/vu/vu1.h).
void shadows_vu1_register();

}  // namespace rac1

#endif
