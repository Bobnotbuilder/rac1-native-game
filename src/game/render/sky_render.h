#ifndef RAC1_GAME_RENDER_SKY_RENDER_H
#define RAC1_GAME_RENDER_SKY_RENDER_H

#include <cstdint>

namespace rac1 {

// orig: 0x002040e0
void sky_init(std::uint32_t sky_offset, std::uint32_t tfrag_textures_offset);

// orig: (host) the sky at an EE address for the host drawer (the title's level), no game state written
bool sky_scene_load(std::uint32_t sky_addr);

// orig: 0x002043b0
void sky_draw();

} // namespace rac1

#endif // RAC1_GAME_RENDER_SKY_RENDER_H
