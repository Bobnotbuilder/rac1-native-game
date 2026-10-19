// Compare the new state mapping with the upstream formula, not with its own output.
// Synthetic register combinations only. No PS2/game-oracle claim.
#ifdef NDEBUG
#error Blend contracts require assertions
#endif
#include <game/render/metal/gs_blend.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace rac1::metal;

static float factor(BlendFactor f, float sa, float da, float k) {
    switch (f) {
        case BlendFactor::Zero: return 0;
        case BlendFactor::One: return 1;
        case BlendFactor::SourceAlpha: return sa;
        case BlendFactor::DestinationAlpha: return da;
        case BlendFactor::ConstantAlpha: return k;
        case BlendFactor::OneMinusSourceAlpha: return 1-sa;
        case BlendFactor::OneMinusDestinationAlpha: return 1-da;
        case BlendFactor::OneMinusConstantAlpha: return 1-k;
    }
    assert(false); return 0;
}
int main() {
    size_t cases = 0;
    size_t reasons[4] = {}, reserved_rejections = 0, legal_rejections = 0;
    for (unsigned a=0; a<4; ++a) for (unsigned b=0; b<4; ++b)
    for (unsigned c=0; c<4; ++c) for (unsigned d=0; d<4; ++d)
    for (unsigned fix : {0u, 64u, 128u, 192u, 255u}) {
        const uint64_t alpha = a | (b<<2) | (c<<4) | (d<<6) | (uint64_t{fix}<<32);
        assert(!gs_blend_state(0, alpha).enabled);
        const auto state = gs_blend_state(uint64_t{1}<<6, alpha);
        assert(state.enabled);
        ++reasons[state.approximation_reasons];
        if (state.reference_approximation) {
            if (a==3 || b==3 || c==3 || d==3) ++reserved_rejections;
            else ++legal_rejections;
        }
        for (float sa : {0.f, .25f, 1.f}) for (float da : {0.f, .75f, 1.f}) {
            const float cs=.8f, cd=.3f, k=std::min(float(fix)/128,1.f);
            const float fc = c==2 ? k : c==1 ? da : sa;
            // Independent coefficient evaluation of the documented OpenGL reference policy.
            auto coefficient = [&](unsigned who) {
                const int constant = int(d==who), slope=int(a==who)-int(b==who);
                if (constant && slope>0) return 1.f; // upstream's explicit 1+C approximation
                return float(constant) + float(slope)*fc;
            };
            const float want=std::clamp(cs*coefficient(0)+cd*coefficient(1),0.f,1.f);
            const float src=cs*factor(state.source,sa,da,k), dst=cd*factor(state.destination,sa,da,k);
            const float got=std::clamp(state.operation==BlendOperation::Add ? src+dst
                                      : state.operation==BlendOperation::Subtract ? src-dst : dst-src,0.f,1.f);
            assert(std::fabs(want-got)<1e-6f);
            ++cases;
        }
    }
    // Orxon's subtractive barrier state: (Cd-Cs) FIX, not an opaque source copy.
    const auto subtractive=gs_blend_state(1u<<6,0xa1u|(uint64_t{64}<<32));
    assert(subtractive.operation==BlendOperation::ReverseSubtract);
    assert(subtractive.source==BlendFactor::ConstantAlpha && subtractive.destination==BlendFactor::ConstantAlpha);
    std::printf("PASS: %zu GS blend coefficient cases vs OpenGL reference\n",cases);
    assert(reasons[1]+reasons[2]+reasons[3]==188);
    std::printf("STRICT_BLEND_AUDIT total=188 one_plus_only=%zu fixed_clamp_only=%zu both=%zu legal=%zu reserved_selector=%zu\n",
                reasons[1],reasons[2],reasons[3],legal_rejections,reserved_rejections);
    std::puts("STRICT_REJECTIONS_REMAIN_ENFORCED=YES; PS2_HARDWARE_EQUIVALENCE=NOT_VERIFIED");
}
