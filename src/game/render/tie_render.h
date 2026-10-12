#ifndef RAC1_GAME_RENDER_TIE_RENDER_H
#define RAC1_GAME_RENDER_TIE_RENDER_H

#include <array>
#include <span>
#include <cstdint>

namespace rac1 {

// orig: 0x00203730
void register_tie_class(std::int32_t o_class, std::span<const std::byte> tie_class_data, std::span<const std::uint8_t, 16> texture_remap);

// A scene whose data is in EE memory, not a level WAD (the title's level).
struct HostTieClass {
    std::int32_t o_class;
    std::uint32_t vaddr;                    // the class header's EE address
    std::array<std::uint8_t, 16> textures;  // its texture remap into the tie texture table
};
// orig: (host) replaces the host's tie classes and instances (instances_blob: the gameplay blob)
void tie_scene_load(std::span<const HostTieClass> classes, std::uint32_t instances_blob);

// Runs per-frame TIE static prop rendering dispatch
// orig: 0x00203338
void tie_render_update();

} // namespace rac1

#endif // RAC1_GAME_RENDER_TIE_RENDER_H
