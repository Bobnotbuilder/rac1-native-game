// Local-only actual asset preparation check. Does not render a game frame or upload data.
#include <game/render/metal/mrp_texture.hpp>
#include <algorithm>
#include <filesystem>
#include <cstdio>
#include <map>
int main() {
    size_t files=0,textures=0,mips=0,pixels=0,unuploadable=0,buckets=0,strict_rejected_buckets=0,
           rejected_buckets=0,zero_filled_mips=0;
    for (const auto& entry:std::filesystem::recursive_directory_iterator("assets/generated/mrp")) {
        if (!entry.is_regular_file() || entry.path().extension()!=".mrpt") continue;
        std::vector<rac1::mrp::Texture> input;
        if (!rac1::mrp::read_textures(entry.path().string(),input)) return 1;
        ++files;
        std::map<std::pair<uint32_t,uint32_t>,std::vector<const rac1::mrp::Texture*>> groups;
        for (const auto& t:input) {
            ++textures;
            // Match the explicit guard in BOTH original GL MRP bucket builders.
            // Empty/unuploadable format entries are counted, never called decoded or rendered.
            if (!t.width || !t.height || t.mips.empty() ||
                t.mips[0].size()<uint64_t(t.width)*t.height) { ++unuploadable; continue; }
            groups[{t.width,t.height}].push_back(&t);
            for (uint32_t level=0;level<t.mips.size();++level) {
                std::vector<uint8_t> rgba; std::string error;
                if (!rac1::metal::decode_texture_mip(t,level,rgba,error)) {
                    std::fprintf(stderr,"FAIL: MRP texture decode %s (file index %zu, texture index %zu, mip %u)\n",
                                 error.c_str(),files,textures,level); return 1;
                }
                // Independently index the raw source palette and compare every channel.
                for (size_t p=0;p<t.mips[level].size();++p) {
                    const unsigned i=t.mips[level][p];
                    const unsigned csm=(i&~0x18u)|((i&8u)<<1)|((i&16u)>>1);
                    const uint32_t colour=t.palette[csm];
                    for (unsigned c=0;c<4;++c) if (rgba[p*4+c]!=uint8_t(colour>>(c*8))) return 1;
                }
                ++mips; pixels+=rgba.size()/4;
            }
        }
        for (const auto& [dimensions,layers]:groups) {
            uint32_t levels=1;
            for (const auto* t:layers) levels=std::max(levels,uint32_t(std::min(size_t(4),t->mips.size())));
            uint32_t max_level=1;
            while (max_level<levels && (dimensions.first>>max_level)>0 && (dimensions.second>>max_level)>0) ++max_level;
            levels=std::min(levels,max_level); // original GL MRP bucket mip allocation
            rac1::metal::TextureArrayShape shape; std::string error;
            ++buckets;
            if (!rac1::metal::validate_texture_array(layers,levels,shape,error)) ++strict_rejected_buckets;
            if (!rac1::metal::validate_texture_array(layers,levels,shape,error,rac1::metal::MissingMipPolicy::UpstreamZeroFill)) {
                ++rejected_buckets; continue;
            }
            zero_filled_mips+=shape.zero_filled_mips;
            for (const auto* t:layers) for (uint32_t level=uint32_t(t->mips.size());level<levels;++level) {
                std::vector<uint8_t> zero;
                if (!rac1::metal::decode_texture_mip(*t,level,zero,error,rac1::metal::MissingMipPolicy::UpstreamZeroFill)) return 1;
                for (uint8_t value:zero) if (value) return 1;
            }
        }
    }
    if (files!=88 || !textures || !mips || !pixels) return 1;
    std::printf("ACTUAL_TEXTURE_READBACK files=%zu textures=%zu mips=%zu decoded_pixels=%zu\n",
                files,textures,mips,pixels);
    std::printf("UPSTREAM_BUCKET_GUARD_NON_UPLOADABLE_ENTRIES=%zu (not counted as decoded)\n",unuploadable);
    std::printf("ORIGINAL_DIMENSION_BUCKETS=%zu STRICT_REJECTED_BUCKETS=%zu UPSTREAM_POLICY_REJECTED_BUCKETS=%zu EXPLICIT_ZERO_FILLED_MIPS=%zu\n",
                buckets,strict_rejected_buckets,rejected_buckets,zero_filled_mips);
    std::puts("GPU_RENDERED=NO; GAME_MENU_PLAYABILITY=NOT_TESTED");
    if (rejected_buckets) { std::puts("STRICT_UPLOAD_INTEGRATION=BLOCKED; do not silently zero-fill missing data"); return 1; }
    std::puts("PASS: all uploadable mip pixels and source-derived bucket policy; no GPU game render claim");
}
