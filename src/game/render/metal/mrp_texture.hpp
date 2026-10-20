// MIT. Native Metal upload preparation for upstream MRP's indexed textures.
// Raw GS alpha is preserved (0x80 means one in blending, NOT 255).
#pragma once
#include <formats/moby_mrp.h>
#include <span>
#include <string>
#include <vector>
namespace rac1::metal {
enum class MissingMipPolicy { Reject, UpstreamZeroFill };
struct TextureArrayShape { uint32_t width=0, height=0, levels=0; size_t layers=0, zero_filled_mips=0; };
bool validate_texture_array(std::span<const mrp::Texture* const> textures, uint32_t levels,
                            TextureArrayShape& shape, std::string& error,
                            MissingMipPolicy policy=MissingMipPolicy::Reject);
bool decode_texture_mip(const mrp::Texture& texture, uint32_t mip,
                        std::vector<uint8_t>& rgba, std::string& error,
                        MissingMipPolicy policy=MissingMipPolicy::Reject);
}
#ifdef __OBJC__
#import <Metal/Metal.h>
namespace rac1::metal {
// Load-time only. No command queue, per-frame copy, mip generation or alpha conversion.
id<MTLTexture> upload_texture_array(id<MTLDevice> device,
    std::span<const mrp::Texture* const> textures, uint32_t levels, NSError** error,
    MissingMipPolicy policy=MissingMipPolicy::Reject);
}
#endif
