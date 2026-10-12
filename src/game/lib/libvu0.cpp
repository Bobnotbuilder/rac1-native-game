#include <game/lib/libvu0.h>
#include <game/ovl/vu0_macro.h>

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace rac1 {


// orig: 0x00125180
// vmul.xyz vf5,vf4,vf4 ; vaddy.x ; vaddz.x ; vsqrt Q,vf5x ; vaddq.x vf5,vf0,Q ;
// vdiv Q,vf0w,vf5x ; vsub.xyzw vf6,vf0,vf0 ; vmulq.xyz vf6,vf4,Q
void vu0_normalize(Vec4& d, const Vec4& s) {
    const float x = vuc(s.x), y = vuc(s.y), z = vuc(s.z);
    float l2 = vuc(vuc(x * x) + vuc(y * y));
    l2 = vuc(l2 + vuc(z * z));
    const float len = vuc(0.0f + vuc(std::sqrt(std::fabs(l2))));
    float q;
    if (len == 0.0f) q = std::signbit(len) ? -FLT_MAX : FLT_MAX;
    else q = vuc(1.0f / len);
    d = Vec4{vuc(x * q), vuc(y * q), vuc(z * q), 0.0f};
}

// orig: 0x00125218
// vsub vf4,vf0,vf0 ; vadd.w vf4,vf4,vf0 ; vmr32 vf5,vf4 ; vmr32 vf6,vf5 ; vmr32 vf7,vf6 ;
// sqc2 vf4..vf7 to rows 3..0
void vu0_unit_matrix(Mat4& m) {
    namespace v = vu0m;
    const float z = v::sub(0.0f, 0.0f), o = v::add(z, 1.0f);
    m.r[3] = Vec4{z, z, z, o};
    m.r[2] = Vec4{z, z, o, z};
    m.r[1] = Vec4{z, o, z, z};
    m.r[0] = Vec4{o, z, z, z};
}

// orig: 0x00125240
// vf5 = the table 0x132e00; vf4.x = x; x2 = x * x; vf8 = vf5 * x, then times x2 lane by lane so
// that w, z, y, x hold the x^3 .. x^9 terms added to vf4.x in that order; s = vf4.x copied to .xy;
// Q = sqrt(1 - s * s); .x = 0 +- Q.
void vu0_sincos(float x, bool neg, float& out_x, float& out_y) {
    namespace v = vu0m;
    const v::Vf c = v::lqc2(0x00132e00);
    float acc = v::add(0.0f, x);
    const float x2 = v::mul(x, x);
    float p[4];
    for (int i = 0; i < 4; ++i) p[i] = v::mul(c.v[i], x);
    for (int i = 0; i < 4; ++i) p[i] = v::mul(p[i], x2);
    for (int i = 0; i < 3; ++i) p[i] = v::mul(p[i], x2);
    acc = v::add(acc, p[3]);
    for (int i = 0; i < 2; ++i) p[i] = v::mul(p[i], x2);
    acc = v::add(acc, p[2]);
    p[0] = v::mul(p[0], x2);
    acc = v::add(acc, p[1]);
    acc = v::add(acc, p[0]);
    const float s = v::add(0.0f, acc);
    const float one_m = v::sub(1.0f, v::mul(s, s));
    const float q = vuc(std::sqrt(std::fabs(vuc(one_m))));
    const float r = v::add(0.0f, q);
    out_x = neg ? v::sub(0.0f, r) : v::add(0.0f, r);
    out_y = s;
}

namespace {
// orig: (helper) repeated in 0x001252b8 / 0x00125360 / 0x00125408: the FPU argument pi/2 -+ a of
// the sine kernel (a < 0: pi/2 + a, the result's .x negated)
void rot_sincos(float a, float& sn, float& cs) {
    const float half_pi = u2f(0x3fc90fdbu);
    const bool neg = a < 0.0f;
    const float x = neg ? fpu_add(half_pi, a) : fpu_sub(half_pi, a);
    vu0_sincos(x, neg, sn, cs);
}
// orig: (helper) repeated in 0x001252b8 / 0x00125360 / 0x00125408: the four rows of s through the
// rotation rows r (VMULAx / VMADDAy / VMADDAz / VMADDw), row by row into d
void rot_apply(Mat4& d, const Mat4& s, const vu0m::Vf r[4]) {
    for (int i = 0; i < 4; ++i) {
        const Vec4 row = s.r[i];
        const vu0m::Vf o = vu0m::transform(r, row.x, row.y, row.z, row.w);
        d.r[i] = Vec4{o.v[0], o.v[1], o.v[2], o.v[3]};
    }
}
}  // namespace

// orig: 0x001252b8
// rows: (cos, sin, 0, 0), (-sin, cos, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1)
void vu0_rot_matrix_z(Mat4& d, const Mat4& s, float a) {
    namespace v = vu0m;
    float sn, cs;
    rot_sincos(a, sn, cs);
    const float z = v::sub(0.0f, 0.0f);
    v::Vf r[4];
    r[0] = v::Vf{{v::add(0.0f, cs), v::add(0.0f, sn), 0.0f, 0.0f}};
    r[1] = v::Vf{{v::sub(0.0f, sn), v::add(0.0f, cs), 0.0f, 0.0f}};
    r[2] = v::Vf{{z, z, 1.0f, z}};
    r[3] = v::Vf{{z, z, z, 1.0f}};
    rot_apply(d, s, r);
}

// orig: 0x00125360
// rows: (1, 0, 0, 0), (0, cos, sin, 0), (0, -sin, cos, 0), (0, 0, 0, 1)
void vu0_rot_matrix_x(Mat4& d, const Mat4& s, float a) {
    namespace v = vu0m;
    float sn, cs;
    rot_sincos(a, sn, cs);
    v::Vf r[4];
    r[0] = v::Vf{{v::add(0.0f, 1.0f), 0.0f, 0.0f, 0.0f}};
    r[1] = v::Vf{{0.0f, v::add(0.0f, cs), v::add(0.0f, sn), 0.0f}};
    r[2] = v::Vf{{0.0f, v::sub(0.0f, sn), v::add(0.0f, cs), 0.0f}};
    r[3] = v::Vf{{0.0f, 0.0f, 0.0f, v::add(0.0f, 1.0f)}};
    rot_apply(d, s, r);
}

// orig: 0x00125408
// rows: (cos, 0, -sin, 0), (0, 1, 0, 0), (sin, 0, cos, 0), (0, 0, 0, 1)
void vu0_rot_matrix_y(Mat4& d, const Mat4& s, float a) {
    namespace v = vu0m;
    float sn, cs;
    rot_sincos(a, sn, cs);
    v::Vf r[4];
    r[0] = v::Vf{{v::add(0.0f, cs), 0.0f, v::sub(0.0f, sn), 0.0f}};
    r[1] = v::Vf{{0.0f, v::add(0.0f, 1.0f), 0.0f, 0.0f}};
    r[2] = v::Vf{{v::add(0.0f, sn), 0.0f, v::add(0.0f, cs), 0.0f}};
    r[3] = v::Vf{{0.0f, 0.0f, 0.0f, v::add(0.0f, 1.0f)}};
    rot_apply(d, s, r);
}

}  // namespace rac1
