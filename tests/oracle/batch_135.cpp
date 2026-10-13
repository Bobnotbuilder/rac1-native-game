// Oracle batch 135 (task 0379): the underwater oxygen indicator, the HUD element hero_water_air requests in slot 4 with the key
// 0x753f: its start 0x236268, update 0x236458 and draw 0x2365a8 (src/game/ovl/hud_o2.cpp), and vec3_mul 0x1ff348. The level's HUD
// data loaded as batch 67's (needs the ISO); the element's fields, the frame counter and the bubble table varied. Run with
// ORACLE_LEVEL=1 too (Novalis's own original against the relocated port).

#include "ovl_oracle.h"

#include <game/math/vecmath.h>
#include <game/ovl/hud_o2.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>

#include <cstring>

namespace rac1::oracle {

void batch_135() {
    if (g_only_batch != 135) return;
    auto boot = [] { veldin_boot(); };
    g_batch = 135;
    g_ignored_words = vcross_lanes();
    g_extra_noop_hooks.clear();
    static std::vector<uint8_t> snap;
    auto level = [boot] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty()) {
            boot();
            level_setup_body(1, 0);
            snap.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
        w32(OV(0x001611b8), ee_read<uint32_t>(OV(0x00173e08)));
        w32(OV(0x001611bc), ee_read<uint32_t>(OV(0x00173e08)) + 0x30000);
        w32(OV(0x001611d0), 0);
        w32(OV(0x001611c0), ee_read<uint32_t>(OV(0x001611b8)) + u(ri(0, 0x100)) * 0x10);
        w32(OV(0x0015f418), u(ri(0, 3) == 0 ? ri(0x3c, 0x40) : ri(0, 0x20)));
        w32(0x0015ee74, ee_read<uint32_t>(0x0015ee78) + u(ri(0, 0x40)) * 0x100);
    };
    auto rnd = [](float lo, float hi) { return lo + (hi - lo) * static_cast<float>(ri(0, 100000)) / 100000.0f; };
    // the oxygen element in slot 4 as hero_water_air requests it (key 0x753f, the air P+0x22a0 of 10000)
    auto element = [level, rnd](bool bubbles) {
        level();
        const uint32_t TAB = OV(0x0017e660);
        for (uint32_t i = 0; i < 13; ++i) {
            const uint32_t s = OV(0x0017dc50) + i * 0x90;
            w32(s + 0x64, coin() ? u(ri(0, 20)) : 0xffffffffu);
            w32(s + 0x7c, u(ri(-2, 12)));
            w32(s + 0x6c, u(ri(-7, 0x1f)));
        }
        const uint32_t e = OV(0x0017dc50) + 4 * 0x90;
        w32(e, 0x753f);
        w32(e + 8, 10000);
        w32(e + 0xc, 0x0013f350 + 0x22a0);
        w32(0x0013f350 + 0x22a0, u(ri(0, 10000)));
        w32(e + 0x50, u(ri(0, 500)));
        w32(e + 0x54, u(ri(0, 400)));
        w16(e + 0x48, u(ri(-30, 30)));
        w16(e + 0x4a, u(ri(-30, 30)));
        w32(e + 0x58, u(ri(0, 0x200)));
        w32(e + 0x5c, u(ri(0, 0x200)));
        w32(e + 0x60, R() & 0xf);
        w32(e + 0x6c, u(ri(-6, 0x1f)));
        w32(e + 0x7c, u(ri(-2, 12)));
        w32(e + 0x74, ri(0, 3) == 0 ? u(ri(0, 3333)) : u(ri(0, 10000)));
        w32(e + 0x78, u(ri(0, 10000)));
        w32(OV(0x0015f3f8), ri(0, 5) == 0 ? u(ri(-5, 5)) : u(ri(0, 100000)));
        // the layout numbers (from the level's data) sometimes varied: tiers, the count, the frame's size and ratios
        if (ri(0, 1) == 0) {
            w32(OV(0x0015f750), f2u(rnd(0, 16)));
            w32(OV(0x0015f754), f2u(rnd(0, 16)));
            w32(OV(0x0015f758), f2u(rnd(0, 16)));
            w32(OV(0x0015f75c), u(ri(-3, 200)));
            w32(OV(0x0015f760), u(ri(-300, 400)));
            w32(OV(0x0015f764), u(ri(-300, 400)));
            w32(OV(0x0015f768), f2u(rnd(0, 1)));
            w32(OV(0x0015f76c), f2u(rnd(0, 1)));
        }
        if (ri(0, 2) == 0) w32(OV(0x0015f3f8), u(60 * ri(0, 1000) + (coin() ? ri(43, 46) : ri(0, 59))));  // the blink's edges
        if (bubbles) {
            for (uint32_t i = 0; i < 100; ++i) {
                const bool wide = ri(0, 3) == 0;
                for (uint32_t k = 0; k < 4; ++k) w32(TAB + i * 16 + k * 4, f2u(k < 2 ? (wide ? rnd(-300, 300) : rnd(-20, 40)) : (wide ? rnd(-20, 20) : rnd(-1.5f, 1.5f))));
            }
        }
        return e;
    };
    check("hud_o2_start", 0x00236268, 40, Ret::None, [element] { return Args{{element(false)}, {}}; },
          [](A a) { hud_o2_start(a.i[0]); return uint64_t{0}; });
    // Aridia's (2) and Nebula G34's (6) own overlays carry other element functions here (the update calls the immediate counter
    // 0x236128 / its address in the level, the draw is a shorter one); nothing requests the element there (no water with air on
    // Aridia; Blarg's hero code never reaches it), so it is dead code on those two levels: not ported, not compared
    const bool dead_level = g_oracle_level == 2 || g_oracle_level == 6;
    if (dead_level) std::printf("hud_o2_update / hud_o2_draw  skipped (level %d's own element functions, never requested)\n", g_oracle_level);
    if (!dead_level) check("hud_o2_update", 0x00236458, 200, Ret::None, [element] { return Args{{element(true)}, {}}; },
          [](A a) { hud_o2_update(a.i[0]); return uint64_t{0}; });
    if (!dead_level) check("hud_o2_draw", 0x002365a8, 150, Ret::Int32, [element] {
        const uint32_t e = element(true);
        if (ri(0, 2) == 0) w32(e + 0x74, u(ri(0, 3333)));  // the blinking third
        return Args{{e}, {}};
    }, [](A a) { return ri32(hud_o2_draw(a.i[0])); });
    check("vec3_mul", 0x001ff348, 100, Ret::None, [level, rnd] {
        level();
        for (uint32_t i = 0; i < 12; ++i) w32(OUT + i * 4, f2u(ri(0, 8) == 0 ? rnd(-1e30f, 1e30f) : rnd(-100, 100)));
        return Args{{OUT, OUT + 0x10, OUT + 0x20}, {}};
    }, [](A a) { vec3_mul(V(a.i[0]), V(a.i[1]), V(a.i[2])); return uint64_t{0}; });
}

}  // namespace rac1::oracle
