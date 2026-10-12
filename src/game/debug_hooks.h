#ifndef RAC1_GAME_DEBUG_HOOKS_H
#define RAC1_GAME_DEBUG_HOOKS_H

// Host debug hooks for the debug menu (src/_devtools): not part of the original, off (null) by
// default, so the ported code runs as the original does unless the menu sets them.

#include <cstdint>

namespace rac1 {

// When set and true for a moby, the frame's moby update pass (0x264848) leaves that moby as it
// is: no animation step, no class update, no transform (a frozen moby).
extern bool (*g_debug_moby_frozen)(uint32_t moby);

// When set, the projection setup (0x1f7980) makes the y fov tangent (0x16caf4) from what this
// returns for the x one (0x16caf0) instead of the x one itself: the host widens the x tangent to
// the window in widescreen and gives back the tangent without that widening, so the vertical
// field of view stays the 4:3 frame's.
extern float (*g_debug_fov_y_base)(float x_tangent);

}  // namespace rac1

#endif  // RAC1_GAME_DEBUG_HOOKS_H
