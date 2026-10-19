// MIT. Independent evaluation of the GS ALPHA equation documented in gs_draw.cpp.
// Building block only: framebuffer format, dithering, COLCLAMP and destination-alpha tests
// still belong to the GS/MRP caller. This is NOT a claim of PS2 pixel equivalence.
#include <metal_stdlib>
using namespace metal;

// Caller MUST reject reserved selectors (3) before encoding this shader.
float4 rac1_gs_programmable_blend(float4 source, float4 destination,
                                 uint4 selectors, uint fixed_alpha) {
    const float3 zero = float3(0);
    const float3 a = selectors.x == 0 ? source.rgb : selectors.x == 1 ? destination.rgb : zero;
    const float3 b = selectors.y == 0 ? source.rgb : selectors.y == 1 ? destination.rgb : zero;
    const float3 d = selectors.w == 0 ? source.rgb : selectors.w == 1 ? destination.rgb : zero;
    // These inputs are blend-domain alpha, not raw GS texture-domain alpha.
    // In particular FIX remains /128, including values greater than one.
    const float c = selectors.z == 0 ? source.a : selectors.z == 1 ? destination.a : float(fixed_alpha) / 128.0f;
    return float4(clamp((a-b)*c+d, 0.0f, 1.0f), source.a);
}
