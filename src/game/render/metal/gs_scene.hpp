// Native Metal consumer of the ORIGINAL GS/VU primitive stream. Not a new game implementation.
#pragma once
#include <runtime/gs/gs_emu.h>
#include <span>
#include <string>
namespace rac1::metal {
struct SceneView {
    uint32_t width=512,height=416,screen_fbp=0;
    bool depth_valid=false;
    double depth_a=0,depth_b=0;
};
struct alignas(16) SceneVertex { float position[4]{},colour[4]{},tex[4]{},fog[4]{}; };
struct alignas(16) SceneUniform {
    uint32_t texture[4]{}; // TME,TFX,TCC,FGE
    uint32_t alpha[4]{};   // ATE,ATST,AREF,AFAIL
    uint32_t blend[4]{};   // A,B,C,D (defined selectors only)
    uint32_t flags[4]{};   // ABE,FIX,DATE,DATM
    float fog[4]{};
    uint32_t mask[4]{};    // per-channel preserved bits
};
struct SceneDraw {
    uint32_t first=0,count=0;
    bool lines=false,depth_test=false,depth_write=false;
    uint32_t depth_compare=1; // GS NEVER,ALWAYS,GEQUAL,GREATER -> Metal NEVER,ALWAYS,LEQUAL,LESS
    uint32_t scissor[4]{};
    SceneUniform uniform;
    std::shared_ptr<const gs::TexImage> image;
    bool linear=false,repeat_s=false,repeat_t=false;
};
struct SceneFrame { uint32_t width=0,height=0; std::vector<SceneVertex> vertices; std::vector<SceneDraw> draws; };
// Validate the WHOLE input before publishing a frame. Unsupported paths return explicit
// blockers, never dropped primitives or successful-looking blank frames.
bool prepare_gs_scene(std::span<const gs::Primitive> primitives,const SceneView& view,
                      SceneFrame& out,std::string& error);
}
#ifdef __OBJC__
#import <Metal/Metal.h>
namespace rac1::metal {
// Encodes onto a caller-owned command buffer/target; no waitUntilCompleted in production.
// The library must contain the precompiled gs_scene.metal entry points.
class SceneEncoder {
public:
    SceneEncoder(id<MTLDevice> device,id<MTLLibrary> library);
    ~SceneEncoder();
    bool encode(id<MTLCommandBuffer> command,id<MTLTexture> colour,id<MTLTexture> depth,
                const SceneFrame& frame,bool clear,std::string& error);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
#endif
