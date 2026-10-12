#ifndef RAC1_FORMATS_SKY_MODEL_H
#define RAC1_FORMATS_SKY_MODEL_H

#include <formats/level_mesh.h>
#include <formats/level_textures.h> // for DecodedTexture
#include <cstdint>
#include <vector>
#include <span>
#include <array>
#include <string>

namespace rac1 {

struct DecodedSkyShell {
    DecodedMesh mesh;
    bool textured = false;
    bool bloom = false;
    std::array<float, 3> rotation = {0.0f, 0.0f, 0.0f};
    std::array<float, 3> angular_velocity = {0.0f, 0.0f, 0.0f};
};

struct DecodedSky {
    std::uint8_t color_r = 0;
    std::uint8_t color_g = 0;
    std::uint8_t color_b = 0;
    std::uint8_t color_a = 0;
    bool clear_screen = false;
    std::vector<DecodedSkyShell> shells;
    std::vector<DecodedTexture> textures;
};

// Decodes a single level sky sub-asset blob (decompressed).
DecodedSky decode_level_sky(std::span<const std::byte> sky_data, int level_id);

} // namespace rac1

#endif // RAC1_FORMATS_SKY_MODEL_H
