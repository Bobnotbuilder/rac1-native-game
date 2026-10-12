#ifndef RAC1_FORMATS_TIE_MODEL_H
#define RAC1_FORMATS_TIE_MODEL_H

#include <formats/level_mesh.h>
#include <cstdint>
#include <vector>
#include <span>
#include <array>

namespace rac1 {

struct DecodedTieModel {
    std::int32_t o_class = -1;
    float scale = 1.0f;
    std::array<float, 4> bounding_sphere = {0.0f, 0.0f, 0.0f, 0.0f}; // [x, y, z, r]
    
    std::vector<DecodedVertex> vertices;
    std::vector<DecodedSubMesh> submeshes;
};

// Decodes a single TIE class model blob.
// - o_class: the class ID
// - tie_class_data: Span of bytes of the sub-asset (decompressed).
// - texture_remap: The 16-byte texture mapping array from the Level's ClassRecord.
DecodedTieModel decode_tie_class_model(
    std::int32_t o_class,
    std::span<const std::byte> tie_class_data,
    std::span<const std::uint8_t, 16> texture_remap
);

} // namespace rac1

#endif // RAC1_FORMATS_TIE_MODEL_H
