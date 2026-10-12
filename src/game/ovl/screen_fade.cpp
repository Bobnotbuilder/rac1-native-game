// The screen fade 0x1f9838. Every level's overlay and the boot module carry the same
// code at their own addresses (Novalis 0x21b438, Aridia 0x20a770, Kerwan 0x1f31a0, Eudora 0x1ec1a0,
// Rilgar 0x2284f0, ..., the boot module's 0x1f4a58: tools/overlay_match's maps), relocated here
// through OV(); the frames are host frames through the game's
// coroutine (ADR 0012).
#include <game/ovl/screen_fade.h>

#include <game/enhance/advanced.h>

#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <game/port_stubs.h>
#include <runtime/ee/ee_thread.h>

namespace rac1 {

namespace {
// The vsync wait (0x122298, a0 = 0): with "Original fades" the game's thread suspended
// until the host's next frame, `kicked` the frame whose packets the loop sends after the wait (else
// the screen is held); by default the wait returns at once (a vsync passed: the host's per-vsync
// work done, the sound's time): the fade's frames all run inside the host frame, none shown.
// orig: (host) the kernel's vsync wait 0x00122298 inside the fade's loop
void fade_vsync(bool kicked) { ee_thread_vsync(kicked ? EeVsyncFrame::Kicked : EeVsyncFrame::Held, advanced_original_fades()); }

// orig: (helper) repeated in 0x001f9838: the frame counter 0x15f3f8 ($gp - 0x7808) + 1
void frame_count() { sw(OV(0x00166c00u - 0x7808), static_cast<uint32_t>(lw(OV(0x0015f3f8)) + 1)); }
}  // namespace

// The fade's start: the DMA wait, a vsync, the frame counter, the packet buffers reset (the
// caller's frame so far dropped: never sent). Then n frames, s = n - 1 down to 0: the display
// environment 0x151780+0x30 (the render buffer, page 0x70), 0x152040's packet, a black sprite of
// alpha 0x80 over the render buffer (0x1fa050: RGBAQ, then 0x13cc90), the display buffer's
// environment +0xc0 (page 0, z masked), RGBAQ = alpha 0x80 - (s * 0x80) / (s + 1) (= 0x80 / (s + 1)),
// the sprite 0x13cdd0 over the display buffer (a DMA ref tag of 0x14 quadwords + VIF DIRECT,
// written word by word at the cursor 0x1611c0), the DMA wait, the vsync, the counter, the kick
// (0x2a08c8: the host sends the frame's packets at the vsync), the packet buffers flipped. Then a
// last DMA wait, vsync and counter, the buffers reset, the display environment and 0x152040's
// packet started in the fresh buffer.
// orig: 0x001f9838
void screen_fade(int32_t n) {
    int32_t s = static_cast<int32_t>(static_cast<uint32_t>(n) - 1u);
    dma_wait(1);
    fade_vsync(false);
    frame_count();
    sub_002a07f8();
    if (s >= 0) {
        do {
            packet_call_display();
            sub_00200fd8();
            sub_001fa050(0, 0, 0, 0x80);
            sub_00201040();
            const int32_t q = ee_div(static_cast<int32_t>(static_cast<uint32_t>(s) << 7), static_cast<int32_t>(static_cast<uint32_t>(s) + 1u));
            s = static_cast<int32_t>(static_cast<uint32_t>(s) - 1u);
            const int64_t alpha = static_cast<int32_t>(0x80u - static_cast<uint32_t>(q));  // (subu: the word sign-extended, then dsll 24)
            sub_002a0ba8(1, static_cast<uint64_t>(alpha) << 24);
            constexpr OvConst<0x001611c0u> CUR{};
            sw(lwu(CUR) + 0x0, 0x30000014u);
            sw(lwu(CUR) + 0x4, 0x0013cdd0u);
            sw(lwu(CUR) + 0x8, 0);
            sw(lwu(CUR) + 0xc, 0x50000014u);
            sw(OV(0x00166c00u - 0x5a40), lwu(CUR) + 0x10);
            dma_wait(1);
            fade_vsync(true);
            frame_count();
            TODO_PORT(0x002a08c8, "DMA: packet kick")();  // (the host sends the frame's packets at the vsync above)
            sub_002a0858();
        } while (s >= 0);
    }
    dma_wait(1);
    fade_vsync(false);
    frame_count();
    sub_002a07f8();
    packet_call_display();
    sub_00200fd8();
}

}  // namespace rac1
