#include <runtime/kernel/vu0.h>
#include <runtime/kernel/ee_memory.h>
#include <cmath>
#include <glm/glm.hpp>
#include <iostream>


// PS2 float representation of PI: bit pattern 0x40490fdb
static constexpr float PS2_PI = 3.14159274101257324218750f; // exact value of 0x40490fdb
static constexpr float PS2_TWO_PI = PS2_PI * 2.0f;

namespace rac1 {

void vu0_nlerp_q15(const Q15Quat* src, const Q15Quat* dst, Q15Quat* out, uint16_t blend_weight) {
    int32_t comp = 0x8000 - blend_weight;
    out->x = static_cast<int16_t>(((static_cast<int32_t>(src->x) * comp + static_cast<int32_t>(dst->x) * blend_weight) >> 15));
    out->y = static_cast<int16_t>(((static_cast<int32_t>(src->y) * comp + static_cast<int32_t>(dst->y) * blend_weight) >> 15));
    out->z = static_cast<int16_t>(((static_cast<int32_t>(src->z) * comp + static_cast<int32_t>(dst->z) * blend_weight) >> 15));
    out->w = static_cast<int16_t>(((static_cast<int32_t>(src->w) * comp + static_cast<int32_t>(dst->w) * blend_weight) >> 15));
}

Q15Quat vu0_quat_mul(const Q15Quat& p, const Q15Quat& q) {
    Q15Quat out;
    out.w = static_cast<int16_t>(((static_cast<int32_t>(p.w) * q.w - static_cast<int32_t>(p.x) * q.x - static_cast<int32_t>(p.y) * q.y - static_cast<int32_t>(p.z) * q.z) >> 15));
    out.x = static_cast<int16_t>(((static_cast<int32_t>(p.w) * q.x + static_cast<int32_t>(p.x) * q.w + static_cast<int32_t>(p.y) * q.z - static_cast<int32_t>(p.z) * q.y) >> 15));
    out.y = static_cast<int16_t>(((static_cast<int32_t>(p.w) * q.y - static_cast<int32_t>(p.x) * q.z + static_cast<int32_t>(p.y) * q.w + static_cast<int32_t>(p.z) * q.x) >> 15));
    out.z = static_cast<int16_t>(((static_cast<int32_t>(p.w) * q.z + static_cast<int32_t>(p.x) * q.y - static_cast<int32_t>(p.y) * q.x + static_cast<int32_t>(p.z) * q.w) >> 15));
    return out;
}

// The PS2 vcallms 400/402 routines use a minimax polynomial approximation
// over the full circle, but for our runtime plain libc sin/cos with the
// correct PS2-PI constant is indistinguishable at float precision.

float vu0_sin(float rad) {
    return std::sin(rad);
}

float vu0_cos(float rad) {
    return std::cos(rad);
}

void vu0_vec3_add(float dest[4], const float a[4], const float b[4]) {
    dest[0] = a[0] + b[0];
    dest[1] = a[1] + b[1];
    dest[2] = a[2] + b[2];
    dest[3] = 0.0f;
}

void vu0_vec3_sub(float dest[4], const float a[4], const float b[4]) {
    dest[0] = a[0] - b[0];
    dest[1] = a[1] - b[1];
    dest[2] = a[2] - b[2];
    dest[3] = 0.0f;
}

void vu0_vec3_scale(float dest[4], const float src[4], float scale) {
    dest[0] = src[0] * scale;
    dest[1] = src[1] * scale;
    dest[2] = src[2] * scale;
    dest[3] = 0.0f;
}

float angle_wrap_normalize(float angle, float delta) {
    float sum = angle + delta;
    // Bring into [-PI, PI] by subtracting/adding 2*PI.
    while (sum >  PS2_PI) sum -= PS2_TWO_PI;
    while (sum < -PS2_PI) sum += PS2_TWO_PI;
    return sum;
}

void vu0_mat3x4_transform(float dest[4], const float mat[3][4], const float v[4]) {
    dest[0] = mat[0][0]*v[0] + mat[0][1]*v[1] + mat[0][2]*v[2] + mat[0][3];
    dest[1] = mat[1][0]*v[0] + mat[1][1]*v[1] + mat[1][2]*v[2] + mat[1][3];
    dest[2] = mat[2][0]*v[0] + mat[2][1]*v[1] + mat[2][2]*v[2] + mat[2][3];
    dest[3] = 1.0f;
}

void camera_clip_matrix_vu0() {
    float target_x = ee_read<float>(0x13f350 + 0x80);
    float target_y = ee_read<float>(0x13f350 + 0x84);
    
    // Validation guard: if target coordinates are invalid (uninitialized), skip update
    if (target_x < 2.0f || target_y < 2.0f || target_x > 65280.0f || target_y > 65280.0f) {
        static int fail_count = 0;
        if (fail_count < 10) {
            std::cout << "[camera_clip_matrix_vu0] Validation FAILED! target_x=" << target_x 
                      << ", target_y=" << target_y << std::endl;
            fail_count++;
        }
        return;
    }
    
    float yaw = ee_read<float>(0x13f350 + 0x98);
    float pitch = ee_read<float>(0x13f350 + 0x110);
    
    // Calculate right and look direction vectors
    glm::vec3 look(
        std::sin(yaw) * std::cos(pitch),
        std::cos(yaw) * std::cos(pitch),
        std::sin(pitch)
    );
    glm::vec3 right = glm::normalize(glm::cross(look, glm::vec3(0.0f, 0.0f, 1.0f)));
    glm::vec3 up = glm::cross(right, look);
    glm::vec3 look_back = -look; // look vector points away in view matrix
    
    // Determine camera position
    glm::vec3 cam_pos;
    uint32_t active_cam = ee_read<uint32_t>(0x13f350 + 0x2080);
    if (active_cam != 0) {
        cam_pos.x = ee_read<float>(active_cam + 0x10);
        cam_pos.y = ee_read<float>(active_cam + 0x14);
        cam_pos.z = ee_read<float>(active_cam + 0x18);
    } else {
        float r = ee_read<float>(0x13f350 + 0x234);
        if (r <= 0.0f) r = 4.0f;
        cam_pos = glm::vec3(target_x, target_y, ee_read<float>(0x13f350 + 0x88)) - look * r;
    }
    
    // Translate positions
    float t0 = -glm::dot(right, cam_pos);
    float t1 = -glm::dot(up, cam_pos);
    float t2 = -glm::dot(look_back, cam_pos);
    
    // Write camera orientation + translation to 0x166c80
    float rot_r0[4] = { right.x, right.y, right.z, t0 };
    float rot_r1[4] = { up.x,    up.y,    up.z,    t1 };
    float rot_r2[4] = { look_back.x, look_back.y, look_back.z, t2 };
    for (int i = 0; i < 4; ++i) {
        ee_write<float>(0x166c80 + i * 4, rot_r0[i]);
        ee_write<float>(0x166c90 + i * 4, rot_r1[i]);
        ee_write<float>(0x166ca0 + i * 4, rot_r2[i]);
    }
    
    // Perspective projection factors
    float p11 = 1.0f / std::tan(glm::radians(60.0f) * 0.5f);
    float aspect = 1.34f;
    float p00 = p11 / aspect;
    
    // Build final view-projection matrix rows
    float vp_r0[4] = { right.x * p00, right.y * p00, right.z * p00, t0 * p00 };
    float vp_r1[4] = { up.x * p11,    up.y * p11,    up.z * p11,    t1 * p11 };
    float vp_r2[4] = { look.x,        look.y,        look.z,        -glm::dot(look, cam_pos) };
    
    // Write world->clip projection matrix to 0x16ccc0
    for (int i = 0; i < 4; ++i) {
        ee_write<float>(0x16ccc0 + i * 4, vp_r0[i]);
        ee_write<float>(0x16ccd0 + i * 4, vp_r1[i]);
        ee_write<float>(0x16cce0 + i * 4, vp_r2[i]);
    }

    static int print_count = 0;
    if (print_count < 5) {
        std::cout << "[camera_clip_matrix_vu0] vp_r0: " << vp_r0[0] << ", " << vp_r0[1] << ", " << vp_r0[2] << ", " << vp_r0[3] << std::endl;
        std::cout << "[camera_clip_matrix_vu0] vp_r1: " << vp_r1[0] << ", " << vp_r1[1] << ", " << vp_r1[2] << ", " << vp_r1[3] << std::endl;
        std::cout << "[camera_clip_matrix_vu0] vp_r2: " << vp_r2[0] << ", " << vp_r2[1] << ", " << vp_r2[2] << ", " << vp_r2[3] << std::endl;
        print_count++;
    }
}

} // namespace rac1
