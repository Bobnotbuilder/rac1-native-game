// The loading screen's scene for the renderer: the ship's flight between planets (see scene_export.h).
//
// The scene has its own container on the disc, found through the table of contents (sector 1500) at
// +0x13b8 (its first sector, then its sector count), as the level exit registers it before the
// loading screen starts. Once decompressed, its header H holds:
//   H[+0x00]  the GS RAM block (palettes, and mips 2 and 3), in units of 256 bytes from its start
//   H[+0x04]  the base the class offsets are relative to
//   H[+0x10]  the number of moby classes, records of 0x20 bytes at H + H[+0x14]
//             (the class at base + the record's +0, its class id at +4, its texture remap at +0x10)
//   H[+0x18]  the number of moby textures, records of 16 bytes at H + H[+0x1c]
//   H[+0x30]  the texels, relative to the base (mip 1 follows mip 0)
// The title scene has the same shape with other offsets (its moby part is exported by the moby step,
// mrp_export.cpp). The loading screen's drawing runs no tfrag, tie or shrub pass: its world is
// empty, and the renderer draws only its sky and its mobys.
#include "scene_export.h"
#include "kick_trace.h"

#include <formats/level_file.h>
#include <formats/moby_mrp.h>
#include <formats/scene_wad.h>
#include <formats/world_mrp.h>
#include <game/render/mrp_world_assets.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace rac1::setup {

