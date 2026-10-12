#ifndef RAC1_FORMATS_LEVEL_COLLISION_H
#define RAC1_FORMATS_LEVEL_COLLISION_H

#include <cstdint>
#include <vector>
#include <span>

namespace rac1 {

struct CollisionVec3 {
    float x, y, z;
};

struct DecodedCollFace {
    std::uint8_t v0;
    std::uint8_t v1;
    std::uint8_t v2;
    std::uint8_t v3; // Only valid if is_quad is true
    std::uint8_t type;
    bool is_quad;
};

struct DecodedCollisionOctant {
    std::int32_t offset;
    CollisionVec3 displacement;
    std::vector<CollisionVec3> vertices;
    std::vector<DecodedCollFace> faces;
};

struct DecodedHeroCollisionVertex {
    float x, y, z;
};

struct DecodedHeroCollisionTriangle {
    std::uint8_t v0, v1, v2;
};

struct DecodedHeroCollisionGroup {
    float bsphere_x;
    float bsphere_y;
    float bsphere_z;
    float bsphere_radius;
    std::vector<DecodedHeroCollisionVertex> vertices;
    std::vector<DecodedHeroCollisionTriangle> triangles;
};

struct DecodedCollision {
    std::int32_t mesh_offset = 0;
    std::int32_t hero_groups_offset = 0;

    std::uint32_t z_node_count = 0;
    std::uint32_t y_node_count = 0;
    std::uint32_t x_node_count = 0;
    std::uint32_t octant_count = 0;
    std::uint32_t max_depth = 0;

    std::vector<DecodedCollisionOctant> octants;
    std::vector<DecodedHeroCollisionGroup> hero_groups;
};

// Decodes a native level collision blob.
// If the blob starts with the 'WAD' magic, it is automatically decompressed.
DecodedCollision decode_level_collision(std::span<const std::byte> data);

// Extracts the raw collision blob from a level data WAD (the data blob corresponding to a level).
// Returns true on success, false on failure.
// If successful, outputs the raw collision bytes to out_collision_blob.
// Optionally reports the offset and size of the block in the decompressed core_data,
// and whether the collision block itself was compressed.
bool extract_collision_from_level_wad(
    std::span<const std::byte> level_wad_data,
    std::vector<std::uint8_t>& out_collision_blob,
    std::uint32_t& out_offset,
    std::uint32_t& out_size,
    bool& out_was_compressed
);

} // namespace rac1

#endif // RAC1_FORMATS_LEVEL_COLLISION_H
