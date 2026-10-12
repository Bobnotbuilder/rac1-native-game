// The particles' draw (hand-written EE code with VU0 macro ops): the live particles are culled
// against the view, bucket-sorted far to near by their view depth on the scratchpad, then
// written as VIF packets for the VU1 particle program into the frame's packet buffer
// (0x1611c0), 0x800-byte chunks at a time through the scratchpad (toSPR / fromSPR DMA).
#ifndef RAC1_GAME_OVL_PARTICLES_DRAW_H
#define RAC1_GAME_OVL_PARTICLES_DRAW_H

#include <cstdint>

namespace rac1 {

// orig: 0x00267c98
void particles_draw();

}  // namespace rac1

#endif
