#include "mrp_texture.hpp"
#include <algorithm>
#include <limits>
namespace rac1::metal {
namespace {
bool mip_shape(const mrp::Texture& t, uint32_t level, uint32_t& w, uint32_t& h, std::string& error,
               MissingMipPolicy policy) {
    if (!t.width || !t.height || level >= 32 || t.mips.empty()) {
        error="MRP_TEXTURE_MISSING_DIMENSION_OR_MIP"; return false;
    }
    if (uint64_t(t.width)*t.height!=t.mips[0].size()) {
        error="MRP_TEXTURE_BASE_MIP_SIZE_MISMATCH"; return false;
    }
    uint32_t max_level=0, dimension=std::max(t.width,t.height);
    while (dimension>1) { dimension >>= 1; ++max_level; }
    if (level>max_level) { error="MRP_TEXTURE_EXCESS_MIP_LEVEL"; return false; }
    w=std::max(1u,t.width>>level); h=std::max(1u,t.height>>level);
    const uint64_t pixels=uint64_t(w)*h;
    if (pixels>std::numeric_limits<size_t>::max()/4) {
        error="MRP_TEXTURE_MIP_SIZE_OVERFLOW"; return false;
    }
    if (level>=t.mips.size()) {
        if (policy==MissingMipPolicy::UpstreamZeroFill) return true;
        error="MRP_TEXTURE_MISSING_MIP"; return false;
    }
    if (pixels!=t.mips[level].size()) {
        error="MRP_TEXTURE_MIP_SIZE_MISMATCH"; return false;
    }
    return true;
}
}
bool validate_texture_array(std::span<const mrp::Texture* const> textures, uint32_t levels,
                            TextureArrayShape& shape, std::string& error, MissingMipPolicy policy) {
    if (textures.empty() || !textures.front() || !levels || levels>32) {
        error="MRP_TEXTURE_ARRAY_EMPTY_OR_INVALID_LEVELS"; return false;
    }
    TextureArrayShape result{textures.front()->width,textures.front()->height,levels,textures.size()};
    for (const auto* texture:textures) {
        if (!texture || texture->width!=result.width || texture->height!=result.height) {
            error="MRP_TEXTURE_ARRAY_DIMENSIONS_MISMATCH"; return false;
        }
        for (uint32_t level=0;level<levels;++level) {
            uint32_t w=0,h=0;
            if (!mip_shape(*texture,level,w,h,error,policy)) return false;
            if (level>=texture->mips.size()) ++result.zero_filled_mips;
        }
    }
    shape=result; error.clear(); return true;
}
bool decode_texture_mip(const mrp::Texture& t, uint32_t level,
                        std::vector<uint8_t>& rgba, std::string& error, MissingMipPolicy policy) {
    uint32_t w=0,h=0;
    if (!mip_shape(t,level,w,h,error,policy)) return false;
    std::vector<uint8_t> result(size_t(w)*h*4);
    // Explicit upstream bucket behavior: absent upper mips are RGBA zero, not generated,
    // repeated from another mip, or passed off as decoded asset data.
    if (level>=t.mips.size()) { rgba.swap(result); error.clear(); return true; }
    for (size_t pixel=0;pixel<t.mips[level].size();++pixel) {
        const uint32_t index=t.mips[level][pixel];
        // CSM1 swaps bits 3 and 4, exactly as the original GL MRP upload does.
        const uint32_t swizzled=((index&8)!=0)!=((index&16)!=0) ? index^0x18u : index;
        const uint32_t colour=t.palette[swizzled];
        for (unsigned channel=0;channel<4;++channel)
            result[pixel*4+channel]=uint8_t(colour>>(8*channel));
    }
    rgba.swap(result); error.clear(); return true;
}
}
