// Oracle batch 102 (task 0326): the EE side of the world's render passes on the level's real data (needs the ISO): the tfrag / tie / shrub
// passes 0x294240 / 0x295c80 / 0x286e60 (the culling and chain building, their packet parts) and the end routines 0x2941a0 / 0x295b38 /
// 0x286d58, and 0x24f6e0. The level started as a game does, then per case the camera (position and rotation, the view setup run), the zone
// mask, the entries' light fields and the packet cursor varied. The original runs in the EE interpreter, the port on the same memory,
// every word of EE RAM and the scratchpad compared (the frame packet's chain, the lists, the lit data patched by the end routines).

#include "ovl_oracle.h"

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/world_cull.h>
#include <game/ovl/world_lighting.h>

#include <chrono>

namespace rac1::oracle {

void batch_102() {
    if (g_only_batch != 102) return;
    g_batch = 102;
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;
    static std::vector<uint8_t> snap;
    auto level = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty()) {
            veldin_boot();
            w16(0x001518d0, 512);
            w16(0x001518d2, 416);
            particle_table_init();
            level_setup_body(1, 0);
            w32(OV(0x0016c060), 0xf);
            w32(0x0013f2e8, 0xffffffffu);
            gameplay_frame();
            snap.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    // the frame's state a render pass starts from (the oracle's level has none: the host sets it per frame): all render enables on, the packet
    // cursor in a free area, the zone mask, the camera (position and rotation rows) and the view matrices from it (render_view_setup)
    auto frame_state = [&] {
        level();
        screen_setup();  // (the host's per-run setup: the projection constants the view setup multiplies in)
        fog_setup();
        render_enables_init();
        w32(OV(0x001611c0), 0x01e20000u + 16u * u(ri(0, 64)));
        if (ri(0, 3) == 0) w32(OV(0x0015ee78), u(ri(0, 0x1800)) * 0x100u);
        const int zk = ri(0, 3);  // the zone mask (0x80 bytes: one per zone cell; bit set = visible)
        for (uint32_t o = 0; o < 0x80; ++o) w8(OV(0x00173d00) + o, zk == 0 ? R() : zk == 1 ? (ri(0, 3) == 0 ? R() : 0xffu) : 0xffu);
        // the camera: near the centre of a random tfrag entry (spheres in 1/1024 units), looking toward a random entry (with some noise)
        const uint32_t tb = ee_read<uint32_t>(OV(0x00160f4c));
        const int32_t nt = ee_read<int32_t>(OV(0x00160f50));
        if (ri(0, 7) != 0 && nt > 0) {
            auto center = [&](int32_t k, float* c) { for (int i = 0; i < 3; ++i) c[i] = ee_read<float>(tb + u(k) * 0x40 + 4 * u(i)) / 1024.0f; };
            float a[3], b[3];
            center(ri(0, nt - 1), a);
            center(ri(0, nt - 1), b);
            const float dist = ri(0, 3) == 0 ? rf(0, 300) : rf(0, 60);
            float p[3], f[3];
            for (int i = 0; i < 3; ++i) p[i] = a[i] + rf(-1, 1) * dist;
            for (int i = 0; i < 3; ++i) f[i] = (ri(0, 4) == 0 ? rf(-1, 1) : b[i] - p[i] + rf(-1, 1) * 20.0f);
            auto norm = [](float* v) { const float l = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); for (int i = 0; i < 3; ++i) v[i] = l > 1e-6f ? v[i] / l : (i == 0 ? 1.0f : 0.0f); };
            norm(f);
            float up[3] = {rf(-0.3f, 0.3f), rf(-0.3f, 0.3f), 1.0f}, l[3] = {f[1] * up[2] - f[2] * up[1], f[2] * up[0] - f[0] * up[2], f[0] * up[1] - f[1] * up[0]};
            norm(l);
            const float u2[3] = {l[1] * f[2] - l[2] * f[1], l[2] * f[0] - l[0] * f[2], l[0] * f[1] - l[1] * f[0]};
            for (int i = 0; i < 3; ++i) {
                wf(OV(0x00166dc0) + 4 * u(i), p[i]);
                wf(OV(0x00166fd0) + 4 * u(i), f[i]);
                wf(OV(0x00166fe0) + 4 * u(i), l[i]);
                wf(OV(0x00166ff0) + 4 * u(i), u2[i]);
            }
        }
        render_view_setup();
    };
    // some entries' lighting fields varied (what the lists for the lighting pick: the dirty flag, the point lights' nibbles)
    auto vary_entries = [&](uint32_t table, int32_t count, uint32_t stride, uint32_t dirty, uint32_t points) {
        if (count <= 0) return;
        const int n = ri(0, 3) == 0 ? 0 : ri(1, 40);
        for (int i = 0; i < n; ++i) {
            const uint32_t e = table + u(ri(0, count - 1)) * stride;
            if (ri(0, 1) == 0) w8(e + dirty, u(ri(0, 1)));
            else w16(e + points, ri(0, 2) == 0 ? (R() & 0xffffu) : (0xfff0u | u(ri(0, 8))));
        }
    };

