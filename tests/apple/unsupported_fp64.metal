// Negative capability probe, NEVER part of the renderer. The existing tfrag GLSL uses double.
#include <metal_stdlib>
using namespace metal;
kernel void fp64_probe(device double* values [[buffer(0)]], uint i [[thread_position_in_grid]]) {
    values[i] = values[i] * 1.5;
}
