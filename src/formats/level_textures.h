#ifndef RAC1_FORMATS_LEVEL_TEXTURES_H
#define RAC1_FORMATS_LEVEL_TEXTURES_H

// A level's textures: every table of the asset index decoded to
// RGBA. The level file's records are in level_file.h.

#include <formats/level_file.h>

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace rac1 {

struct DecodedTexture {
    std::string name;
    std::string category; // "tfrag", "moby", "tie", "shrub", "billboard", "particle", "fx"
    int index;
    int width;
    int height;
    std::vector<std::uint8_t> rgba_data; // RGBA32 pixels (width * height * 4 bytes)
};

// Decodes every texture of a level data file (or of an extracted level file with its 48-byte
// header, see level_file_body).
std::vector<DecodedTexture> decode_level_textures(std::span<const std::byte> level_wad_data);

} // namespace rac1

#endif // RAC1_FORMATS_LEVEL_TEXTURES_H
