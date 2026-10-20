// Source-derived GS stream screen stage. Unsupported host paths are rejected on CPU.
// Not PS2 hardware pixel equivalence or complete MRP rendering acceptance.
#include <metal_stdlib>
using namespace metal;
#include "gs_fragment.metal"
struct SceneVertex { float4 position,colour,tex,fog; };
struct SceneUniform { uint4 texture,alpha,blend,flags; float4 fog; uint4 mask; };
struct SceneRaster {
    float4 position [[position]];
    float4 colour;
    float3 tex;
    float fog;
};
vertex SceneRaster rac1_scene_vertex(uint id [[vertex_id]],device const SceneVertex* vertices [[buffer(0)]]) {
    const SceneVertex v=vertices[id];
    return {v.position,v.colour,v.tex.xyz,v.fog.x};
}
struct SceneColour { float4 colour [[color(0),raster_order_group(0)]]; };
fragment SceneColour rac1_scene_fragment(SceneRaster in [[stage_in]],
    float4 destination [[color(0),raster_order_group(0)]],
    constant SceneUniform& u [[buffer(0)]],texture2d<float> image [[texture(0)]],sampler sample_state [[sampler(0)]]) {
    if(u.flags.z && ((u.flags.w==0)==(destination.a>254.f/255.f)))discard_fragment();
    const float4 texel=u.texture.x ? image.sample(sample_state,in.tex.xy/in.tex.z) : float4(1);
    float4 source=rac1_gs_texture(in.colour,texel,u.texture.y,u.texture.z!=0,u.texture.x!=0);
    if(u.texture.w)source.rgb=mix(u.fog.rgb,source.rgb,in.fog);
    const bool pass=!u.alpha.x || rac1_gs_alpha_pass(source.a,u.alpha.y,u.alpha.z);
    if(!pass && (u.alpha.w==0 || u.alpha.w==2))discard_fragment();
    const float blend_alpha=min(source.a*(255.f/128.f),1.f);
    float3 colour=source.rgb;
    if(u.flags.x) {
        const float3 a=u.blend.x==0 ? source.rgb : u.blend.x==1 ? destination.rgb : float3(0);
        const float3 b=u.blend.y==0 ? source.rgb : u.blend.y==1 ? destination.rgb : float3(0);
        const float3 d=u.blend.w==0 ? source.rgb : u.blend.w==1 ? destination.rgb : float3(0);
        const float c=u.blend.z==0 ? blend_alpha : u.blend.z==1 ? destination.a : float(u.flags.y)/128.f;
        colour=clamp((a-b)*c+d,0.f,1.f);
    }
    const float alpha=!pass && u.alpha.w==3 ? destination.a : blend_alpha;
    const uint4 src=uint4(rint(float4(colour,alpha)*255.f));
    const uint4 dst=uint4(rint(destination*255.f));
    return {float4((src&~u.mask)|(dst&u.mask))/255.f};
}
