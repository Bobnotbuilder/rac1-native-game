// rac1 shim for the {fmt} calls of the sound code: "{}" / "{:x}"-style placeholders, the
// arguments written in order (hex when the spec has 'x'); fmt::format and fmt::print only.
#pragma once
#include <cstdio>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

namespace fmt {
namespace detail {
inline void put(std::ostringstream& o, std::string_view& f) { o << f; f = {}; }
template <typename T, typename... R>
void put(std::ostringstream& o, std::string_view& f, const T& v, const R&... r) {
    const size_t a = f.find('{');
    const size_t b = a == std::string_view::npos ? a : f.find('}', a);
    if (b == std::string_view::npos) { o << f; f = {}; return; }
    o << f.substr(0, a);
    const std::string_view spec = f.substr(a, b - a);
    if (spec.find('x') != std::string_view::npos) {
        if constexpr (std::is_integral_v<T>) o << std::hex << +v << std::dec; else o << v;
    } else {
        if constexpr (std::is_integral_v<T> && sizeof(T) == 1) o << +v; else o << v;
    }
    f = f.substr(b + 1);
    put(o, f, r...);
}
}  // namespace detail
template <typename... A>
std::string format(std::string_view f, const A&... a) {
    std::ostringstream o;
    detail::put(o, f, a...);
    return o.str();
}
template <typename... A>
void print(std::string_view f, const A&... a) { std::fputs(format(f, a...).c_str(), stdout); }
}  // namespace fmt
