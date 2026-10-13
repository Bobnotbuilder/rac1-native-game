// Oracle batch 100 (task 0323): the nanotech crate's orbs' draw callback 0x2d9810 with the
// two helpers only it calls, the textured primitive packet builder 0x1fde98 and the VU0
// remainder 0x200260, on Veldin's real level data (needs the ISO; the tables 0x1e14c0 ..
// 0x1e3da0 are the overlay's own). The original runs in the EE interpreter, the port on the
// same memory, every word compared.

#include "ovl_oracle.h"

#include <game/ovl/nanotech_draw.h>
#include <game/ovl/ovl_reloc.h>
#include <runtime/kernel/disc.h>

#include <cmath>

namespace rac1::oracle {

void batch_100() {
    if (g_only_batch != 100) return;
    g_batch = 100;
    g_ignored_words = vcross_lanes();
    g_ignored_words.push_back(OV(0x00160188));  // the particle update's saved return address
    g_ignored_words.push_back(OV(0x0016018c));
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;
    // the callees left TODO_PORT: the kernel's vsync wait
    g_extra_noop_hooks = {0x00122298};
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
    static std::vector<uint8_t> snap;
    static std::vector<uint32_t> mobys;  // the level's mobys
    auto level = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty()) {
            veldin_boot();
            w16(0x001518d0, 512);
            w16(0x001518d2, 416);
            particle_table_init();
            level_setup_body(1, 0);
            hud_init();
            w32(OV(0x0016c060), 0xf);
            w32(0x0015ec8c, 1);
            w32(0x00137b00, 0);
            sub_00264bf8();
            w32(0x0013f2e8, 0xffffffffu);  // the sound listener's cached points stale, as at a game's first frame (batch 93's note)
            gameplay_frame();
            screen_setup();
            projection_setup();
            render_view_setup();
            snap.assign(ram, ram + 0x02000000);
            for (uint32_t m = ee_read<uint32_t>(OV(0x0015ffd8)); ee_read<uint8_t>(m + 0x20) != 0xff; m += 0x100) {
                const uint32_t fn = ee_read<uint32_t>(m + 0x74);
                if (fn != 0 && !ovl_moby_fn_ported(fn)) g_extra_noop_hooks.push_back(fn);
                mobys.push_back(m);
            }
            std::printf("batch 100: %zu mobys\n", mobys.size());
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    level();
    auto pick = [] { return mobys[static_cast<size_t>(ri(0, static_cast<int32_t>(mobys.size()) - 1))]; };

    // ---- the VU0 remainder: whole, fractional, negative, zero and huge operands
    check("float_fmod_vu", 0x00200260, 400, Ret::Float, [] {
        const int k = ri(0, 9);
        float a = rf(-20, 20), b = rf(0.1f, 12);
        if (k == 0) a = rf(-1, 1);
        if (k == 1) b = -b;
        if (k == 2) b = 8.0f;
        if (k == 3) a = static_cast<float>(ri(-40, 40)) * b;
        if (k == 4) b = coin() ? 0.0f : -0.0f;
        if (k == 5) a = coin() ? 0.0f : rf(-1e-30f, 1e-30f);
        if (k == 6) a = rf(-1e12f, 1e12f);
        if (k == 7) b = rf(-1e-20f, 1e-20f);
        return Args{{}, {a, b}};
    }, [](const Args& a) { return fbits(float_fmod_vu(a.f[0], a.f[1])); });

    // ---- the primitive packet builder: n vertices from three lists at the packet cursor
    check("prim_list_packet", 0x001fde98, 80, Ret::None, [level] {
        level();
        const int32_t n = ri(0, 6) == 0 ? ri(1, 3) : ri(1, 0x96);
        wrand(T, n * 3);
        wrand(T2, n);
        wrand(T3, n * 2);
        w32(OV(0x001611c0), 0x01e10000u + 0x10 * u(ri(0, 0x100)));
        return Args{{u(n), T, T2, T3, R()}, {}};
    }, [](A a) { prim_list_packet(static_cast<int32_t>(a.i[0]), a.i[1], a.i[2], a.i[3], a.i[4]); return uint64_t{0}; });

    // ---- the callback: a moby near the camera with a fake pvars block (the glow's scale, the crate's matrix), in state 1
    // (the second part: four quads of wrapped coordinates) or not
    auto orb = [level, pick] {
        level();
        const uint32_t m = pick();
        const uint32_t c = pick();
        const uint32_t pv = T4;
        const float cx = ee_read<float>(OV(0x00166dc0)), cy = ee_read<float>(OV(0x00166dc4)), cz = ee_read<float>(OV(0x00166dc8));
        const float d = ri(0, 4) == 0 ? rf(-10, 1) : rf(1, 40), ox = rf(-0.5f, 0.5f) * d, oy = rf(-0.4f, 0.4f) * d;
        wf(m + 0x10, cx + ee_read<float>(OV(0x00166c88)) * d + ee_read<float>(OV(0x00166c80)) * ox + ee_read<float>(OV(0x00166c84)) * oy);
        wf(m + 0x14, cy + ee_read<float>(OV(0x00166c88) + 0x10) * d + ee_read<float>(OV(0x00166c80) + 0x10) * ox + ee_read<float>(OV(0x00166c84) + 0x10) * oy);
        wf(m + 0x18, cz + ee_read<float>(OV(0x00166c88) + 0x20) * d + ee_read<float>(OV(0x00166c80) + 0x20) * ox + ee_read<float>(OV(0x00166c84) + 0x20) * oy);
        w32(m + 0x78, pv);
        wf(pv, ri(0, 5) == 0 ? rf(-1, 3) : rf(0, 1.5f));
        w32(pv + 0xc, c);
        w8(m + 0x20, ri(0, 2) == 0 ? 1u : u(ri(0, 3)));
        const float r[12] = {rf(-1, 1), rf(-1, 1), rf(-1, 1), 0, rf(-1, 1), rf(-1, 1), rf(-1, 1), 0, rf(-1, 1), rf(-1, 1), rf(-1, 1), 0};
        for (uint32_t i = 0; i < 12; ++i) wf(c + 0xc0 + 4 * i, r[i]);
        if (ri(0, 4) == 0) w32(OV(0x00161b10), R());
        if (coin()) { w32(OV(0x0016d180) + 0xb * 0x10, 0); w32(OV(0x0016d184) + 0xb * 0x10, 0); }  // its texture not set up yet
        w32(OV(0x001611c0), 0x01e10000u + 0x10 * u(ri(0, 0x100)));
        return m;
    };
    check("nanotech_orbs_draw", 0x002d9810, 60, Ret::None, [orb] { return Args{{orb()}, {}}; },
          [](A a) { nanotech_orbs_draw(a.i[0]); return uint64_t{0}; });
    check("nanotech_orbs_draw_state1", 0x002d9810, 40, Ret::None, [orb] {
        const uint32_t m = orb();
        w8(m + 0x20, 1);
        return Args{{m}, {}};
    }, [](A a) { nanotech_orbs_draw(a.i[0]); return uint64_t{0}; });
    g_extra_noop_hooks.clear();
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
