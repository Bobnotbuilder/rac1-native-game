#ifndef RAC1_FORMATS_SCENE_WAD_H
#define RAC1_FORMATS_SCENE_WAD_H

#include <io/byte_reader.h>
#include <vector>
#include <string>
#include <span>
#include <array>
#include <cstdint>

namespace rac1 {

enum class SubAssetType {
    Sound,
    Wad,
    Unknown
};

// One scene's row of the scene file's header: the sectors of its sounds and of its data blocks.
struct SceneHeader {
    std::array<std::int32_t, 6> sounds;  // sector offsets
    std::array<std::int32_t, 68> wads;   // sector offsets
};

// The scene file's header (0x22b8 bytes): its own size, a pad word, then 30 scene rows.
struct WadHeader {
    std::int32_t header_size;
    std::int32_t pad_4;
    std::array<SceneHeader, 30> scenes;
};

struct SubAsset {
    int index;
    SubAssetType type;
    std::int32_t sector_offset;
    std::size_t byte_offset;
    std::size_t size_bytes;
    std::size_t decompressed_size;
    std::string tag; // WAD tag or VAG name
    bool is_compressed;
    
    // Tracks which scenes reference this sub-asset.
    // Index mapping: scene_index -> lists of references (sound slot or wad slot index)
    struct Reference {
        int scene_index;
        int slot_index; // 0-5 for sounds, 0-67 for wads
        SubAssetType ref_type;
    };
    std::vector<Reference> references;
};

// Parses a scene.wad and extracts all unique non-empty sub-assets
std::vector<SubAsset> parse_scene_wad(std::span<const std::byte> data);

// Decompresses an LZ-compressed WAD sub-asset.
// Returns true on success, false if the header magic is invalid or decompression fails.
bool decompress_wad(std::span<const std::byte> src, std::vector<std::uint8_t>& dest);

} // namespace rac1

#endif // RAC1_FORMATS_SCENE_WAD_H
