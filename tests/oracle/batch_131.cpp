// Oracle batch 131 (task 0366): the Vendor's screens' draw 0x2a02b0 (vendor_tr.cpp, transliterated at Veldin's addresses) in
// its second mode, the one every frame of the screens runs: the block 0x1ca4c0's +8 = 1, the stored frame picture put back
// (0x2a0fc8), the view, the moby pass with the item models (the selected item's +0x200 / +0x300, the screens' moby +0x1c),
// the screens' panels (0x29ff58, 0x29f3c0, 0x29fcd0, 0x29f990), the HUD. Batches 113 / 115 never compared the draw's body
// (113 runs it with the start's skip flag 0x15f5d8 set; 115 left it out: its first mode renders a whole frame, 0x1f85b8,
// whose passes need the main loop's set-up). Here the port runs the first mode once in the set-up (the frame rendered, its
// picture stored, the item mobys made: 0x2a048c), then the original and the port run the second mode on the same memory.
// Run with ORACLE_LEVEL=1 too (Novalis's own original against the relocated port).

#include "ovl_oracle.h"

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/vendor_tr.h>
#include <game/lib/libgraph.h>
#include <game/ovl/pause_menu.h>
#include <runtime/gs/gs_emu.h>
#include <game/ovl/vu0_macro.h>

#include <runtime/kernel/disc.h>

#include <cstring>

