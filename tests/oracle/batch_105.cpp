// Oracle batch 105 (task 0329): the armor load 0x23fa00 with its four GS uploads (the 64 x 64 and 32 x 32 texture levels and the two palettes
// of the armor file's blocks, sceGsSetDefLoadImage / sceGsExecLoadImage / sceGsSyncPath at the loop 0x23fa88). Veldin's real level (needs the ISO),
// the armor 0..3 and the level's texture count varied. The original runs in the EE interpreter with sceGsExecLoadImage (0x122658) running for real
// on the host's GS, the port on the same memory: every word of EE RAM (outside the interpreter's stack, which holds the original's saved registers)
// and the host GS's local memory compared.

#include "ovl_oracle.h"

#include <game/ovl/ovl_reloc.h>
#include <game/lib/libgraph.h>
#include <runtime/gs/gs_emu.h>

#include <cstdio>
#include <cstring>
#include <vector>

namespace rac1::oracle {

void batch_105() {
    if (g_only_batch != 105) return;
    g_batch = 105;
    veldin_boot();
    level_setup_body(1, 0);
    uint8_t* ram = ee_get_ram_base();
    const std::vector<uint8_t> snap(ram, ram + 0x02000000);
    const uint32_t core = ee_read<uint32_t>(OV(0x00173e04));
    gs::Gs& g = gs::gs_global();
    // the original's call to sceGsExecLoadImage (0x00122658) runs the host's: the port's lib function
    g_extra_hooks = {{0x00122658u, [](EeCpu& c) { gs_exec_load_image(c.r32(4), c.r32(5)); c.set32(2, 0); }}};
    int bad = 0, cases = 0;
    for (int armor = 0; armor < 4; ++armor)
        for (int rep = 0; rep < 3; ++rep) {
            const uint32_t dst = 0x01c00000u + static_cast<uint32_t>(rep) * 0x40000u;
            auto setup = [&] {
                std::memcpy(ram, snap.data(), snap.size());
                ee_write<int16_t>(0x0013e056, static_cast<int16_t>(armor));
                ee_write<uint32_t>(OV(0x0015ffc8), 0x20u + static_cast<uint32_t>(rep));
                std::memset(g.vram(), 0, 4u << 20);
            };
            setup();
            const OrigResult o = call_original(0x0023fa00, {dst, core});
            const std::vector<uint8_t> ram_o(ram, ram + 0x01fe0000), vram_o(g.vram(), g.vram() + (4u << 20));
            const uint32_t ret_o = static_cast<uint32_t>(o.v0);
            setup();
            const uint32_t ret_p = armor_load(dst, core);
            const bool same_ram = std::memcmp(ram_o.data(), ram, ram_o.size()) == 0, same_vram = std::memcmp(vram_o.data(), g.vram(), vram_o.size()) == 0;
            size_t nz = 0;
            for (uint8_t v : vram_o) nz += v != 0;
            ++cases;
            if (!o.ok || ret_o != ret_p || !same_ram || !same_vram) {
                ++bad;
                std::printf("  armor %d rep %d: ok %d ret %08x / %08x, ram %s, vram %s\n", armor, rep, o.ok, ret_o, ret_p, same_ram ? "same" : "DIFFERS", same_vram ? "same" : "DIFFERS");
            } else if (rep == 0) {
                std::printf("  armor %d: same RAM and GS memory (%zu non-zero VRAM bytes uploaded), returns %08x\n", armor, nz, ret_p);
            }
        }
    g_extra_hooks.clear();
    std::printf("armor_load                 0x0023fa00  %d / %d cases equal\n", cases - bad, cases);
    if (bad != 0) ++g_fail;
}

}  // namespace rac1::oracle
