// Oracle batch 101 (task 0324): the world's static lighting, the three hand-written routines
// 0x295350 (tfrag), 0x297728 (tie), 0x288908 (shrub) on the level's real data (needs the ISO):
// the level started as a game does (its tfrag / tie / shrub entries, light sets and point lights
// loaded), then per case a list of entries, the light sets / point lights / entries' light fields
// varied. The original runs in the EE interpreter, the port on the same memory, every word of EE
// RAM and the scratchpad compared (the lit colours the routines write into the level's data).

#include "ovl_oracle.h"

#include <game/boot/boot_start.h>
#include <game/boot/front_end.h>
#include <game/boot/front_image.h>
#include <game/boot/title.h>
#include <game/lib/libmc.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/world_light_vu0.h>
#include <game/ovl/world_lighting.h>
#include <runtime/kernel/disc.h>

#include <chrono>

namespace rac1::oracle {

namespace {
// 989snd's IOP (batch 85's / 89's): replies a function of the command, the stream system's disc reads done at once
void iop_stand_in() {
    auto result = [](uint32_t id, uint32_t a0) -> uint32_t {
        const uint32_t h = (id * 0x9e3779b1u) ^ (a0 * 0x85ebca6bu);
        return ((h >> 7) % 4 == 0) ? 0u : ((h >> 3) | 1u);
    };
    sif_rpc_register(0x123456, [result](uint32_t fno, uint32_t send, uint32_t, uint32_t recv, uint32_t) {
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
}
}  // namespace

void batch_101() {
    if (g_only_batch != 101) return;
    g_batch = 101;
    world_light_vu0_register();  // (task 0363) the port's side runs the lighting set's VU0 programs as their port (the original's: the interpreter)
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
            w32(0x0013f2e8, 0xffffffffu);  // the sound listener's cached points stale, as at a game's first frame (batch 93's note)
            gameplay_frame();
            snap.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    int32_t ntfrag = 0, ntie = 0, nshrub = 0;  // (the entries' counts, read when a case's level is up)


    if (std::getenv("ORACLE_PROBE") != nullptr) {  // what each routine reads / writes on the level's own data (RE aid)
        level();
        ntfrag = ee_read<int32_t>(OV(0x00160f50));
        std::printf("batch 101: %d tfrag entries, %d tie instances, %d shrub instances\n", ntfrag, ee_read<int32_t>(OV(0x00160fcc)), ee_read<int32_t>(OV(0x00160490)));
        auto run_probe = [&](const char* name, uint32_t list, void (*fn)(uint32_t), uint32_t k) {
            level();
            w16(list, k);
            w16(list + 2, 0xffff);
            std::vector<uint8_t> before(ee_get_ram_base(), ee_get_ram_base() + 0x02000000);
            std::vector<uint8_t> spr0(ee_get_scratchpad_base(), ee_get_scratchpad_base() + 0x4000);
            fn(list);
            const uint8_t* now = ee_get_ram_base();
            std::printf("probe %s entry %u:", name, k);
            uint32_t a = 0;
            int ranges = 0;
            while (a < 0x02000000 && ranges < 12) {
                if (now[a] == before[a]) { ++a; continue; }
                uint32_t e = a, last = a;
                while (e < 0x02000000 && e - last < 0x20) { if (now[e] != before[e]) last = e; ++e; }
                std::printf(" [%06x..%06x)", a, last + 1);
                a = last + 1;
                ++ranges;
            }
            int nspr = 0;
            for (uint32_t i = 0; i < 0x4000; ++i) nspr += ee_get_scratchpad_base()[i] != spr0[i];
            std::printf("  (spr bytes changed %d)\n", nspr);
        };
        const uint32_t tb = ee_read<uint32_t>(OV(0x00160f4c)), ib = ee_read<uint32_t>(OV(0x00160fc0)), sb = ee_read<uint32_t>(OV(0x00160494));
        std::printf("tfrag table %06x tie bounds %06x data %06x shrub bounds %06x data %06x colours %06x\n", tb, ib, ee_read<uint32_t>(OV(0x00160fc8)), sb, ee_read<uint32_t>(OV(0x0016049c)), ee_read<uint32_t>(OV(0x001604a0)));
        int neg = 0, nz = 0, fl = 0;
        for (int i = 0; i < ntfrag; ++i) {
            const uint32_t e = tb + u(i) * 0x40;
            neg += ee_read<int8_t>(e + 0x34) < 0;
            nz += ee_read<uint8_t>(e + 0x29) == 0;
            fl += ee_read<uint8_t>(e + 0x35) != 0;
        }
        std::printf("tfrag entries: %d with +0x34 < 0, %d with +0x29 == 0, %d with +0x35 != 0\n", neg, nz, fl);
        for (int i = 0; i < 6; ++i) {
            const uint32_t e = tb + u(i) * 0x40;
            std::printf("  tfrag %d: +0x10 %08x +0x1e %04x +0x28 %02x +0x29 %02x +0x30 %04x +0x34 %02x +0x35 %02x +0x36 %04x +0x3c %02x\n", i, ee_read<uint32_t>(e + 0x10), ee_read<uint16_t>(e + 0x1e), ee_read<uint8_t>(e + 0x28), ee_read<uint8_t>(e + 0x29), ee_read<uint16_t>(e + 0x30), ee_read<uint8_t>(e + 0x34), ee_read<uint8_t>(e + 0x35), ee_read<uint16_t>(e + 0x36), ee_read<uint8_t>(e + 0x3c));
        }
        (void)ib; (void)sb;
        for (uint32_t k : {0u, 1u, 7u, 100u, 300u}) run_probe("tfrag", 0x70003000u, world_tfrag_lighting, k);
        for (uint32_t k : {0u, 1u, 50u, 500u, 1000u}) run_probe("tie", 0x70000000u, world_tie_lighting, k);
        for (uint32_t k : {0u, 1u, 50u, 500u, 1500u}) run_probe("shrub", 0x70000000u, world_shrub_lighting, k);
    }

    if (std::getenv("ORACLE_PROBE") != nullptr) {  // (RE aid) the entries' spheres
        const uint32_t tb2 = ee_read<uint32_t>(OV(0x00160f4c)), ib2 = ee_read<uint32_t>(OV(0x00160fc0)), sb2 = ee_read<uint32_t>(OV(0x00160494));
        for (int i = 0; i < 3; ++i) {
            const uint32_t e = tb2 + u(i) * 0x40;
            std::printf("  tfrag %d sphere %g %g %g %g\n", i, ee_read<float>(e), ee_read<float>(e + 4), ee_read<float>(e + 8), ee_read<float>(e + 12));
        }
        for (int i = 0; i < 3; ++i) {
            const uint32_t e = ib2 + u(i) * 0x20;
            std::printf("  tie %d bounds %g %g %g %g  +0x18 %04x +0x1a %02x +0x1b %02x +0x1c %04x +0x1e %04x\n", i, ee_read<float>(e), ee_read<float>(e + 4), ee_read<float>(e + 8), ee_read<float>(e + 12), ee_read<uint16_t>(e + 0x18), ee_read<uint8_t>(e + 0x1a), ee_read<uint8_t>(e + 0x1b), ee_read<uint16_t>(e + 0x1c), ee_read<uint16_t>(e + 0x1e));
        }
        for (int i = 0; i < 3; ++i) {
            const uint32_t e = sb2 + u(i) * 0x20;
            std::printf("  shrub %d bounds %g %g %g %g  +0x1b %02x +0x1c %04x +0x1e %04x\n", i, ee_read<float>(e), ee_read<float>(e + 4), ee_read<float>(e + 8), ee_read<float>(e + 12), ee_read<uint8_t>(e + 0x1b), ee_read<uint16_t>(e + 0x1c), ee_read<uint16_t>(e + 0x1e));
        }
        std::printf("  camera %g %g %g\n", ee_read<float>(OV(0x00166dc0)), ee_read<float>(OV(0x00166dc4)), ee_read<float>(OV(0x00166dc8)));
        std::printf("  lights: constants vf21 %g %g %g %g; sets 0..2 (0x40 each): ", ee_read<float>(OV(0x0015fb40)), ee_read<float>(OV(0x0015fb44)), ee_read<float>(OV(0x0015fb48)), ee_read<float>(OV(0x0015fb4c)));
        for (int i = 0; i < 16; ++i) std::printf("%g ", ee_read<float>(OV(0x0017fec0) + 4 * u(i)));
        std::printf("\n");
    }

    if (std::getenv("ORACLE_BENCH") != nullptr) {  // the port's cost on the level's own data (the whole level: what the load does; per entry: what a frame's visible list costs)
        level();
        auto bench = [&](const char* name, uint32_t list, int32_t n, void (*fn)(uint32_t)) {
            for (int32_t i = 0; i < n; ++i) w16(list + 2 * u(i), u(i));
            w16(list + 2 * u(n), 0xffff);
            const int reps = 20;
            const auto t0 = std::chrono::steady_clock::now();
            for (int r = 0; r < reps; ++r) fn(list);
            const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / reps;
            std::printf("bench %s: %d entries in %.3f ms (%.2f us per entry)\n", name, n, ms, ms * 1000.0 / n);
        };
        bench("tfrag", 0x70003000u, std::min<int32_t>(ee_read<int32_t>(OV(0x00160f50)), 0x3ff), world_tfrag_lighting);
        bench("tie", 0x70000000u, std::min<int32_t>(ee_read<int32_t>(OV(0x00160fcc)), 0x3ff), world_tie_lighting);
        bench("shrub", 0x70000000u, std::min<int32_t>(ee_read<int32_t>(OV(0x00160490)), 0x3ff), world_shrub_lighting);
    }

    // the light sets (12 x 0x40: colour 1 (w: the back-face factor), direction 1, colour 2, direction 2) and the point lights
    // (0x20 each: colour (w: the floor factor), then position + radius), the latter near (ax, ay, az) so that some reach it
    auto vary_lights = [](float ax, float ay, float az, float ar) {
        if (ri(0, 4) != 0) {
            for (uint32_t s = 0; s < 12; ++s) {
                const uint32_t a = OV(0x0017fec0) + s * 0x40;
                if (ri(0, 1) == 0) continue;
                wfrand(a, 3, 0, 1.2f); wf(a + 0xc, ri(0, 2) == 0 ? rf(-0.5f, 0.5f) : 0.0f);
                wfrand(a + 0x10, 3, -1, 1);
                wfrand(a + 0x20, 3, 0, 1.2f); wf(a + 0x2c, ri(0, 2) == 0 ? rf(-0.5f, 0.5f) : 0.0f);
                wfrand(a + 0x30, 3, -1, 1);
            }
        }
        if (ri(0, 4) != 0) {
            for (uint32_t p = 0; p < 8; ++p) {
                const uint32_t a = OV(0x001802c0) + p * 0x20;
                wfrand(a, 3, 0, 1.2f); wf(a + 0xc, rf(0, 1));
                wf(a + 0x10, ax + rf(-1, 1) * ar); wf(a + 0x14, ay + rf(-1, 1) * ar); wf(a + 0x18, az + rf(-1, 1) * ar);
                wf(a + 0x1c, ri(0, 5) == 0 ? rf(0, 2) : rf(0.3f, 2.5f) * ar + 0.5f);
            }
        }
    };
    // an entry's light field: nibbles of light-set indices (and, above, the blend)
    auto light_word = [] {
        const int k = ri(0, 5);
        if (k == 0) return u(ri(0, 11));
        if (k == 1) return u(ri(0, 11) | ri(0, 11) << 4 | ri(1, 255) << 8);
        if (k == 2) return R() & 0xffffu;
        return u(ri(0, 11) | ri(0, 11) << 4 | (ri(0, 2) == 0 ? ri(1, 255) << 8 : 0));
    };
    // a list of entries (halfword indices ended by 0xffff, as the callers build it): the entries it names are the ones varied
    auto list_at = [](uint32_t at, int32_t count, std::vector<uint32_t>& named) {
        const int n = ri(0, 7) == 0 ? ri(0, 1) : ri(1, 6);
        uint32_t p = at;
        named.clear();
        for (int i = 0; i < n && count > 0; ++i, p += 2) {
            const uint32_t k = ri(0, 3) == 0 ? (named.empty() ? u(ri(0, count - 1)) : named[0] + u(ri(0, 1))) % u(count) : u(ri(0, count - 1));
            w16(p, k);
            named.push_back(k);
        }
        w16(p, 0xffff);
    };
    // the point-light nibbles (0xf = none; the real data has none set): a few lights, sometimes the whole word random
    auto point_word = [] {
        const int k = ri(0, 4);
        if (k == 0) return 0xffffu;
        if (k == 1) return R() & 0xffffu;
        uint32_t w = 0xffffu;
        for (int i = ri(1, 4) - 1; i >= 0; --i) w = (w & ~(0xfu << (4 * i))) | u(ri(0, 8)) << (4 * i);
        return w;
    };

    // the checks of one image: Veldin's overlay (the ports as they are), or the boot module's own (the title's tfrag / tie
    // lighting 0x234f98 / 0x237370 are the same code as 0x295350 / 0x297728 with other addresses; ports relocated by OV())
    auto suite = [&](const char* tag, const std::function<void()>& level_fn, uint32_t a_tfrag, uint32_t a_tie, uint32_t a_shrub) {
        const std::string nm_tfrag = std::string("tfrag_lighting") + tag, nm_tie = std::string("tie_lighting") + tag, nm_shrub = std::string("shrub_lighting") + tag;
    // ---- the tfrag entries (0x40 bytes at 0x160f4c): the list at 0x70003000 as the level load builds it. The level's entries
    // are all "per vertex" (+0x34 = -1) lit by the grid's light sets (+0x36); the others (+0x34 >= 0: one light set) are varied in
    check(nm_tfrag.c_str(), a_tfrag, 160, Ret::None, [&] {
        level_fn();
        ntfrag = ee_read<int32_t>(OV(0x00160f50));
        std::vector<uint32_t> named;
        list_at(0x70003000u, ntfrag, named);
        const uint32_t base = ee_read<uint32_t>(OV(0x00160f4c));
        const uint32_t e0 = base + (named.empty() ? 0 : named[0]) * 0x40;  // (tfrag spheres are in 1/1024 world units)
        vary_lights(ee_read<float>(e0) / 1024, ee_read<float>(e0 + 4) / 1024, ee_read<float>(e0 + 8) / 1024, ee_read<float>(e0 + 12) / 1024);
        for (uint32_t k : named) {
            const uint32_t e = base + k * 0x40;
            if (ri(0, 2) == 0) w8(e + 0x34, u(ri(0, 11)));
            if (ri(0, 2) != 0) w16(e + 0x36, ri(0, 1) == 0 ? point_word() : light_word());
            if (ri(0, 3) == 0) w8(e + 0x35, u(ri(0, 1)));
        }
        return Args{{0x70003000u}, {}};
    }, [](A a) { world_tfrag_lighting(a.i[0]); return uint64_t{0}; });

    // ---- the tie instances (bounds 0x20 at 0x160fc0): the list at 0x70000000; +0x1b a flag the routine clears, +0x1c the light
    // sets (nibbles, blend above), +0x1e the point lights' nibbles
    check(nm_tie.c_str(), a_tie, 160, Ret::None, [&] {
        level_fn();
        ntie = ee_read<int32_t>(OV(0x00160fcc));
        std::vector<uint32_t> named;
        list_at(0x70000000u, ntie, named);
        const uint32_t base = ee_read<uint32_t>(OV(0x00160fc0));
        const uint32_t e0 = base + (named.empty() ? 0 : named[0]) * 0x20;
        vary_lights(ee_read<float>(e0), ee_read<float>(e0 + 4), ee_read<float>(e0 + 8), 3.0f);
        for (uint32_t k : named) {
            const uint32_t e = base + k * 0x20;
            if (ri(0, 1) == 0) w16(e + 0x1c, light_word());
            if (ri(0, 1) == 0) w16(e + 0x1e, point_word());
            if (ri(0, 2) == 0) w8(e + 0x1b, u(ri(0, 1)));
            if (ri(0, 3) == 0) wfrand(e, 3, -300, 300);  // (the instance's position: the point lights' distance)
        }
        return Args{{0x70000000u}, {}};
    }, [](A a) { world_tie_lighting(a.i[0]); return uint64_t{0}; });

    // ---- the shrub instances (bounds 0x20 at 0x160494): the list at 0x70000000, the same fields
    if (a_shrub == 0) skip_check(nm_shrub.c_str(), 0x0022a5e0, "the boot module's shrub lighting is a variant of 0x288908, not ported");
    else check(nm_shrub.c_str(), a_shrub, 160, Ret::None, [&] {
        level_fn();
        nshrub = ee_read<int32_t>(OV(0x00160490));
        std::vector<uint32_t> named;
        list_at(0x70000000u, nshrub, named);
        const uint32_t base = ee_read<uint32_t>(OV(0x00160494));
        const uint32_t e0 = base + (named.empty() ? 0 : named[0]) * 0x20;
        vary_lights(ee_read<float>(e0), ee_read<float>(e0 + 4), ee_read<float>(e0 + 8), 3.0f);
        for (uint32_t k : named) {
            const uint32_t e = base + k * 0x20;
            if (ri(0, 1) == 0) w16(e + 0x1c, light_word());
            if (ri(0, 1) == 0) w16(e + 0x1e, point_word());
            if (ri(0, 2) == 0) w8(e + 0x1b, u(ri(0, 1)));
            if (ri(0, 3) == 0) wfrand(e, 3, -300, 300);
        }
        return Args{{0x70000000u}, {}};
    }, [](A a) { world_shrub_lighting(a.i[0]); return uint64_t{0}; });
    };
    suite("", level, 0x00295350, 0x00297728, 0x00288908);

    // ---- the boot module's image: the title loaded and running (batch 89's states)
    if (g_oracle_level != 0) {
        skip_check("tfrag_lighting_front", 0x00234f98, "the boot module is not a level");
    } else {
        static std::vector<uint8_t> fsnap;
        auto front_level = [] {
            uint8_t* ram = ee_get_ram_base();
            if (fsnap.empty()) {
                iop_stand_in();
                front_image_load();
                ovl_reloc_set_level(OVL_FRONT);
                mc_reset();
                boot_start();
                w16(0x001518d0, 512);
                w16(0x001518d2, 416);
                w32(0x0015ec8c, 1);
                w32(0x00137b00, 0);
                const uint32_t bank = 0x100000;  // the title's sound tables (batch 89's)
                ee_write<uint32_t>(0x001861e0 + 0x9c, bank);
                ee_write<uint32_t>(0x00186100 + 0xdc, bank);
                ee_write<uint32_t>(0x0015f634, 0x00186100);
                ee_write<uint32_t>(0x0015f630, 7);
                for (uint32_t o : {0x1cu, 0x3cu, 0x5cu, 0x7cu, 0x9cu, 0xbcu}) ee_write<uint32_t>(0x00186100 + o, bank);
                for (uint32_t o : {0x1cu, 0x3cu, 0x5cu, 0x7cu}) ee_write<uint32_t>(0x001861e0 + o, bank);
                title_loop_start();
                title_loop_ready();
                for (int i = 0; i < 70; ++i) title_loop_frame();
                fsnap.assign(ram, ram + 0x02000000);
            } else {
                ovl_reloc_set_level(OVL_FRONT);
                std::memcpy(ram, fsnap.data(), fsnap.size());
            }
        };
        front_level();
        std::printf("batch 101 (boot module): %d tfrag entries, %d tie instances, %d shrub instances\n", ee_read<int32_t>(OV(0x00160f50)),
                    ee_read<int32_t>(OV(0x00160fcc)), ee_read<int32_t>(OV(0x00160490)));
        g_oracle_front = true;
        suite("_front", front_level, 0x00234f98, 0x00237370, 0x0022a5e0);  // (0x22a5e0: 0x288908's code, task 0365)
        g_oracle_front = false;
        ovl_reloc_set_level(0);
    }

    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
