#include <formats/level_collision.h>
#include <formats/level_file.h>
#include <formats/scene_wad.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace rac1 {

namespace {

// Bounds-checked little-endian reads from the collision data; a read past the end throws.
class Reader {
public:
    explicit Reader(std::span<const std::byte> b) : b_(b) {}
    template <class T>
    T at(std::size_t pos) const {
        if (pos + sizeof(T) > b_.size()) throw std::runtime_error("Out of bounds read at offset " + std::to_string(pos));
        T v;
        std::memcpy(&v, b_.data() + pos, sizeof(T));
        return v;
    }
    std::size_t size() const { return b_.size(); }

private:
    std::span<const std::byte> b_;
};

// The 4 x 4 x 4 cell (x, y, z) of the grid: its contents at `cell`.
DecodedCollisionOctant read_cell(const Reader& r, std::uint32_t cell, std::uint32_t cell_rel, std::int32_t x, std::int32_t y,
                                 std::int32_t z) {
    DecodedCollisionOctant out;
    out.offset = static_cast<std::int32_t>(cell_rel);
    out.displacement = {x * 4.0f + 2.0f, y * 4.0f + 2.0f, z * 4.0f + 2.0f};
    const std::uint16_t faces = r.at<std::uint16_t>(cell);
    const std::uint8_t points = r.at<std::uint8_t>(cell + 2);
    const std::uint8_t quads = r.at<std::uint8_t>(cell + 3);
    std::uint32_t p = cell + 4;

    // A point: one word, x in bits 0..9 and y in 10..19 (signed, 1/16), z in 20..31 (signed, 1/64),
    // relative to the cell's centre.
    out.vertices.reserve(points);
    for (int i = 0; i < points; ++i, p += 4) {
        const std::uint32_t w = r.at<std::uint32_t>(p);
        const std::int32_t px = static_cast<std::int32_t>(w << 22) >> 22;
        const std::int32_t py = static_cast<std::int32_t>(w << 12) >> 22;
        const std::int32_t pz = static_cast<std::int32_t>(w) >> 20;
        out.vertices.push_back({static_cast<float>(px) / 16.0f, static_cast<float>(py) / 16.0f, static_cast<float>(pz) / 64.0f});
    }
    // A face: three point indices and the surface type; the first `quads` faces then get a fourth
    // point, one byte each.
    out.faces.reserve(faces);
    for (int i = 0; i < faces; ++i, p += 4) {
        out.faces.push_back({r.at<std::uint8_t>(p), r.at<std::uint8_t>(p + 1), r.at<std::uint8_t>(p + 2), 0,
                             r.at<std::uint8_t>(p + 3), false});
    }
    for (int i = 0; i < quads; ++i) {
        if (static_cast<std::size_t>(i) >= out.faces.size()) continue;
        out.faces[static_cast<std::size_t>(i)].v3 = r.at<std::uint8_t>(p);
        out.faces[static_cast<std::size_t>(i)].is_quad = true;
        ++p;
    }
    return out;
}

// The grid: z rows of y rows of x cells, each row (first coordinate s16, count u16, then the
// entries). z entries are u16 offsets / 4, y entries u32 offsets, x entries u32 (cell offset << 8
// | cell size / 16); all from the grid's start, 0 for an empty slot.
void read_grid(const Reader& r, std::uint32_t grid, DecodedCollision& out) {
    const std::int16_t z0 = r.at<std::int16_t>(grid);
    const std::uint16_t zn = r.at<std::uint16_t>(grid + 2);
    out.z_node_count = 1;
    out.max_depth = 1;
    for (std::uint16_t zi = 0; zi < zn; ++zi) {
        const std::uint32_t zrel = r.at<std::uint16_t>(grid + 4 + zi * 2);
        if (zrel == 0) continue;
        const std::uint32_t zrow = grid + zrel * 4;
        const std::int16_t y0 = r.at<std::int16_t>(zrow);
        const std::uint16_t yn = r.at<std::uint16_t>(zrow + 2);
        ++out.y_node_count;
        out.max_depth = std::max(out.max_depth, 2u);
        for (std::uint16_t yi = 0; yi < yn; ++yi) {
            const std::uint32_t yrel = r.at<std::uint32_t>(zrow + 4 + yi * 4);
            if (yrel == 0) continue;
            const std::uint32_t yrow = grid + yrel;
            const std::int16_t x0 = r.at<std::int16_t>(yrow);
            const std::uint16_t xn = r.at<std::uint16_t>(yrow + 2);
            ++out.x_node_count;
            out.max_depth = std::max(out.max_depth, 3u);
            for (std::uint16_t xi = 0; xi < xn; ++xi) {
                const std::uint32_t cell_rel = r.at<std::uint32_t>(yrow + 4 + xi * 4) >> 8;
                if (cell_rel == 0) continue;
                ++out.octant_count;
                out.octants.push_back(read_cell(r, grid + cell_rel, cell_rel, x0 + xi, y0 + yi, z0 + zi));
            }
        }
    }
}

// The hero's collision groups: a count (s32), 12 bytes, then 16-byte records: sphere (4 x s16,
// the radius unsigned, 1/64), triangle count, point count (u16), the data's offset (u32) from the
// block's start. The data: points (3 x s16 1/64, 2 bytes), then triangles (3 x u8, 1 byte).
void read_hero_groups(const Reader& r, std::uint32_t block, DecodedCollision& out) {
    if (block + 4 > r.size()) return;
    const std::int32_t count = r.at<std::int32_t>(block);
    if (count <= 0 || block + 0x10 + static_cast<std::size_t>(count) * 16 > r.size()) return;
    for (std::int32_t i = 0; i < count; ++i) {
        const std::uint32_t rec = block + 0x10 + i * 16;
        DecodedHeroCollisionGroup g;
        g.bsphere_x = static_cast<float>(r.at<std::int16_t>(rec)) / 64.0f;
        g.bsphere_y = static_cast<float>(r.at<std::int16_t>(rec + 2)) / 64.0f;
        g.bsphere_z = static_cast<float>(r.at<std::int16_t>(rec + 4)) / 64.0f;
        g.bsphere_radius = static_cast<float>(r.at<std::uint16_t>(rec + 6)) / 64.0f;
        const std::uint16_t triangles = r.at<std::uint16_t>(rec + 8);
        const std::uint16_t points = r.at<std::uint16_t>(rec + 10);
        std::uint64_t p = static_cast<std::uint64_t>(block) + r.at<std::uint32_t>(rec + 12);
        if (p + std::uint64_t{points} * 8 + std::uint64_t{triangles} * 4 > r.size()) {
            throw std::runtime_error("Hero collision group offset/size out of bounds");
        }
        g.vertices.reserve(points);
        for (int k = 0; k < points; ++k, p += 8) {
            g.vertices.push_back({static_cast<float>(r.at<std::int16_t>(p)) / 64.0f, static_cast<float>(r.at<std::int16_t>(p + 2)) / 64.0f,
                                  static_cast<float>(r.at<std::int16_t>(p + 4)) / 64.0f});
        }
        g.triangles.reserve(triangles);
        for (int k = 0; k < triangles; ++k, p += 4) {
            g.triangles.push_back({r.at<std::uint8_t>(p), r.at<std::uint8_t>(p + 1), r.at<std::uint8_t>(p + 2)});
        }
        out.hero_groups.push_back(std::move(g));
    }
}

template <class T>
std::vector<T> rows_or_zeros(std::span<const std::byte> index, std::int64_t at, std::int64_t count) {
    std::vector<T> rows(static_cast<std::size_t>(count));
    const std::size_t first = static_cast<std::size_t>(at), bytes = static_cast<std::size_t>(count) * sizeof(T);
    if (first + bytes <= index.size()) std::memcpy(rows.data(), index.data() + first, bytes);
    return rows;
}

// Where the collision block ends: the nearest start of another part of the asset data the index
// names (the moby sound remap included); (0, 0) when there is no collision or nothing after it.
FileSpan collision_span(std::span<const std::byte> index, const AssetIndexHeader& h) {
    if (h.collision_block <= 0) return {0, 0};
    std::vector<std::int64_t> starts = {h.tfrag_block, h.occlusion_block, h.sky_block, h.collision_block, h.texel_block, h.unpacked_size};
    auto add_classes = [&](TableRef ref, auto row_type) {
        using Row = decltype(row_type);
        if (ref.offset <= 0 || ref.count <= 0) return;
        for (const Row& c : rows_or_zeros<Row>(index, ref.offset, ref.count)) if (c.data > 0) starts.push_back(c.data);
    };
    add_classes(h.moby_classes, ClassRecord{});
    add_classes(h.tie_classes, ClassRecord{});
    add_classes(h.shrub_classes, ShrubClassRecord{});
    if (h.moby_sound_remap > 0) starts.push_back(h.moby_sound_remap);
    if (h.hero_sequences > 0) {
        for (std::int32_t s : rows_or_zeros<std::int32_t>(index, h.hero_sequences, 256)) if (s > 0) starts.push_back(s);
    }
    if (h.gadget_table > 0 && h.gadget_count > 0) {
        for (const GadgetRecord& g : rows_or_zeros<GadgetRecord>(index, h.gadget_table, h.gadget_count)) {
            if (g.data > 0) starts.push_back(g.data);
        }
    }
    std::int64_t next = -1;
    for (std::int64_t s : starts) {
        if (s > h.collision_block && (next == -1 || s < next)) next = s;
    }
    if (next == -1) return {0, 0};
    return {h.collision_block, static_cast<std::int32_t>(next - h.collision_block)};
}

} // namespace

