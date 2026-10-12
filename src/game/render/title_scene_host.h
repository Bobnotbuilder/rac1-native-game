// The host renderers' copy of the title's level. The boot module's title level (TOC
// +0x14e8) is not a level WAD: the tie / shrub / sky renderers read a WAD's core header, so this
// adapter hands them the title's own data, as the ported loader (title_load, front:0x1ea830) left
// it in EE memory: the class headers and texture remaps, the texture tables, the instances block,
// the sky. Host code: no game state is written.
#pragma once

namespace rac1 {

// Builds the host's tie / shrub / sky data of the title's level (after title_load). Needs a GL context.
// orig: (host) the title level's assets for the host renderers
void title_scene_host_load();

// Draws the title's sky, ties and shrubs with the game's view (the title camera 0x1eaf88).
// orig: (host) the title scene's static 3D
void title_scene_host_draw();

}  // namespace rac1
