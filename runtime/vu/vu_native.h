// The building blocks of the ported micro programs: one VU instruction as
// the VU interpreter runs it (vu_interpreter.cpp, the reference they are proved against) --
// the pipeline around it (the MAC flags seen 4 instructions late, the EFU's Q landing 7 / 13
// instructions after issue, the branch delay slot, the E bit, the XGKICK one instruction late)
// and the operations (VU float rules: operands clamped, results through the MAC flag update).
#ifndef RAC1_RUNTIME_VU_VU_NATIVE_H
#define RAC1_RUNTIME_VU_VU_NATIVE_H

#include <runtime/vu/vu_interpreter.h>
#include <runtime/vu/vu_xgkick.h>

#include <cmath>
#include <cstdint>
#include <cstring>

// the helpers are inlined into the huge generated functions whatever the compiler's growth limits say
#define VUN_INLINE inline __attribute__((always_inline))

namespace rac1::vun {

// The flag and Q registers the fast ports (the fast form, vu_native_fast.h) keep in
// locals while a program runs: the helpers below take either this or a VUState.
struct FlagRegs {
    uint32_t macflag = 0, statusflag = 0, clipflag = 0;
    uint32_t mac_visible = 0, status_visible = 0, clip_visible = 0;
    float q = 0.0f;
};

VUN_INLINE float f_of(uint32_t u) {
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}
VUN_INLINE uint32_t u_of(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
// an operand: denormals signed zero, Inf / NaN the signed largest float
VUN_INLINE float cop(uint32_t u) {
    const uint32_t e = u & 0x7f800000u;
    if (e == 0) u &= 0x80000000u;
    else if (e == 0x7f800000u) u = (u & 0x80000000u) | 0x7f7fffffu;
    return f_of(u);
}
VUN_INLINE float copf(float f) { return cop(u_of(f)); }
// a result lane through the MAC flags (and clamped)
template <class S>
VUN_INLINE uint32_t macw(S& s, int lane, float f) {
    uint32_t v = u_of(f);
    const int sh = 3 - lane;
    const uint32_t sg = v & 0x80000000u;
    if (sg != 0) s.macflag |= 0x10u << sh;
    else s.macflag &= ~(0x10u << sh);
    if (f == 0.0f) {
        s.macflag = (s.macflag & ~(0x1100u << sh)) | (0x1u << sh);
        return v;
    }
    switch ((v >> 23) & 0xff) {
    case 0:
        s.macflag = (s.macflag & ~(0x1000u << sh)) | (0x101u << sh);
        return sg;
    case 255:
        s.macflag = (s.macflag & ~(0x101u << sh)) | (0x1000u << sh);
        return sg | 0x7f7fffffu;
    default:
        s.macflag &= ~(0x1101u << sh);
        return v;
    }
}
template <class S>
VUN_INLINE void macclr(S& s, int lane) { s.macflag &= ~(0x1111u << (3 - lane)); }
template <class S>
VUN_INLINE void status(S& s) {
    uint32_t st = 0;
    if ((s.macflag & 0x000f) != 0) st |= 1;
    if ((s.macflag & 0x00f0) != 0) st |= 2;
    if ((s.macflag & 0x0f00) != 0) st |= 4;
    if ((s.macflag & 0xf000) != 0) st |= 8;
    s.statusflag = (s.statusflag & 0xfc0) | st | (st << 6);
}
template <class S>
VUN_INLINE void status_efu(S& s, uint32_t efu) { s.statusflag = (s.statusflag & 0x3cf) | efu | (efu << 6); }
// MAX / MINI on the words
VUN_INLINE uint32_t fmax_u(uint32_t a, uint32_t b) {
    const int32_t x = static_cast<int32_t>(a), y = static_cast<int32_t>(b);
    return static_cast<uint32_t>((x < 0 && y < 0) ? (x < y ? x : y) : (x > y ? x : y));
}
VUN_INLINE uint32_t fmin_u(uint32_t a, uint32_t b) {
    const int32_t x = static_cast<int32_t>(a), y = static_cast<int32_t>(b);
    return static_cast<uint32_t>((x < 0 && y < 0) ? (x > y ? x : y) : (x < y ? x : y));
}
// FTOIn (the operand not clamped)
VUN_INLINE uint32_t ftoi(uint32_t u, float scale) {
    float f = f_of(u) * scale;
    const uint32_t r = u_of(f);
    if ((r & 0x7f800000u) >= 0x4f000000u) return (r & 0x80000000u) != 0 ? 0x80000000u : 0x7fffffffu;
    return static_cast<uint32_t>(static_cast<int32_t>(f));
}
// the EFU
template <class S>
VUN_INLINE void vdiv(S& s, uint32_t fs_u, uint32_t ft_u) {
    const float ft = cop(ft_u), fs = cop(fs_u);
    uint32_t efu = 0;
    if (ft == 0.0f) {
        efu |= fs == 0.0f ? 0x10u : 0x20u;
        s.q = f_of(((ft_u ^ fs_u) & 0x80000000u) != 0 ? 0xff7fffffu : 0x7f7fffffu);
    } else {
        s.q = copf(fs / ft);
    }
    status_efu(s, efu);
}
template <class S>
VUN_INLINE void vsqrt(S& s, uint32_t ft_u) {
    const float ft = cop(ft_u);
    s.q = copf(std::sqrt(std::fabs(ft)));
    status_efu(s, ft < 0.0f ? 0x10u : 0u);
}
template <class S>
VUN_INLINE void vrsqrt(S& s, uint32_t fs_u, uint32_t ft_u) {
    const float ft = cop(ft_u), fs = cop(fs_u);
    uint32_t efu = 0;
    if (ft == 0.0f) {
        efu |= 0x20;
        const bool n = ((ft_u ^ fs_u) & 0x80000000u) != 0;
        if (fs != 0.0f) s.q = f_of(n ? 0xff7fffffu : 0x7f7fffffu);
        else {
            s.q = f_of(n ? 0x80000000u : 0u);
            efu |= 0x10;
        }
    } else {
        if (ft < 0.0f) efu |= 0x10;
        s.q = copf(fs / std::sqrt(std::fabs(ft)));
    }
    status_efu(s, efu);
}
// CLIP: the flag shifted by 6, the six comparisons of fs's x / y / z with +-|ft.w|
template <class S>
VUN_INLINE void clip(S& s, uint32_t x, uint32_t y, uint32_t z, uint32_t w) {
    const int32_t v = (w & 0x7f800000u) != 0 ? static_cast<int32_t>(w & 0x7fffffffu) : 0x007fffff;
    uint32_t c = s.clipflag << 6;
    const uint32_t a[3] = {x, y, z};
    for (int k = 0; k < 3; ++k) {
        if (static_cast<int32_t>(a[k]) > v) c |= 1u << (2 * k);
        if (static_cast<int32_t>(a[k] ^ 0x80000000u) > v) c |= 2u << (2 * k);
    }
    s.clipflag = c & 0xffffff;
}
// VU data memory addressing (16-byte units, the interpreter's 10-bit wrap)
VUN_INLINE uint32_t* qw(VUState& s, int32_t a) { return reinterpret_cast<uint32_t*>(&s.mem[(static_cast<uint32_t>(a) & 0x3ff) * 16]); }

// an integer register as a conditional branch reads it: the value from before the previous instruction's write when that was an ALU op / MTIR of
// this register (the integer branch hazard, vu_interpreter.cpp)
VUN_INLINE int16_t hzv(const VuPipe& p, const VUState& s, int r) { return p.vi_hz_reg == r ? p.vi_hz_old : s.vi[static_cast<size_t>(r)]; }

// the pipeline around one instruction
VUN_INLINE void begin(VUState& s, VuPipe& p, bool efu) {
    s.mac_visible = p.mac_ring[p.issued & 3];
    s.status_visible = p.status_ring[p.issued & 3];
    s.clip_visible = p.clip_ring[p.issued & 3];
    if (p.q_pending && p.issued >= p.q_ready) {
        s.q = p.q_next;
        p.q_pending = false;
    }
    if (efu && p.q_pending) {
        s.q = p.q_next;
        p.q_pending = false;
    }
}
VUN_INLINE void kick_now(VUState& s, uint32_t addr) {
    if (s.xgkick_hook) s.xgkick_hook(addr);
    else vu_execute_xgkick(s, addr, s.xgkick_primitives);
}
VUN_INLINE void xgkick(VUState& s, VuPipe& p, uint32_t addr) {
    if (p.kick_delay != 0) kick_now(s, p.kick_addr);
    p.kick_addr = addr;
    p.kick_delay = 2;
}
// a lower EFU op issued: its Q lands later (WAITQ: none)
VUN_INLINE void efu_issued(VUState& s, VuPipe& p, float q_before, int op) {
    p.q_next = s.q;
    s.q = q_before;
    p.q_pending = op != 0x3b;
    p.q_ready = p.issued + (op == 0x3a ? 13 : 7);
}
VUN_INLINE bool end(VUState& s, VuPipe& p, bool taken, uint32_t target) {
    p.mac_ring[p.issued & 3] = s.macflag;
    p.status_ring[p.issued & 3] = s.statusflag;
    p.clip_ring[p.issued & 3] = s.clipflag;
    ++p.issued;
    if (p.kick_delay != 0 && --p.kick_delay == 0) kick_now(s, p.kick_addr);
    if (p.branch_pending) {
        p.pc = p.branch_target;
        p.branch_pending = false;
    } else {
        ++p.pc;
    }
    if (taken) {
        p.branch_pending = true;
        p.branch_target = target;
    }
    if (p.halt_pending && --p.halt_delay == 0) p.halted = true;
    return !p.halted && p.pc < p.size;
}
VUN_INLINE void ebit(VuPipe& p) {
    if (!p.halt_pending) {
        p.halt_pending = true;
        p.halt_delay = 2;
    }
}

}  // namespace rac1::vun

#endif
