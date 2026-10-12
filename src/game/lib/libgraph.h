#ifndef RAC1_GAME_LIB_LIBGRAPH_H
#define RAC1_GAME_LIB_LIBGRAPH_H

// Ports of the Sony libgraph functions in the boot ELF that game code calls. The GS is the
// host's (runtime/gs/gs_emu): what the console sends by GIF DMA goes to it at once.

#include <cstdint>

namespace rac1 {

// orig: 0x00122330 — sceGsSetDefLoadImage(lp, dbp, dbw, dpsm, x, y, w, h): the 6-quadword
// GIF packet at `lp` (A+D BITBLTBUF / TRXPOS / TRXREG / TRXDIR, then the IMAGE tag of the data's
// quadwords). 6, or 0 for more than 0x7fff quadwords.
int32_t gs_set_def_load_image(uint32_t lp, int32_t dbp, int32_t dbw, int32_t dpsm, int32_t x, int32_t y, int32_t w, int32_t h);
// orig: 0x00122658 — sceGsExecLoadImage(lp, src): the packet, then the data, by GIF DMA. 0.
int32_t gs_exec_load_image(uint32_t lp, uint32_t src);
// orig: 0x00122518 — sceGsSetDefStoreImage(lp, sbp, sbw, spsm, x, y, w, h): the 7-quadword packet
// (A+D BITBLTBUF / TRXPOS / TRXREG, FINISH, TRXDIR = 1: the GS memory read back). 7.
int32_t gs_set_def_store_image(uint32_t lp, int32_t dbp, int32_t dbw, int32_t dpsm, int32_t x, int32_t y, int32_t w, int32_t h);
// orig: (host) stands in for 0x001227d8 — sceGsExecStoreImage(lp, dst): the packet's rectangle of the
// GS memory (BITBLTBUF source, TRXPOS, TRXREG) read from the host's GS into the buffer `dst`, pixels
// in transfer order (3 bytes a pixel for PSMCT24). 0.
int32_t gs_exec_store_image(uint32_t lp, uint32_t dst);
// orig: 0x00121a08 — sceGsGetGParam(): the library's parameters (0x132d40).
uint32_t gs_get_gparam();
// orig: 0x00121f00 — sceGszbufaddr(psm, w, h): the frame's size in 2 KB pages (twice that
// unless the mode word of the parameters is 1), as a 16-bit value.
int32_t gs_zbuf_addr(int32_t psm, int32_t w, int32_t h);
// orig: 0x00121fc8 — sceGsSetDefDrawEnv(env, psm, w, h, ztest, zpsm): the 8 A+D register
// writes (FRAME, ZBUF, XYOFFSET, SCISSOR, PRMODECONT, COLCLAMP, DTHE, TEST) at `env`. 8.
int32_t gs_set_def_draw_env(uint32_t env, int32_t psm, int32_t w, int32_t h, int32_t ztest, int32_t zpsm);

// orig: (host) stands in for 0x00120558 — sceGsSyncPath(mode, timeout): the wait for the GIF /
// VIF paths to finish; the host's GS has finished at once. 0.
int32_t gs_sync_path(int32_t mode, int32_t timeout);

}  // namespace rac1

#endif
