// MIT. Metal backend state derived from runtime/gs/gs_draw.cpp::blend_state, upstream 3fd14c12.
// No game decisions or game data. Keep the OpenGL renderer untouched as the reference.
#pragma once
#include <cstdint>

namespace rac1::metal {
enum class BlendFactor : uint8_t {
    Zero, One, SourceAlpha, DestinationAlpha, ConstantAlpha,
    OneMinusSourceAlpha, OneMinusDestinationAlpha, OneMinusConstantAlpha
};
enum class BlendOperation : uint8_t { Add, Subtract, ReverseSubtract };
enum BlendApproximation : uint8_t {
    BlendExact = 0,
    BlendOnePlusFactor = 1,
    BlendFixedAlphaClamped = 2
};
struct GsBlendState {
    bool enabled = false;
    BlendFactor source = BlendFactor::One, destination = BlendFactor::Zero;
    BlendOperation operation = BlendOperation::Add;
    float constant_alpha = 0;
    // The upstream GL path approximates 1+C and clamps FIX above 128. Do not conceal these
    // limitations or call this PS2-hardware verification. A strict caller may reject them.
    bool reference_approximation = false;
    uint8_t approximation_reasons = BlendExact;
};
inline GsBlendState gs_blend_state(uint64_t prim, uint64_t alpha) {
    GsBlendState out;
    if (!(prim & (uint64_t{1} << 6))) return out;
    out.enabled = true;
    const unsigned a = alpha & 3, b = (alpha >> 2) & 3;
    const unsigned c = (alpha >> 4) & 3, d = (alpha >> 6) & 3;
    const unsigned fix = (alpha >> 32) & 255;
    out.constant_alpha = fix < 128 ? float(fix) / 128.0f : 1.0f;
    const BlendFactor f = c == 2 ? BlendFactor::ConstantAlpha : c == 1 ? BlendFactor::DestinationAlpha : BlendFactor::SourceAlpha;
    const BlendFactor inverse = c == 2 ? BlendFactor::OneMinusConstantAlpha : c == 1 ? BlendFactor::OneMinusDestinationAlpha : BlendFactor::OneMinusSourceAlpha;
    auto factor = [&](unsigned who, BlendFactor& value) {
        const int k0 = d == who ? 1 : 0;
        const int k1 = int(a == who) - int(b == who);
        if (!k1) { value = k0 ? BlendFactor::One : BlendFactor::Zero; return 1; }
        if (c == 2 && fix > 128) out.approximation_reasons |= BlendFixedAlphaClamped;
        if (!k0) { value = f; return k1; }
        if (k1 > 0) out.approximation_reasons |= BlendOnePlusFactor;
        value = k1 < 0 ? inverse : BlendFactor::One;
        return 1;
    };
    const int source_sign = factor(0, out.source), destination_sign = factor(1, out.destination);
    if (source_sign >= 0 && destination_sign >= 0) out.operation = BlendOperation::Add;
    else if (source_sign < 0 && destination_sign >= 0) out.operation = BlendOperation::ReverseSubtract;
    else if (source_sign >= 0) out.operation = BlendOperation::Subtract;
    else out.source = out.destination = BlendFactor::Zero;
    out.reference_approximation = out.approximation_reasons != BlendExact;
    return out;
}
} // namespace rac1::metal
