// Synthetic raster fixture ONLY; never used as a game renderer or packaged app.
#include <metal_stdlib>
using namespace metal;
vertex float4 blend_fixture_vertex(uint vertex_id [[vertex_id]]) {
    const float2 positions[3] = {float2(-1,-1),float2(3,-1),float2(-1,3)};
    return float4(positions[vertex_id],0,1);
}
fragment float4 blend_fixture_fragment(constant float4& colour [[buffer(0)]]) { return colour; }
