#ifndef RAC1_RUNTIME_GS_STATE_H
#define RAC1_RUNTIME_GS_STATE_H

#include <runtime/kernel/ee_types.h>
#include <cstdint>
#include <vector>

namespace rac1 {

enum class PrimitiveType {
    Point = 0,
    Line = 1,
    LineStrip = 2,
    Triangle = 3,
    TriangleStrip = 4,
    TriangleFan = 5,
    Sprite = 6
};

struct GsVertex {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float f = 0.0f;
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
    float s = 0.0f;
    float t = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    bool adc = false;
};

struct GsPrimitive {
    PrimitiveType type;
    std::vector<GsVertex> vertices;
    uint64_t prim_reg = 0;
    uint64_t tex0_reg = 0;
    uint64_t alpha_reg = 0;
    uint64_t scissor_reg = 0;
    uint64_t test_reg = 0;
    uint64_t frame_reg = 0;
    uint64_t zbuf_reg = 0;
    bool is_sky = false;
};

struct GsState {
    // General-purpose GS registers (raw 64-bit values)
    uint64_t prim = 0;
    uint64_t rgbaq = 0;
    uint64_t st = 0;
    uint64_t uv = 0;
    uint64_t fog = 0;
    uint64_t tex0_1 = 0;
    uint64_t tex0_2 = 0;
    uint64_t clamp_1 = 0;
    uint64_t clamp_2 = 0;
    uint64_t tex1_1 = 0;
    uint64_t tex1_2 = 0;
    uint64_t tex2_1 = 0;
    uint64_t tex2_2 = 0;
    uint64_t xyoffset_1 = 0x710000006c00ULL;
    uint64_t xyoffset_2 = 0x710000006c00ULL;
    uint64_t prmodecont = 1;
    uint64_t prmode = 0;
    uint64_t texclut = 0;
    uint64_t scanmsk = 0;
    uint64_t miptbp1_1 = 0;
    uint64_t miptbp1_2 = 0;
    uint64_t miptbp2_1 = 0;
    uint64_t miptbp2_2 = 0;
    uint64_t texa = 0;
    uint64_t fogcol = 0;
    uint64_t texflush = 0;
    uint64_t scissor_1 = 0;
    uint64_t scissor_2 = 0;
    uint64_t alpha_1 = 0;
    uint64_t alpha_2 = 0;
    uint64_t dimx = 0;
    uint64_t dthe = 0;
    uint64_t colclamp = 0;
    uint64_t test_1 = 0;
    uint64_t test_2 = 0;
    uint64_t pabe = 0;
    uint64_t fba_1 = 0;
    uint64_t fba_2 = 0;
    uint64_t frame_1 = 0;
    uint64_t frame_2 = 0;
    uint64_t zbuf_1 = 0;
    uint64_t zbuf_2 = 0;
    uint64_t bitbltbuf = 0;
    uint64_t trxpos = 0;
    uint64_t trxreg = 0;
    uint64_t trxdir = 0;
    uint64_t hwreg = 0;
    uint64_t signal = 0;
    uint64_t finish = 0;
    uint64_t label = 0;

    // Intermediate state for primitive assembly
    std::vector<GsVertex> vertex_queue;

    // Log of drawn primitives
    std::vector<GsPrimitive> primitives;

    void reset();
};

// Parse a DMA/GIF packet starting at vaddr, updating the GS state and logging primitives
// Returns the size of the parsed packet in bytes
uint32_t parse_dma_packet(u32 vaddr, GsState& state);

extern bool g_parsing_sky;
extern GsState* g_active_gs_state;

} // namespace rac1

#endif // RAC1_RUNTIME_GS_STATE_H
