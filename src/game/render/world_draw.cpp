// See world_draw.h.
// orig: (host) none of this is the game's
#include <game/render/world_draw.h>

#include <algorithm>
#include <cmath>
#include <runtime/gs/gs_renderer.h>
#include <runtime/kernel/ee_memory.h>

namespace rac1 {

WorldFrustum::WorldFrustum(const glm::mat4& m) {
    // clip = m * (x y z 1); the planes are rows 3 +- 0 and 3 +- 1 of m
    const int row[4] = {0, 0, 1, 1};
    const float sign[4] = {1.0f, -1.0f, 1.0f, -1.0f};
    for (int i = 0; i < 4; ++i)
        for (int c = 0; c < 4; ++c) p[i][c] = m[c][3] + sign[i] * m[c][row[i]];
}

bool WorldFrustum::box_outside(const glm::vec3& mn, const glm::vec3& mx) const {
    for (int i = 0; i < 4; ++i) {
        const float a = p[i][0], b = p[i][1], c = p[i][2], d = p[i][3];
        // the box's corner furthest along the plane's normal: if even it is behind the plane, all are
        const float x = a > 0.0f ? mx.x : mn.x, y = b > 0.0f ? mx.y : mn.y, z = c > 0.0f ? mx.z : mn.z;
        const float dist = a * x + b * y + c * z + d;
        const float mag = std::fabs(a * x) + std::fabs(b * y) + std::fabs(c * z) + std::fabs(d);
        if (dist < -(1e-4f * mag + 1e-3f)) return true;
    }
    return false;
}

unsigned int WorldMaterials::resolve(int material) {
    if (material < 0) return 0;
    const std::size_t m = static_cast<std::size_t>(material);
    if (m >= stamp_.size()) {
        stamp_.resize(m + 1, 0);
        tex_.resize(m + 1, 0);
    }
    if (stamp_[m] == frame_) return tex_[m];
    const std::uint32_t packed_ptr = ee_read<std::uint32_t>(table_ + static_cast<std::uint32_t>(material) * 4);
    const std::uint32_t pixel_addr = packed_ptr & 0x0FFFFFFF;
    gs_bind_texture_vaddr(tbp_base_ + static_cast<std::uint32_t>(material), pixel_addr);
    tex_[m] = get_or_upload_texture(tbp_base_ + static_cast<std::uint32_t>(material));
    stamp_[m] = frame_;
    return tex_[m];
}

void world_box_of(const void* vertices, std::size_t count, glm::vec3& mn, glm::vec3& mx) {
    const float* v = static_cast<const float*>(vertices);
    mn = glm::vec3(1e30f);
    mx = glm::vec3(-1e30f);
    for (std::size_t i = 0; i < count; ++i, v += 9) {
        mn = glm::min(mn, glm::vec3(v[0], v[1], v[2]));
        mx = glm::max(mx, glm::vec3(v[0], v[1], v[2]));
    }
}

}  // namespace rac1
