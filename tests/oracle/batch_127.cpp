// Oracle batch 127 (task 0362b): particle type 0x16's update 0x267b10 (Kerwan's ambient particles, L3:0x255cc8) (needs the ISO).

#include "ovl_oracle.h"

#include <game/ovl/ovl_reloc.h>
namespace rac1::oracle {

void batch_127() {
    if (g_only_batch != 127) return;
    auto boot = [] { veldin_boot(); };
    g_batch = 127;
    g_ignored_words = vcross_lanes();
    g_ignored_words.push_back(OV(0x00160188));  // the update's saved return address
    g_ignored_words.push_back(OV(0x0016018c));
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;
    static std::vector<uint8_t> snap;
    auto level = [boot] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty()) {
            boot();
            particle_table_init();
            level_setup_body(1, 0);
            w32(OV(0x0016c060), 0xf);
            gameplay_frame();
            snap.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    // a type 0x16 particle: position, velocity (w the gravity), colours, frames left (some 0 / 1: freed) and total, size and growth
    auto p16 = [] {
        const uint32_t p = alloc_particle(0x16);
        if (p == 0) return p;
        wfrand(p + 0x10, 4, 100, 140);
        wfrand(p + 0x20, 4, -0.2f, 0.2f);
        w32(p + 0x30, R());
        w32(p + 0x34, R());
        const int32_t total = ri(1, 120);
        w16(p + 0x38, u(total));
        w16(p + 0xa, u(ri(0, 4) == 0 ? ri(0, 2) : ri(0, total)));
        w8(p + 8, R());
        wf(p + 0xc, rf(0, 2));
        wf(p + 0x3c, rf(-0.05f, 0.05f));
        return p;
    };
    check("particle_16_update", 0x00267b10, 300, Ret::None, [level, p16] { level(); return Args{{p16()}, {}}; },
          [](A a) { particle_16_update(a.i[0]); return uint64_t{0}; });
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
