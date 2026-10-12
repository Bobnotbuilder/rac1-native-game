#include <formats/tie_model.h>
#include <formats/tie_packet.h>

#include <array>
#include <cstring>
#include <unordered_map>

namespace rac1 {

namespace {

template <class T>
T get(std::span<const std::byte> b, std::size_t at) {
    T v;
    std::memcpy(&v, b.data() + at, sizeof(T));
    return v;
}

// The GS words of the class prim (5 A+D register writes of 16 bytes) for local texture `slot`:
// TEX0 as stored, and CLAMP_1 as the level setup (0x240aa8) builds it from the stored pair
// (low word WMS, high word << 2 WMT).
struct PrimWords {
    std::uint64_t tex0 = 0;
    std::uint64_t clamp = 0;
};
PrimWords class_prim_words(std::span<const std::byte> cls, int slot) {
    PrimWords w;
    const std::uint32_t prims = get<std::uint32_t>(cls, tie::kClassPrims);
    if (prims == 0 || slot < 0) return w;
    const std::size_t at = prims + static_cast<std::size_t>(slot) * 0x50;
    if (at + 0x50 > cls.size()) return w;
    for (int r = 0; r < 5; ++r) {
        const std::size_t q = at + static_cast<std::size_t>(r) * 16;
        const std::int32_t lo = get<std::int32_t>(cls, q), hi = get<std::int32_t>(cls, q + 4);
        switch (get<std::uint8_t>(cls, q + 8)) {
        case 0x06: w.tex0 = static_cast<std::uint64_t>(hi) << 32 | static_cast<std::uint32_t>(lo); break;
        case 0x08: w.clamp = static_cast<std::uint64_t>(static_cast<std::uint32_t>(lo)) |
                             static_cast<std::uint64_t>(static_cast<std::uint32_t>(hi)) << 2; break;
        default: break;
        }
    }
    return w;
}

} // namespace

DecodedTieModel decode_tie_class_model(
    std::int32_t o_class,
    std::span<const std::byte> tie_class_data,
    std::span<const std::uint8_t, 16> texture_remap
) {
    const std::span<const std::byte> cls = tie_class_data;
    DecodedTieModel model;
    model.o_class = o_class;
    if (cls.size() < tie::kHeaderBytes) return model;

    const float scale = get<float>(cls, tie::kScale);
    model.scale = scale;
    for (int i = 0; i < 4; ++i) model.bounding_sphere[i] = get<float>(cls, tie::kSphere + 4 * i);

    // The submeshes, one per (level texture, CLAMP_1): key = texture | clamp << 32. The map's
    // iteration order is the submeshes' order.
    std::unordered_map<std::uint64_t, std::vector<std::array<int, 3>>> by_key;
    std::unordered_map<std::uint64_t, std::uint64_t> tex0_of;

    // Only the most detailed LOD (the first packet table) is decoded.
    const std::size_t table = static_cast<std::size_t>(get<std::int32_t>(cls, tie::kLodTables));
    const std::size_t packets = get<std::uint8_t>(cls, tie::kLodPacketCounts);
    if (table > 0 && table + packets * 16 <= cls.size()) {
        for (std::size_t p = 0; p < packets; ++p) {
            for (const tie::Strip& strip : tie::packet_strips(cls, table, table + p * 16, {})) {
                int texture = 0;
                if (strip.texture_slot >= 0 && strip.texture_slot < 16) {
                    texture = texture_remap[strip.texture_slot];
                    if (texture == 0xff) texture = 0;
                }
                const PrimWords gs = class_prim_words(cls, strip.texture_slot);
                const std::uint64_t key = static_cast<std::uint32_t>(texture) | gs.clamp << 32;
                tex0_of[key] = gs.tex0;

                const int first = static_cast<int>(model.vertices.size());
                for (std::size_t k = 0; k < strip.corners.size(); ++k) {
                    const tie::Corner& c = strip.corners[k];
                    DecodedVertex v{};
                    v.x = c.x * (scale / 1024.0f);
                    v.y = c.y * (scale / 1024.0f);
                    v.z = c.z * (scale / 1024.0f);
                    v.u = c.s * (1.0f / 4096.0f);
                    v.v = c.t * (1.0f / 4096.0f);
                    v.nz = 1.0f;
                    v.r = v.g = v.b = v.a = 255;
                    model.vertices.push_back(v);
                    if (k < 2) continue;
                    const int n = first + static_cast<int>(k);
                    // Alternate corners flip the strip's facing; the winding flag says which.
                    if (static_cast<int>(k % 2) == (strip.odd_winding ? 1 : 0)) {
                        by_key[key].push_back({n - 2, n - 1, n});
                    } else {
                        by_key[key].push_back({n, n - 1, n - 2});
                    }
                }
            }
        }
    }

    for (const auto& [key, tris] : by_key) {
        DecodedSubMesh sm;
        sm.material_index = static_cast<int>(static_cast<std::uint32_t>(key));
        sm.clamp = key >> 32;
        sm.triangles = tris;
        sm.tex0 = tex0_of[key];
        model.submeshes.push_back(std::move(sm));
    }
    return model;
}

} // namespace rac1
