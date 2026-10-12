// The sparkles' draw: the 16 sparkle slots (0x16ea40, 0x20 bytes: +0 the position, +0x10 the
// frames left, +0x12 the alpha, +0x18 the turn angle, +0x1c the size; queued by sparkle_add
// 0x1fe990) drawn as screen-facing turned quads by GIF packets (path 2) in the frame's packet
// buffer, additive, with the effect texture 0x13.
#ifndef RAC1_GAME_OVL_SPARKLES_DRAW_H
#define RAC1_GAME_OVL_SPARKLES_DRAW_H

namespace rac1 {

// orig: 0x001fead8
void sparkles_draw();

}  // namespace rac1

#endif
