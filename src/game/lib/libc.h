#ifndef RAC1_GAME_LIB_LIBC_H
#define RAC1_GAME_LIB_LIBC_H

// Ports of the C library functions in the boot ELF that game code calls.

#include <cstdint>
#include <vector>

namespace rac1 {

// orig: 0x001160d8 — rand(): seed = seed * 0x41c64e6d + 0x3039 (seed in the
// reentrancy struct *(0x0012f76c) + 0x58); returns seed & 0x7fffffff.
int32_t libc_rand();

// orig: 0x001160c8 — srand(): the seed (*(0x12f76c) + 0x58) = s.
void libc_srand(uint32_t s);

// orig: 0x001166cc — strlen of an EE string.
int32_t libc_strlen(uint32_t s);

// orig: 0x001151b4 — memcmp on EE memory: the difference of the first differing bytes, 0 if
// none (its 16-byte steps over aligned blocks only skip equal ones).
int32_t libc_memcmp(uint32_t a, uint32_t b, uint32_t n);

// orig: 0x00115248 — newlib memcpy on EE memory, forward in the original's steps (32 bytes as two quadwords while both
// addresses are 16-aligned and >= 32 bytes remain, then 8-byte words, then bytes; bytes only when unaligned or short);
// returns dst. (found as the unported base-ELF callee of Orxon's class 794.)
uint32_t libc_memcpy(uint32_t dst, uint32_t src, uint32_t n);

// orig: 0x001153fc — memset on EE memory; returns dst.
uint32_t libc_memset(uint32_t dst, int32_t c, uint32_t n);

// orig: 0x00116428 — strchr on EE memory; the address found or 0.
uint32_t libc_strchr(uint32_t s, int32_t c);

// orig: 0x001169c8 — strstr on EE strings; the address found or 0.
uint32_t libc_strstr(uint32_t s, uint32_t f);

// orig: 0x001165b8 — strcpy on EE addresses; returns dst.
uint32_t libc_strcpy(uint32_t dst, uint32_t src);

// orig: 0x00116808 — strncpy on EE addresses; returns dst.
uint32_t libc_strncpy(uint32_t dst, uint32_t src, uint32_t n);

// orig: 0x00113ae0 — newlib __sinit (the standard streams of the reentrancy struct).
void libc_sinit(uint32_t reent);

// orig: 0x00116248 — newlib sprintf to EE memory (args: the a2.. registers).
int32_t libc_sprintf(uint32_t dst, uint32_t fmt, const std::vector<uint64_t>& args);

}  // namespace rac1

#endif
