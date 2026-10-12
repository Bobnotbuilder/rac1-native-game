// The ship sequences' scene file: read from the disc through the stream system into the buffer
// *(0x16c8bc), cut into blocks (0x60 frames each, 0x50 on PAL), a block decompressed into
// *(0x16c8b8) and its mobys spawned / given their animation.
#include <game/ovl/arrival.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/music_streams.h>
#include <game/ovl/ovl_common.h>
#include <game/lib/snd989.h>
#include <game/ovl/screen_fade.h>
#include <game/port_stubs.h>

namespace rac1 {

namespace {
constexpr OvConst<0x0016c860u> SC{};  // the scene
constexpr uint32_t G = 0x0013e030;   // the ship sequence
constexpr uint32_t STREAMS = 0x001516d0;
}  // namespace

// The stream system's disc read awaited (`wait`: until it is done, a frame of the streams and
// 989snd each time) or polled (one frame); the read still pending (*(0x1516d8)).
// orig: 0x002660c0
int32_t stream_reads_wait(int32_t wait) {
    if (wait != 0) {
        while (lh(STREAMS + 8) != 0) {
            TODO_PORT(0x00122298, "kernel: vsync wait")(0);
            music_streams_frame();
            snd_release();
            snd_flush();
            snd_hold();
        }
        return lh(STREAMS + 8);
    }
    music_streams_frame();
    snd_release();
    snd_flush();
    snd_hold();
    return lh(STREAMS + 8);
}

// The block *(0x16c8bc) decompressed into *(0x16c8b8): its frames +0 (0x16c8a0), +8 (0x16c8a8),
// its mobys +0xc (0x16c8a4), the camera's frames (+0x10, 0x16c8b4), +4 (0x16c8ac, none below
// 0x400); per moby (the offsets from +0x14): its class (+0; in game mode 6 the first moby of
// class 0x215 is the ship's variant 0x160548[*(s16*)0x13e056]), its positions (+0xc) and its
// animation (from +0x10, its pointers +0x1c.. relocated): spawned the first time (always drawn,
// a slot of its class's animation table, the hero's colour or 0xe0e0e000 << 22), the positions
// to +0x78 and the animation into its class's table +0x48.
// orig: 0x00245270
void scene_block_decode() {
    // 0x118a80 (0) before and after: no memory effect
    wad_decompress(lwu(SC + 0x5c), lwu(SC + 0x58));
    const uint32_t s2 = lwu(SC + 0x58);
    sw(SC + 0x38, 0);
    uint32_t s5 = s2 + 0x14;
    sh(SC + 0x40, lhu(s2));
    sh(SC + 0x48, lhu(s2 + 8));
    const int32_t a1 = lw(s2 + 4);
    sh(SC + 0x44, lhu(s2 + 0xc));
    sw(SC + 0x54, s2 + lwu(s2 + 0x10));
    if (a1 < 0x400) sw(SC + 0x4c, 0);
    else sw(SC + 0x4c, s2 + static_cast<uint32_t>(a1));
    if (!(0 < lh(SC + 0x44))) return;
    int32_t i = 0;
    do {
        const uint32_t ent = s2 + lwu(s5);
        s5 += 4;
        uint32_t cls = lwu(ent);
        const uint32_t s0 = ent + 0x10;
        const uint32_t pos = s2 + lwu(ent + 0xc);
        if (lw(OV(0x0015f5c4)) == 6 && i == 0 && cls == 0x215) cls = lwu(OV(0x00160548) + static_cast<uint32_t>(lh(G + 0x26)) * 4);
        const uint32_t slot = SC + 0x178 + static_cast<uint32_t>(i) * 4;
        uint32_t m = lwu(slot);
        if (m == 0) {
            m = moby_spawn(cls);
            const uint32_t cd = lwu(m + 0x24);
            const uint32_t n = lbu(cd + 0xc);
            sb(cd + 0xc, n + 1);
            sb(m + 0x53, n);
            sh(m + 0x32, 0x1ff);
            sb(m + 0x72, 0xff);
            sh(m + 0x34, lhu(m + 0x34) | 6u);
            sb(m + 0x52, n);
            sw(m + 0x94, 0);
            const uint32_t hero = lwu(0x0013f350 + 0x2080);
            if (hero != 0) sd(m + 0x38, ld(hero + 0x38));
            else sd(m + 0x38, uint64_t{0xe0e0e000u} << 22);
            if (lbu(lwu(m + 0x24) + 6) != 0) sb(m + 0x73, 0x18);
            sw(slot, m);
        }
        sw(m + 0x78, pos);
        sw(lwu(m + 0x24) + static_cast<uint32_t>(lbu(m + 0x52)) * 4 + 0x48, s0);
        for (int32_t k = 0; k < static_cast<int32_t>(lbu(s0 + 0x10)); ++k) {
            const uint32_t p = s0 + 0x1c + static_cast<uint32_t>(k) * 4;
            sw(p, s0 + lwu(p));
        }
        ++i;
    } while (i < lh(SC + 0x44));
}

// Block `block` of the scene: its compressed data (0x16c8c0[block]) decoded, the read buffer
// back to *(0x173e08) + *(0x1611cc).
// orig: 0x00245720
void scene_block_load(int32_t block) {
    sw(SC + 0x5c, lwu(SC + 0x60 + static_cast<uint32_t>(block) * 4));
    scene_block_decode();
    sw(SC + 0x5c, lwu(OV(0x00173e08)) + lwu(OV(0x001611cc)));
}

// Scene `scene` read from the disc (the level header's table 0x138e68, PAL 0x138eb8: sector,
// count) into *(0x16c8bc), the screen faded over `fade` frames meanwhile, the read awaited; its
// blocks (the table at its start: offset, size; up to 0x46, ending with a zero size) to
// 0x16c8c0.. (past the 0x800-byte table).
// orig: 0x00245610
void scene_read(int32_t scene, int32_t fade) {
    const uint32_t e = 0x00137b80 + static_cast<uint32_t>(scene) * 8;
    if (lw(0x0015ed80) == 0) stream_disc_read(lwu(SC + 0x5c), lwu(e + 0x12e8), lwu(e + 0x12ec));
    else stream_disc_read(lwu(SC + 0x5c), lwu(e + 0x1338), lwu(e + 0x133c));
    if (fade != 0) {
        stream_reads_wait(0);
        screen_fade(fade);
    }
    stream_reads_wait(1);
    uint32_t a1 = lwu(SC + 0x5c);
    if (lw(a1 + 4) == 0) return;
    uint32_t t0 = SC + 0x60;
    int32_t n = 0;
    do {
        const uint32_t off = lwu(a1);
        ++n;
        const uint32_t base = lwu(SC + 0x5c);
        a1 += 8;
        sw(t0, base + off + 0x800);
        if (!(n < 0x46)) return;
        t0 += 4;
    } while (lw(a1 + 4) != 0);
}

}  // namespace rac1
