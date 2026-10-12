#ifndef RAC1_FORMATS_MOBY_MODEL_H
#define RAC1_FORMATS_MOBY_MODEL_H

#include <formats/level_mesh.h>
#include <cstdint>
#include <vector>
#include <span>
#include <array>

#include <formats/animation.h>

namespace rac1 {

struct DecodedMobyModel {
    std::int32_t o_class = -1;
    float scale = 1.0f;
    std::array<float, 4> bounding_sphere = {0.0f, 0.0f, 0.0f, 0.0f}; // [x, y, z, r]
    
    std::vector<DecodedVertex> vertices;
    std::vector<DecodedSubMesh> submeshes;
    DecodedMobyAnimationSection anims;
};

// Decodes a single Moby class model blob.
// - moby_class_data: Span of bytes of the sub-asset (decompressed).
// - texture_remap: The 16-byte texture mapping array from the Level's ClassRecord.
// The triangles the game draws for a high-LOD packet (from the packet's MRP kick program,
// src/setup/kick_trace.cpp): per triangle its three VU1 slots (1-based) and the texture
// prim then in effect (-1: the state before the packet).
struct MobyPacketTriangles {
    std::vector<std::array<std::uint8_t, 3>> slots;
    std::vector<std::int16_t> prim;
};

// `game`: when given (one entry per high-LOD packet), the triangles are taken from it instead of
// the legacy walk of the index list.
DecodedMobyModel decode_moby_class_model(
    std::int32_t o_class,
    std::span<const std::byte> moby_class_data,
    std::span<const std::uint8_t, 16> texture_remap,
    const std::vector<MobyPacketTriangles>* game = nullptr
);

} // namespace rac1

#endif // RAC1_FORMATS_MOBY_MODEL_H
