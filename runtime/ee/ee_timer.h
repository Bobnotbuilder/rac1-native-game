#ifndef RAC1_RUNTIME_EE_EE_TIMER_H
#define RAC1_RUNTIME_EE_EE_TIMER_H
// The EE's timer 1 (T1_COUNT 0x10000800) on the host, as on an ideal PS2.
//
// The game runs timer 1 on BUSCLK / 256 (T1_MODE 0x82: 576000 counts a second, 0x2580 = 9600 a 1/60 s frame,
// the main loop's own budget constant) and reads it to measure its frames:
// at a frame's start (the time since the last frame's start, then reset), at the render's start and after the
// frame's work (how loaded the frame was: 0x15f5d4 / 0x15f5d0, the load shedding of particles, lights,
// explosions), and before the frame's vsync wait (the work ran past the frame: an extra update).
//
// The host's EE takes no PS2 time: the counter moves only when a vsync passes, by 0x2580 (an NTSC frame), and
// wraps at 16 bits as the hardware's does. So a frame's work reads 0 (an unloaded console: no load shedding, no
// catch-up update), a frame's start reads 0x2580 per vsync since the last start (1 normally; n + 3 after a
// screen fade of n frames, as on the PS2), whatever the host's own speed.
//
// Hardware layer: no game decision here.
#include <cstdint>

namespace rac1 {

uint32_t ee_t1_count();   // T1_COUNT read (16 bits)
void ee_t1_reset();       // T1_COUNT = 0 written
void ee_t1_vsync();       // a vsync passed: the counter + 0x2580

}  // namespace rac1

#endif
