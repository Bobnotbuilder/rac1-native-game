#ifdef NDEBUG
#error Metal texture contracts require assertions
#endif
#include <game/render/metal/mrp_texture.hpp>
#include <cassert>
#include <cstdio>
int main() {
    @autoreleasepool {
        id<MTLDevice> device=MTLCreateSystemDefaultDevice();
        if (!device) { std::puts("SKIP: no Metal device; not a PASS"); return 77; }
        rac1::mrp::Texture a; a.width=16; a.height=16;
        a.mips={std::vector<uint8_t>(256),std::vector<uint8_t>(64,17),std::vector<uint8_t>(16,31)};
        for (unsigned i=0;i<256;++i) { a.mips[0][i]=uint8_t(i); a.palette[i]=0x80000000u|(i<<16)|((255-i)<<8)|i; }
        rac1::mrp::Texture b=a;
        for (auto& colour:b.palette) colour^=0x00ffffffu;
        b.mips.pop_back(); // original bucket uploader explicitly zero-fills absent upper mips
        const rac1::mrp::Texture* inputs[]={&a,&b};
        NSError* error=nil;
        assert(!rac1::metal::upload_texture_array(device,inputs,3,&error) && error);
        id<MTLTexture> texture=rac1::metal::upload_texture_array(device,inputs,3,&error,
            rac1::metal::MissingMipPolicy::UpstreamZeroFill);
        assert(texture && !error && texture.arrayLength==2 && texture.mipmapLevelCount==3);
        assert(texture.pixelFormat==MTLPixelFormatRGBA8Unorm);
        NSString* source=@"#include <metal_stdlib>\nusing namespace metal;\n"
            "kernel void read_tex(texture2d_array<float,access::read> t [[texture(0)]],"
            "device uint4* out [[buffer(0)]],constant uint3& args [[buffer(1)]],uint i [[thread_position_in_grid]]) {"
            "if(i<args.z*args.z) out[i]=uint4(rint(t.read(uint2(i%args.z,i/args.z),args.x,args.y)*255.0f)); }";
        id<MTLLibrary> library=[device newLibraryWithSource:source options:nil error:&error];
        assert(library && !error);
        id<MTLComputePipelineState> pipeline=[device newComputePipelineStateWithFunction:[library newFunctionWithName:@"read_tex"] error:&error];
        assert(pipeline && !error);
        id<MTLCommandQueue> queue=[device newCommandQueue];
        size_t pixels=0;
        for (unsigned layer=0;layer<2;++layer) for (unsigned mip=0;mip<3;++mip) {
            const uint32_t width=16u>>mip;
            // uint3 is 16-byte aligned in MSL.
            const uint32_t args[4]={layer,mip,width,0};
            id<MTLBuffer> output=[device newBufferWithLength:width*width*4*sizeof(uint32_t) options:MTLResourceStorageModeShared];
            id<MTLCommandBuffer> command=[queue commandBuffer];
            id<MTLComputeCommandEncoder> encoder=[command computeCommandEncoder];
            [encoder setComputePipelineState:pipeline];
            [encoder setTexture:texture atIndex:0]; [encoder setBuffer:output offset:0 atIndex:0];
            [encoder setBytes:args length:sizeof(args) atIndex:1];
            [encoder dispatchThreads:MTLSizeMake(width*width,1,1) threadsPerThreadgroup:MTLSizeMake(pipeline.threadExecutionWidth,1,1)];
            [encoder endEncoding]; [command commit]; [command waitUntilCompleted]; // verification only
            assert(command.status==MTLCommandBufferStatusCompleted);
            const auto* read=static_cast<const uint32_t*>(output.contents);
            const auto& original=*inputs[layer];
            for (size_t i=0;i<size_t(width)*width;++i) {
                uint32_t want=0;
                if (mip<original.mips.size()) {
                    const unsigned index=original.mips[mip][i];
                    const unsigned swapped=(index&~0x18u)|((index&8u)<<1)|((index&16u)>>1);
                    want=original.palette[swapped];
                }
                for (unsigned c=0;c<4;++c) assert(read[i*4+c]==uint8_t(want>>(8*c)));
                ++pixels;
            }
        }
        a.mips[1].pop_back();
        assert(!rac1::metal::upload_texture_array(device,inputs,3,&error) && error);
        assert(!rac1::metal::upload_texture_array(device,inputs,3,&error,rac1::metal::MissingMipPolicy::UpstreamZeroFill) && error);
        std::printf("PASS: %zu Metal texture-array GPU readbacks, two layers/three mips, raw GS alpha and malformed-input rejection\n",pixels);
    }
}
