// The EE's timer 1 on the host: see ee_timer.h.
#include "ee_timer.h"

namespace rac1 {

namespace {
uint32_t g_t1 = 0;
}  // namespace

// orig: (host) T1_COUNT (0x10000800), read
uint32_t ee_t1_count() { return g_t1 & 0xffffu; }
// orig: (host) T1_COUNT (0x10000800), written 0
void ee_t1_reset() { g_t1 = 0; }
// orig: (host) T1 counting BUSCLK / 256 over a vsync's 1/60 s (0x2580), 16 bits
void ee_t1_vsync() { g_t1 = (g_t1 + 0x2580u) & 0xffffu; }

}  // namespace rac1
