// Ported overlay code 0x002e0000..0x002effff.
#include <game/ovl/ovl_common.h>

#include <game/ovl/ovl_reloc.h>
namespace rac1 {

namespace {
// orig: 0x002ea9d8 (inline prologue shared with 0x002eaa30): the moby with uid 5,
// *(gp+0x200) when it matches, else a table search.
uint32_t uid5_moby() {
    const uint32_t m = lwu(OV(0x00166e00));
    return lh(m + 0x86) == 5 ? m : moby_table_find_by_uid(5);
}
}  // namespace

// orig: 0x002ea9d8 — copy a quadword into the pvars (+0x80) of moby uid 5.
void uid5_set_pvar_80(uint32_t src) { lq_sq(lwu(uid5_moby() + 0x70) + 0x80, src); }

// orig: 0x002eaa30 — same at +0x90.
void uid5_set_pvar_90(uint32_t src) { lq_sq(lwu(uid5_moby() + 0x70) + 0x90, src); }

}  // namespace rac1
