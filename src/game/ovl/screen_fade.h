#ifndef RAC1_GAME_OVL_SCREEN_FADE_H
#define RAC1_GAME_OVL_SCREEN_FADE_H
// The screen fade:, ADR 0012.
#include <cstdint>

namespace rac1 {

// The screen faded out over `n` frames: n + 2 vsync waits inside the call (the frame counter
// 0x15f3f8 + n + 2), each of the n frames darkening the display buffer by 1 / (s + 1) of what is
// left (s = n - 1 .. 0: black at the end). By default the frames run inside the call,
// nothing of them shown; with "Original fades" (advanced_original_fades) and on the game's coroutine
// each wait is a host frame (runtime/ee/ee_thread.h). The same RAM writes either way.
void screen_fade(int32_t n);  // orig: 0x001f9838

}  // namespace rac1

#endif
