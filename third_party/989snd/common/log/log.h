// rac1 shim for OpenGOAL's common/log/log.h: messages to stderr.
#pragma once
#include <cstdio>
#include "fmt/format.h"

namespace lg {
template <typename... A> void error(std::string_view f, const A&... a) { std::fprintf(stderr, "[989snd] %s\n", fmt::format(f, a...).c_str()); }
template <typename... A> void warn(std::string_view f, const A&... a) { std::fprintf(stderr, "[989snd] %s\n", fmt::format(f, a...).c_str()); }
template <typename... A> void info(std::string_view f, const A&... a) { std::fprintf(stderr, "[989snd] %s\n", fmt::format(f, a...).c_str()); }
template <typename... A> void debug(std::string_view, const A&...) {}
}  // namespace lg
