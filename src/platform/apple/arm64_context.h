// MIT: rac1-native Apple ARM64 coroutine context. Not game code.
#pragma once
#include <cstddef>
#include <cstdint>

namespace rac1::platform::apple {
struct alignas(16) Context {
    uint64_t x19_x30[12]{};
    uint64_t sp{};
    uint64_t d8_d15[8]{};
    uint64_t fpcr{};
    uint64_t fpsr{};
};
static_assert(offsetof(Context, sp) == 96);
static_assert(offsetof(Context, d8_d15) == 104);
static_assert(offsetof(Context, fpcr) == 168);
static_assert(offsetof(Context, fpsr) == 176);
static_assert(sizeof(Context) == 192);
extern "C" void rac1_arm64_context_swap(Context* from, const Context* to);
} // namespace rac1::platform::apple
