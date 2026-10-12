// Base ELF: the table of contents and a level's files read from the disc.
#pragma once

#include <cstdint>

namespace rac1 {

bool toc_read();                       // orig: 0x0012f2b8
bool level_header_read(int32_t level); // orig: 0x0012f368
bool level_files_read();               // orig: 0x00204428 (steps 0, 1, 2)
int32_t level_bank_step();             // orig: 0x00204428 (steps 3..7)
void snd_reply_store(int32_t reply, uint64_t data);  // orig: 0x0022dd78

}  // namespace rac1
