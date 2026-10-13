// Oracle batch 61: the follow camera on Veldin's real level data (needs the ISO).

#include "ovl_oracle.h"

#include <game/ovl/ovl_reloc.h>
namespace rac1::oracle {

void batch_61() {
    if (g_only_batch != 61) return;
    auto boot = [] { veldin_boot(); };
    g_batch = 61;
    g_ignored_words = vcross_lanes();
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
    };
    auto drive = [] {  // the hero moves, turns, the stick moves the camera
        const float ang = rf(-3.1f, 3.1f);
        wf(0x0013f5e0, std::cos(ang)); wf(0x0013f5e4, std::sin(ang)); wf(0x0013f5e8, 0.0f);
        wf(0x0013f490, coin() ? 0.0f : rf(-0.3f, 0.3f)); wf(0x0013f494, coin() ? 0.0f : rf(-0.3f, 0.3f)); wf(0x0013f498, coin() ? 0.0f : rf(-0.1f, 0.1f));
        for (int k = 0; k < 3; ++k) wf(0x0013f3d0 + 4 * u(k), ee_read<float>(0x0013f3d0 + 4 * u(k)) + ee_read<float>(0x0013f490 + 4 * u(k)));
        wf(0x0013ca40, ri(0, 2) ? 0.0f : rf(-1, 1)); wf(0x0013ca44, ri(0, 2) ? 0.0f : rf(-1, 1));
        if (ri(0, 7) == 0) {  // the hero's state, mode and hang / swing values
            static const uint32_t hs[] = {0, 1, 2, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x14, 0x50, 0x73, 0x77, 0x7b, 0x7f, 0x81};
            static const uint32_t ss[] = {0, 2, 4, 5, 0xf, 0x10, 0x11, 0x12};
            w32(0x001413d4, hs[ri(0, 16)]); w32(0x001413dc, ss[ri(0, 7)]); w8(0x001413f4, u(ri(0, 2)));
            w16(0x0013f65c, ri(0, 3) == 0 ? u(ri(1, 80)) : 0u); wf(0x0013f62c, rf(0, 40)); w16(0x0013f76e, u(ri(0, 1)));
            w8(0x001413fa, ri(0, 5) == 0 ? 1u : 0u);
        }
    };
    check("camera_update", 0x001ed0a8, 200, Ret::None, [level, drive] {
        level();
        w32(OV(0x0015f5c4), 0);
        const int frames = ri(0, 40);
        for (int f = 0; f < frames; ++f) { drive(); camera_update(); }
        drive();
        return Args{};
    }, [](A) { camera_update(); return uint64_t{0}; });
}

}  // namespace rac1::oracle