DecodedCollision decode_level_collision(std::span<const std::byte> data) {
    std::vector<std::uint8_t> unpacked;
    std::span<const std::byte> bytes = data;
    if (data.size() >= 3 && std::memcmp(data.data(), "WAD", 3) == 0) {
        if (!decompress_wad(data, unpacked)) throw std::runtime_error("Failed to decompress collision WAD");
        bytes = std::span<const std::byte>(reinterpret_cast<const std::byte*>(unpacked.data()), unpacked.size());
    }
    if (bytes.size() < 8) throw std::runtime_error("Collision data too small for its header");

    // The block's head: the grid's offset, the hero groups' offset (0: none).
    const Reader r(bytes);
    DecodedCollision out;
    out.mesh_offset = r.at<std::int32_t>(0);
    out.hero_groups_offset = r.at<std::int32_t>(4);
    if (out.mesh_offset > 0 && static_cast<std::size_t>(out.mesh_offset) < bytes.size()) {
        read_grid(r, static_cast<std::uint32_t>(out.mesh_offset), out);
    }
    if (out.hero_groups_offset > 0 && static_cast<std::size_t>(out.hero_groups_offset) < bytes.size()) {
        read_hero_groups(r, static_cast<std::uint32_t>(out.hero_groups_offset), out);
    }
    return out;
}