    if (std::getenv("ORACLE_PROBE") != nullptr) {  // (RE aid) the projection constants the view setup reads
        level();
        for (uint32_t o = 0; o < 0x110; o += 16) std::printf("0x%x: %g %g %g %g\n", OV(0x0016cb00) + o, ee_read<float>(OV(0x0016cb00) + o), ee_read<float>(OV(0x0016cb00) + o + 4), ee_read<float>(OV(0x0016cb00) + o + 8), ee_read<float>(OV(0x0016cb00) + o + 12));
    }
    // (RE aid / sanity: ORACLE_STATS=1) what a pass left: the chain's size, the lighting list's length
    auto stats = [](const char* name, uint32_t before, uint32_t list) {
        if (std::getenv("ORACLE_STATS") == nullptr) return;
        int n = 0;
        while (n < 0x200 && ee_read<uint16_t>(list + 2 * u(n)) != 0xffffu) ++n;
        std::printf("stats %s: chain %u bytes, lighting list %d entries\n", name, ee_read<uint32_t>(OV(0x001611c0)) - before, n);
    };
    if (std::getenv("ORACLE_BENCH") != nullptr) {  // the port's cost per frame on the level's own camera (the passes + the lighting of what they list)
        frame_state();
        auto bench = [&](const char* name, void (*pass)(), void (*light)(uint32_t), uint32_t list, void (*end)()) {
            const int reps = 100;
            const uint32_t cur = ee_read<uint32_t>(OV(0x001611c0));
            double ms_pass = 0, ms_light = 0;
            for (int r = 0; r < reps; ++r) {
                ee_write<uint32_t>(OV(0x001611c0), cur);
                auto t0 = std::chrono::steady_clock::now();
                pass();
                auto t1 = std::chrono::steady_clock::now();
                light(list);
                end();
                auto t2 = std::chrono::steady_clock::now();
                ms_pass += std::chrono::duration<double, std::milli>(t1 - t0).count();
                ms_light += std::chrono::duration<double, std::milli>(t2 - t1).count();
            }
            std::printf("bench %s: pass %.3f ms, lighting + end %.3f ms per frame (chain %u bytes)\n", name, ms_pass / reps, ms_light / reps, ee_read<uint32_t>(OV(0x001611c0)) - cur);
        };
        bench("tfrag", world_tfrag_pass, world_tfrag_lighting, OV(0x001c5400), world_tfrag_end);
        bench("tie", world_tie_pass, world_tie_lighting, OV(0x001c7300), world_tie_end);
        bench("shrub", world_shrub_pass, world_shrub_lighting, OV(0x001bcfb0), world_shrub_end);
        std::printf("visible: %zu tfrag, %zu tie, %zu shrub\n", world_visible().tfrag.size(), world_visible().tie.size(), world_visible().shrub.size());
    }
    check("world_tfrag_pass", 0x00294240, 60, Ret::None, [&] {
        frame_state();
        vary_entries(ee_read<uint32_t>(OV(0x00160f4c)), ee_read<int32_t>(OV(0x00160f50)), 0x40, 0x35, 0x36);
        return Args{{}, {}};
    }, [&](A) {
        const uint32_t cur0 = ee_read<uint32_t>(OV(0x001611c0));
        world_tfrag_pass();
        stats("tfrag", cur0, OV(0x001c5400));
        if (std::getenv("ORACLE_PKT") != nullptr) {  // (RE aid) the chain the port built, words of the first quadwords
            static int shown = 0;
            if (shown++ < 1) {
                const uint32_t c = 0x01e20000u;
                for (uint32_t o = 0; o < 0x440; o += 16) std::printf("pkt %04x: %08x %08x %08x %08x\n", o, ee_read<uint32_t>(c + o), ee_read<uint32_t>(c + o + 4), ee_read<uint32_t>(c + o + 8), ee_read<uint32_t>(c + o + 12));
            }
        }
        return uint64_t{0};
    });
    check("world_tie_pass", 0x00295c80, 60, Ret::None, [&] {
        frame_state();
        vary_entries(ee_read<uint32_t>(OV(0x00160fc0)), ee_read<int32_t>(OV(0x00160fcc)), 0x20, 0x1b, 0x1e);
        return Args{{}, {}};
    }, [&](A) {
        const uint32_t cur0 = ee_read<uint32_t>(OV(0x001611c0));
        world_tie_pass();
        stats("tie", cur0, OV(0x001c7300));
        return uint64_t{0};
    });
    check("world_shrub_pass", 0x00286e60, 60, Ret::None, [&] {
        frame_state();
        vary_entries(ee_read<uint32_t>(OV(0x00160494)), ee_read<int32_t>(OV(0x00160490)), 0x20, 0x1b, 0x1e);
        return Args{{}, {}};
    }, [&](A) {
        const uint32_t cur0 = ee_read<uint32_t>(OV(0x001611c0));
        world_shrub_pass();
        stats("shrub", cur0, OV(0x001bcfb0));
        return uint64_t{0};
    });

    // the end routines run on the state the passes leave (made by the port: proved above)
    check("world_tfrag_end", 0x002941a0, 40, Ret::None, [&] {
        frame_state();
        world_tfrag_pass();
        return Args{{}, {}};
    }, [](A) { world_tfrag_end(); return uint64_t{0}; });
    check("world_tie_end", 0x00295b38, 40, Ret::None, [&] {
        frame_state();
        world_tie_pass();
        return Args{{}, {}};
    }, [](A) { world_tie_end(); return uint64_t{0}; });
    check("world_shrub_end", 0x00286d58, 40, Ret::None, [&] {
        frame_state();
        world_shrub_pass();
        return Args{{}, {}};
    }, [](A) { world_shrub_end(); return uint64_t{0}; });

    // the moby animation lerp slots: some slots hold mobys (the level's first), some flagged dead, some with another byte +0x52
    check("moby_anim_lerp_slots_validate", 0x0024f6e0, 40, Ret::None, [&] {
        level();
        for (uint32_t i = 0; i < 16; ++i) {
            const uint32_t m = M + 0x40 * i;
            w32(OV(0x00196cc0) + 4 * i, ri(0, 3) == 0 ? 0u : m);
            w8(m + 0x20, R());
            w8(m + 0x52, ri(0, 1) == 0 ? 0xffu : R());
        }
        return Args{{}, {}};
    }, [](A) { moby_anim_lerp_slots_validate(); return uint64_t{0}; });

    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
