// Ported overlay code: the Walloper (item 0x12, hand moby class 0xb4). Its hand moby's update 0x2bb558
// keeps the swing's trail: ten ribbon segments (u16 lives at 0x1dbec0, five points each at 0x1dbba0 +
// 0x50 * i: the base and four chained points) started every frame of the hero's swing (state 0x20,
// the swing animation past 6) at the point behind the hand (the hand's +0xd0 vector scaled -0.5 from
// its place), whose points then drift by a small random amount while the segment lives; the trail's
// fade progress goes to 0x1616d4 and the draw callback 0x2bbb00 is registered.
#include <game/math/vecmath.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/weapons.h>
#include <game/port_stubs.h>

namespace rac1 {

namespace {
constexpr uint32_t P = 0x0013f350;
}  // namespace

// The Walloper's hand frame (see the file's head). State 0 clears the segments' tables (lives
// 0x1dbec0, 0x1dbef0 = 2, 0x1dbf08 = 1) and starts the two timers 0x1616cc / 0x1616d0; state 2 runs
// the swing: not in the hero's state 0x20 the timers are restarted and the animation set 1 played
// (12 frames); in it, with the animation time (0x24edc8) past 6, the trail is fed (a flag), the first
// timer measures the swing's start (progress 0x1616d4 = 1 - left / period), past 12 the second one
// its end (progress left / period, 1 between, 0 when both ran out). Then each of the ten segments
// ages (-1 a frame): a free one is started when fed (life 2, a random direction of length 0x1616a4,
// four points chained by rotating it about the line to the camera by 0.17..0.79 rad, alternating
// the sign); a live one drifts its points (each offset plus a random 0..0.1 on each axis) from its
// new base point; its draw weight 0x1dbed8 is 0x20 while it lives (0x40 when started).
// orig: 0x002bb558
void walloper_update(uint32_t m) {
    const uint32_t frame = ee_stack_alloc(0x140);
    const uint32_t sp = frame;
    const uint32_t A = OV(0x001dbec0), Bt = OV(0x001dbef0), Ct = OV(0x001dbf08), D = OV(0x001dbed8), PTS = OV(0x001dbba0);
    sw(sp + 0x80, 0);
    if (lbu(lwu(P + 0x2080) + 0x53) != 0x82) swc1(OV(0x0017aebc), u2f(0x3dcccccdu));
    const uint32_t st = lbu(m + 0x20);
    if (st == 0) {
        sb(m + 0x20, 2);
        for (uint32_t i = 0; i < 10; ++i) {
            sh(A + i * 2, 0);
            sh(Bt + i * 2, 2);
            sh(Ct + i * 2, 1);
        }
        sw(OV(0x001616cc), static_cast<uint32_t>(frame_scale_i(lw(OV(0x001616a8)))));
        sw(OV(0x001616d0), static_cast<uint32_t>(frame_scale_i(lw(OV(0x001616ac)))));
    } else if (st == 2) {
        if (lw(P + 0x2084) != 0x20) {
            sw(OV(0x001616cc), static_cast<uint32_t>(frame_scale_i(lw(OV(0x001616a8)))));
            sw(OV(0x001616d0), static_cast<uint32_t>(frame_scale_i(lw(OV(0x001616ac)))));
            const int32_t a3 = frame_scale_i(0xc);
            moby_anim_set(m, 1, 0, a3);
        } else {
            float f20 = moby_anim_time(m);
            if (u2f(0x40c00000u) <= f20) {
                sw(sp + 0x80, 1);
                if (timer_tick_i32(*static_cast<int32_t*>(ee_ram_ptr(OV(0x001616cc)))) != 0) {
                    if (!(u2f(0x41400000u) <= f20)) {
                        swc1(OV(0x001616d4), 1.0f);
                    } else if (timer_tick_i32(*static_cast<int32_t*>(ee_ram_ptr(OV(0x001616d0)))) != 0) {
                        sw(OV(0x001616d4), 0);
                    } else {
                        f20 = int_to_float(lw(OV(0x001616d0)));
                        const float f0 = int_to_float(frame_scale_i(lw(OV(0x001616ac))));
                        swc1(OV(0x001616d4), fpu_div(f20, f0));
                    }
                } else {
                    f20 = int_to_float(lw(OV(0x001616cc)));
                    const float f0 = int_to_float(frame_scale_i(lw(OV(0x001616a8))));
                    f20 = fpu_div(f20, f0);
                    swc1(OV(0x001616d4), 1.0f - f20);
                }
            }
        }
    }
    vec3_scale(V(sp), V(m + 0xd0), u2f(0xbf000000u));
    vec3_add(V(sp), V(sp), V(m + 0x10));
    bool any = lh(A) != 0;
    for (int32_t a1 = 1; !any && a1 < 10; ++a1) any = lh(A + static_cast<uint32_t>(a1) * 2) != 0;
    if (lw(sp + 0x80) == 0 && !any) {
        ee_stack_free(0x140);
        return;
    }
    for (int32_t s4 = 0; s4 < 10;) {
        const uint32_t a = A + static_cast<uint32_t>(s4) * 2;
        const int32_t v1 = static_cast<int32_t>(lhu(a)) - 1;
        sh(a, static_cast<uint32_t>(v1));
        if (static_cast<int32_t>(static_cast<uint32_t>(v1) << 16) < 0) sh(a, 0);
        const uint32_t base = PTS + static_cast<uint32_t>(s4) * 0x50;
        if (lh(a) == 0) {
            if (lw(sp + 0x80) == 0) {
                ++s4;
                continue;
            }
            sh(a, 2);
            sh(D + static_cast<uint32_t>(s4) * 2, 0x40);
            const float f = lwc1(OV(0x001616a4));
            rand_dir_scaled(sp + 0x10, f, f);
            V(base) = V(sp);
            float f20 = 1.0f;
            for (int32_t s3 = 3; s3 >= 0; --s3) {
                const uint32_t pk = base + static_cast<uint32_t>(3 - s3) * 0x10;
                vec3_sub(V(sp + 0x20), V(pk), V(OV(0x00166dc0)));
                const float r = rand_range_f(u2f(0x3e32b8c2u), u2f(0x3f490fdbu));
                const float ang = r * f20;
                vec_rotate_axis(V(sp + 0x10), V(sp + 0x10), V(sp + 0x20), ang);
                f20 = -f20;
                vec3_add(V(pk + 0x10), V(pk), V(sp + 0x10));
            }
        } else {
            for (int32_t s3 = 3; s3 >= 0; --s3) {
                const uint32_t k = static_cast<uint32_t>(3 - s3);
                vec3_sub(V(sp + 0x40 + k * 0x10), V(base + 0x10 + k * 0x10), V(base + k * 0x10));
                const float x = lwc1(sp + 0x40 + k * 0x10) + rand_range_signed_f(0.0f, u2f(0x3dcccccdu));
                swc1(sp + 0x40 + k * 0x10, x);
                const float y = lwc1(sp + 0x44 + k * 0x10) + rand_range_signed_f(0.0f, u2f(0x3dcccccdu));
                swc1(sp + 0x44 + k * 0x10, y);
                const float z = lwc1(sp + 0x48 + k * 0x10) + rand_range_signed_f(0.0f, u2f(0x3dcccccdu));
                swc1(sp + 0x48 + k * 0x10, z);
            }
            V(base) = V(sp);
            for (int32_t s3 = 3; s3 >= 0; --s3) {
                const uint32_t k = static_cast<uint32_t>(3 - s3);
                vec3_add(V(base + 0x10 + k * 0x10), V(base + k * 0x10), V(sp + 0x40 + k * 0x10));
            }
            if (lh(a) > 0) sh(D + static_cast<uint32_t>(s4) * 2, 0x20);
            else sh(D + static_cast<uint32_t>(s4) * 2, 0);
        }
        ++s4;
    }
    draw_callback_add(OVF(0x002bbb00), m);
    ee_stack_free(0x140);
}

// orig: (none) — the Walloper's update for the hand moby's indirect call.
void walloper_fns_register() {
    ovl_register_moby_fn(0x002bb558, walloper_update);
}

}  // namespace rac1
