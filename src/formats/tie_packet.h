#ifndef RAC1_FORMATS_TIE_PACKET_H
#define RAC1_FORMATS_TIE_PACKET_H

// A tie class's packets, read back as the triangle strips the tie VU1 programs draw
// ("Tie classes"). Shared by the tie class decoder
// (tie_model.cpp) and the level geometry decoder (level_mesh.cpp).

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace rac1::tie {

// Byte offsets in a tie class's 0x70-byte header.
inline constexpr std::size_t kHeaderBytes = 0x70;
inline constexpr std::size_t kLodTables = 0x00;     // s32[3]: each LOD's packet table
inline constexpr std::size_t kLodPacketCounts = 0x20; // u8[3]
inline constexpr std::size_t kClassPrims = 0x2c;    // u32: the class's texture prims (0x50 bytes each)
inline constexpr std::size_t kSphere = 0x30;        // f32[4]
inline constexpr std::size_t kScale = 0x40;         // f32

// One corner of a strip: position (in 1/1024 of the class scale) and texture coordinates
// (in 1/4096). `slot` is where the VU1 program writes it in the GS packet, in qwords.
struct Corner {
    std::int16_t x, y, z;
    std::uint16_t slot;
    std::uint16_t s, t, q;
    std::uint16_t slot2;              // the second strip the vertex is used in (0 / slot: none)
};

struct Strip {
    int texture_slot;                 // the class prim in effect: its offset / 0x50
    bool odd_winding;                 // the strip's winding flag
    std::vector<Corner> corners;
};

struct WalkRules {
    // A strip takes no more corners than its count says.
    bool cap_at_strip_count = false;
    // A gap in the slots (no strip, corner or texture switch at the next slot) is jumped over
    // instead of ending the walk.
    bool jump_gaps = false;
};

// The strips of the packet whose 16-byte entry is at `entry` in `cls` (the class's bytes); its data
// starts at `lod_table` + the entry's first word. Empty when the packet does not fit in `cls`.
std::vector<Strip> packet_strips(std::span<const std::byte> cls, std::size_t lod_table, std::size_t entry,
                                 const WalkRules& rules);

} // namespace rac1::tie

#endif // RAC1_FORMATS_TIE_PACKET_H
