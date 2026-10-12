// The VU1 particle program (217 instructions, uploaded by MPG from the boot ELF's 0x101088 by the
// particles' draw 0x267c98): at 0 its start-up (the view, screen and GIF constants into vf16..
// vf31), at 0x14 one chunk of particles (4 quadwords each, 6 for a streak) turned into GIF
// packets (a sprite quad, a line or a streak quad) kicked to the GS.
#ifndef RAC1_GAME_OVL_PARTICLES_VU1_H
#define RAC1_GAME_OVL_PARTICLES_VU1_H

#include <runtime/vu/vu_interpreter.h>

#include <cstdint>
#include <functional>

namespace rac1 {

// orig: 0x00101088
void particles_vu1(VUState& vu, uint32_t start, const std::function<void(uint32_t)>& kick);
// Registers it with the VU1 backend (by the hash of its code: runtime/vu/vu1.h).
void particles_vu1_register();

}  // namespace rac1

#endif
