// Ported overlay code 0x002b0000..0x002dffff (moby spawners used by the hero).
#include <game/ovl/ovl_common.h>

#include <game/ovl/ovl_reloc.h>
namespace rac1 {

// orig: 0x002ce1f0 — spawn moby class 0x1df at pos with value `val` (pvar +0x44): records its
// heading/pitch/distance from the hero (pvar +0x28/+0x24/+0x20) and the hero height (+0x3c),
// and files it in the first free of the 6 slots PLAYER+0x2020 (index in pvar +0x54).
uint32_t spawn_moby_1df_v(uint32_t val, const Vec4& pos) {
    const uint32_t m = moby_spawn(0x1df);
    if (m == 0) return 0;
    const uint32_t P = g::PLAYER;
    sb(m + 0x30, 0xff);
    sh(m + 0x32, 0xff);
    sb(m + 0x31, 1);
    sb(m + 0x20, 2);
    const uint32_t pv = lw(m + 0x78);
    sb(m + 0xbc, 0);
    V(m + 0x10) = pos;
    sw(pv + 0x44, val);
    sb(P + 0x1ff6, lbu(P + 0x1ff6) + 1u);
    moby_update_transform(m);
    moby_copy_hero_color(m);
    swc1(pv + 0x28, atan2_f(lwc1(m + 0x10) - lwc1(P + 0x80), lwc1(m + 0x14) - lwc1(P + 0x84)));
    const float d = vec2_distance(V(P + 0x80), V(m + 0x10));
    swc1(pv + 0x24, atan2_f(d, lwc1(m + 0x18) - lwc1(P + 0x88)));
    swc1(pv + 0x20, vec2_distance(V(m + 0x10), V(P + 0x80)));
    sh(pv + 0x54, 0);
    swc1(pv + 0x3c, lwc1(P + 0x88));
    for (;;) {
        const int32_t i = lh(pv + 0x54);
        if (!(i < 6)) break;
        const uint32_t slot = P + 0x2020 + static_cast<uint32_t>(i) * 4;
        if (lw(slot) == 0) {
            sw(slot, m);
            break;
        }
        sh(pv + 0x54, lhu(pv + 0x54) + 1u);
    }
    sh(pv + 0x56, 100);
    sw(m + 0x94, 0);
    return m;
}
// orig: 0x002ce1f0 (EE-address form)
uint32_t spawn_moby_1df(uint32_t val, uint32_t pos) { return spawn_moby_1df_v(val, V(pos)); }

// orig: 0x002d7e90 — spawn moby class 0x307 at pos, scaled by `scale`, hero-tinted, with a
// random spin angle +0x48.
uint32_t spawn_moby_307_v(const Vec4& pos, float scale) {
    const uint32_t m = moby_spawn(0x307);
    if (m == 0) return 0;
    moby_copy_hero_color(m);
    moby_set_color_hi(m, 0x28, 0x28, 0x46);
    sb(m + 0x30, 0xff);
    const float s = lwc1(m + 0x2c) * scale;
    sh(m + 0x32, 0x40);
    swc1(m + 0x2c, s);
    V(m + 0x10) = pos;
    swc1(m + 0x48, rand_angle());
    moby_update_transform(m);
    return m;
}
// orig: 0x002d7e90 (EE-address form)
uint32_t spawn_moby_307(uint32_t pos, float scale) { return spawn_moby_307_v(V(pos), scale); }


// orig: 0x002cdcc0 — use item 0x18 (if owned and in stock, sub_00233e98/0x233db8): bump its
// use counters (0x141740 +0xc0/+0xc2, level bit in +0xc4) and spawn agents (moby 0x1df) in
// front of the hero until there are 6 (count PLAYER+0x1ff6), each linked to the previous one
// (pvar +0x44) and given the launch vector (0.25 along the facing, 0.25 up). The first agent
// links to whatever the caller had in $s1 (`prev`); the vector's w is uninitialised stack.
void use_agents(uint32_t prev) {
    const uint32_t P = g::PLAYER;
    sb(P + 0x1ff5, 0);
    if (sub_00233e98(0x18) == 0) return;
    Vec4 launch{0.0f, 0.0f, 0.0f, 0.0f};
    launch.x = cos_vu(lwc1(P + 0x98)) * 0.25f;
    launch.y = sin_vu(lwc1(P + 0x98)) * 0.25f;
    launch.z = 0.25f;
    if (!(lbu(P + 0x1ff6) < 6u)) return;
    sub_00233db8(0x18, 1);
    constexpr uint32_t S = 0x00141680;
    const uint32_t uses = lhu(S + 0xc0);
    if (!(0xfffeu < uses)) sh(S + 0xc0, uses + 1);
    if (static_cast<int32_t>(lhu(S + 0xc2)) < frame_scale_i(lw(0x0015eea4)) / 600) sh(S + 0xc2, static_cast<uint32_t>(frame_scale_i(lw(OV(0x00166c00u - 0x7d5c))) / 600));
    sw(S + 0xc4, static_cast<uint32_t>(lw(S + 0xc4)) | (1u << (static_cast<uint32_t>(lw(0x0015ed84)) & 31)) | 0x80000000u);
    Vec4 pos;
    vec3_add(pos, V(P + 0x80), V(lw(P + 0x2080) + 0xe0));
    vec3_add(pos, pos, V(lw(P + 0x2080) + 0xc0));
    for (int32_t n = lbu(P + 0x1ff6); n < 6; ++n) {
        const uint32_t m = spawn_moby_1df_v(prev, pos);
        prev = m;
        if (m != 0) V(lw(m + 0x78)) = launch;
    }
}

}  // namespace rac1
