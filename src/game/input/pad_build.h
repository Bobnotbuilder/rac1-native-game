#ifndef RAC1_GAME_INPUT_PAD_BUILD_H
#define RAC1_GAME_INPUT_PAD_BUILD_H

#include <cstdint>

namespace rac1 {

void pad_build(uint32_t pad, const uint8_t* data, int32_t len);        // orig: 0x002671b0
void pad_read_packet(uint32_t pad, const uint8_t* data, int32_t len);  // orig: 0x00266f50

}  // namespace rac1

#endif  // RAC1_GAME_INPUT_PAD_BUILD_H
