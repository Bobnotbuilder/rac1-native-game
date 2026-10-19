// Raster readback of the real Metal GS blend-state adapter. No game assets or fabricated gameplay.
#ifdef NDEBUG
#error Metal raster contracts require assertions
#endif
#include <game/render/metal/gs_blend_metal.h>
#import <Foundation/Foundation.h>
#include <simd/simd.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <unordered_map>
#include <vector>
using namespace rac1::metal;

int main(int argc,const char** argv) {
    @autoreleasepool {
        assert(argc==2);
        id<MTLDevice> device=MTLCreateSystemDefaultDevice();
        if(!device) { std::fprintf(stderr,"SKIP: Metal device unavailable, not PASS\n"); return 77; }
        NSError* error=nil;
        id<MTLLibrary> lib=[device newLibraryWithURL:[NSURL fileURLWithPath:@(argv[1])] error:&error];
        if(!lib) { std::fprintf(stderr,"%s\n",error.description.UTF8String); return 1; }
        id<MTLFunction> vs=[lib newFunctionWithName:@"blend_fixture_vertex"];
        id<MTLFunction> fs=[lib newFunctionWithName:@"blend_fixture_fragment"];
        assert(vs && fs);
        struct Case { uint64_t alpha; bool enabled; };
        std::vector<Case> cases;
        for(unsigned a=0;a<4;++a) for(unsigned b=0;b<4;++b)
        for(unsigned c=0;c<4;++c) for(unsigned d=0;d<4;++d)
        for(unsigned fix:{0u,64u,128u,192u,255u})
            cases.push_back({a|(b<<2)|(c<<4)|(d<<6)|(uint64_t{fix}<<32),true});
        cases.push_back({0,false});
        const NSUInteger width=32,height=(cases.size()+width-1)/width;
        MTLTextureDescriptor* texture_desc=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:width height:height mipmapped:NO];
        texture_desc.storageMode=MTLStorageModeShared;
        texture_desc.usage=MTLTextureUsageRenderTarget;
        id<MTLTexture> target=[device newTextureWithDescriptor:texture_desc];
        assert(target);
        MTLRenderPassDescriptor* pass=[MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture=target;
        pass.colorAttachments[0].loadAction=MTLLoadActionClear;
        pass.colorAttachments[0].storeAction=MTLStoreActionStore;
        const float destination[4]={77.f/255,153.f/255,204.f/255,191.f/255};
        const simd_float4 source={.8f,.4f,.2f,.5f};
        pass.colorAttachments[0].clearColor=MTLClearColorMake(destination[0],destination[1],destination[2],destination[3]);
        id<MTLCommandQueue> queue=[device newCommandQueue];
        id<MTLCommandBuffer> command=[queue commandBuffer];
        id<MTLRenderCommandEncoder> encoder=[command renderCommandEncoderWithDescriptor:pass];
        std::unordered_map<unsigned,id<MTLRenderPipelineState>> cache;
        size_t strict_rejections=0;
        for(size_t i=0;i<cases.size();++i) {
            const auto fixture=cases[i];
            const auto state=gs_blend_state(fixture.enabled ? 1u<<6 : 0,fixture.alpha);
            const unsigned key=unsigned(state.source)|(unsigned(state.destination)<<4)|(unsigned(state.operation)<<8)|(unsigned(state.enabled)<<10);
            auto pipeline=cache.find(key);
            if(pipeline==cache.end()) {
                MTLRenderPipelineDescriptor* descriptor=[[MTLRenderPipelineDescriptor alloc] init];
                descriptor.vertexFunction=vs;
                descriptor.fragmentFunction=fs;
                descriptor.colorAttachments[0].pixelFormat=MTLPixelFormatRGBA8Unorm;
                assert(configure_gs_blend(descriptor.colorAttachments[0],state,BlendPolicy::MatchOpenGLReference));
                id<MTLRenderPipelineState> native=[device newRenderPipelineStateWithDescriptor:descriptor error:&error];
                if(!native) { std::fprintf(stderr,"%s\n",error.description.UTF8String); return 1; }
                pipeline=cache.emplace(key,native).first;
            }
            // Strict callers cannot accidentally accept a known upstream approximation.
            MTLRenderPipelineColorAttachmentDescriptor* probe=[[MTLRenderPipelineColorAttachmentDescriptor alloc] init];
            probe.blendingEnabled=NO;
            const bool accepted=configure_gs_blend(probe,state,BlendPolicy::RequireDirectFactorization);
            assert(accepted==!state.reference_approximation);
            if(!accepted) { assert(!probe.blendingEnabled); ++strict_rejections; }
            [encoder setRenderPipelineState:pipeline->second];
            [encoder setViewport:MTLViewport{double(i%width),double(i/width),1,1,0,1}];
            [encoder setScissorRect:MTLScissorRect{i%width,i/width,1,1}];
            encode_gs_blend_constant(encoder,state);
            [encoder setFragmentBytes:&source length:sizeof(source) atIndex:0];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        }
        [encoder endEncoding];
        [command commit];
        [command waitUntilCompleted]; // TEST ONLY. No production CPU/GPU wait introduced.
        assert(command.status==MTLCommandBufferStatusCompleted);
        std::vector<uint8_t> pixels(width*height*4);
        [target getBytes:pixels.data() bytesPerRow:width*4 fromRegion:MTLRegionMake2D(0,0,width,height) mipmapLevel:0];
        int largest_error=0;
        for(size_t i=0;i<cases.size();++i) {
            const auto fixture=cases[i];
            const unsigned a=fixture.alpha&3,b=(fixture.alpha>>2)&3,c=(fixture.alpha>>4)&3,d=(fixture.alpha>>6)&3;
            const float fixed=std::min(float((fixture.alpha>>32)&255)/128,1.f);
            const float f=c==2 ? fixed : c==1 ? destination[3] : source.w;
            auto coefficient=[&](unsigned who) {
                const int constant=int(d==who),slope=int(a==who)-int(b==who);
                return constant && slope>0 ? 1.f : float(constant)+float(slope)*f;
            };
            for(int channel=0;channel<4;++channel) {
                const float expected=channel==3 || !fixture.enabled ? source[channel]
                    : std::clamp(source[channel]*coefficient(0)+destination[channel]*coefficient(1),0.f,1.f);
                const int delta=std::abs(int(pixels[4*i+channel])-int(std::lround(expected*255)));
                largest_error=std::max(largest_error,delta);
                if(delta>2) { std::fprintf(stderr,"FAIL blend case=%zu channel=%d delta=%d\n",i,channel,delta); return 1; }
            }
        }
        assert(strict_rejections>0);
        std::printf("PASS: %zu Metal raster blend cases; pipelines=%zu max_byte_error=%d strict_rejections=%zu\n",cases.size(),cache.size(),largest_error,strict_rejections);
        std::puts("GAME_RENDERING_ACCEPTANCE=NOT_TESTED");
    }
}
