// The host renderers' copy of the title's level (see title_scene_host.h).
#include <game/render/title_scene_host.h>

#include <game/ovl/ovl_reloc.h>
#include <game/render/render_toggles.h>
#include <game/render/shrub_render.h>
#include <game/render/sky_render.h>
#include <game/render/tie_render.h>
#include <formats/level_textures.h>
#include <runtime/gs/gs_renderer.h>
#include <runtime/gs/static_geom_cache.h>
#include <runtime/kernel/disc.h>
#include <runtime/kernel/ee_memory.h>
#include <formats/scene_wad.h>
#include <runtime/level/host_layout.h>
#include <runtime/level/level_loader.h>

#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <vector>

namespace rac1 {

namespace {

constexpr std::uint32_t TOC = 0x00137b80;           // the disc's table of contents (base ELF)
constexpr std::uint32_t TITLE_COPY = 0x02d00000;    // the host's pristine copy of the title's level (header H), under the texture heap

bool g_loaded = false;

std::uint32_t lwu(std::uint32_t a) { return ee_read<std::uint32_t>(a); }

// The texture records of one table (H + count_off: count, H + list_off: 16-byte records of the
// level's TextureRecord form) into the host's table at `table`: a header, the 1 KB palette and the
// pixels in the host's texture heap, the packed pointer (log2 width << 28 | pixels) in the table.
// The pixels are at `pixels_base` + the record's offset, the palette at `gs_ram` + palette * 256
// (the level loader's rules: register_level_textures).
// orig: (host) as LevelLoader::register_level_textures for the title's tables
void register_table(std::uint32_t H, std::uint32_t count_off, std::uint32_t list_off, std::uint32_t pixels_base,
                    std::uint32_t gs_ram, std::uint32_t table, std::uint32_t& heap) {
    const std::int32_t n = static_cast<std::int32_t>(lwu(H + count_off));
    const std::uint32_t list = H + lwu(H + list_off);
    for (std::int32_t i = 0; i < n; ++i) {
        const TextureRecord e = ee_read<TextureRecord>(list + static_cast<std::uint32_t>(i) * sizeof(TextureRecord));
        heap = (heap + 15) & ~15u;
        const std::uint32_t hdr = heap, pal = hdr + sizeof(TextureHeader), pix = pal + 1024;
        const std::size_t np = static_cast<std::size_t>(e.width) * static_cast<std::size_t>(e.height);
        if (e.width <= 0 || e.height <= 0 || !ee_is_valid_addr(hdr, static_cast<std::uint32_t>(sizeof(TextureHeader) + 1024 + np))) continue;
        TextureHeader th{};
        th.palette_addr = pal;
        th.width = static_cast<std::uint16_t>(e.width);
        th.height = static_cast<std::uint16_t>(e.height);
        th.type = static_cast<std::uint16_t>(e.mip_count);
        th.pad = static_cast<std::uint16_t>(e.mip2_page);
        ee_write<TextureHeader>(hdr, th);
        const std::uint32_t pal_src = gs_ram + static_cast<std::uint32_t>(e.palette) * 256u;
        const std::uint32_t pix_src = pixels_base + static_cast<std::uint32_t>(e.texels);
        if (ee_is_valid_addr(pal_src, 1024)) std::memcpy(ee_ram_ptr(pal), ee_ram_ptr(pal_src), 1024);
        else std::memset(ee_ram_ptr(pal), 0, 1024);
        if (ee_is_valid_addr(pix_src, static_cast<std::uint32_t>(np))) std::memcpy(ee_ram_ptr(pix), ee_ram_ptr(pix_src), np);
        else std::memset(ee_ram_ptr(pix), 0, np);
        heap = pix + static_cast<std::uint32_t>(np);
        const std::uint32_t log2w = 0x1e - static_cast<std::uint32_t>(std::countl_zero(static_cast<std::uint32_t>(e.width)));
        ee_write<std::uint32_t>(table + static_cast<std::uint32_t>(i) * 4, (log2w << 28) | pix);
    }
}

}  // namespace

// orig: (host) the title level's assets for the host renderers
void title_scene_host_load() {
    // The game relocates the title's data in place (class headers, the sky's tables) as it loads
    // it: the host reads its own pristine copy of the same file, as it does for a level (the
    // level WAD's core): the sectors TOC +0x14e8 read and decompressed into the host's area.
    const std::uint32_t lba = lwu(TOC + 0x14e8), sectors = lwu(TOC + 0x14ec);
    if (sectors == 0 || !ee_is_valid_addr(host_layout::STAGING, sectors << 11)) return;
    if (!disc_read_ee(host_layout::STAGING, lba, sectors)) {
        std::fprintf(stderr, "[title_scene] cannot read the title's level from the disc\n");
        return;
    }
    std::vector<std::uint8_t> level;
    if (!decompress_wad(std::span<const std::byte>(static_cast<const std::byte*>(ee_ram_ptr(host_layout::STAGING)), static_cast<std::size_t>(sectors) << 11), level) ||
        !ee_is_valid_addr(TITLE_COPY, static_cast<std::uint32_t>(level.size())) || TITLE_COPY + level.size() > host_layout::TEX_HEAP) {
        std::fprintf(stderr, "[title_scene] cannot decompress the title's level\n");
        return;
    }
    std::memcpy(ee_ram_ptr(TITLE_COPY), level.data(), level.size());
    const std::uint32_t H = TITLE_COPY;
    const std::uint32_t s5 = H + lwu(H + 4);        // the data base (class headers, models)
    const std::uint32_t s4 = s5 + lwu(H + 0x60);    // the texture pixels' base
    const std::uint32_t gs_ram = H + lwu(H);        // the GS RAM blocks (the palettes)

    gs_texture_cache_clear();
    std::uint32_t heap = host_layout::TEX_HEAP;
    register_table(H, 0x40, 0x44, s4, gs_ram, host_layout::TEX_TABLE_TIE, heap);
    register_table(H, 0x48, 0x4c, s4, gs_ram, host_layout::TEX_TABLE_SHRUB, heap);

    std::vector<HostTieClass> ties;
    {
        std::uint32_t r = H + lwu(H + 0x24);
        for (std::int32_t i = 0; i < static_cast<std::int32_t>(lwu(H + 0x20)); ++i, r += 0x20) {
            if (lwu(r) == 0) continue;
            HostTieClass c{};
            c.vaddr = s5 + lwu(r);
            c.o_class = static_cast<std::int32_t>(lwu(r + 4));
            std::memcpy(c.textures.data(), ee_ram_ptr(r + 0x10), 16);
            ties.push_back(c);
        }
    }
    std::vector<HostShrubClass> shrubs;
    {
        std::uint32_t r = H + lwu(H + 0x2c);
        for (std::int32_t i = 0; i < static_cast<std::int32_t>(lwu(H + 0x28)); ++i, r += 0x30) {
            if (lwu(r) == 0) continue;
            HostShrubClass c{};
            c.vaddr = s5 + lwu(r);
            c.o_class = static_cast<std::int32_t>(lwu(r + 4));
            std::memcpy(c.textures.data(), ee_ram_ptr(r + 0x10), 16);
            shrubs.push_back(c);
        }
    }
    const std::uint32_t blob = s5 + lwu(H + 0x7c);  // the instances block (0x1e9b10)
    tie_scene_load(ties, blob);
    shrub_scene_load(shrubs, blob);
    sky_scene_load(s5 + lwu(H + 0x14));
    static_geom_cache_init();
    g_loaded = true;
    std::printf("[title_scene] ties %zu shrubs %zu classes; sky\n", ties.size(), shrubs.size());
}

// orig: (host) the title scene's static 3D
void title_scene_host_draw() {
    if (!g_loaded) return;
    if (g_render_sky) sky_draw();
    if (g_render_tie) tie_render_update();
    if (g_render_shrub) shrub_render_update();
}

}  // namespace rac1
