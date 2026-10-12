#include <formats/shrub_model.h>
#include <formats/vif_stream.h>

#include <array>
#include <cstring>
#include <unordered_map>
#include <variant>

namespace rac1 {

namespace {

template <class T>
T get(std::span<const std::byte> b, std::size_t at) {
    T v;
    std::memcpy(&v, b.data() + at, sizeof(T));
    return v;
}

// A shrub class ("Shrub classes"): a 0x40-byte header, its packet
// list right after it (8 bytes per packet: offset, size of the packet's VIF list), 24 normals.
constexpr std::size_t kHeaderBytes = 0x40;
constexpr std::size_t kSphere = 0x00;        // f32[4]
constexpr std::size_t kScale = 0x20;         // f32
constexpr std::size_t kPacketCount = 0x28;   // s16
constexpr std::size_t kNormals = 0x2c;       // s32: 24 normals of 4 halfwords (x, y, z, -), 1/32767
constexpr int kNormalCount = 24;

// A packet's three UNPACKs: [0] its head (texture prim count, GIF tag count, vertex count, -),
// the GIF tags (16 bytes: the 12-byte tag, the GS packet slot) and the texture prims (64 bytes);
// [1] the positions (x, y, z, slot); [2] the texture coordinates and normal index (s, t, h, n).
constexpr std::size_t kTagBytes = 16;
constexpr std::size_t kPrimBytes = 64;
constexpr std::size_t kCornerBytes = 8;

// Slot costs in the GS packet the shrub program builds, in qwords.
constexpr int kTagQwords = 1;
constexpr int kPrimQwords = 5;
constexpr int kCornerQwords = 3;

struct TextureState {
    std::int32_t texture;              // TEX0's low word as stored: the class's local texture
    std::uint64_t tex0;
    std::uint64_t clamp;               // CLAMP_1 as the level setup (0x240e80) builds it: lo | hi << 2
};

struct Corner {
    std::int16_t x, y, z;
    std::int16_t s, t, h;
    std::int16_t normal;
};

struct VertexRun {
    bool list;                         // a triangle list (PRIM type 3), else a strip
    std::uint32_t prim;                // the GIF tag's PRIM field
    bool pre;                          // the tag's PRE bit
    std::vector<Corner> corners;
};

using Item = std::variant<TextureState, VertexRun>;

// Replays one packet's GS packet layout slot by slot (GIF tags, texture prims and corners in slot
// order) into `items`.
void walk_packet(std::span<const std::byte> list, std::vector<Item>& items) {
    const std::vector<VifCommand> unpacks = vif_unpacks(split_vif_commands(list));
    if (unpacks.size() != 3) return;
    const std::span<const std::byte> head(reinterpret_cast<const std::byte*>(unpacks[0].payload.data()), unpacks[0].payload.size());
    if (head.size() < 16) return;
    const std::int32_t prim_count = get<std::int32_t>(head, 0);
    const std::int32_t tag_count = get<std::int32_t>(head, 4);
    const std::int32_t corner_count = get<std::int32_t>(head, 8);
    const std::size_t tags_at = 16, prims_at = tags_at + tag_count * kTagBytes;
    if (prims_at + prim_count * kPrimBytes > head.size()) return;
    const std::size_t corner_bytes = corner_count * kCornerBytes;
    if (corner_bytes > unpacks[1].payload.size() || corner_bytes > unpacks[2].payload.size()) return;
    const std::span<const std::byte> pos(reinterpret_cast<const std::byte*>(unpacks[1].payload.data()), corner_bytes);
    const std::span<const std::byte> uvn(reinterpret_cast<const std::byte*>(unpacks[2].payload.data()), corner_bytes);

    auto tag_slot = [&](std::int32_t i) { return get<std::int32_t>(head, tags_at + i * kTagBytes + 12); };
    auto prim_slot = [&](std::int32_t i) { return get<std::int32_t>(head, prims_at + i * kPrimBytes + 12); };
    auto corner_slot = [&](std::int32_t i) { return static_cast<std::int32_t>(get<std::int16_t>(pos, i * kCornerBytes + 6)); };

    bool list_mode = false;
    std::uint32_t prim = 0;
    bool pre = false;
    VertexRun* run = nullptr;
    std::int32_t tag = 0, tex = 0, corner = 0, slot = 0;
    while (tag < tag_count || tex < prim_count || corner < corner_count) {
        if (tag < tag_count && tag_slot(tag) == slot) {
            const std::uint64_t low = get<std::uint64_t>(head, tags_at + tag * kTagBytes);
            prim = static_cast<std::uint32_t>((low >> 47) & 0x7ff);
            pre = ((low >> 46) & 1) != 0;
            list_mode = (prim & 7) == 3;
            run = nullptr;
            ++tag;
            slot += kTagQwords;
        } else if (tex < prim_count && prim_slot(tex) == slot) {
            const std::size_t p = prims_at + tex * kPrimBytes;  // TEX1, CLAMP (+0x10), MIPTBP1, TEX0 (+0x30)
            const std::int32_t lo = get<std::int32_t>(head, p + 0x30), hi = get<std::int32_t>(head, p + 0x34);
            const std::uint64_t clamp = static_cast<std::uint64_t>(get<std::uint32_t>(head, p + 0x10)) |
                                        static_cast<std::uint64_t>(get<std::uint32_t>(head, p + 0x14)) << 2;
            items.emplace_back(TextureState{lo, static_cast<std::uint64_t>(hi) << 32 | static_cast<std::uint32_t>(lo), clamp});
            run = nullptr;
            ++tex;
            slot += kPrimQwords;
        } else if (corner < corner_count && corner_slot(corner) == slot) {
            if (run == nullptr) run = &std::get<VertexRun>(items.emplace_back(VertexRun{list_mode, prim, pre, {}}));
            const std::size_t a = corner * kCornerBytes;
            run->corners.push_back({get<std::int16_t>(pos, a), get<std::int16_t>(pos, a + 2), get<std::int16_t>(pos, a + 4),
                                    get<std::int16_t>(uvn, a), get<std::int16_t>(uvn, a + 2), get<std::int16_t>(uvn, a + 4),
                                    static_cast<std::int16_t>(get<std::int16_t>(uvn, a + 6) & 0x7fff)});
            ++corner;
            slot += kCornerQwords;
        } else {
            break;  // padding corners at the end, or nothing at this slot
        }
    }
}

} // namespace

DecodedShrubModel decode_shrub_class_model(
    std::int32_t o_class,
    std::span<const std::byte> shrub_class_data,
    std::span<const std::uint8_t, 16> texture_remap
) {
    const std::span<const std::byte> cls = shrub_class_data;
    DecodedShrubModel model;
    model.o_class = o_class;
    if (cls.size() < kHeaderBytes) return model;

    const float scale = get<float>(cls, kScale);
    model.scale = scale;
    for (int i = 0; i < 4; ++i) model.bounding_sphere[i] = get<float>(cls, kSphere + 4 * i);

    std::array<std::int16_t, kNormalCount * 4> normals{};
    const std::int32_t normals_at = get<std::int32_t>(cls, kNormals);
    if (normals_at > 0 && static_cast<std::size_t>(normals_at) + sizeof normals <= cls.size()) {
        std::memcpy(normals.data(), cls.data() + normals_at, sizeof normals);
    }

    // Every packet's texture switches and vertex runs, in order.
    std::vector<Item> items;
    const std::int16_t packets = get<std::int16_t>(cls, kPacketCount);
    if (kHeaderBytes + packets * std::size_t{8} <= cls.size()) {
        for (std::int16_t p = 0; p < packets; ++p) {
            const std::size_t e = kHeaderBytes + static_cast<std::size_t>(p) * 8;
            const std::int32_t at = get<std::int32_t>(cls, e), size = get<std::int32_t>(cls, e + 4);
            if (at <= 0 || size <= 0 || static_cast<std::size_t>(at) >= cls.size() ||
                static_cast<std::size_t>(at) + static_cast<std::size_t>(size) > cls.size()) {
                continue;
            }
            walk_packet(cls.subspan(static_cast<std::size_t>(at), static_cast<std::size_t>(size)), items);
        }
    }

    // Triangles grouped by (level texture, CLAMP_1, PRIM | PRE << 11): key = texture (16 bits) |
    // clamp (16 bits) << 16 | prim << 32. The map's iteration order is the submeshes' order.
    std::unordered_map<std::uint64_t, std::vector<std::array<int, 3>>> by_key;
    std::unordered_map<std::uint64_t, std::uint64_t> tex0_of;
    TextureState state{-1, 0, 0};  // carried over from packet to packet
    for (const Item& item : items) {
        if (const auto* t = std::get_if<TextureState>(&item)) {
            state = *t;
            continue;
        }
        const VertexRun& run = std::get<VertexRun>(item);
        int texture = 0;
        if (state.texture >= 0 && state.texture < 16) {
            texture = texture_remap[static_cast<std::size_t>(state.texture)];
            if (texture == 0xff) texture = 0;
        } else if (state.texture >= 0) {
            texture = state.texture;
        }
        const std::uint64_t key = (static_cast<std::uint64_t>(texture) & 0xffff) | (state.clamp & 0xffff) << 16 |
                                  static_cast<std::uint64_t>(run.prim | (run.pre ? 1u << 11 : 0u)) << 32;
        tex0_of[key] = state.tex0;

        const int first = static_cast<int>(model.vertices.size());
        for (const Corner& c : run.corners) {
            DecodedVertex v{};
            v.x = c.x * scale * (1.0f / 1024.0f);
            v.y = c.y * scale * (1.0f / 1024.0f);
            v.z = c.z * scale * (1.0f / 1024.0f);
            v.nz = 1.0f;
            if (c.normal >= 0 && c.normal < kNormalCount) {
                v.nx = normals[c.normal * 4 + 0] * (1.0f / 32767.0f);
                v.ny = normals[c.normal * 4 + 1] * (1.0f / 32767.0f);
                v.nz = normals[c.normal * 4 + 2] * (1.0f / 32767.0f);
            }
            v.u = c.s * (1.0f / 4096.0f);
            v.v = c.t * (1.0f / 4096.0f);
            v.r = v.g = v.b = v.a = 255;
            model.vertices.push_back(v);
        }
        const int n = static_cast<int>(run.corners.size());
        if (run.list) {
            for (int i = 0; i + 2 < n; i += 3) by_key[key].push_back({first + i, first + i + 1, first + i + 2});
        } else {  // a strip: every other triangle has its first two corners swapped
            for (int i = 0; i + 2 < n; ++i) {
                const int a = first + i + (i % 2), b = first + i + 1 - (i % 2), c = first + i + 2;
                if (a != b && b != c && c != a) by_key[key].push_back({a, b, c});
            }
        }
    }

    for (const auto& [key, tris] : by_key) {
        DecodedSubMesh sm;
        sm.material_index = static_cast<int>(key & 0xffff);
        sm.clamp = (key >> 16) & 0xffff;
        sm.prim = static_cast<std::uint32_t>(key >> 32) & 0x7ff;
        sm.has_prim = ((key >> 43) & 1) != 0;
        sm.triangles = tris;
        sm.tex0 = tex0_of[key];
        model.submeshes.push_back(std::move(sm));
    }
    return model;
}

} // namespace rac1
