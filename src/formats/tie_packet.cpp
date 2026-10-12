#include <formats/tie_packet.h>

#include <algorithm>
#include <cstring>

namespace rac1::tie {

namespace {

template <class T>
T get(std::span<const std::byte> b, std::size_t at) {
    T v;
    std::memcpy(&v, b.data() + at, sizeof(T));
    return v;
}

// Packet data layout (offsets from the packet's data).
constexpr std::size_t kSwitchSlots = 0x00;    // s32[4]: GS packet slots of texture switches 1..3
constexpr std::size_t kSwitchPrims = 0x10;    // s32[4]: the class prim (byte offset) of each texture
constexpr std::size_t kStripCount = 0x23;     // u8
constexpr std::size_t kDinkyWords = 0x28;     // u8: 2 x dinky vertex count + 4
constexpr std::size_t kStrips = 0x2c;         // 4 bytes per strip: count, -, slot, winding flag
constexpr std::size_t kDinkyBytes = 16;       // x y z slot | s t q slot2
constexpr std::size_t kFatBytes = 24;         // 3 halfwords, slot | x y z - | s t q slot2

// Slot costs in the GS packet the VU1 program builds, in qwords.
constexpr int kTextureSwitchQwords = 6;
constexpr int kStripTagQwords = 1;
constexpr int kCornerQwords = 3;

// Every corner of the vertex block once per strip slot it is written to, ordered by slot; of
// corners with the same slot, the first one after the (unstable) sort is kept.
std::vector<Corner> corners_by_slot(std::span<const std::byte> block, int dinky) {
    std::vector<Corner> all;
    auto add = [&all](const Corner& c) {
        all.push_back(c);
        if (c.slot2 != 0 && c.slot2 != c.slot) {
            Corner again = c;
            again.slot = c.slot2;
            all.push_back(again);
        }
    };
    for (int i = 0; i < dinky; ++i) {
        const std::size_t at = static_cast<std::size_t>(i) * kDinkyBytes;
        Corner c;
        c.x = get<std::int16_t>(block, at);
        c.y = get<std::int16_t>(block, at + 2);
        c.z = get<std::int16_t>(block, at + 4);
        c.slot = get<std::uint16_t>(block, at + 6);
        c.s = get<std::uint16_t>(block, at + 8);
        c.t = get<std::uint16_t>(block, at + 10);
        c.q = get<std::uint16_t>(block, at + 12);
        c.slot2 = get<std::uint16_t>(block, at + 14);
        add(c);
    }
    const std::size_t fat_start = static_cast<std::size_t>(dinky) * kDinkyBytes;
    const std::size_t fat = (block.size() - fat_start) / kFatBytes;
    for (std::size_t i = 0; i < fat; ++i) {
        const std::size_t at = fat_start + i * kFatBytes;
        Corner c;
        c.slot = get<std::uint16_t>(block, at + 6);
        c.x = get<std::int16_t>(block, at + 8);
        c.y = get<std::int16_t>(block, at + 10);
        c.z = get<std::int16_t>(block, at + 12);
        c.s = get<std::uint16_t>(block, at + 16);
        c.t = get<std::uint16_t>(block, at + 18);
        c.q = get<std::uint16_t>(block, at + 20);
        c.slot2 = get<std::uint16_t>(block, at + 22);
        add(c);
    }
    std::sort(all.begin(), all.end(), [](const Corner& a, const Corner& b) { return a.slot < b.slot; });
    std::vector<Corner> unique;
    for (const Corner& c : all) {
        if (unique.empty() || unique.back().slot != c.slot) unique.push_back(c);
    }
    return unique;
}

} // namespace

std::vector<Strip> packet_strips(std::span<const std::byte> cls, std::size_t lod_table, std::size_t entry,
                                 const WalkRules& rules) {
    const std::size_t data_at = lod_table + static_cast<std::size_t>(get<std::int32_t>(cls, entry));
    if (data_at >= cls.size()) return {};
    const std::span<const std::byte> data = cls.subspan(data_at);
    if (data.size() < kStrips) return {};

    std::int32_t switch_slot[4], switch_prim[4];
    std::memcpy(switch_slot, data.data() + kSwitchSlots, sizeof switch_slot);
    std::memcpy(switch_prim, data.data() + kSwitchPrims, sizeof switch_prim);

    struct StripTag {
        std::uint8_t count, slot;
        bool odd;
    };
    const std::size_t strip_count = get<std::uint8_t>(data, kStripCount);
    if (kStrips + strip_count * 4 > data.size()) return {};
    std::vector<StripTag> tags(strip_count);
    for (std::size_t i = 0; i < strip_count; ++i) {
        const std::size_t at = kStrips + i * 4;
        tags[i] = {get<std::uint8_t>(data, at), get<std::uint8_t>(data, at + 2), get<std::uint8_t>(data, at + 3) != 0};
    }

    const std::size_t block_at = static_cast<std::size_t>(get<std::uint8_t>(cls, entry + 8)) * 16;
    const std::size_t block_size = static_cast<std::size_t>(get<std::uint8_t>(cls, entry + 9)) * 16;
    if (block_at + block_size > data.size()) return {};
    const int dinky = (static_cast<int>(get<std::uint8_t>(data, kDinkyWords)) - 4) / 2;
    if (dinky < 0 || static_cast<std::size_t>(dinky) * kDinkyBytes > block_size) return {};
    const std::vector<Corner> corners = corners_by_slot(data.subspan(block_at, block_size), dinky);

    // Replay the GS packet's layout slot by slot: the first texture's A+D block, then strip tags,
    // corners and texture switches in slot order.
    std::vector<Strip> strips;
    int texture = switch_prim[0] / 0x50;
    int slot = kTextureSwitchQwords;
    std::size_t next_tag = 0, next_corner = 0;
    int next_switch = 1;
    const std::size_t tag_end = tags.size(), corner_end = corners.size();
    while (next_tag < tag_end || next_corner < corner_end) {
        if (next_tag < tag_end && tags[next_tag].slot == slot) {
            strips.push_back({texture, tags[next_tag].odd, {}});
            ++next_tag;
            slot += kStripTagQwords;
        } else if (next_corner < corner_end && corners[next_corner].slot == slot) {
            const bool open = !strips.empty();
            if (open && (!rules.cap_at_strip_count || strips.back().corners.size() < tags[next_tag - 1].count)) {
                strips.back().corners.push_back(corners[next_corner]);
            }
            ++next_corner;
            slot += kCornerQwords;
        } else if (next_switch < 4 && switch_slot[next_switch - 1] == slot) {
            texture = switch_prim[next_switch] / 0x50;
            ++next_switch;
            slot += kTextureSwitchQwords;
        } else {
            if (!rules.jump_gaps) break;
            int nearest = 9999999;
            if (next_tag < tag_end) nearest = std::min(nearest, static_cast<int>(tags[next_tag].slot));
            if (next_corner < corner_end) nearest = std::min(nearest, static_cast<int>(corners[next_corner].slot));
            if (next_switch < 4 && switch_slot[next_switch - 1] > slot) nearest = std::min(nearest, switch_slot[next_switch - 1]);
            if (nearest == 9999999 || nearest <= slot) break;
            slot = nearest;
        }
    }
    return strips;
}

} // namespace rac1::tie
