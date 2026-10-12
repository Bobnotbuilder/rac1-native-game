// The Vendor's screens' frame picture: the vsync waits of the draw 0x2a02b0 around the frame it renders and
// stores. See vendor_frame.h, and ADR 0012 (the game's blocking loops on a coroutine).
#include <game/ovl/vendor_frame.h>

#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <game/port_stubs.h>
#include <runtime/ee/ee_thread.h>

namespace rac1 {

// orig: (host) the kernel's vsync wait 0x00122298 called by the vendor's screens (the call sites by their return address)
void vendor_vsync_wait(uint32_t ra) {
    switch (ra) {
        case NOT_ADDR(0x002a0378u):
            // the draw 0x2a02b0: the frame's render 0x1f85b8 done, a DMA wait, this vsync, the frame counter, the kick 0x2a08c8
            // (the host sends the packets of a frame at its vsync), the buffers flipped (0x2a0858): the frame shown, always (the
            // host draws it: its picture is what 0x1f99c0 stores, runtime_host's store hook)
            ee_thread_vsync(EeVsyncFrame::Scene, true);
            break;
        case NOT_ADDR(0x001f99e0u):
            // 0x1f99c0, before the frame buffer is stored: a DMA wait, this vsync, the counter, the buffers reset: nothing new
            // sent (a vsync passes: the host's per-vsync work; the screen keeps the frame above)
            ee_thread_vsync(EeVsyncFrame::Held, false);
            break;
        default:
            TODO_PORT(0x00122298, "base ELF function (not ported)")();
            break;
    }
}

// orig: (host) game mode 5 (0x15f5c4) with the vendor's draw in its second mode (the state block 0x1ca4c0's +8 = 1: the stored
// picture put back first, 0x2a0584)
bool vendor_picture_frame() { return lw(OV(0x0015f5c4)) == 5 && lw(OV(0x001ca4c8)) == 1; }

}  // namespace rac1
