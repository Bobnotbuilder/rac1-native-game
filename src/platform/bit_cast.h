// C++20 object-representation cast for older Apple SDKs whose libc++ lacks std::bit_cast.
// No arithmetic/conversion/aliasing change. Never add definitions to namespace std.
#pragma once
#include <bit>
#include <type_traits>
namespace rac1 {
template<class To,class From>
constexpr To portable_bit_cast(const From& value) noexcept {
    static_assert(sizeof(To)==sizeof(From));
    static_assert(std::is_trivially_copyable_v<To> && std::is_trivially_copyable_v<From>);
#if defined(__cpp_lib_bit_cast) && __cpp_lib_bit_cast >= 201806L
    return std::bit_cast<To>(value);
#elif defined(__has_builtin)
#if __has_builtin(__builtin_bit_cast)
    return __builtin_bit_cast(To,value);
#else
#error This compiler lacks a correct constexpr object-representation cast
#endif
#else
#error C++20 bit_cast support is required
#endif
}
}
