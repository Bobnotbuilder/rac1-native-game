// MIT, adapted from rac1-native's MRP kFs (mrp_moby_renderer.cpp, upstream 3fd14c12).
// A tested fragment-stage building block, NOT yet a complete MRP backend.
#include <metal_stdlib>
using namespace metal;

// Deliberately separate texture evaluation from sampling: each MRP material retains its original
// texture array/mip and clamp decisions. Alpha here remains raw GS alpha (0x80 == one for blending).
float4 rac1_gs_texture(float4 vertex, float4 texel, uint tfx, bool tcc, bool textured) {
    if (!textured) return clamp(vertex, 0.0f, 1.0f);
    float4 out;
    if (tfx == 0) {
        out = float4(texel.rgb * vertex.rgb * (255.0f / 128.0f), tcc ? texel.a * vertex.a * (255.0f / 128.0f) : vertex.a);
    } else if (tfx == 1) {
        out = float4(texel.rgb, tcc ? texel.a : vertex.a);
    } else if (tfx == 2) {
        out = float4(texel.rgb * vertex.rgb * (255.0f / 128.0f) + vertex.a, tcc ? texel.a + vertex.a : vertex.a);
    } else {
        out = float4(texel.rgb * vertex.rgb * (255.0f / 128.0f) + vertex.a, tcc ? texel.a : vertex.a);
    }
    return clamp(out, 0.0f, 1.0f);
}
bool rac1_gs_alpha_pass(float alpha, uint atst, uint aref) {
    const float a = alpha * 255.0f;
    switch (atst) {
        case 0: return false;
        case 1: return true;
        case 2: return a < float(aref);
        case 3: return a <= float(aref);
        case 4: return abs(a - float(aref)) < 0.5f;
        case 5: return a >= float(aref);
        case 6: return a > float(aref);
        case 7: return abs(a - float(aref)) >= 0.5f;
    }
    return false;
}
// Same two-pass decision as the OpenGL MRP. Pipeline colour/depth masks remain the caller's job.
bool rac1_gs_keep(bool pass, uint afail, uint draw_pass) {
    return pass || (draw_pass == 0 ? (afail != 0 && afail != 2) : afail == 2);
}
struct GsFixture {
    float4 vertex;
    float4 texel;
    uint4 state; // TFX, TCC, ATST, AREF
};
kernel void rac1_gs_contract(device const GsFixture* fixtures [[buffer(0)]],
                             device float4* result [[buffer(1)]],
                             uint i [[thread_position_in_grid]]) {
    const GsFixture f = fixtures[i];
    result[2 * i] = rac1_gs_texture(f.vertex, f.texel, f.state.x, f.state.y != 0, true);
    const float alpha = result[2 * i].a;
    uint mask = 0;
    const bool pass = rac1_gs_alpha_pass(alpha, f.state.z, f.state.w);
    for (uint afail = 0; afail < 4; ++afail)
        for (uint p = 0; p < 2; ++p)
            if (rac1_gs_keep(pass, afail, p)) mask |= 1u << (afail * 2 + p);
    result[2 * i + 1] = float4(float(pass), float(mask), 0, 0);
}
