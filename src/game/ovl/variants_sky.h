// The sky's per-level dispatchers: each level's build has its own 31-instruction one (sky_levels.cpp); the table is literal addresses
// (variants_sky.cpp). The variants it calls are Veldin's ports (the same code on every level).
#ifndef RAC1_GAME_OVL_VARIANTS_SKY_H
#define RAC1_GAME_OVL_VARIANTS_SKY_H

#include <cstdint>

#include <game/ovl/novalis_mobys2.h>
#include <game/ovl/variants.h>

namespace rac1 {

using SkyTrFn = TrRet (*)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, float, float, float, float);

struct SkyLevel {
    int level;
    uint32_t dispatcher;  // the level's own function (0x7c bytes)
    SkyTrFn variant;      // the level's variant: the port of the same code as Veldin's
};

// orig: (host) the level's row; null for Veldin and for a level without one
const SkyLevel* sky_level(int level);

// orig: (host) the sky dispatcher of level `level` as a translation row (Veldin's 0x23d968 -> the level's own); false for Veldin
bool sky_dispatcher_row(int level, VariantRow* row);

}  // namespace rac1

#endif
