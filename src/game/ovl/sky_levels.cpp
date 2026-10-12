// See sky_levels.h and variants_sky.cpp (the table of the per-level dispatchers). The per-level dispatchers are hand-written from their listings (31 instructions each, the same shape on all eighteen levels
// checked by diffing the instruction streams); the helpers they call are the ports of sky_ee.cpp (Veldin's addresses, translated).
#include <game/ovl/sky_levels.h>

#include <game/ovl/ee_regs.h>
#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/sky_ee.h>
#include <game/ovl/variants_sky.h>

namespace rac1 {

// orig: 0x0023d968
// orig: L1:0x00252570 L2:0x00240048 L3:0x0022c6a8 L4:0x00230f48 L5:0x00267520 L6:0x0024d218 L7:0x00265b58 L8:0x00248148 L9:0x0025d648
// orig: L10:0x00230fd0 L11:0x00263908 L12:0x00256748 L13:0x002497d8 L14:0x00245940 L15:0x0022dad0 L16:0x00239c30 L17:0x00238c50 L18:0x00240840
void sky_render() {
    const SkyLevel* s = sky_level(ovl_reloc_level());
    if (s == nullptr) {  // Veldin's build (and every level played on its image): the twenty-way dispatcher
        sky_render_tr(0);
        return;
    }
    // the level's own build: jal init (0x28b680); level == N ? variant : default (0x2892d0); jal tail (0x28b710); the two 0x2a0ba8 calls
    ovl_28b680_tr(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (lw(0x0015ed84) == static_cast<uint32_t>(s->level)) s->variant(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    else ovl_2892d0_tr(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ovl_28b710_tr(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    sub_002a0ba8(0x47, 0x5360b);
    // (or a1, v0, a1 in the delay slot: the sign-extended arithmetic shift of the word, ORed with 0x01000000)
    sub_002a0ba8(0x4e, 0x01000000ull | static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(lw(OV(0x0015ee88))) >> 13)));
}

}  // namespace rac1
