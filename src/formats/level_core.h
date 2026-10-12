#ifndef RAC1_FORMATS_LEVEL_CORE_H
#define RAC1_FORMATS_LEVEL_CORE_H

// A level's asset index and asset data: the parts of the asset data
// the index names, with their sizes.

#include <formats/level_file.h>

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace rac1 {

struct LevelCoreSubAsset {
    std::string kind;          // "tfrags", "occlusion", "sky", "collision", "moby_class", "tie_class",
                               // "shrub_class", "sequence", "gadget", "textures", "part_bank", "fx_bank"
    std::int32_t id = -1;      // the class number or the sequence number; -1 otherwise
    std::uint32_t offset = 0;  // in the decompressed asset data
    std::uint32_t size = 0;    // in bytes (up to the next part the index names)
    bool packed = false;       // the part is itself WAD-compressed
};

struct DecodedLevelCoreIndex {
    AssetIndexHeader header;
    std::vector<LevelCoreSubAsset> sub_assets;  // by offset
};

// The asset index (as stored) and the asset data (decompressed) of a level data file (or of an
// extracted level file, see level_file_body). The decompressed bytes are appended to
// `out_core_data_decompressed`. False when a part is out of range or does not decompress.
bool extract_core_from_level_wad(
    std::span<const std::byte> level_wad_data,
    std::vector<std::uint8_t>& out_core_index,
    std::vector<std::uint8_t>& out_core_data_decompressed
);

// The index's header and the parts of the asset data it names.
DecodedLevelCoreIndex decode_level_core_index(
    std::span<const std::byte> core_index_bytes,
    std::span<const std::byte> core_data_decompressed_bytes
);

} // namespace rac1

#endif // RAC1_FORMATS_LEVEL_CORE_H