namespace rac1::oracle {

namespace {
// the VU0 registers the original started from (vf / ACC / Q), given to the port (as batch 110): the moby draw 0x2559c0's billboard
// rows (0x256c30..) carry vf5..vf7's stale w lanes into the record's matrix (sqc2 0x256ca0), which the VU1 pass reads whole
vu0m::Regs g_vu0_in;
void vu0_capture(EeInterpreter& ee) {
    for (int i = 0; i < 32; ++i)
        for (int k = 0; k < 4; ++k) g_vu0_in.vf[i].v[k] = ee.vu0().vf[static_cast<size_t>(i)].f[k];
    for (int k = 0; k < 4; ++k) g_vu0_in.acc.v[k] = ee.vu0().acc.f[k];
    g_vu0_in.q = ee.vu0().q;
}
}  // namespace

void batch_131() {
    if (g_only_batch != 131) return;
    g_batch = 131;
    const std::vector<uint32_t> saved_ignored = g_ignored_words;
    g_ignored_words = vcross_lanes();
    g_ignored_words.push_back(OV(0x001e3d5c));  // (a VCROSS result's stale w lane: the vector 0x1e3d50 of the mode's camera rows, as batch 113)
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;  // as in the runtime

    // 989snd's IOP server (as batch 115's): replies a function of the command, does the stream reads
    sif_rpc_register(0x123456, [](uint32_t fno, uint32_t send, uint32_t, uint32_t recv, uint32_t) {
        auto result = [](uint32_t id, uint32_t a0) -> uint32_t {
            const uint32_t h = (id * 0x9e3779b1u) ^ (a0 * 0x85ebca6bu);
            return ((h >> 7) % 4 == 0) ? 0u : ((h >> 3) | 1u);
        };
        if (fno == 0x4d) {
            const uint32_t n = ee_read<uint32_t>(send);
            uint32_t p = send + 4;
            for (uint32_t i = 0; i < n; ++i) {
                const uint32_t id = ee_read<uint16_t>(p), size = ee_read<uint16_t>(p + 2);
                ee_write<uint32_t>(recv + 4 + i * 4, result(id, size != 0 ? ee_read<uint32_t>(p + 4) : 0u));
                if (id == 0x38) {
                    disc_read_ee(ee_read<uint32_t>(p + 0xc), ee_read<uint32_t>(p + 4), ee_read<uint32_t>(p + 8));
                    ee_write<uint32_t>(0x00137b10, 0);
                    ee_write<uint32_t>(0x00137b00, 0);
                }
                p += 4 + ((size + 3) & ~3u);
            }
            ee_write<uint32_t>(recv, 0xffffffffu);
            ee_write<uint32_t>(recv + 4 + n * 4, 0xffffffffu);
            return;
        }
        ee_write<uint32_t>(recv + 4, result(fno, send != 0 ? ee_read<uint32_t>(send) : 0u));
        ee_write<uint32_t>(recv, 0xffffffffu);
        ee_write<uint32_t>(recv + 8, 0xffffffffu);
    });
    // the level after one gameplay frame (the port's), kept as a snapshot
    static std::vector<uint8_t> snap;
    static int snap_level = -1;
    auto level = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty() || snap_level != g_oracle_level) {
            veldin_boot();
            level_setup_body(g_oracle_level == 0 ? 1 : g_oracle_level, 0);  // (level 0: Novalis's data on Veldin's image, as batch 113)
            w32(OV(0x0016c060), 0xf);
            gameplay_frame();
            snap.assign(ram, ram + 0x02000000);
            snap_level = g_oracle_level;
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    auto mobys_of = [](int cls) {
        std::vector<uint32_t> v;
        for (uint32_t m = ee_read<uint32_t>(OV(0x0015ffd8)); m < ee_read<uint32_t>(OV(0x0015ffe0)); m += 0x100)
            if (ee_read<uint8_t>(m + 0x20) < 0xfe && ee_read<int16_t>(m + 0xa6) == cls) v.push_back(m);
        return v;
    };

    // The DMA kick, FlushCache and the vsync wait are stubbed on both sides (as batches 113 / 115; the port's vsync wait,
    // vendor_frame.h, returns at once off the game's coroutine).
    {
        g_extra_noop_hooks = {OVF(0x002a08c8), 0x00122298, 0x00118a80};
        constexpr OvConst<0x001ca4c0u> S{};
        auto pad = [] {  // the pad (+0x1a0 / +0x1a4 of the pad block 0x13c940)
            const uint32_t b = coin() ? 0u : (R() & 0xf0f0u);
            w32(0x0013c940 + 0x1a4, b);
            w32(0x0013c940 + 0x1a0, b | (coin() ? 0u : (R() & 0xf0f0u)));
        };
        auto second_mode = [level, mobys_of, pad, S] {
            level();
            for (const int cls : {730, 731, 790})  // (the Agnogg ship starts a scene: its stream read needs the IOP, as batch 113)
                for (const uint32_t m : mobys_of(cls)) w32(m + 0x74, 0);
            item_give(0xa, 0);  // (an item owned, as on any real visit: batch 115)
            if (coin()) item_give(0xf, 0);
            w32(0x0015ed98, u(ri(0, 20000)));  // (the bolts)
            const std::vector<uint32_t> vs = mobys_of(11);
            vendor_screen_start_tr(vs.empty() || coin() ? 0u : vs[static_cast<size_t>(ri(0, static_cast<int>(vs.size()) - 1))]);
            // the first mode's frame with the picture asked for (+0xc), the start's skip flag down: the frame rendered and
            // stored, the item mobys made, +8 = 1
            w32(OV(0x0015f5d8), 0);
            w32(S + 0x8, 0);
            w32(S + 0xc, 1);
            // (the Vendor's voice stream read 0x265fa0 of that frame skipped: the halfword 0x1516d8 non-zero; its disc read needs
            // the IOP's stream state, not modelled here)
            const uint32_t h = ee_read<uint16_t>(0x001516d8);
            w16(0x001516d8, 1);
            // (the moby groups drawn, 0x19a600, -1 ended: the moby pass's texture step 0x2555c0 writes it when the mobys are drawn,
            // 0x16a018 / 0x16a014; a level's snapshot that has not drawn them yet holds zeros there, which the slots' step 0x24fad0
            // reads as an endless list)
            if (ee_read<uint32_t>(OV(0x0019a600)) == 0) w32(OV(0x0019a600), 0xffffffffu);
            vendor_mode_draw_tr(0);
            w16(0x001516d8, h);
            // the state word put (1 browsing .. 3 closing; the opening's 0 left in some cases: the update moves it on), the item
            // selected, the panels' timers, then k frames of the draw (the update is not run: the opening's mobys it moves are not
            // all set up when the frame is forced)
            if (coin()) w32(S, u(ri(1, 3)));
            const int n = static_cast<int>(ee_read<uint32_t>(S + 0x210));
            if (n > 0) w32(S + 0x58, u(ri(0, n - 1)));
            w32(S + 4, u(ri(0, 200)));
            const int k = ri(0, 4);
            for (int i = 0; i < k; ++i) {
                pad();
                vendor_mode_draw_tr(0);
            }
            pad();
            w32(OV(0x0015f5cc), R() & 0xffff);
            w32(OV(0x0015f5d8), 0);
            seed_rand();
            if (std::getenv("B131_INFO") != nullptr)
                std::printf("  after %d frames: state %u, +8 %u, +0x20 %08x, item %d, game mode %d\n", k, ee_read<uint32_t>(S), ee_read<uint32_t>(S + 8),
                            ee_read<uint32_t>(S + 0x20), ee_read<int32_t>(S + 0x58), ee_read<int32_t>(OV(0x0015f5c4)));
            if (std::getenv("B131_INFO") != nullptr)
                std::printf("  +0x1c %08x +0x14 %08x +0x2c %08x +0x18 %08x mobys %08x..%08x\n", ee_read<uint32_t>(S + 0x1c), ee_read<uint32_t>(S + 0x14), ee_read<uint32_t>(S + 0x2c),
                            ee_read<uint32_t>(S + 0x18), ee_read<uint32_t>(OV(0x0015ffd8)), ee_read<uint32_t>(OV(0x0015ffe0)));
            return Args{{0u}, {}};
        };
        g_pre_call = vu0_capture;
        // (the moby draw 0x2559c0's billboard rows, 0x256c30..0x256ca8: the original's matrix rows carry w lanes the port leaves 0, in
        // the scratchpad record and its copy in the packet; x, y, z equal. Not explained yet: task 0366's notes)
        g_ignore_lone_w_lanes = true;
        check("vendor_draw_picture", 0x002a02b0, 60, Ret::None, second_mode, [](A) {
            vu0m::regs().vf = g_vu0_in.vf;
            vu0m::regs().acc = g_vu0_in.acc;
            vu0m::regs().q = g_vu0_in.q;
            vendor_mode_draw_tr(0);
            return uint64_t{0};
        });
        g_pre_call = nullptr;
        g_ignore_lone_w_lanes = false;
        g_extra_noop_hooks.clear();
    }

    // ---- (task 0366b) the frame's store 0x2a0f18 (gs_store_rows: the pause menu's start 0x276368 and the vendor's 0x1f99c0
    // call it): the GS memory filled with random words, a destination, a block and a byte count (rounded up to 16 KB; 0 or
    // negative: nothing); libgraph's GS -> EE transfer 0x1227d8 runs as the host's stand-in on both sides (the GS replica's
    // memory read), its set-up 0x122518 and the loop the original's
    {
        g_extra_hooks = {{0x001227d8u, [](EeCpu& c) { gs_exec_store_image(c.r32(4), c.r32(5)); c.set32(2, 0); }}};
        {
            uint32_t x = 0x9e3779b9u;
            uint8_t* vram = gs::gs_global().vram();
            for (uint32_t i = 0; i < 4u << 20; i += 4) {
                x = x * 1664525u + 1013904223u;
                std::memcpy(vram + i, &x, 4);
            }
        }
        check("gs_store_rows", 0x002a0f18, 40, Ret::None, [] {
            const int32_t bytes = coin() ? ri(-0x100, 0x4000) : ri(0, 0x38000);
            const uint32_t dbp = u(ri(0, 0x3000));
            const uint32_t dst = 0x01400000u + (R() & 0x1ffff0u);
            seed_rand();
            return Args{{dst, dbp, u(bytes)}, {}};
        }, [](A a) { gs_store_rows(a.i[0], static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2])); return uint64_t{0}; });
        g_extra_hooks.clear();
    }

    // ---- (task 0366b) the pause menu mobys' panel corners 0x2df690: the joint list 0x161ba0 read by ldl / ldr (unaligned on
    // the levels whose copy is not on 16 bytes: Aridia, Kerwan, Eudora, Rilgar, Blarg, Gaspar's 0x161c58 ...); on the hero
    // (joints 0..3, the list's), its data pointer +0x78 put on a scratch area. Run with ORACLE_LEVEL=3 too.
    {
        check("menu_moby_corners", 0x002df690, 30, Ret::None, [level] {
            level();
            const uint32_t m = ee_read<uint32_t>(0x001413d0);
            w32(m + 0x78, 0x01500000u + (R() & 0xff0u));
            seed_rand();
            return Args{{m}, {}};
        }, [](A a) { menu_moby_corners(a.i[0]); return uint64_t{0}; });
    }

    g_ignored_words = saved_ignored;
    g_ee_null_is_ram = null_ram;
    g_extra_noop_hooks.clear();
}

}  // namespace rac1::oracle
