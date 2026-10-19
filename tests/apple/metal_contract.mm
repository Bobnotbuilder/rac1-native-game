// Synthetic, source-derived GS fragment contracts. No copyrighted assets or captured game data.
#ifdef NDEBUG
#error Metal contracts require assertions enabled
#endif
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

struct Fixture { simd_float4 vertex, texel; simd_uint4 state; };
static_assert(sizeof(Fixture) == 48);
static simd_float4 expected(Fixture f) {
    simd_float4 out{};
    for (int c = 0; c < 3; ++c)
        out[c] = f.state.x == 1 ? f.texel[c] : f.texel[c] * f.vertex[c] * (255.0f / 128.0f) + (f.state.x >= 2 ? f.vertex.w : 0);
    out.w = !f.state.y ? f.vertex.w : f.state.x == 0 ? f.texel.w * f.vertex.w * (255.0f / 128.0f)
                                                 : f.state.x == 2 ? f.texel.w + f.vertex.w : f.texel.w;
    for (int c = 0; c < 4; ++c) out[c] = std::clamp(out[c], 0.0f, 1.0f);
    return out;
}
int main(int argc, const char** argv) {
    @autoreleasepool {
        assert(argc == 2);
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) { std::fprintf(stderr, "SKIP: no Metal device; not a PASS\n"); return 77; }
        NSError* error = nil;
        id<MTLLibrary> lib = [device newLibraryWithURL:[NSURL fileURLWithPath:@(argv[1])] error:&error];
        if (!lib) { std::fprintf(stderr, "%s\n", error.description.UTF8String); return 1; }
        id<MTLComputePipelineState> pipeline = [device newComputePipelineStateWithFunction:[lib newFunctionWithName:@"rac1_gs_contract"] error:&error];
        assert(pipeline);
        std::vector<Fixture> fixtures;
        for (unsigned tfx = 0; tfx < 4; ++tfx)
            for (unsigned tcc = 0; tcc < 2; ++tcc)
                for (unsigned atst = 0; atst < 8; ++atst)
                    for (unsigned alpha : {0u, 1u, 127u, 128u, 254u, 255u})
                        for (unsigned ref : {0u, 1u, 127u, 128u, 254u, 255u})
                            fixtures.push_back({{0.3f, 0.7f, 0.9f, float(alpha) / 255}, {0.2f, 0.8f, 0.6f, 128.0f / 255}, {tfx, tcc, atst, ref}});
        id<MTLBuffer> input = [device newBufferWithBytes:fixtures.data() length:fixtures.size() * sizeof(Fixture) options:MTLResourceStorageModeShared];
        id<MTLBuffer> output = [device newBufferWithLength:fixtures.size() * 2 * sizeof(simd_float4) options:MTLResourceStorageModeShared];
        id<MTLCommandQueue> queue = [device newCommandQueue];
        id<MTLCommandBuffer> command = [queue commandBuffer];
        id<MTLComputeCommandEncoder> encoder = [command computeCommandEncoder];
        [encoder setComputePipelineState:pipeline];
        [encoder setBuffer:input offset:0 atIndex:0];
        [encoder setBuffer:output offset:0 atIndex:1];
        [encoder dispatchThreads:MTLSizeMake(fixtures.size(), 1, 1) threadsPerThreadgroup:MTLSizeMake(pipeline.threadExecutionWidth, 1, 1)];
        [encoder endEncoding];
        [command commit];
        [command waitUntilCompleted]; // verification executable only; never the production frame loop
        assert(command.status == MTLCommandBufferStatusCompleted);
        auto* result = static_cast<const simd_float4*>(output.contents);
        for (size_t i = 0; i < fixtures.size(); ++i) {
            const auto f = fixtures[i];
            const auto want = expected(f);
            for (int c = 0; c < 4; ++c) assert(std::fabs(want[c] - result[2 * i][c]) < 2e-6f);
            const float a = want.w * 255;
            const float ref = f.state.w;
            const bool passes[8] = {false, true, a < ref, a <= ref, std::fabs(a - ref) < .5f, a >= ref, a > ref, std::fabs(a - ref) >= .5f};
            const bool pass = passes[f.state.z];
            unsigned mask = 0;
            for (unsigned af = 0; af < 4; ++af)
                for (unsigned p = 0; p < 2; ++p)
                    if (pass || (p == 0 ? (af == 1 || af == 3) : af == 2)) mask |= 1u << (af * 2 + p);
            assert(result[2 * i + 1].x == float(pass));
            assert(result[2 * i + 1].y == float(mask));
        }
        std::printf("PASS: %zu Metal GS texture/alpha fixtures (not rendering/gameplay acceptance)\n", fixtures.size());
    }
}
