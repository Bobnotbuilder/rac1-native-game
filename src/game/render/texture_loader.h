#ifndef RAC1_GAME_RENDER_TEXTURE_LOADER_H
#define RAC1_GAME_RENDER_TEXTURE_LOADER_H

#include <filesystem>

namespace rac1 {

// orig: 0x00202d78
// Game-space level texture loader corresponding to original PS2 function.
// Loops through tfrag, moby, TIE, shrub, particle, fx textures in the core
// WAD index and registers them in EE RAM virtual memory.
void texture_loader(const std::filesystem::path& level_wad_path);

} // namespace rac1

#endif // RAC1_GAME_RENDER_TEXTURE_LOADER_H
