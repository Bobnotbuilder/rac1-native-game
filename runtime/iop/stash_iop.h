// The game's IOP_stash_daemon (irx.bin), on the host: a 0x94000-byte buffer in IOP RAM that the
// EE fills by SIF DMA (the HUD's and the level's textures kept off the EE) and reads back by
// RPC. Server 0x11: function 2 answers {buffer, size}; function 1 answers the IOP RAM at the
// address in the send buffer's first word (rsize bytes); 0 answers its send buffer.
#ifndef RAC1_RUNTIME_IOP_STASH_IOP_H
#define RAC1_RUNTIME_IOP_STASH_IOP_H

#include <cstdint>

namespace rac1 {

// The module's load address in IOP RAM is the host's choice (the IOP's allocator decided it on
// the console); the buffer is its bss at +0x4a0.
constexpr uint32_t kStashIopBase = 0x00100000;
constexpr uint32_t kStashBuffer = kStashIopBase + 0x4a0;
constexpr uint32_t kStashSize = 0x94000;

void stash_iop_start();  // the server registered with the SIF RPC layer

}  // namespace rac1

#endif
