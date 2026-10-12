#ifndef RAC1_RUNTIME_GS_STATIC_GEOM_CACHE_H
#define RAC1_RUNTIME_GS_STATIC_GEOM_CACHE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

namespace rac1 {

extern bool g_use_gpu_rendering;

// The vertices and indices of all the draw calls of one world renderer (tfrag / tie / shrub) in ONE
// vertex buffer and ONE index buffer: the draw calls are ranges of the index buffer, in the order they
// were built. The vertex layout is the host's NativeVertex (x y z, r g b a, u v: 9 floats).
struct StaticMergedGeom {
    unsigned int vao = 0, vbo = 0, ebo = 0;
    std::vector<float> vtx;         // staging until upload()
    std::vector<unsigned int> idx;  // staging until upload()
    // Appends a draw call's vertices (nv * 9 floats) and its indices (relative to them); returns the
    // first index of the draw call in the merged index buffer.
    unsigned int append(const void* vertices, std::size_t nv, const unsigned int* indices, std::size_t ni);
    void upload();   // creates the VAO / buffers from the staging data
    void release();  // frees the GL objects
    unsigned int base_vertex_count() const { return static_cast<unsigned int>(vtx.size() / 9); }
};

struct StaticDrawCall {
    int index_count = 0;
    unsigned int first_index = 0;  // in the renderer's StaticMergedGeom index buffer

    // The box of the draw call's vertices (world space, exact): the view's side planes cull with it.
    glm::vec3 aabb_min = {0.0f, 0.0f, 0.0f};
    glm::vec3 aabb_max = {0.0f, 0.0f, 0.0f};

    // Culling parameters (world space)
    std::array<float, 4> bsphere = {0.0f, 0.0f, 0.0f, 0.0f};
    float max_draw_dist = 0.0f;
    glm::vec3 world_center = {0.0f, 0.0f, 0.0f};

    // GS State parameters
    int material_index = -1;
    bool use_texture = false;
    bool is_blended = false;
    bool is_masked = false;
    uint64_t tex0_reg = 0;
    uint64_t test_reg = 0;
    uint64_t zbuf_reg = 0;
    uint64_t alpha_reg = 0;
    uint64_t prim_reg = 0;
    uint64_t clamp_reg = 0;  // CLAMP_1 (WMS / WMT) as the level setup writes it for the texture
};

// Global lists of draw calls built at level load
extern std::vector<StaticDrawCall> g_static_tfrag_draw_calls;
extern std::vector<StaticDrawCall> g_static_tie_draw_calls;
extern std::vector<StaticDrawCall> g_static_shrub_draw_calls;

// The buffers of the three lists
extern StaticMergedGeom g_static_tfrag_geom;
extern StaticMergedGeom g_static_tie_geom;
extern StaticMergedGeom g_static_shrub_geom;

// Lists of visible draw call indices populated per frame during culling update
extern std::vector<int> g_visible_tfrag_indices;
extern std::vector<int> g_visible_tie_indices;
extern std::vector<int> g_visible_shrub_indices;

// Callbacks registered by the game layer to build caches
extern void (*g_tfrag_build_static_cache_callback)();
extern void (*g_tie_build_static_cache_callback)();
extern void (*g_shrub_build_static_cache_callback)();

// Build the OpenGL static geometry cache
void static_geom_cache_init();

// Clear the cache (frees OpenGL buffers)
void static_geom_cache_clear();

} // namespace rac1

#endif // RAC1_RUNTIME_GS_STATIC_GEOM_CACHE_H
