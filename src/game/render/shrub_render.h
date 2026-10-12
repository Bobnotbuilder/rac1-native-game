#ifndef RAC1_GAME_RENDER_SHRUB_RENDER_H
#define RAC1_GAME_RENDER_SHRUB_RENDER_H

#include <array>
#include <span>
#include <cstdint>

namespace rac1 {

// orig: 0x00203b08
void register_shrub_class(std::int32_t o_class, std::span<const std::byte> shrub_class_data, std::span<const std::uint8_t, 16> texture_remap);

// A scene whose data is in EE memory, not a level WAD (the title's level).
struct HostShrubClass {
    std::int32_t o_class;
    std::uint32_t vaddr;                    // the class header's EE address
    std::array<std::uint8_t, 16> textures;  // its texture remap into the shrub texture table
};
// orig: (host) replaces the host's shrub classes and instances (instances_blob: the gameplay blob)
void shrub_scene_load(std::span<const HostShrubClass> classes, std::uint32_t instances_blob);

// Runs per-frame Shrub static prop rendering dispatch
// orig: 0x00203e00
void shrub_render_update();

} // namespace rac1

#endif // RAC1_GAME_RENDER_SHRUB_RENDER_H