bool extract_collision_from_level_wad(
    std::span<const std::byte> level_wad_data,
    std::vector<std::uint8_t>& out_collision_blob,
    std::uint32_t& out_offset,
    std::uint32_t& out_size,
    bool& out_was_compressed
) {
    const std::span<const std::byte> file = level_file_body(level_wad_data);
    if (file.size() < sizeof(LevelFileHeader)) return false;
    LevelFileHeader fh;
    std::memcpy(&fh, file.data(), sizeof fh);
    for (const FileSpan& s : {fh.asset_index, fh.asset_data}) {
        if (s.offset < 0 || s.size <= 0 || static_cast<std::size_t>(s.offset + s.size) > file.size()) return false;
    }
    const std::span<const std::byte> index = file.subspan(fh.asset_index.offset, fh.asset_index.size);
    std::vector<std::uint8_t> data;
    if (!decompress_wad(file.subspan(fh.asset_data.offset, fh.asset_data.size), data)) return false;
    if (index.size() < sizeof(AssetIndexHeader)) return false;
    AssetIndexHeader h;
    std::memcpy(&h, index.data(), sizeof h);

    const FileSpan span = collision_span(index, h);
    out_offset = static_cast<std::uint32_t>(span.offset);
    out_size = static_cast<std::uint32_t>(span.size);
    out_was_compressed = false;
    if (span.size <= 0 || span.offset + span.size > static_cast<std::int32_t>(data.size())) return false;

    const auto* first = data.data() + span.offset;
    const std::span<const std::byte> block(reinterpret_cast<const std::byte*>(first), static_cast<std::size_t>(span.size));
    if (block.size() >= 3 && std::memcmp(first, "WAD", 3) == 0) {
        std::vector<std::uint8_t> unpacked;
        if (decompress_wad(block, unpacked)) {
            out_collision_blob = std::move(unpacked);
            out_was_compressed = true;
            return true;
        }
    }
    out_collision_blob.assign(first, first + span.size);
    return true;
}

} // namespace rac1
