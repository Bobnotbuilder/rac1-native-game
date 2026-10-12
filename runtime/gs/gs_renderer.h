#ifndef RAC1_RUNTIME_GS_RENDERER_H
#define RAC1_RUNTIME_GS_RENDERER_H

#include <runtime/gs/gs_state.h>

namespace rac1 {

struct GLVertex {
    float x, y, z;
    float r, g, b, a;
    float u, v;
};

class GsRenderer {
public:
    GsRenderer();
    ~GsRenderer();

    // Compile shaders, setup VAO, VBO, etc.
    bool init();

    // Render the primitives logged in the GsState
    void render(const GsState& state, int screen_width, int screen_height);

    // Free OpenGL resources
    void shutdown();

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_shader_program = 0;
};

// Clear the frame and depth buffers with a given color
void gs_clear_buffers(float r, float g, float b, float a);

// Associate a fake TBP0 with a real EE RAM pixel virtual address
void gs_bind_texture_vaddr(uint32_t tbp0, uint32_t pixel_vaddr);

// Get or upload a texture to OpenGL
unsigned int get_or_upload_texture(uint32_t tbp0);
// The uploaded textures and their bindings dropped (another level's are registered next).
void gs_texture_cache_clear();

// The GL sampler object that samples as the GS does under a CLAMP_1 value: WMS (bits 0..1) and
// WMT (bits 2..3), 0 REPEAT, 1 CLAMP (the coordinate clamped to the texture's edge texels);
// the filters those of the uploaded textures. The region modes (2, 3) are not modelled: drawn
// as CLAMP. Bind it to the texture unit for the draw, and unbind (sampler 0) after.
unsigned int gs_clamp_sampler(uint64_t clamp);

} // namespace rac1

#endif // RAC1_RUNTIME_GS_RENDERER_H
