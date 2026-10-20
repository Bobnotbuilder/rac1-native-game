#include "mrp_texture.hpp"
#include <algorithm>
#include <TargetConditionals.h>
namespace rac1::metal {
id<MTLTexture> upload_texture_array(id<MTLDevice> device,
    std::span<const mrp::Texture* const> textures, uint32_t levels, NSError** error, MissingMipPolicy policy) {
    if (error) *error=nil;
    auto fail=[&](const std::string& reason)->id<MTLTexture> {
        if (error) *error=[NSError errorWithDomain:@"RAC1Native.MRPTexture" code:1
            userInfo:@{NSLocalizedDescriptionKey:@(reason.c_str())}];
        return nil;
    };
    TextureArrayShape shape;
    std::string reason;
    if (!device) return fail("METAL_DEVICE_UNAVAILABLE");
    if (!validate_texture_array(textures,levels,shape,reason,policy)) return fail(reason);
    MTLTextureDescriptor* descriptor=[[MTLTextureDescriptor alloc] init];
    descriptor.textureType=MTLTextureType2DArray;
    descriptor.pixelFormat=MTLPixelFormatRGBA8Unorm; // no sRGB decode or GS alpha rescaling
    descriptor.width=shape.width; descriptor.height=shape.height;
    descriptor.arrayLength=shape.layers; descriptor.mipmapLevelCount=shape.levels;
    descriptor.usage=MTLTextureUsageShaderRead;
#if TARGET_OS_OSX && !defined(__arm64__) && !defined(__aarch64__)
    // Intel/discrete macOS still needs managed CPU upload storage. Do not name the
    // deprecated managed enum in an Apple Silicon translation unit (Xcode 27).
    descriptor.storageMode=device.hasUnifiedMemory ? MTLStorageModeShared : MTLStorageModeManaged;
#else
    descriptor.storageMode=MTLStorageModeShared;
#endif
    id<MTLTexture> result=[device newTextureWithDescriptor:descriptor];
    if (!result) return fail("METAL_TEXTURE_ALLOCATION_FAILED");
    std::vector<uint8_t> rgba;
    for (size_t layer=0;layer<textures.size();++layer) {
        for (uint32_t level=0;level<levels;++level) {
            if (!decode_texture_mip(*textures[layer],level,rgba,reason,policy)) return fail(reason);
            const NSUInteger w=std::max(1u,shape.width>>level),h=std::max(1u,shape.height>>level);
            [result replaceRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:level slice:layer
                         withBytes:rgba.data() bytesPerRow:w*4 bytesPerImage:w*h*4];
        }
    }
    return result;
}
}
