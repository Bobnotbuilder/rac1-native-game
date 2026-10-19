// MIT. Native Metal fixed-function state, preserving the existing GS/MRP reference equations.
#include "gs_blend_metal.h"

namespace rac1::metal {
namespace {
MTLBlendFactor native_factor(BlendFactor factor) {
    switch (factor) {
        case BlendFactor::Zero: return MTLBlendFactorZero;
        case BlendFactor::One: return MTLBlendFactorOne;
        case BlendFactor::SourceAlpha: return MTLBlendFactorSourceAlpha;
        case BlendFactor::DestinationAlpha: return MTLBlendFactorDestinationAlpha;
        case BlendFactor::ConstantAlpha: return MTLBlendFactorBlendAlpha;
        case BlendFactor::OneMinusSourceAlpha: return MTLBlendFactorOneMinusSourceAlpha;
        case BlendFactor::OneMinusDestinationAlpha: return MTLBlendFactorOneMinusDestinationAlpha;
        case BlendFactor::OneMinusConstantAlpha: return MTLBlendFactorOneMinusBlendAlpha;
    }
    __builtin_trap();
}
}
bool configure_gs_blend(MTLRenderPipelineColorAttachmentDescriptor* attachment,
                        const GsBlendState& state, BlendPolicy policy) {
    if (!attachment || (state.reference_approximation && policy == BlendPolicy::RequireDirectFactorization)) return false;
    attachment.blendingEnabled = state.enabled;
    attachment.sourceRGBBlendFactor = native_factor(state.source);
    attachment.destinationRGBBlendFactor = native_factor(state.destination);
    attachment.rgbBlendOperation = state.operation == BlendOperation::Subtract ? MTLBlendOperationSubtract
                                 : state.operation == BlendOperation::ReverseSubtract ? MTLBlendOperationReverseSubtract : MTLBlendOperationAdd;
    // GS alpha is written from source, never blended with destination alpha.
    attachment.sourceAlphaBlendFactor = MTLBlendFactorOne;
    attachment.destinationAlphaBlendFactor = MTLBlendFactorZero;
    attachment.alphaBlendOperation = MTLBlendOperationAdd;
    return true;
}
void encode_gs_blend_constant(id<MTLRenderCommandEncoder> encoder, const GsBlendState& state) {
    [encoder setBlendColorRed:0 green:0 blue:0 alpha:state.constant_alpha];
}
} // namespace rac1::metal
