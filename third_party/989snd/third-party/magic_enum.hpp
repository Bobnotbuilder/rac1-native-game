// rac1 shim for magic_enum (only enum_name in a debug print): the value as a number.
#pragma once
#include <string>
#include <variant>  // the real header brings it in; sfxgrain.h relies on that
namespace magic_enum {
template <typename E> std::string enum_name(E e) { return std::to_string(static_cast<long long>(e)); }
}  // namespace magic_enum
