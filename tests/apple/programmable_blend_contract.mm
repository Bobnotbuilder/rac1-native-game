// GPU raster/readback contract. Preserves the previous fixed-function reference tests.
#ifdef NDEBUG
#error Programmable blending contracts require assertions
#endif
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

int main(int argc, const char** argv) {
    @autoreleasepool {
        assert(argc==2);
        id<MTLDevice> device=MTLCreateSystemDefaultDevice();
        if (!device || ![device supportsFamily:MTLGPUFamilyApple4] || !device.rasterOrderGroupsSupported) {
            std::fputs("SKIP: Apple framebuffer-fetch/raster-order support unavailable; not PASS\n",stderr);
            return 77;
        }
        NSError* error=nil;
        id<MTLLibrary> library=[device newLibraryWithURL:[NSURL fileURLWithPath:@(argv[1])] error:&error];
        if (!library) { std::fprintf(stderr,"%s\n",error.description.UTF8String); return 1; }
        MTLRenderPipelineDescriptor* descriptor=[[MTLRenderPipelineDescriptor alloc] init];
        descriptor.vertexFunction=[library newFunctionWithName:@"programmable_vertex"];
        descriptor.fragmentFunction=[library newFunctionWithName:@"programmable_fragment"];
        descriptor.colorAttachments[0].pixelFormat=MTLPixelFormatRGBA8Unorm;
        descriptor.colorAttachments[0].blendingEnabled=NO; // never double-blend fetched output
        id<MTLRenderPipelineState> pipeline=[device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        if (!pipeline) { std::fprintf(stderr,"%s\n",error.description.UTF8String); return 1; }
        struct Case { simd_uint4 selectors; uint32_t fixed; };
        std::vector<Case> cases;
        for(unsigned a=0;a<3;++a) for(unsigned b=0;b<3;++b)
        for(unsigned c=0;c<3;++c) for(unsigned d=0;d<3;++d)
        for(unsigned fixed:{0u,64u,128u,192u,255u}) cases.push_back({{a,b,c,d},fixed});
        const NSUInteger width=32,height=(cases.size()+width-1)/width;
        MTLTextureDescriptor* td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:width height:height mipmapped:NO];
        td.usage=MTLTextureUsageRenderTarget;
        td.storageMode=MTLStorageModeShared;
        id<MTLTexture> target=[device newTextureWithDescriptor:td];
        assert(target);
        MTLRenderPassDescriptor* pass=[MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture=target;
        pass.colorAttachments[0].loadAction=MTLLoadActionClear;
        pass.colorAttachments[0].storeAction=MTLStoreActionStore;
        const std::array<float,4> initial={77.f/255,153.f/255,204.f/255,191.f/255};
        pass.colorAttachments[0].clearColor=MTLClearColorMake(initial[0],initial[1],initial[2],initial[3]);
        const simd_float4 source={.8f,.4f,.2f,.5f};
        id<MTLCommandQueue> queue=[device newCommandQueue];
        id<MTLCommandBuffer> command=[queue commandBuffer];
        id<MTLRenderCommandEncoder> encoder=[command renderCommandEncoderWithDescriptor:pass];
        assert(queue && command && encoder);
        [encoder setRenderPipelineState:pipeline];
        for(size_t i=0;i<cases.size();++i) {
            [encoder setViewport:MTLViewport{double(i%width),double(i/width),1,1,0,1}];
            [encoder setScissorRect:MTLScissorRect{i%width,i/width,1,1}];
            [encoder setFragmentBytes:&source length:sizeof(source) atIndex:0];
            [encoder setFragmentBytes:&cases[i].selectors length:sizeof(simd_uint4) atIndex:1];
            [encoder setFragmentBytes:&cases[i].fixed length:sizeof(uint32_t) atIndex:2];
            // Three overlapping draws prove ordered destination fetch, not just clear-colour reads.
            for(unsigned layer=0;layer<3;++layer) [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        }
        [encoder endEncoding];
        [command commit];
        [command waitUntilCompleted]; // fixture only; never a production pacing mechanism
        if(command.status!=MTLCommandBufferStatusCompleted) {
            std::fprintf(stderr,"GPU failure: %s\n",command.error.description.UTF8String); return 1;
        }
        std::vector<uint8_t> pixels(width*height*4);
        [target getBytes:pixels.data() bytesPerRow:width*4 fromRegion:MTLRegionMake2D(0,0,width,height) mipmapLevel:0];
        int max_error=0;
        for(size_t i=0;i<cases.size();++i) {
            auto dst=initial;
            const auto s=cases[i].selectors;
            for(unsigned layer=0;layer<3;++layer) {
                const float factor=s.z==0 ? source.w : s.z==1 ? dst[3] : float(cases[i].fixed)/128;
                auto next=dst;
                for(unsigned channel=0;channel<3;++channel) {
                    const auto value=[&](unsigned selector) { return selector==0 ? source[channel] : selector==1 ? dst[channel] : 0.f; };
                    const float rgb=std::clamp((value(s.x)-value(s.y))*factor+value(s.w),0.f,1.f);
                    next[channel]=std::round(rgb*255)/255;
                }
                next[3]=std::round(source.w*255)/255;
                dst=next;
            }
            for(unsigned channel=0;channel<4;++channel) {
                const int delta=std::abs(int(pixels[4*i+channel])-int(std::lround(dst[channel]*255)));
                max_error=std::max(max_error,delta);
                if(delta>3) { std::fprintf(stderr,"FAIL programmable case=%zu channel=%u delta=%d\n",i,channel,delta); return 1; }
            }
        }
        std::printf("PASS: %zu programmable blend states, 3 ordered layers, one pipeline; max_byte_error=%d\n",cases.size(),max_error);
        std::puts("PS2_PIXEL_EQUIVALENCE=NOT_VERIFIED; GAME_RENDERING_ACCEPTANCE=NOT_TESTED");
    }
}
