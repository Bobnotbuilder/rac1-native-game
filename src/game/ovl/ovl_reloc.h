// The ports' overlay addresses on the played level's own overlay (ADR 0008). The ports are written
// against Veldin's overlay (level 0); every other level ships the same engine relinked at other
// addresses. An overlay literal of a port goes through OV (data:
// sections 0..5, 0x15ef00..0x1ea300) or OVF (code): Veldin's address on Veldin, the level's own
// address elsewhere, from the maps tools/overlay_match generates from the ISO
// (assets/generated/overlay_map/levelN.txt). Each use site caches its translation until the
// level changes: one compare on the hot path.
#ifndef RAC1_GAME_OVL_OVL_RELOC_H
#define RAC1_GAME_OVL_OVL_RELOC_H

#include <cstdint>

namespace rac1 {

extern uint32_t g_ovl_gen;  // bumped by every change of the translation
// The boot module's image (SCUS_971.99's own engine build: the boot, the title, its menu) as a translation target, like a
// level's overlay (assets/generated/overlay_map/
// front_reloc.txt, tools/overlay_match/front_match.py).
constexpr int OVL_FRONT = 100;
// No overlay: the ports' addresses as they are (the boot module's loader before the overlay load). On
// the NTSC-U disc level 0 is the same; on another region's disc level 0 is its own Veldin, translated.
constexpr int OVL_NONE = -1;
// orig: (host) the translation's level: 0 (or a level played on Veldin's image) is the identity;
// OVL_FRONT the boot module
void ovl_reloc_set_level(int level);
int ovl_reloc_level();
// orig: (host) the translation is the identity (NTSC-U Veldin's image, or OVL_NONE)
bool ovl_reloc_identity();
// orig: (host) Veldin's data / code address on the current level's overlay
uint32_t ovl_xlate_data(uint32_t veldin);
uint32_t ovl_xlate_code(uint32_t veldin);
// orig: (host) Veldin's function `veldin` (or an address inside it) on the current level's overlay,
// 0 when the level has no equivalent (the per-level oracle skips those functions; nothing logged)
uint32_t ovl_code_lookup(uint32_t veldin);
// orig: (host) a code address of the current level's overlay as Veldin's (0: none) -- the
// dispatch of the level's code pointers to the ports
uint32_t ovl_code_to_veldin(uint32_t level_addr);

// orig: (host) one use site's cached translation
template <uint32_t A>
inline uint32_t ov_site() {
    static uint32_t gen = 0xffffffffu, v = 0;
    if (gen != g_ovl_gen) {
        v = ovl_xlate_data(A);
        gen = g_ovl_gen;
    }
    return v;
}
template <uint32_t A>
inline uint32_t ovf_site() {
    static uint32_t gen = 0xffffffffu, v = 0;
    if (gen != g_ovl_gen) {
        v = ovl_xlate_code(A);
        gen = g_ovl_gen;
    }
    return v;
}
// A named overlay address (a namespace-scope constant of a port): converts on use.
template <uint32_t A>
struct OvConst {
    operator uint32_t() const { return ov_site<A>(); }
};

}  // namespace rac1

#define OV(a) (::rac1::ov_site<(a)>())
#define OVF(a) (::rac1::ovf_site<(a)>())
// A literal in the overlay's address range that is a value, not an address (a colour, a flag
// mask, a GS memory address: the original builds it with lui + ori): never translated.
#define NOT_ADDR(a) (a)

#endif
