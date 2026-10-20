#include "gs_scene.hpp"
#include <algorithm>
#include <map>
#include <tuple>
namespace rac1::metal {
struct SceneEncoder::Impl {
    id<MTLDevice> device;
    id<MTLLibrary> library;
    std::map<std::pair<NSUInteger,NSUInteger>,id<MTLRenderPipelineState>> pipelines;
    std::map<unsigned,id<MTLDepthStencilState>> depths;
    std::map<unsigned,id<MTLSamplerState>> samplers;
    // Own the immutable snapshot with its GPU texture; no pointer/id reuse can yield stale data.
    std::map<std::shared_ptr<const gs::TexImage>,id<MTLTexture>,std::owner_less<std::shared_ptr<const gs::TexImage>>> textures;
    id<MTLTexture> white;
    Impl(id<MTLDevice> d,id<MTLLibrary> l):device(d),library(l){}
};
SceneEncoder::SceneEncoder(id<MTLDevice> d,id<MTLLibrary> l):impl_(std::make_unique<Impl>(d,l)){}
SceneEncoder::~SceneEncoder()=default;
bool SceneEncoder::encode(id<MTLCommandBuffer> command,id<MTLTexture> colour,id<MTLTexture> depth,
                           const SceneFrame& frame,bool clear,std::string& error) {
    auto& c=*impl_;
    auto fail=[&](const char* text){error=text;return false;};
    if(!c.device || !c.library || !command || !colour || !frame.width || !frame.height)
        return fail("METAL_SCENE_RESOURCES_REQUIRED");
    if(![c.device supportsFamily:MTLGPUFamilyApple4] || !c.device.rasterOrderGroupsSupported)
        return fail("METAL_ORDERED_FRAMEBUFFER_FETCH_REQUIRED");
    if(depth && (depth.width!=colour.width || depth.height!=colour.height))return fail("METAL_DEPTH_SIZE_MISMATCH");
    const auto key=std::make_pair(NSUInteger(colour.pixelFormat),depth?NSUInteger(depth.pixelFormat):0);
    id<MTLRenderPipelineState> pipeline=c.pipelines[key];
    if(!pipeline) {
        MTLRenderPipelineDescriptor* p=[[MTLRenderPipelineDescriptor alloc]init];
        p.vertexFunction=[c.library newFunctionWithName:@"rac1_scene_vertex"];
        p.fragmentFunction=[c.library newFunctionWithName:@"rac1_scene_fragment"];
        p.colorAttachments[0].pixelFormat=colour.pixelFormat;
        p.depthAttachmentPixelFormat=depth?depth.pixelFormat:MTLPixelFormatInvalid;
        NSError* reason=nil;pipeline=[c.device newRenderPipelineStateWithDescriptor:p error:&reason];
        if(!pipeline)return fail("METAL_SCENE_PIPELINE_CREATION_FAILED");
        c.pipelines[key]=pipeline;
    }
    auto texture=[&](const std::shared_ptr<const gs::TexImage>& image)->id<MTLTexture> {
        if(!image && c.white)return c.white;
        if(image) {auto found=c.textures.find(image);if(found!=c.textures.end())return found->second;}
        auto desc=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
            width:image?image->w:1 height:image?image->h:1 mipmapped:NO];
        desc.storageMode=MTLStorageModeShared;desc.usage=MTLTextureUsageShaderRead;
        id<MTLTexture> result=[c.device newTextureWithDescriptor:desc];
        if(!result)return nil;
        const uint32_t white=0xffffffff;
        [result replaceRegion:MTLRegionMake2D(0,0,desc.width,desc.height) mipmapLevel:0
            withBytes:image?image->px.data():&white bytesPerRow:desc.width*4];
        if(image)c.textures[image]=result;else c.white=result;
        return result;
    };
    // Resolve resources and validate indices BEFORE opening the render encoder.
    for(const auto& draw:frame.draws) {
        if(draw.first>frame.vertices.size() || draw.count>frame.vertices.size()-draw.first)
            return fail("METAL_SCENE_VERTEX_RANGE_INVALID");
        if(draw.depth_test && !depth)return fail("METAL_SCENE_DEPTH_REQUIRED");
        if(!texture(draw.image))return fail("METAL_SCENE_TEXTURE_ALLOCATION_FAILED");
    }
    id<MTLBuffer> vertices=frame.vertices.empty()?nil:[c.device newBufferWithBytes:frame.vertices.data()
        length:frame.vertices.size()*sizeof(SceneVertex) options:MTLResourceStorageModeShared];
    if(!frame.vertices.empty() && !vertices)return fail("METAL_SCENE_VERTEX_ALLOCATION_FAILED");
    MTLRenderPassDescriptor* pass=[MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture=colour;pass.colorAttachments[0].loadAction=clear?MTLLoadActionClear:MTLLoadActionLoad;
    pass.colorAttachments[0].storeAction=MTLStoreActionStore;pass.colorAttachments[0].clearColor=MTLClearColorMake(0,0,0,1);
    if(depth) {pass.depthAttachment.texture=depth;pass.depthAttachment.loadAction=clear?MTLLoadActionClear:MTLLoadActionLoad;
        pass.depthAttachment.storeAction=MTLStoreActionStore;pass.depthAttachment.clearDepth=1;}
    id<MTLRenderCommandEncoder> encoder=[command renderCommandEncoderWithDescriptor:pass];
    if(!encoder)return fail("METAL_SCENE_ENCODER_CREATION_FAILED");
    [encoder setRenderPipelineState:pipeline];[encoder setCullMode:MTLCullModeNone];
    [encoder setVertexBuffer:vertices offset:0 atIndex:0];
    for(const auto& draw:frame.draws) {
        const unsigned dk=draw.depth_test?draw.depth_compare+4u*unsigned(draw.depth_write):1u;
        auto state=c.depths[dk];
        if(!state) {MTLDepthStencilDescriptor* d=[[MTLDepthStencilDescriptor alloc]init];
            const MTLCompareFunction compares[]={MTLCompareFunctionNever,MTLCompareFunctionAlways,MTLCompareFunctionLessEqual,MTLCompareFunctionLess};
            d.depthCompareFunction=compares[draw.depth_test?draw.depth_compare:1];d.depthWriteEnabled=draw.depth_write;
            state=[c.device newDepthStencilStateWithDescriptor:d];c.depths[dk]=state;}
        [encoder setDepthStencilState:state];
        const unsigned sk=unsigned(draw.linear)|unsigned(draw.repeat_s)<<1|unsigned(draw.repeat_t)<<2;
        auto sampler=c.samplers[sk];
        if(!sampler){MTLSamplerDescriptor* s=[[MTLSamplerDescriptor alloc]init];
            s.minFilter=s.magFilter=draw.linear?MTLSamplerMinMagFilterLinear:MTLSamplerMinMagFilterNearest;
            s.sAddressMode=draw.repeat_s?MTLSamplerAddressModeRepeat:MTLSamplerAddressModeClampToEdge;
            s.tAddressMode=draw.repeat_t?MTLSamplerAddressModeRepeat:MTLSamplerAddressModeClampToEdge;
            sampler=[c.device newSamplerStateWithDescriptor:s];c.samplers[sk]=sampler;}
        const auto x0=uint64_t(draw.scissor[0])*colour.width/frame.width,y0=uint64_t(draw.scissor[1])*colour.height/frame.height;
        const auto x1=uint64_t(draw.scissor[0]+draw.scissor[2])*colour.width/frame.width;
        const auto y1=uint64_t(draw.scissor[1]+draw.scissor[3])*colour.height/frame.height;
        if(x1<=x0 || y1<=y0)continue;
        [encoder setScissorRect:MTLScissorRect{NSUInteger(x0),NSUInteger(y0),NSUInteger(x1-x0),NSUInteger(y1-y0)}];
        [encoder setFragmentBytes:&draw.uniform length:sizeof(draw.uniform) atIndex:0];
        [encoder setFragmentTexture:texture(draw.image) atIndex:0];[encoder setFragmentSamplerState:sampler atIndex:0];
        [encoder drawPrimitives:draw.lines?MTLPrimitiveTypeLine:MTLPrimitiveTypeTriangle vertexStart:draw.first vertexCount:draw.count];
    }
    [encoder endEncoding];
    // Keep only this frame's snapshots; submitted command buffers retain their GPU resources.
    for(auto it=c.textures.begin();it!=c.textures.end();) {
        const auto& image=it->first;
        if(std::none_of(frame.draws.begin(),frame.draws.end(),[&](const auto& d){return d.image==image;}))it=c.textures.erase(it);else ++it;
    }
    error.clear();return true;
}
} // namespace rac1::metal
