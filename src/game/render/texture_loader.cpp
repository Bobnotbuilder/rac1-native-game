// orig: 0x00202d78
#include <game/render/texture_loader.h>
#include <runtime/level/host_layout.h>
#include <runtime/level/level_loader.h>
#include <runtime/kernel/ee_memory.h>
#include <formats/level_core.h>
#include <formats/level_textures.h>

#include <fstream>
#include <vector>
#include <cstring>
#include <iostream>
#include <bit>
#include <span>
#include <string>

namespace rac1 {

extern std::uint32_t g_core_data_vaddr;

// orig: 0x00202d78
void texture_loader(const std::filesystem::path& level_wad_path) {
    // 1. Read level WAD file from the host filesystem
    std::ifstream file(level_wad_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("texture_loader: failed to open level WAD file: " + level_wad_path.string());
    }

    const std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::byte> file_data(file_size);
    if (!file.read(reinterpret_cast<char*>(file_data.data()), file_size)) {
        throw std::runtime_error("texture_loader: failed to read level WAD file: " + level_wad_path.string());
    }
    file.close();

    // 2. Locate the inner WAD block (level_data)
    std::span<const std::byte> level_data(file_data.data(), file_data.size());
    if (file_data.size() >= 48) {
        std::int32_t header_size = *reinterpret_cast<const std::int32_t*>(file_data.data());
        if (header_size == 48) {
            std::int32_t data_sector_offset = *reinterpret_cast<const std::int32_t*>(file_data.data() + 0x10);
            std::int32_t data_sector_size = *reinterpret_cast<const std::int32_t*>(file_data.data() + 0x14);
            std::size_t byte_offset = static_cast<std::size_t>(data_sector_offset) * 2048;
            std::size_t byte_size = static_cast<std::size_t>(data_sector_size) * 2048;
            if (byte_offset + byte_size <= file_data.size()) {
                level_data = std::span<const std::byte>(file_data.data() + byte_offset, byte_size);
            }
        }
    }

    if (level_data.size() < sizeof(LevelFileHeader)) {
        throw std::runtime_error("texture_loader: level WAD data too small to contain header");
    }

    // 3. Parse LevelFileHeader
    LevelFileHeader wad_header;
    std::memcpy(&wad_header, level_data.data(), sizeof(LevelFileHeader));

    // Check bounds for gs_ram segment
    const bool invalid_gs_ram = wad_header.vram_image.offset < 0 || wad_header.vram_image.size < 0 ||
        static_cast<std::size_t>(wad_header.vram_image.offset + wad_header.vram_image.size) > level_data.size();
    if (invalid_gs_ram) {
        throw std::runtime_error("texture_loader: gs_ram segment out of bounds in level WAD");
    }

    // 4. Read AssetIndexHeader from virtual EE RAM at 0x004d4000
    AssetIndexHeader core_header = ee_read<AssetIndexHeader>(host_layout::CORE);

    // 5. Initialize the heap pointer starting at 0x01500000
    std::uint32_t heap_ptr = host_layout::TEX_HEAP;
    std::uint32_t core_data_vaddr = host_layout::CORE + wad_header.asset_index.size;
    g_core_data_vaddr = core_data_vaddr;

    auto register_texture = [&](const std::string& category, int index, int width, int height, int type, int pad,
                                const std::uint8_t* pixel_src, const std::uint8_t* palette_src,
                                std::uint32_t table_base_vaddr) -> std::uint32_t {
        // Align heap pointer to 16 bytes
        heap_ptr = (heap_ptr + 15) & ~15;

        std::uint32_t header_vaddr = heap_ptr;
        std::uint32_t palette_vaddr = header_vaddr + sizeof(TextureHeader);
        std::uint32_t pixel_vaddr = palette_vaddr + 1024;
        std::size_t num_pixels = static_cast<std::size_t>(width) * height;

        // Check if layout exceeds virtual RAM bounds
        if (!ee_is_valid_addr(header_vaddr, sizeof(TextureHeader) + 1024 + num_pixels)) {
            throw std::runtime_error("texture_loader: texture allocation exceeds virtual RAM bounds during registration of " + category + " index " + std::to_string(index));
        }

        // TextureHeader
        TextureHeader tex_hdr;
        tex_hdr.palette_addr = palette_vaddr;
        tex_hdr.width = static_cast<std::uint16_t>(width);
        tex_hdr.height = static_cast<std::uint16_t>(height);
        tex_hdr.type = static_cast<std::uint16_t>(type);
        tex_hdr.pad = static_cast<std::uint16_t>(pad);

        // Write to EE RAM
        ee_write<TextureHeader>(header_vaddr, tex_hdr);

        // Copy palette (1024 bytes)
        void* dest_palette = ee_ram_ptr(palette_vaddr);
        if (palette_src) {
            std::memcpy(dest_palette, palette_src, 1024);
        } else {
            std::memset(dest_palette, 0, 1024);
        }

        // Copy pixels
        void* dest_pixels = ee_ram_ptr(pixel_vaddr);
        if (pixel_src) {
            std::memcpy(dest_pixels, pixel_src, num_pixels);
        } else {
            std::memset(dest_pixels, 0, num_pixels);
        }

        // Advance heap_ptr
        heap_ptr = pixel_vaddr + num_pixels;

        // Compute packed pointer
        std::uint32_t log2_val = 0;
        if (width > 0) {
            log2_val = 0x1e - std::countl_zero(static_cast<std::uint32_t>(width));
        }
        std::uint32_t packed_ptr = (log2_val << 28) | pixel_vaddr;

        // Register in global table
        if (table_base_vaddr != 0) {
            ee_write<std::uint32_t>(table_base_vaddr + index * 4, packed_ptr);
        }
        if (category == "particle" && index >= 0 && index < 256) {
            g_particle_texture_pixel_vaddrs[index] = pixel_vaddr;
        }

        return packed_ptr;
    };

    // --- Tfrag Textures ---
    if (core_header.tfrag_textures.count > 0 && core_header.tfrag_textures.offset > 0) {
        std::uint32_t array_vaddr = host_layout::CORE + core_header.tfrag_textures.offset;
        for (int i = 0; i < core_header.tfrag_textures.count; ++i) {
            TextureRecord entry = ee_read<TextureRecord>(array_vaddr + i * sizeof(TextureRecord));
            std::uint32_t pixel_src_vaddr = core_data_vaddr + core_header.texel_block + entry.texels;
            const std::uint8_t* pixel_src = nullptr;
            if (ee_is_valid_addr(pixel_src_vaddr, entry.width * entry.height)) {
                pixel_src = reinterpret_cast<const std::uint8_t*>(ee_ram_ptr(pixel_src_vaddr));
            }
            const std::uint8_t* palette_src = nullptr;
            std::size_t pal_offset = static_cast<std::size_t>(entry.palette) * 256;
            if (pal_offset + 1024 <= static_cast<std::size_t>(wad_header.vram_image.size)) {
                palette_src = reinterpret_cast<const std::uint8_t*>(level_data.data()) + wad_header.vram_image.offset + pal_offset;
            }
            register_texture("tfrag", i, entry.width, entry.height, entry.mip_count, entry.mip2_page, pixel_src, palette_src, host_layout::TEX_TABLE_TFRAG);
        }
    }

    // --- Moby Textures ---
    if (core_header.moby_textures.count > 0 && core_header.moby_textures.offset > 0) {
        std::uint32_t array_vaddr = host_layout::CORE + core_header.moby_textures.offset;
        for (int i = 0; i < core_header.moby_textures.count; ++i) {
            TextureRecord entry = ee_read<TextureRecord>(array_vaddr + i * sizeof(TextureRecord));
            std::uint32_t pixel_src_vaddr = core_data_vaddr + core_header.texel_block + entry.texels;
            const std::uint8_t* pixel_src = nullptr;
            if (ee_is_valid_addr(pixel_src_vaddr, entry.width * entry.height)) {
                pixel_src = reinterpret_cast<const std::uint8_t*>(ee_ram_ptr(pixel_src_vaddr));
            }
            const std::uint8_t* palette_src = nullptr;
            std::size_t pal_offset = static_cast<std::size_t>(entry.palette) * 256;
            if (pal_offset + 1024 <= static_cast<std::size_t>(wad_header.vram_image.size)) {
                palette_src = reinterpret_cast<const std::uint8_t*>(level_data.data()) + wad_header.vram_image.offset + pal_offset;
            }
            register_texture("moby", i, entry.width, entry.height, entry.mip_count, entry.mip2_page, pixel_src, palette_src, host_layout::TEX_TABLE_MOBY);
        }
    }

    // --- TIE Textures ---
    if (core_header.tie_textures.count > 0 && core_header.tie_textures.offset > 0) {
        std::uint32_t array_vaddr = host_layout::CORE + core_header.tie_textures.offset;
        for (int i = 0; i < core_header.tie_textures.count; ++i) {
            TextureRecord entry = ee_read<TextureRecord>(array_vaddr + i * sizeof(TextureRecord));
            std::uint32_t pixel_src_vaddr = core_data_vaddr + core_header.texel_block + entry.texels;
            const std::uint8_t* pixel_src = nullptr;
            if (ee_is_valid_addr(pixel_src_vaddr, entry.width * entry.height)) {
                pixel_src = reinterpret_cast<const std::uint8_t*>(ee_ram_ptr(pixel_src_vaddr));
            }
            const std::uint8_t* palette_src = nullptr;
            std::size_t pal_offset = static_cast<std::size_t>(entry.palette) * 256;
            if (pal_offset + 1024 <= static_cast<std::size_t>(wad_header.vram_image.size)) {
                palette_src = reinterpret_cast<const std::uint8_t*>(level_data.data()) + wad_header.vram_image.offset + pal_offset;
            }
            register_texture("tie", i, entry.width, entry.height, entry.mip_count, entry.mip2_page, pixel_src, palette_src, host_layout::TEX_TABLE_TIE);
        }
    }

    // --- Shrub Textures ---
    if (core_header.shrub_textures.count > 0 && core_header.shrub_textures.offset > 0) {
        std::uint32_t array_vaddr = host_layout::CORE + core_header.shrub_textures.offset;
        for (int i = 0; i < core_header.shrub_textures.count; ++i) {
            TextureRecord entry = ee_read<TextureRecord>(array_vaddr + i * sizeof(TextureRecord));
            std::uint32_t pixel_src_vaddr = core_data_vaddr + core_header.texel_block + entry.texels;
            const std::uint8_t* pixel_src = nullptr;
            if (ee_is_valid_addr(pixel_src_vaddr, entry.width * entry.height)) {
                pixel_src = reinterpret_cast<const std::uint8_t*>(ee_ram_ptr(pixel_src_vaddr));
            }
            const std::uint8_t* palette_src = nullptr;
            std::size_t pal_offset = static_cast<std::size_t>(entry.palette) * 256;
            if (pal_offset + 1024 <= static_cast<std::size_t>(wad_header.vram_image.size)) {
                palette_src = reinterpret_cast<const std::uint8_t*>(level_data.data()) + wad_header.vram_image.offset + pal_offset;
            }
            register_texture("shrub", i, entry.width, entry.height, entry.mip_count, entry.mip2_page, pixel_src, palette_src, host_layout::TEX_TABLE_SHRUB);
        }
    }

    // --- Particle Textures ---
    if (core_header.particle_textures.count > 0 && core_header.particle_textures.offset > 0) {
        std::uint32_t array_vaddr = host_layout::CORE + core_header.particle_textures.offset;
        for (int i = 0; i < core_header.particle_textures.count; ++i) {
            ParticleTextureRecord entry = ee_read<ParticleTextureRecord>(array_vaddr + i * sizeof(ParticleTextureRecord));
            std::uint32_t pixel_src_vaddr = core_data_vaddr + core_header.particle_block + entry.texels;
            const std::uint8_t* pixel_src = nullptr;
            if (ee_is_valid_addr(pixel_src_vaddr, entry.side * entry.side)) {
                pixel_src = reinterpret_cast<const std::uint8_t*>(ee_ram_ptr(pixel_src_vaddr));
            }
            std::uint32_t palette_src_vaddr = core_data_vaddr + core_header.particle_block + entry.palette;
            const std::uint8_t* palette_src = nullptr;
            if (ee_is_valid_addr(palette_src_vaddr, 1024)) {
                palette_src = reinterpret_cast<const std::uint8_t*>(ee_ram_ptr(palette_src_vaddr));
            }
            register_texture("particle", i, entry.side, entry.side, 0, 0, pixel_src, palette_src, 0);
        }
    }

    // --- FX Textures ---
    if (core_header.effect_textures.count > 0 && core_header.effect_textures.offset > 0) {
        std::uint32_t array_vaddr = host_layout::CORE + core_header.effect_textures.offset;
        for (int i = 0; i < core_header.effect_textures.count; ++i) {
            EffectTextureRecord entry = ee_read<EffectTextureRecord>(array_vaddr + i * sizeof(EffectTextureRecord));
            std::uint32_t pixel_src_vaddr = core_data_vaddr + core_header.effect_block + entry.texels;
            const std::uint8_t* pixel_src = nullptr;
            if (ee_is_valid_addr(pixel_src_vaddr, entry.width * entry.height)) {
                pixel_src = reinterpret_cast<const std::uint8_t*>(ee_ram_ptr(pixel_src_vaddr));
            }
            std::uint32_t palette_src_vaddr = core_data_vaddr + core_header.effect_block + entry.palette;
            const std::uint8_t* palette_src = nullptr;
            if (ee_is_valid_addr(palette_src_vaddr, 1024)) {
                palette_src = reinterpret_cast<const std::uint8_t*>(ee_ram_ptr(palette_src_vaddr));
            }
            register_texture("fx", i, entry.width, entry.height, 0, 0, pixel_src, palette_src, 0);
        }
    }

    std::cout << "[texture_loader] Registered all level textures faithfully in virtual EE RAM." << std::endl;
}

// Initializer removed — texture_loader is invoked directly from
// runtime_host main.cpp after LevelLoader::load_level_wad. No callback
// registration global exists for textures (unlike TIE).

} // namespace rac1
