// Synthetic framebuffer-fetch contract; not a stand-in for the game renderer.
#include "../../src/game/render/metal/gs_programmable_blend.metal"
struct BlendTarget { float4 colour [[color(0), raster_order_group(0)]]; };
vertex float4 programmable_vertex(uint i [[vertex_id]]) {
    const float2 points[3] = {float2(-1,-1),float2(3,-1),float2(-1,3)};
    return float4(points[i],0,1);
}
fragment BlendTarget programmable_fragment(BlendTarget previous,
        constant float4& source [[buffer(0)]],
        constant uint4& selectors [[buffer(1)]],
        constant uint& fixed_alpha [[buffer(2)]]) {
    BlendTarget result;
    result.colour = rac1_gs_programmable_blend(source,previous.colour,selectors,fixed_alpha);
    return result;
}
