#include <game/lib/libc.h>

#include <runtime/kernel/ee_memory.h>

#include <cstdio>
#include <cstring>
#include <string>

namespace rac1 {

namespace {
constexpr uint32_t ADDR_IMPURE_PTR = 0x0012f76c;  // _impure_ptr (newlib reentrancy struct)
constexpr uint32_t REENT_RAND_SEED = 0x58;
}  // namespace

// orig: 0x001160c8
void libc_srand(uint32_t s) { ee_write<uint32_t>(ee_read<uint32_t>(0x0012f76c) + 0x58, s); }

// orig: 0x001160d8
int32_t libc_rand() {
    const uint32_t reent = ee_read<uint32_t>(ADDR_IMPURE_PTR);
    const uint32_t seed = ee_read<uint32_t>(reent + REENT_RAND_SEED) * 0x41c64e6du + 0x3039u;
    ee_write<uint32_t>(reent + REENT_RAND_SEED, seed);
    return static_cast<int32_t>(seed & 0x7fffffffu);
}

// orig: 0x001151b4
int32_t libc_memcmp(uint32_t a, uint32_t b, uint32_t n) {
    for (uint32_t i = 0; i < n; ++i) {
        const uint8_t x = ee_read<uint8_t>(a + i), y = ee_read<uint8_t>(b + i);
        if (x != y) return static_cast<int32_t>(x) - static_cast<int32_t>(y);
    }
    return 0;
}

// orig: 0x00115248 — newlib memcpy (see libc.h): the original's step order (a quadword is loaded and stored before the next
// one is loaded, so an overlapping copy gives the same bytes), returns dst.
uint32_t libc_memcpy(uint32_t dst, uint32_t src, uint32_t n) {
    uint32_t d = dst, s = src, left = n;
    if (n >= 0x20 && ((src | dst) & 0xf) == 0) {
        while (left >= 0x20) {  // 0x11526c: lq / sq twice
            for (int k = 0; k < 2; ++k) {
                uint8_t q[16];
                for (uint32_t i = 0; i < 16; ++i) q[i] = ee_read<uint8_t>(s + i);
                for (uint32_t i = 0; i < 16; ++i) ee_write<uint8_t>(d + i, q[i]);
                s += 16;
                d += 16;
            }
            left -= 0x20;
        }
        while (left >= 8) {  // 0x1152a4: ld / sd
            uint8_t q[8];
            for (uint32_t i = 0; i < 8; ++i) q[i] = ee_read<uint8_t>(s + i);
            for (uint32_t i = 0; i < 8; ++i) ee_write<uint8_t>(d + i, q[i]);
            s += 8;
            d += 8;
            left -= 8;
        }
    }
    for (; left != 0; --left) {  // 0x1152d4: lbu / sb
        ee_write<uint8_t>(d, ee_read<uint8_t>(s));
        ++s;
        ++d;
    }
    return dst;
}

// orig: 0x001153fc — newlib memset: `n` bytes at `dst` set to the low byte of `c` (the original
// stores aligned 16 / 8-byte words of the repeated byte, then bytes; the same memory effect);
// returns dst.
uint32_t libc_memset(uint32_t dst, int32_t c, uint32_t n) {
    for (uint32_t i = 0; i < n; ++i) ee_write<uint8_t>(dst + i, static_cast<uint8_t>(c));
    return dst;
}

// orig: 0x00116428 — newlib strchr: the first byte equal to the low byte of `c` in the string at
// `s` (the original scans aligned 8 / 16-byte words with MMI ops, then bytes; the same result),
// its address, or 0 at the end (`c` 0: the terminator's address).
uint32_t libc_strchr(uint32_t s, int32_t c) {
    const uint8_t k = static_cast<uint8_t>(c);
    for (uint32_t p = s;; ++p) {
        const uint8_t b = ee_read<uint8_t>(p);
        if (b == k) return p;
        if (b == 0) return 0;
    }
}

// orig: 0x001165b8 — newlib strcpy (the original copies aligned 8/16-byte chunks while they
// hold no NUL, then bytes; the memory effect is the same as this byte loop).
uint32_t libc_strcpy(uint32_t dst, uint32_t src) {
    uint32_t d = dst, s = src;
    for (;;) {
        const uint8_t c = ee_read<uint8_t>(s++);
        ee_write<uint8_t>(d++, c);
        if (c == 0) return dst;
    }
}

// orig: 0x00116808 — strncpy on EE addresses (newlib's, a word at a time when aligned): at most
// `n` bytes copied, the rest of the `n` filled with 0 past the source's end; returns dst.
uint32_t libc_strncpy(uint32_t dst, uint32_t src, uint32_t n) {
    uint32_t i = 0;
    for (; i < n; ++i) {
        const uint8_t c = ee_read<uint8_t>(src + i);
        ee_write<uint8_t>(dst + i, c);
        if (c == 0) break;
    }
    for (; i < n; ++i) ee_write<uint8_t>(dst + i, 0);
    return dst;
}

// orig: 0x001169c8 — newlib strstr: the first place in the string at `s` where the string at
// `f` starts (bytes compared signed, as `lb` loads them), its address, or 0; `f` empty: `s`.
uint32_t libc_strstr(uint32_t s, uint32_t f) {
    auto lb = [](uint32_t a) { return ee_read<int8_t>(a); };
    if (lb(s) == 0) return lb(f) == 0 ? s : 0u;
    do {
        const int8_t d = lb(f);
        if (d == 0) return s;
        if (d == lb(s)) {
            for (uint32_t k = 1;; ++k) {
                const int8_t c = lb(f + k);
                if (c == 0) return s;
                if (c != lb(s + k)) break;
            }
        }
        ++s;
    } while (lb(s) != 0);
    return 0;
}

// orig: 0x001166cc
int32_t libc_strlen(uint32_t s) {
    int32_t n = 0;
    while (ee_read<uint8_t>(s + static_cast<uint32_t>(n)) != 0) ++n;
    return n;
}

namespace {
// orig: 0x00113a70 — newlib std(): a standard stream's FILE (flags, file number, its
// reentrancy struct, the read / write / seek / close hooks 0x1162b8.. 0x116408).
void libc_std(uint32_t fp, uint32_t flags, uint32_t file, uint32_t reent) {
    ee_write<uint32_t>(fp + 0x54, reent);
    ee_write<uint16_t>(fp + 0xc, static_cast<uint16_t>(flags));
    ee_write<uint16_t>(fp + 0xe, static_cast<uint16_t>(file));
    ee_write<uint32_t>(fp + 0x20, 0x001162b8u);
    ee_write<uint32_t>(fp + 0x24, 0x00116320u);
    ee_write<uint32_t>(fp + 0x28, 0x001163a0u);
    ee_write<uint32_t>(fp + 0x2c, 0x00116408u);
    for (uint32_t o : {0x0u, 0x4u, 0x8u, 0x10u, 0x18u}) ee_write<uint32_t>(fp + o, 0);
    ee_write<uint32_t>(fp + 0x1c, fp);
}
}  // namespace

// orig: 0x00113ae0 — newlib __sinit: the reentrancy struct's cleanup hook 0x113ac8, its
// "initialised" flag +0x38 and the three standard streams (+0x1e4, +0x23c, +0x294) in its
// stream list (+0x1d8..+0x1e0).
void libc_sinit(uint32_t reent) {
    ee_write<uint32_t>(reent + 0x3c, 0x00113ac8u);
    ee_write<uint32_t>(reent + 0x38, 1);
    libc_std(reent + 0x1e4, 4, 0, reent);
    libc_std(reent + 0x23c, 9, 1, reent);
    libc_std(reent + 0x294, 0xa, 2, reent);
    ee_write<uint32_t>(reent + 0x1e0, reent + 0x1e4);
    ee_write<uint32_t>(reent + 0x1dc, 3);
    ee_write<uint32_t>(reent + 0x1d8, 0);
}

// orig: 0x00116248 — newlib sprintf (its formatter 0x116da8): the format at `fmt` (EE) with the
// register arguments `args` (a2.., 64-bit each) into `dst`, NUL-terminated; returns the length
// (the streams set up first, 0x113ae0, as its formatter 0x116da8 does).
// The host's snprintf formats each conversion (the C library's, not game code); covered:
// flags, width / precision (or *), h / l / ll, d i u o x X c s p %.
int32_t libc_sprintf(uint32_t dst, uint32_t fmt, const std::vector<uint64_t>& args) {
    const uint32_t reent = ee_read<uint32_t>(ADDR_IMPURE_PTR);
    if (ee_read<uint32_t>(reent + 0x38) == 0) libc_sinit(reent);
    std::string out;
    size_t ai = 0;
    auto next = [&]() -> uint64_t { return ai < args.size() ? args[ai++] : 0u; };
    uint32_t f = fmt;
    for (;;) {
        char c = static_cast<char>(ee_read<uint8_t>(f++));
        if (c == 0) break;
        if (c != '%') {
            out.push_back(c);
            continue;
        }
        std::string spec = "%";
        for (;;) {  // flags
            c = static_cast<char>(ee_read<uint8_t>(f));
            if (c == '-' || c == '+' || c == ' ' || c == '#' || c == '0') {
                spec.push_back(c);
                ++f;
            } else {
                break;
            }
        }
        auto num = [&](bool allow_star) {
            c = static_cast<char>(ee_read<uint8_t>(f));
            if (allow_star && c == '*') {
                ++f;
                spec += std::to_string(static_cast<int32_t>(next()));
                return;
            }
            while (c >= '0' && c <= '9') {
                spec.push_back(c);
                c = static_cast<char>(ee_read<uint8_t>(++f));
            }
        };
        num(true);
        if (ee_read<uint8_t>(f) == '.') {
            spec.push_back('.');
            ++f;
            num(true);
        }
        int len = 0;  // 0 int, 1 h, 2 l, 3 ll
        for (;;) {
            c = static_cast<char>(ee_read<uint8_t>(f));
            if (c == 'h') { len = 1; ++f; }
            else if (c == 'l') { len = len == 2 ? 3 : 2; ++f; }
            else break;
        }
        c = static_cast<char>(ee_read<uint8_t>(f++));
        char buf[512];
        switch (c) {
        case 'd': case 'i': {
            const uint64_t v = next();
            const long long x = len == 3 || len == 2 ? static_cast<long long>(static_cast<int64_t>(len == 3 ? v : static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(v)))))
                                                     : (len == 1 ? static_cast<int16_t>(v) : static_cast<int32_t>(v));
            std::snprintf(buf, sizeof buf, (spec + "lld").c_str(), x);
            out += buf;
            break;
        }
        case 'u': case 'o': case 'x': case 'X': {
            const uint64_t v = next();
            const unsigned long long x = len == 3 ? v : (len == 1 ? static_cast<uint16_t>(v) : static_cast<uint32_t>(v));
            std::snprintf(buf, sizeof buf, (spec + "ll" + c).c_str(), x);
            out += buf;
            break;
        }
        case 'p': {
            std::snprintf(buf, sizeof buf, "0x%x", static_cast<uint32_t>(next()));
            out += buf;
            break;
        }
        case 'c':
            std::snprintf(buf, sizeof buf, (spec + "c").c_str(), static_cast<int>(static_cast<uint8_t>(next())));
            out += buf;
            break;
        case 's': {
            std::string str;
            for (uint32_t a = static_cast<uint32_t>(next());; ++a) {
                const char ch = static_cast<char>(ee_read<uint8_t>(a));
                if (ch == 0) break;
                str.push_back(ch);
            }
            std::snprintf(buf, sizeof buf, (spec + "s").c_str(), str.c_str());
            out += buf;
            break;
        }
        case '%':
            out.push_back('%');
            break;
        default:
            if (c == 0) --f;
            break;
        }
        if (c == 0) break;
    }
    for (size_t k = 0; k < out.size(); ++k) ee_write<uint8_t>(dst + static_cast<uint32_t>(k), static_cast<uint8_t>(out[k]));
    ee_write<uint8_t>(dst + static_cast<uint32_t>(out.size()), 0);
    return static_cast<int32_t>(out.size());
}

}  // namespace rac1