namespace {

constexpr std::size_t SECTOR = 0x800;
constexpr std::size_t TOC = 1500 * SECTOR;

// Where a scene's parts are: its table of contents entry, and the words of its header.
struct SceneLayout {
    const char* name;  // the output directory
    uint32_t toc;      // the table of contents' entry (first sector, sector count)
    uint32_t classes_n, classes_at, tex_n, tex_at, pixels_at;
};
constexpr SceneLayout kFlightLayout = {"flight", 0x13b8, 0x10, 0x14, 0x18, 0x1c, 0x30};

int export_scene_mobys(std::span<const std::byte> iso, const std::string& out_dir, const SceneLayout& L, bool force) {
    const char* const name = L.name;
    const std::filesystem::path dir = std::filesystem::path(out_dir) / name;
    if (!force && std::filesystem::exists(dir / "textures.mrpt")) return 0;

    uint32_t lba = 0, sectors = 0;
    if (TOC + L.toc + 8 > iso.size()) return 1;
    std::memcpy(&lba, iso.data() + TOC + L.toc, 4);
    std::memcpy(&sectors, iso.data() + TOC + L.toc + 4, 4);
    std::vector<uint8_t> H;
    if (sectors == 0 || static_cast<size_t>(lba + sectors) * SECTOR > iso.size() ||
        !rac1::decompress_wad(std::span<const std::byte>(iso.data() + static_cast<size_t>(lba) * SECTOR, static_cast<size_t>(sectors) * SECTOR), H) || H.size() < 0x90) {
        std::cerr << name << ": cannot read its data\n";
        return 1;
    }
    auto rd = [&H](size_t at) {
        uint32_t v = 0;
        if (at + 4 <= H.size()) std::memcpy(&v, H.data() + at, 4);
        return v;
    };
    const size_t s5 = rd(4), s4 = s5 + rd(L.pixels_at), gs = rd(0);
    const uint32_t ntex = rd(L.tex_n), texl = rd(L.tex_at);
    std::span<const uint8_t> tex_table;
    if (static_cast<size_t>(texl) + static_cast<size_t>(ntex) * 16 <= H.size())
        tex_table = std::span<const uint8_t>(H.data() + texl, static_cast<size_t>(ntex) * 16);
    std::filesystem::create_directories(dir);
    int failed = 0;
    for (uint32_t i = 0; i < rd(L.classes_n); ++i) {
        const size_t r = rd(L.classes_at) + static_cast<size_t>(i) * 0x20;
        const uint32_t off = rd(r);
        const int32_t o_class = static_cast<int32_t>(rd(r + 4));
        if (off == 0 || s5 + off >= H.size()) continue;
        rac1::mrp::MobyClass c;
        c.level = -1;
        c.o_class = o_class;
        if (r + 0x20 <= H.size()) std::memcpy(c.texture_remap.data(), H.data() + r + 0x10, 16);
        std::string err;
        if (!rac1::mrp::decode_class(std::span<const uint8_t>(H).subspan(s5 + off), 0, c, err, tex_table)) {
            std::cerr << name << " class " << o_class << ": " << err << "\n";
            ++failed;
            continue;
        }
        for (auto& pk : c.packets) {
            std::string note;
            if (pk.lod != 2 && !rac1::mrp::trace_kicks(pk, note)) std::cerr << name << " class " << o_class << ": kick trace: " << note << "\n";
        }
        if (!rac1::mrp::write_class((dir / (std::to_string(o_class) + ".mrpm")).string(), c)) ++failed;
    }
    std::vector<rac1::mrp::Texture> out;
    for (uint32_t i = 0; i < ntex && !tex_table.empty(); ++i) {
        rac1::TextureRecord te{};
        std::memcpy(&te, tex_table.data() + static_cast<size_t>(i) * 16, 16);
        rac1::mrp::Texture t;
        t.index = i;
        t.width = static_cast<uint32_t>(te.width);
        t.height = static_cast<uint32_t>(te.height);
        const size_t pal = gs + static_cast<size_t>(te.palette) * 256;
        if (pal + 1024 <= H.size()) std::memcpy(t.palette.data(), H.data() + pal, 1024);
        size_t px = s4 + static_cast<size_t>(te.texels);
        uint32_t w = t.width, h = t.height;
        for (int m = 0; m < 2 && w > 0 && h > 0; ++m) {
            if (px + static_cast<size_t>(w) * h > H.size()) break;
            t.mips.emplace_back(H.begin() + static_cast<long>(px), H.begin() + static_cast<long>(px + static_cast<size_t>(w) * h));
            px += static_cast<size_t>(w) * h;
            w /= 2;
            h /= 2;
        }
        const int16_t at2[2] = {te.mip1_page, te.mip2_page};
        for (int m = 0; m < 2 && t.mips.size() == static_cast<size_t>(2 + m) && w > 0 && h > 0 && at2[m] > 0; ++m) {
            const size_t o = gs + static_cast<size_t>(at2[m]) * 256;
            if (o + static_cast<size_t>(w) * h > H.size()) break;
            t.mips.emplace_back(H.begin() + static_cast<long>(o), H.begin() + static_cast<long>(o + static_cast<size_t>(w) * h));
            w /= 2;
            h /= 2;
        }
        out.push_back(std::move(t));
    }
    if (!rac1::mrp::write_textures((dir / "textures.mrpt").string(), out)) ++failed;
    return failed == 0 ? 0 : 1;
}

// An empty world and empty texture tables, so that the renderer's world runs (and draws the sky)
// while the loading screen shows.
int export_flight_world(const std::string& out_dir, bool force) {
    const std::filesystem::path dir = std::filesystem::path(out_dir) / rac1::mrpw::level_dir_name(rac1::mrpw::kFlightLevel);
    if (!force && std::filesystem::exists(dir / "world.mrpw")) return 0;
    std::filesystem::create_directories(dir);
    const rac1::mrpw::World w;
    int failed = rac1::mrpw::write_world((dir / "world.mrpw").string(), w) ? 0 : 1;
    for (const char* t : {"tfrag", "tie", "shrub"})
        if (!rac1::mrp::write_textures((dir / (std::string("textures_") + t + ".mrpt")).string(), {})) ++failed;
    return failed == 0 ? 0 : 1;
}

} // namespace

bool export_flight(std::span<const std::byte> iso, const std::string& moby_dir, const std::string& world_dir, bool force) {
    const int failed = export_scene_mobys(iso, moby_dir, kFlightLayout, force) + export_flight_world(world_dir, force);
    return failed == 0;
}

} // namespace rac1::setup
