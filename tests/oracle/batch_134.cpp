// Oracle batch 134 (task 0380): the item (ammo) pickups' update 0x2c53c0 (src/game/ovl/ammo_pickup.cpp) and the bounding-sphere
// gap 0x250430 it measures the hero with, on the level after its first gameplay frame (the port's). The pickups are spawned
// as a crate spawns them (pickup_spawn 0x2c52a8 at a crate, at the hero or in the air, the crates' throw), then run some frames
// by the port before the compared call, so the falls, the landings, the takes, the flights to the hero and the pulses all
// start from states the game reaches; their pvars, the hero's place / health / reach, the weapons' ammo and the frame
// counters are varied. Run with ORACLE_LEVEL=1 / 4 too (the levels' own copies of the update).

#include "ovl_oracle.h"

#include <game/ovl/ammo_pickup.h>
#include <game/ovl/ovl_reloc.h>

namespace rac1::oracle {

void batch_134() {
    if (g_only_batch != 134) return;
    g_batch = 134;
    const std::vector<uint32_t> saved_ignored = g_ignored_words;
    g_ignored_words = vcross_lanes();
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;  // as in the runtime
    static std::vector<uint8_t> snap;
    static int snap_level = -1;
    auto level1 = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty() || snap_level != g_oracle_level) {
            veldin_boot();
            level_setup_body(1, 0);
            w32(OV(0x0016c060), 0xf);
            gameplay_frame();
            snap.assign(ram, ram + 0x02000000);
            snap_level = g_oracle_level;
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    auto mobys_with = [](uint32_t fn) {
        std::vector<uint32_t> v;
        for (uint32_t m = ee_read<uint32_t>(OV(0x0015ffd8)); m < ee_read<uint32_t>(OV(0x0015ffe0)); m += 0x100)
            if (ee_read<uint8_t>(m + 0x20) < 0xfe && ee_read<uint32_t>(m + 0x74) == fn) v.push_back(m);
        return v;
    };
    auto anymoby = [] {
        for (int k = 0; k < 100; ++k) {
            const uint32_t m = ee_read<uint32_t>(OV(0x0015ffd8)) + 0x100 * u(ri(0, 150));
            if (ee_read<uint8_t>(m + 0x20) < 0xfe) return m;
        }
        return ee_read<uint32_t>(OV(0x0015ffd8));
    };

    check("moby_sphere_gap", 0x00250430, 200, Ret::Float, [level1, anymoby] {
        level1();
        const uint32_t a = anymoby(), b = coin() ? ee_read<uint32_t>(0x001413d0) : anymoby();
        if (coin()) { wfrand(a, 3, -5, 5); wfrand(b, 3, -5, 5); }
        if (coin()) { wf(a + 0xc, rf(0, 8000)); wf(b + 0xc, rf(0, 8000)); }
        return Args{{a, b}, {}};
    }, [](A a) { return rf32(moby_sphere_gap(a.i[0], a.i[1])); });

    // the weapons with an ammo maximum (0x1c40b0[item].+0xe), the items a crate gives
    std::vector<uint32_t> ammo_items;
    level1();
    for (uint32_t it = 0; it < 0x24; ++it)
        if (ee_read<uint16_t>(OV(0x001c40b0) + it * 0x18 + 0xe) != 0 && ee_read<int16_t>(OV(0x00179ac0) + it * 0x4c + 0x3a) > 0) ammo_items.push_back(it);
    std::printf("batch 134: %zu ammo items with a pickup class\n", ammo_items.size());
    if (ammo_items.empty()) {
        skip_check("ammo_pickup_update", 0x002c53c0, "no ammo item with a pickup class");
    } else {
        for (int pass = 0; pass < 3; ++pass) {
            const char* name = pass == 0 ? "ammo_pickup_update" : pass == 1 ? "ammo_pickup_update_run" : "ammo_pickup_update_hero";
            check(name, 0x002c53c0, 300, Ret::None,
                  [level1, mobys_with, ammo_items, pass] {
                level1();
                const uint32_t hero = ee_read<uint32_t>(0x001413d0);
                // where: at a crate (on the ground), at the hero, or in the air
                const std::vector<uint32_t> crates = mobys_with(OVF(0x002cf218u));
                Vec4 at = ee_ref<Vec4>(0x0013f3d0);
                const int where = pass == 2 ? 1 : ri(0, 2);
                if (where == 0 && !crates.empty()) at = ee_ref<Vec4>(crates[static_cast<size_t>(ri(0, static_cast<int>(crates.size()) - 1))] + 0x10);
                at.x += rf(-1.5f, 1.5f); at.y += rf(-1.5f, 1.5f); at.z += where == 2 ? rf(1, 6) : rf(-0.2f, 1.0f);
                ee_ref<Vec4>(OUT) = at;
                uint32_t item = 0, m = 0;
                for (int t = 0; t < 20 && m == 0; ++t) {
                    item = ammo_items[static_cast<size_t>(ri(0, static_cast<int>(ammo_items.size()) - 1))];
                    m = pickup_spawn(OUT, item, coin() ? -1 : ri(1, 40), 0);
                }
                const uint32_t p = ee_read<uint32_t>(m + 0x78);
                // the crate's throw (crate_break_drops 0x2d0538) or none
                if (ri(0, 2) != 0) {
                    const float a = rf(-3.14159f, 3.14159f), s = rf(0, 0.03f);
                    wf(p + 0x10, std::cos(a) * s); wf(p + 0x14, std::sin(a) * s); wf(p + 0x18, rf(0.02f, 0.1f));  // (the game's throw: ~0.08 up a frame)
                }
                // the weapon's ammo: empty, short, full
                const uint32_t mx = ee_read<uint16_t>(OV(0x001c40b0) + item * 0x18 + 0xe);
                const int k = ri(0, 3);
                w32(0x0013d428 + item * 4, k == 0 ? 0u : k == 1 ? u(ri(0, static_cast<int>(mx))) : mx);
                if (ri(0, 4) == 0) w32(0x001415f8, 0);  // the hero's health
                w32(OV(0x0015f5cc), u(ri(0, 100000)));
                w32(OV(0x00161838), u(ri(0, 4) == 0 ? ri(0, 100000) : static_cast<int>(ee_read<uint32_t>(OV(0x0015f5cc))) - ri(0, 20)));
                if (pass == 2) {  // the hero walked onto it: his data and moby where the pickup is
                    Vec4 h = at;
                    const float r = coin() ? 1.0f : 4.0f;  // (sometimes out of his xy reach)
                    h.x += rf(-r, r); h.y += rf(-r, r); h.z += rf(-1.5f, 1.5f);
                    if (coin()) w32(p, 1);  // one round: the HUD's singular text
                    if (ri(0, 3) == 0) w32(0x0013d428 + item * 4, ee_read<uint16_t>(OV(0x001c40b0) + item * 0x18 + 0xe) - 1u);
                    if (ri(0, 3) == 0) wf(p + 0x28, at.z + (coin() ? rf(2, 5) : rf(1.2f, 2.4f)));  // it fell (far) below where it started
                    if (ri(0, 3) == 0) w32(0x001415f8, 0);  // the hero dead
                    ee_ref<Vec4>(0x0013f3d0) = h;
                    ee_ref<Vec4>(hero + 0x10) = h;
                    Vec4 bs = h; bs.w = ee_read<float>(hero + 0xc);
                    if (coin()) ee_ref<Vec4>(hero) = bs;
                }
                // its frames so far (the port's), then a state of its own sometimes
                const int frames = pass == 0 ? ri(0, 3) : ri(0, 120);
                for (int f = 0; f < frames && ee_read<uint8_t>(m + 0x20) < 0xfe; ++f) {
                    ammo_pickup_update(m);
                    w32(OV(0x0015f5cc), ee_read<uint32_t>(OV(0x0015f5cc)) + 1);
                }
                if (ee_read<uint8_t>(m + 0x20) >= 0xfe) return Args{{m}, {}};
                if (pass == 2 && coin()) {  // where it lies now: the hero's reach (hero_update.cpp sets 2.125 / 1.25 or 15 / 3),
                    const uint32_t P = 0x0013f350;  // the hero near one of its edges, the start height near the 2-below test
                    const float rxy = coin() ? 2.125f : rf(0.5f, 4.0f), rz = coin() ? 1.25f : rf(0.3f, 2.0f);
                    wf(P + 0x2288, rxy); wf(P + 0x228c, rz);
                    Vec4 h = ee_ref<Vec4>(m + 0x10);
                    const int edge = ri(0, 2);
                    const float a = rf(-3.14159f, 3.14159f), d = edge == 1 ? rxy + rf(-0.5f, 0.5f) : rf(0, edge == 0 ? rxy : 3.0f);
                    h.x += std::cos(a) * d; h.y += std::sin(a) * d;
                    h.z += edge == 0 ? (coin() ? 1.0f : -1.0f) * (rz + rf(-0.5f, 0.5f)) : rf(-1.6f, 1.6f);
                    ee_ref<Vec4>(0x0013f3d0) = h;
                    if (ri(0, 3) == 0) wf(p + 0x28, ee_read<float>(m + 0x18) + rf(1.3f, 2.3f));
                    if (ri(0, 3) == 0) w32(0x0013d428 + item * 4, ee_read<uint16_t>(OV(0x001c40b0) + item * 0x18 + 0xe));
                    else w32(0x0013d428 + item * 4, 0);
                    if (ri(0, 4) != 0) w32(0x001415f8, 4);
                }
                if (pass == 0) {
                    if (ri(0, 2) == 0) w8(m + 0x20, u(ri(0, 6)));
                    if (coin()) w32(p + 0x20, u(ri(-2, 3)));
                    if (coin()) wf(p + 0x24, rf(-0.5f, 0.5f));
                    if (coin()) w8(m + 0x23, u(ri(0, 255)));
                    if (ri(0, 4) == 0) wf(m + 0x2c, coin() ? 0.0f : rf(0, 2));
                    if (coin()) wf(p + 0x28, at.z + rf(-1, 4));
                    if (ri(0, 3) == 0) wfrand(p + 0x10, 3, -0.2f, 0.2f);
                    if (coin()) { Vec4 h = ee_ref<Vec4>(m + 0x10); h.x += rf(-3, 3); h.y += rf(-3, 3); h.z += rf(-2, 2); ee_ref<Vec4>(0x0013f3d0) = h; ee_ref<Vec4>(0x0013f420) = h; }
                }
                return Args{{m}, {}};
            }, [](A a) { ammo_pickup_update(a.i[0]); return uint64_t{0}; });
        }
    }
    g_ee_null_is_ram = null_ram;
    g_ignored_words = saved_ignored;
}

}  // namespace rac1::oracle
