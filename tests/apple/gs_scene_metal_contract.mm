#ifdef NDEBUG
#error GS scene GPU assertions must remain enabled
#endif
#include <game/render/metal/gs_scene.hpp>
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace rac1;
static gs::Primitive sprite(float x0,float y0,float x1,float y1,uint8_t r,uint8_t g,uint8_t b,uint8_t a) {
    gs::Primitive p;p.type=6;p.st.colclamp=1;p.st.scissor=15ull<<16|15ull<<48;p.v.resize(2);
    p.v[0].x=x0;p.v[0].y=y0;p.v[1].x=x1;p.v[1].y=y1;
    p.v[1].r=r;p.v[1].g=g;p.v[1].b=b;p.v[1].a=a;return p;
}
int main(int argc,const char** argv) {
    @autoreleasepool {
        assert(argc==2);auto device=MTLCreateSystemDefaultDevice();
        if(!device || ![device supportsFamily:MTLGPUFamilyApple4] || !device.rasterOrderGroupsSupported)return 77;
        NSError* reason=nil;
        auto library=[device newLibraryWithURL:[NSURL fileURLWithPath:@(argv[1])] error:&reason];assert(library && !reason);
        auto td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:16 height:16 mipmapped:NO];
        td.storageMode=MTLStorageModeShared;td.usage=MTLTextureUsageRenderTarget;
        auto target=[device newTextureWithDescriptor:td];assert(target);
        auto red=sprite(0,0,16,16,255,0,0,128),blue=sprite(4,4,12,12,0,0,255,64);
        blue.st.prim=1ull<<6;blue.st.alpha=0x44;
        auto textured=sprite(0,0,4,4,255,255,255,128);
        textured.st.prim=1ull<<4|1ull<<8;
        textured.st.tex0=1ull<<26|1ull<<30|1ull<<34|1ull<<35;
        auto snapshot=std::make_shared<gs::TexImage>();snapshot->w=2;snapshot->h=2;
        snapshot->px={0x800000ffu,0x8000ff00u,0x80ff0000u,0x80ffffffu};
        textured.tex=snapshot;textured.v[1].u=2;textured.v[1].v=2;
        const std::vector<gs::Primitive> original_stream{red,blue,textured};
        metal::SceneFrame frame;std::string error;metal::SceneView view;view.width=view.height=16;
        assert(metal::prepare_gs_scene(original_stream,view,frame,error));
        metal::SceneEncoder renderer(device,library);auto queue=[device newCommandQueue];
        for(unsigned repeat=0;repeat<2;++repeat) {
            auto command=[queue commandBuffer];assert(renderer.encode(command,target,nil,frame,true,error));
            [command commit];[command waitUntilCompleted]; // verification only, not production pacing
            assert(command.status==MTLCommandBufferStatusCompleted);
            uint8_t pixels[16*16*4];[target getBytes:pixels bytesPerRow:16*4 fromRegion:MTLRegionMake2D(0,0,16,16) mipmapLevel:0];
            auto check=[&](unsigned x,unsigned y,unsigned r,unsigned g,unsigned b,unsigned a){
                const unsigned expected[]={r,g,b,a};for(unsigned c=0;c<4;++c)assert(std::abs(int(pixels[(y*16+x)*4+c])-int(expected[c]))<=1);
            };
            check(14,14,255,0,0,255);check(8,8,128,0,128,128);
            check(0,0,255,0,0,255);check(3,0,0,255,0,255);
            check(0,3,0,0,255,255);check(3,3,255,255,255,255);
        }
        std::puts("PASS: original GS primitive -> validated scene -> Metal raster/texture/blend readback; two frames reuse renderer resources. Synthetic fixtures only; original game rendering NOT validated.");
    }
}
