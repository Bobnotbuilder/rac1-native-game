// Aridia's own moby classes, part 1: the small props, platforms and mechanisms. Header
// aridia_mobys.h. Addresses are Aridia's (`orig: L2:`); data only Aridia has
// is L2_ADDR, the globals the engine shares with Veldin are the level's own (OV), the base ELF's are bare.
#include <game/math/vecmath.h>
#include <game/ovl/aridia_mobys.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <game/port_stubs.h>

namespace rac1 {

// Class 331 (Trackdrive Left): a moby of the level's initial array (below the spawn slots 0x15ffdc) clears
// its pvar word +0x58 and hides itself (+0x34 |= 0x40).
// orig: L2:0x002cb978
void trackdrive_left_update_l2(uint32_t m) {
    if (!(m < lwu(OV(0x0015ffdc)))) return;
    const uint32_t flags = lhu(m + 0x34);
    sw(m + 0x58, 0);
    sh(m + 0x34, flags | 0x40u);
}

// Classes 690 (Liftcar), 789 (Liftcar Pole), 1820: the update function is a bare `jr ra` (static scenery); also the
// hero-worn item classes 433 (item 5, the one Skid's agent gives), 618 (item 0x22) and 1289 (item 6), the same bare `jr ra`.
// orig: L2:0x002dd4d0 (also 0x002e2220, 0x002f2058, 0x002cbd98, 0x002daf58, 0x002ee2a0)
void empty_update_l2(uint32_t) {}

// Classes 707 and 734 (Platform): turn about z by 0.0873 (fs6c x 0x3db2b8c2) a frame and set the attached frame
// (pvar +0x20) from the zero position and the angles as they were.
// orig: L2:0x002ddc00 (also 0x002df8c0)
void platform_update_l2(uint32_t m) {
    const uint32_t pv = lwu(m + 0x78);
    const Vec4 zero = {0.0f, 0.0f, 0.0f, 0.0f};
    const Vec4 base = V(m + 0x40);
    swc1(m + 0x48, angle_add(lwc1(m + 0x48), lwc1(0x0015ed6c) * u2f(0x3db2b8c2u)));
    attach_frame_set_v(pv + 0x20, zero, base, V(m + 0x40));
}

// A waiting time in frames, 60 fs-scaled: rand_range_f(lo, hi) x the frame scale, as a byte at m+0xbc.
// orig: (helper) the tails of 0x002dca10 / 0x002dd370 (frame_scale_f 0x1fef10, float_to_int 0x200250)
void wait_frames_set(uint32_t m, float lo, float hi) {
    const float r = rand_range_f(lo, hi);
    const float s = frame_scale_f(r);
    sb(m + 0xbc, static_cast<uint32_t>(float_to_int(s)));
}

// Class 656 (Bulldozer): animation 0 / 2 play out (+0x70 bit 1), then 1 / 3 for 180..240 frames (the byte timer
// +0xbc), then back to 2 / 0.
// orig: L2:0x002dca10
void bulldozer_update_l2(uint32_t m) {
    const uint32_t anim = lbu(m + 0x53);
    if (anim == 1) {
        if (countdown_u8(m + 0xbc) == 0) return;
        if (lbu(m + 0x53) != 2) moby_anim_set(m, 2, 0, 0);
        return;
    }
    if (anim == 0 || anim == 2) {
        if ((lbu(m + 0x70) & 2) == 0) return;
        moby_anim_set(m, anim == 0 ? 1u : 3u, 0, 0);
        wait_frames_set(m, 180.0f, 240.0f);
        return;
    }
    if (anim == 3) {
        if (countdown_u8(m + 0xbc) == 0) return;
        if (lbu(m + 0x53) != 0) moby_anim_set(m, 0, 0, 0);
    }
}

// Class 675 (Grab Crane): animation 0 plays out into 2; 2 waits a timer (+0xbc) into 3; 3 plays out into 7 and sets
// the timer to 60..180 frames; 7 waits it out into 0.
// orig: L2:0x002dd370
void grab_crane_update_l2(uint32_t m) {
    const uint32_t anim = lbu(m + 0x53);
    if (anim == 2) {
        if (countdown_u8(m + 0xbc) == 0) return;
        if (lbu(m + 0x53) == 3) return;
        moby_anim_set(m, 3, 0, frame_scale_i(5));
        return;
    }
    if (anim == 0) {
        if ((lbu(m + 0x70) & 2) == 0) return;
        moby_anim_set(m, 2, 0, frame_scale_i(5));
        wait_frames_set(m, 60.0f, 180.0f);
        return;
    }
    if (anim == 3) {
        if ((lbu(m + 0x70) & 2) == 0) return;
        moby_anim_set(m, 7, 0, frame_scale_i(5));
        wait_frames_set(m, 60.0f, 180.0f);
        return;
    }
    if (anim == 7) {
        if (countdown_u8(m + 0xbc) == 0) return;
        if (lbu(m + 0x53) == 0) return;
        moby_anim_set(m, 0, 0, frame_scale_i(5));
    }
}

// Class 1819 (White Flower): state 0 -> 1; in state 1 a hit record (sub_0025a420 with the mask 0x10000) with a
// positive damage (+0x2c) -> state 2; state 2 bursts: the sound, a bolt drop (4..7, the extra pair of flags 2 near
// the hero), a debris burst of class 0x71d and a copy of class 0x71c, and the moby is removed.
// orig: L2:0x002f1f60
void white_flower_update_l2(uint32_t m) {
    const uint32_t hit = sub_0025a420(m, 0x10000, 0);
    const uint32_t st = lbu(m + 0x20);
    if (st == 1) {
        bool hurt = false;
        if (hit != 0 && 0.0f < lwc1(hit + 0x2c)) hurt = true;
        if (hurt) sb(m + 0x20, 2);
        return;
    }
    if (st == 0) {
        sb(m + 0x20, 1);
        return;
    }
    if (st != 2) return;
    moby_sound_start(0, 0, m);
    plant_bolts_drop_l1(m);
    debris_burst(m, 0x71d, 1, 0x71d, 1, 5, 2);
    moby_clone_l1(m, 0x71c);
    moby_retire(m);
}

// The Trespasser doors (classes 743 / 744): state 0 keeps the start position in the pvar (+0..); state 1 waits
// for the moby the pvar +0x10 names (class 0x267 or 0x23f, the Trespasser pad's) to reach state 4, then opens
// (state 2; door 1 with its sound 0); state 2 slides the door by 2 (door 2: -2) along its direction +0xd0 from
// the start position, at most 2 frames' worth of the speed per frame, until it arrives (state 3; door 1 with its
// sound 1).
// orig: (helper) the shared body of 0x002dffc8 / 0x002e0138
static void trespasser_door(uint32_t m, float len, bool sounds) {
    const uint32_t pv = lwu(m + 0x78);
    const uint32_t st = lbu(m + 0x20);
    if (st == 0) {
        lq_sq(pv, m + 0x10);
        sb(m + 0x20, 1);
        return;
    }
    if (st == 1) {
        const int32_t idx = lw(pv + 0x10);
        if (idx < 0) return;
        const uint32_t t = lwu(OV(0x0015ffd8)) + (static_cast<uint32_t>(idx) << 8);
        const int32_t cls = lh(t + 0xa6);
        if (cls != 0x267 && cls != 0x23f) return;
        if (lbu(t + 0x20) != 4) return;
        sb(m + 0x20, 2);
        if (sounds) moby_sound_start(0, 0, m);
        return;
    }
    if (st != 2) return;
    const uint32_t buf = ee_stack_alloc(0x20);
    Vec4& to = V(buf);
    Vec4& delta = V(buf + 0x10);
    vec3_set_length(to, V(m + 0xd0), len);
    vec3_add(to, V(pv), to);
    vec3_sub(delta, to, V(m + 0x10));
    vec3_clamp_length_v(delta, lwc1(0x0015ed6c) + lwc1(0x0015ed6c));
    vec3_add(V(m + 0x10), V(m + 0x10), delta);
    if (vec3_length(delta) == 0.0f) {
        if (sounds) moby_sound_start(1, 0, m);
        sb(m + 0x20, 3);
    }
    ee_stack_free(0x20);
}

// Class 743 (Tresspasser Door1).
// orig: L2:0x002dffc8
void trespasser_door1_update_l2(uint32_t m) { trespasser_door(m, 2.0f, true); }

// Class 744 (Tresspasser Door2): the same door the other way (-2), silent.
// orig: L2:0x002e0138
void trespasser_door2_update_l2(uint32_t m) { trespasser_door(m, -2.0f, false); }

// Classes 735 / 736 (Rocket Door1 / 2): a door that swings open by quaternion from its start rotation (pvar +0..: the
// moby's angles +0x40 as a quaternion) to the open rotation (pvar +0x10..: the euler angles in degrees, made a
// quaternion on the first frame). Hidden (+0x34 bit 1, +0x31 0) in game mode 2 until the flag +0xbc; state 0
// starts it (already opened when its checkpoint flag 0x14c050[level * 16 + +0xb0] is set: state 3, the open angles),
// state 1 waits for +0xbc, state 2 turns the blend +0x24 to 1 (yaw_seek, speed +0x20) and sets the matrix +0xc0
// from the nlerp of the two rotations.
// orig: L2:0x002df948
void rocket_door_update_l2(uint32_t m) {
    const uint32_t pv = lwu(m + 0x78);
    uint32_t flags;
    if (lw(OV(0x0015f5c4)) == 2 && lbu(m + 0xbc) == 0) {
        sb(m + 0x31, 0);
        flags = lhu(m + 0x34) | 1u;
    } else {
        flags = lhu(m + 0x34);
        sb(m + 0x31, 1);
        flags &= 0xfffeu;
    }
    sh(m + 0x34, flags);
    const uint32_t st = lbu(m + 0x20);
    if (st == 1) {
        if (lbu(m + 0xbc) == 0) return;
        sb(m + 0x20, 2);
        sw(pv + 0x24, 0);
        sw(pv + 0x20, 0);
        return;
    }
    if (st == 2) {
        const float fs70 = lwc1(0x0015ed70);
        yaw_seek(pv + 0x24, pv + 0x20, 1.0f, fs70 * 0.5f, fs70 * 0.5f, lwc1(0x0015ed6c));
        const uint32_t buf = ee_stack_alloc(0x10);
        quat_nlerp(V(buf), V(pv), V(pv + 0x10), lwc1(pv + 0x24));
        quat_to_mat3(V(buf), M(m + 0xc0));
        ee_stack_free(0x10);
        return;
    }
    if (st != 0) return;
    lq_sq(pv, m + 0x40);
    sw(pv + 0xc, 0);
    const uint32_t level = static_cast<uint32_t>(lw(0x0015ed84));
    if (lbu(0x0014c050 + lbu(m + 0xb0) + (level << 4)) == 0xff) {
        swc1(m + 0x40, lwc1(pv + 0x10) * u2f(0x3c8efa35u));
        swc1(m + 0x44, lwc1(pv + 0x14) * u2f(0x3c8efa35u));
        sb(m + 0x20, 3);
        swc1(m + 0x48, lwc1(pv + 0x18) * u2f(0x3c8efa35u));
        return;
    }
    sb(m + 0x20, 1);
    sh(m + 0x34, lhu(m + 0x34) | 0x100u);
    const uint32_t buf = ee_stack_alloc(0x50);
    const float k = u2f(0x3c8efa35u);
    quat_from_axis_angle(V(buf), 0, lwc1(pv + 0x10) * k);
    quat_from_axis_angle(V(buf + 0x10), 1, lwc1(pv + 0x14) * k);
    quat_from_axis_angle(V(buf + 0x20), 2, lwc1(pv + 0x18) * k);
    quat_mul(V(buf + 0x30), V(buf), V(buf + 0x10));
    quat_mul(V(pv + 0x10), V(buf + 0x30), V(buf + 0x20));
    quat_from_axis_angle(V(buf), 0, lwc1(m + 0x40));
    quat_from_axis_angle(V(buf + 0x10), 1, lwc1(m + 0x44));
    quat_from_axis_angle(V(buf + 0x20), 2, lwc1(m + 0x48));
    quat_mul(V(buf + 0x40), V(buf), V(buf + 0x10));
    quat_mul(V(pv), V(buf + 0x40), V(buf + 0x20));
    ee_stack_free(0x50);
}

// Whether the sound channel `slot` (-1: none), if it is this moby's and playing, is stopped: the idiom the original
// inlines (the channel record is 0x70 bytes at 0x13e550: owner +0x88, playing +0x74).
// orig: (helper) repeated in 0x002df1a8 (the novalis_platforms.cpp one is the same code)
static void sound_release_l2(uint32_t m, int32_t slot) {
    if (slot == -1) return;
    const uint32_t c = 0x0013e550 + static_cast<uint32_t>(slot) * 0x70;
    if (lwu(c + 0x88) != m) return;
    if (lbu(c + 0x74) == 0) return;
    sub_0028d918(slot);
}

// Class 732 (Crane): the animations 1 / 3 / 5 / 7 play out (+0x70 bit 1) then the byte +0xbc (the next animation)
// is entered, the old sound released (the pvar +0, the channel) and the wait 180..240 frames set; 0, 2 and 4 wait
// out the timer +0xbc and pick the next of the cycle (0 -> 1, 2 -> 3 or 7, 4 -> 5), +0xbc then 2 / 4 / 0 / 2, with its sound
// (sound 0 / 4, the channel kept in the pvar +0). The speed word +0x58 is set from 0x161c68 each frame.
// orig: L2:0x002df1a8 (the jump table 0x1faa80 on the animation +0x53)
void crane_update_l2(uint32_t m) {
    const uint32_t pv = lwu(m + 0x78);
    const float spd = lwc1(L2_ADDR(0x00161c68));
    const uint32_t anim = lbu(m + 0x53);
    swc1(m + 0x58, spd);
    if (!(anim < 8)) return;
    switch (anim) {
    case 1: case 3: case 5: case 7: {
        if ((lbu(m + 0x70) & 2) == 0) return;
        const uint32_t next = lbu(m + 0xbc);
        const bool differs = lbu(m + 0x53) != next;
        sw(m + 0x54, 0);  // (the branch's delay slot: stored either way)
        if (differs) moby_anim_set(m, next, 0, 0);
        sound_release_l2(m, lw(pv));
        sw(pv, 0xffffffffu);
        moby_sound_start(1, 0, m);
        const float r = rand_range_f(180.0f, 240.0f);
        sb(m + 0xbc, static_cast<uint32_t>(float_to_int(frame_scale_f(r))));
        return;
    }
    case 0:
    case 4: {
        if (countdown_u8(m + 0xbc) == 0) return;
        const uint32_t to = anim == 0 ? 1u : 5u;
        if (lbu(m + 0x53) != to) moby_anim_set(m, to, 0, frame_scale_i(10));
        sb(m + 0xbc, 2);  // (2 either way: the branch's delay slot loads it)
        sw(pv, static_cast<uint32_t>(moby_sound_start(0, 4, m)));
        return;
    }
    case 2: {
        if (countdown_u8(m + 0xbc) == 0) return;
        if ((rand_mod(0xff) & 1) != 0) {
            if (lbu(m + 0x53) != 3) moby_anim_set(m, 3, 0, frame_scale_i(10));
            sb(m + 0xbc, 4);  // (4 either way: the branch's delay slot loads it)
        } else {
            if (lbu(m + 0x53) != 7) moby_anim_set(m, 7, 0, frame_scale_i(10));
            sb(m + 0xbc, 0);
        }
        sw(pv, static_cast<uint32_t>(moby_sound_start(0, 4, m)));
        return;
    }
    default:
        return;
    }
}

// Classes 652 / 653 (Escalator, Escalator End): the belt's offset vector (pvar +0x40, along the moby's yaw +0x48)
// run back and forth: state 0 clears it and starts the wait (the frame-scaled word 0x161c18, +0xa0); state 1 moves
// it forward by the speed 0x161c14 x fs6c (the stored belt speed +0x58 = 1 / -1), until a wait of that length,
// then eases back by 0x161c1c x fs70 steps; state 2 the other way; state 3 holds the end position. The attach
// frame (pvar +0x60) follows from the offset and the angles.
// orig: L2:0x002dc6b0
void escalator_update_l2(uint32_t m) {
    const uint32_t pv = lwu(m + 0x78);
    if (pv == 0) return;
    const uint32_t st = lbu(m + 0x20);
    const uint32_t pos = pv + 0x40;
    const uint32_t wait_frames = L2_ADDR(0x00161c18), speed = L2_ADDR(0x00161c14), accel = L2_ADDR(0x00161c1c);
    auto put = [&](float len) {  // the offset vector along the yaw: (cos, sin, 0) x len
        swc1(pos, cos_vu(lwc1(m + 0x48)) * len);
        swc1(pos + 4, sin_vu(lwc1(m + 0x48)) * len);
        sw(pv + 0x48, 0);
    };
    if (st == 0) {
        vec_clear(V(pos));
        const int32_t t = frame_scale_i(lw(wait_frames));
        const uint32_t flags = lwu(pv + 0x9c) | 4u;
        sw(pv + 0xa0, static_cast<uint32_t>(t));
        sw(pv + 0x9c, flags);
        sb(m + 0x20, lwu(pv + 0xa4) != 0 ? 3u : 1u);
    } else if (st == 1) {
        swc1(m + 0x58, 1.0f);
        float len;
        if (timer_tick_i32(*static_cast<int32_t*>(ee_ram_ptr(pv + 0xa0))) != 0) {
            len = vec3_length(V(pos)) - lwc1(accel) * lwc1(0x0015ed70);
            if (len <= 0.0f) {
                sw(pv + 0xa0, static_cast<uint32_t>(frame_scale_i(lw(wait_frames))));
                sb(m + 0x20, 2);
            } else {
                const float n = -len;
                swc1(pos, cos_vu(lwc1(m + 0x48)) * n);
                swc1(pos + 4, sin_vu(lwc1(m + 0x48)) * n);
                swc1(pv + 0x48, 0.0f);
            }
        } else {
            len = vec3_length(V(pos));
            const float lim = abs_f(lwc1(speed) * lwc1(0x0015ed6c));
            if (len < lim) len = len + lwc1(accel) * lwc1(0x0015ed70);
            put(-len);
        }
        swc1(m + 0x58, fpu_div(len, lwc1(speed) * lwc1(0x0015ed6c)));
    } else if (st == 2) {
        swc1(m + 0x58, -1.0f);
        float len;
        if (timer_tick_i32(*static_cast<int32_t*>(ee_ram_ptr(pv + 0xa0))) != 0) {
            len = vec3_length(V(pos)) - lwc1(accel) * lwc1(0x0015ed70);
            if (len <= 0.0f) {
                sw(pv + 0xa0, static_cast<uint32_t>(frame_scale_i(lw(wait_frames))));
                sb(m + 0x20, 1);
            } else {
                swc1(pos, cos_vu(lwc1(m + 0x48)) * len);
                swc1(pos + 4, sin_vu(lwc1(m + 0x48)) * len);
                swc1(pv + 0x48, 0.0f);
            }
        } else {
            len = vec3_length(V(pos));
            const float lim = abs_f(lwc1(speed) * lwc1(0x0015ed6c));
            if (len < lim) len = len + lwc1(accel) * lwc1(0x0015ed70);
            put(len);
        }
        swc1(m + 0x58, fpu_div(len, -(lwc1(speed) * lwc1(0x0015ed6c))));
    } else if (st == 3) {
        const float k = -(lwc1(speed) * lwc1(0x0015ed6c));
        swc1(pos, cos_vu(lwc1(m + 0x48)) * k);
        sw(pv + 0x48, 0);
        swc1(pos + 4, sin_vu(lwc1(m + 0x48)) * k);
    }
    attach_frame_set_v(pv + 0x60, V(pos), V(m + 0x40), V(m + 0x40));
}

// Append (fn, arg) to the third draw-hook list of the frame (64 at most: 0x16dc80 / 0x16dd80, count 0x15f42c);
// Veldin's two lists' appenders are 0x1f93e0 / 0x1f9598.
// orig: L2:0x0020a390
void draw_hook_add_dc80_l2(uint32_t fn, uint32_t arg) {
    const int32_t n = lw(OV(0x0015f42c));
    if (!(n < 0x40)) return;
    sw(OV(0x0016dc80) + static_cast<uint32_t>(n) * 4, fn);
    sw(OV(0x0016dd80) + static_cast<uint32_t>(n) * 4, arg);
    sw(OV(0x0015f42c), static_cast<uint32_t>(n + 1));
}

// The ocean's wave table: the header (four words from 0x1612a0..0x1612ac) and 248 floats of the table 0x1cb510
// scaled by `k` into 0x1cb9d0 (the original also stores a byte 0x80 at +0x3f of its caller's dead frame).
// orig: L2:0x002a4058 (through 0x002a40d0, 0x002a47f8)
void ocean_wave_scale_l2(float k) {
    const uint32_t dst = L2_ADDR(0x001cb9c0), src = L2_ADDR(0x001cb500);
    sw(dst, lwu(L2_ADDR(0x001612a0)));
    sw(dst + 4, lwu(L2_ADDR(0x001612a4)));
    sw(dst + 8, lwu(L2_ADDR(0x001612a8)));
    sw(dst + 0xc, lwu(L2_ADDR(0x001612ac)));
    for (uint32_t i = 0; i < 0xf8; ++i) swc1(dst + 0x10 + i * 4, lwc1(src + 0x10 + i * 4) * k);
}

// Class 854 (Ocean): state 0 builds the wave table (k = 1/6) and shows the moby (+0x30 = 0xff); state 1 each
// frame hooks the ocean's draw function 0x2ea048 (GS packets: not ported) into the draw list.
// orig: L2:0x002ea198
void ocean_update_l2(uint32_t m) {
    const uint32_t st = lbu(m + 0x20);
    if (st == 0) {
        ocean_wave_scale_l2(u2f(0x3e2aaaabu));
        sb(m + 0x30, 0xff);
        sb(m + 0x20, 1);
        return;
    }
    if (st == 1) draw_hook_add_dc80_l2(L2_ADDR(0x002ea048), m);
}

}  // namespace rac1
