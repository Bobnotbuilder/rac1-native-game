// Host helpers shared by the world renderers (tfrag / tie / shrub): the view's side-plane culling of the
// static draw calls, the per-frame texture resolution of a material, and the drawing of the visible calls
// in their own order with the consecutive ones of the same state merged into one multi-draw.
// orig: (host) none of this is the game's
#ifndef RAC1_GAME_RENDER_WORLD_DRAW_H
#define RAC1_GAME_RENDER_WORLD_DRAW_H

#include <cstdint>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <runtime/gs/static_geom_cache.h>

namespace rac1 {

// The four side planes of a view-projection matrix (left, right, bottom, top), as the clip volume
// -w <= x <= w, -w <= y <= w. The near / far planes are not used: the world passes run with
// GL_DEPTH_CLAMP, which keeps what is in front of / behind them.
struct WorldFrustum {
    float p[4][4];
    explicit WorldFrustum(const glm::mat4& m);
    // true when the box is entirely outside one side plane (so every triangle inside it is clipped
    // away: the plane test is linear, as the clip). Conservative with a margin far above float error.
    bool box_outside(const glm::vec3& mn, const glm::vec3& mx) const;
};

// The GL texture of a material (an index of a texture table in EE memory), resolved once per frame
// (the table is read each frame: the game may rewrite it) instead of once per draw call.
class WorldMaterials {
public:
    // table: the EE address of the renderer's texture table; tbp_base: the renderer's TBP0 range
    WorldMaterials(std::uint32_t table, std::uint32_t tbp_base) : table_(table), tbp_base_(tbp_base) {}
    void begin_frame() { ++frame_; }
    unsigned int resolve(int material);  // as the per-draw-call code did: bind the vaddr, get / upload
private:
    std::uint32_t table_, tbp_base_;
    std::uint32_t frame_ = 0;
    std::vector<std::uint32_t> stamp_;
    std::vector<unsigned int> tex_;
};

// What a draw call needs bound; two calls with an equal key can be one draw.
struct WorldStateKey {
    bool use_texture = false, blended = false, masked = false;
    unsigned int tex = 0, sampler = 0;
    bool operator==(const WorldStateKey&) const = default;
};

// Draws the calls `visible` (indices into `calls`, in order) from `geom`: `key(dc)` gives a call's
// state key, `apply(key, prev)` sets the GL state for `key` (prev: the previous key or null for the
// first). Consecutive calls with the same key become one glMultiDrawElements (their index ranges
// adjacent in the buffer merge into one range).
template <class KeyFn, class ApplyFn>
void world_draw_runs(const std::vector<StaticDrawCall>& calls, const std::vector<int>& visible, unsigned int vao, KeyFn key, ApplyFn apply) {
    static std::vector<GLsizei> counts;
    static std::vector<const void*> offs;
    counts.clear();
    offs.clear();
    glBindVertexArray(vao);
    WorldStateKey cur;
    bool have = false;
    auto flush = [&] {
        if (!counts.empty()) glMultiDrawElements(GL_TRIANGLES, counts.data(), GL_UNSIGNED_INT, offs.data(), static_cast<GLsizei>(counts.size()));
        counts.clear();
        offs.clear();
    };
    for (int idx : visible) {
        const StaticDrawCall& dc = calls[idx];
        const WorldStateKey k = key(dc);
        if (!have || !(k == cur)) {
            flush();
            apply(k, have ? &cur : nullptr);
            cur = k;
            have = true;
        }
        const std::uintptr_t off = static_cast<std::uintptr_t>(dc.first_index) * sizeof(unsigned int);
        if (!counts.empty() && reinterpret_cast<std::uintptr_t>(offs.back()) + static_cast<std::uintptr_t>(counts.back()) * sizeof(unsigned int) == off)
            counts.back() += dc.index_count;
        else {
            counts.push_back(dc.index_count);
            offs.push_back(reinterpret_cast<const void*>(off));
        }
    }
    flush();
}

// The box of a vertex list's positions (x y z first in each 9 floats), for the draw call's culling box.
void world_box_of(const void* vertices, std::size_t count, glm::vec3& mn, glm::vec3& mx);

}  // namespace rac1

#endif
