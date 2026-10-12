#ifndef RAC1_FORMATS_LEVEL_MESH_H
#define RAC1_FORMATS_LEVEL_MESH_H

#include <cstdint>
#include <vector>
#include <string>
#include <span>
#include <array>

namespace rac1 {

struct DecodedVertex {
    float x, y, z;
    float u, v;
    float nx, ny, nz;
    std::uint8_t r, g, b, a;
    int joint_idx = 0;  // the main joint (the largest weight)
    // the skin: up to 4 joints and their weights (summing to 1; joint_idx alone when unset)
    int joints[4] = {0, 0, 0, 0};
    float weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct DecodedSubMesh {
    int material_index; // Index of the texture in the level's material/texture list
    std::uint64_t tex0 = 0; // Raw GS TEX0 register value
    // GS CLAMP_1 as the level setup writes it from the class's CLAMP A+D pair (tie 0x240aa8,
    // shrub 0x240e80, tfrag 0x241458): the pair's low word is WMS, its high word << 2 is WMT
    // (0 REPEAT, 1 CLAMP); the texture index the setup also puts in bits 24.. is not kept.
    std::uint64_t clamp = 0;
    // The GS PRIM of the GIF tag that draws the triangles, when the class data holds the tag
    // (shrubs: PRE set); has_prim false otherwise (tie / tfrag: their VU1 programs make the tags).
    std::uint32_t prim = 0;
    bool has_prim = false;
    std::vector<std::array<int, 3>> triangles; // 3 vertex indices per triangle
    std::vector<std::array<int, 4>> quads;     // 4 vertex indices per quad
};

struct DecodedMesh {
    std::string name;
    std::vector<DecodedVertex> vertices;
    std::vector<DecodedSubMesh> submeshes;
    std::array<float, 4> bsphere = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct LevelStaticGeometry {
    std::vector<DecodedMesh> tfrags;
    std::vector<DecodedMesh> tie_classes;
};

LevelStaticGeometry decode_level_meshes(std::span<const std::byte> level_wad_data);

} // namespace rac1

#endif // RAC1_FORMATS_LEVEL_MESH_H
