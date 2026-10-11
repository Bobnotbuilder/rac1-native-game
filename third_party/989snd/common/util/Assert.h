// rac1 shim for OpenGOAL's common/util/Assert.h (only ASSERT is used by the sound code).
#pragma once
#include <cassert>
#define ASSERT(x) assert(x)
#define ASSERT_MSG(x, m) assert((x) && (m))
