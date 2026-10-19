#pragma once
#include "gs_blend.h"
#import <Metal/Metal.h>

namespace rac1::metal {
enum class BlendPolicy { MatchOpenGLReference, RequireDirectFactorization };
// False leaves the attachment untouched. Callers must not silently continue on a rejected state.
bool configure_gs_blend(MTLRenderPipelineColorAttachmentDescriptor* attachment,
                        const GsBlendState& state, BlendPolicy policy);
// Call on the active render encoder when the batch's state changes (not once per primitive).
void encode_gs_blend_constant(id<MTLRenderCommandEncoder> encoder, const GsBlendState& state);
} // namespace rac1::metal
