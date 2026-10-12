// The sky's EE side on a level's own overlay. Every level's build has its own dispatcher of the sky (Veldin's, 0x23d968, picks
// one of twenty variants by the level number through a table; a level's own is `init; if (level == N) variant else default; tail`), the
// variants and helpers being the same code as Veldin's (matched by tools/overlay_match: the Veldin ports run on them, translated).
#ifndef RAC1_GAME_OVL_SKY_LEVELS_H
#define RAC1_GAME_OVL_SKY_LEVELS_H

#include <cstdint>

namespace rac1 {

// orig: 0x0023d968
// The sky: the frame packet gets its chain (the sky's GIF packets, the texture uploads) and the sky's state moves one frame. Veldin's
// dispatcher, or the one of the level whose overlay is the translation target (sky_levels.cpp: every level's own, L1:0x00252570 ...).
void sky_render();

}  // namespace rac1

#endif
