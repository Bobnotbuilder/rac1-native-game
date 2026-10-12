// Ported overlay code: the music and the dialog streams (989snd's VAG streams). State at 0x1516d0 (S): +8 a disc read through the
// stream system (0 none, 1 reading, 2 to retry; its sector +0xc, count +0x10, buffer +0x14),
// +0xa a read awaited, +0xb streams off, +0x1c a dialog line asked for (-1 none), +0x20 the
// music's sequence step, +0x22 / +0x23 the next music track / the track to fade over to, +0x24 /
// +0x28 the time left when asked, +0x2c a delay; three stream slots of 0x1c bytes: the music
// +0x34, the dialog +0x50, the second music +0x6c (+0 the handle (-1 a reply awaited), +4 the
// track / line, +6 volume, +8 flags, +0xa the state, +0xc pause request (0x8000) / its timer
// +0xe, +0x10, +0x14 / +0x18). The music's sectors come from the level header's table 0x13a628
// (the intro, then the next entry looped).
#ifndef RAC1_GAME_OVL_MUSIC_STREAMS_H
#define RAC1_GAME_OVL_MUSIC_STREAMS_H

#include <cstdint>

namespace rac1 {

void music_streams_frame();                                         // orig: 0x00265aa8
void stream_slot_update(uint32_t r);                                // orig: 0x002658c0
void music_start(int32_t track, int32_t flags, int32_t vol);       // orig: 0x00265418
void music_preload(int32_t track, int32_t flags, int32_t vol);     // orig: 0x00265340
void music_queue_next(int32_t track, int32_t flags, int32_t vol);  // orig: 0x002654f0
int32_t music2_start(int32_t a, int32_t track, int32_t b, int32_t vol);  // orig: 0x002655d8
void dialog_play(int32_t id, int32_t a, int32_t vol);              // orig: 0x00265148
void dialog_play_60000(int32_t id, int32_t a, int32_t vol);        // orig: 0x00264c18
void dialog_play_50000(int32_t id, int32_t a, int32_t vol);        // orig: 0x00264cf0
void dialog_play_40000(int32_t id, int32_t a, int32_t vol);        // orig: 0x00264dd8
void dialog_play_30000(int32_t id, int32_t a, int32_t vol);        // orig: 0x00264eb0
void dialog_play_20000(int32_t id, int32_t a, int32_t vol);        // orig: 0x00264fa8
void dialog_play_10000(int32_t id, int32_t a, int32_t vol);        // orig: 0x00265078
int32_t stream_disc_read(uint32_t buf, uint32_t lsn, uint32_t n);  // orig: 0x00265fa0
void music_stop_all();                                              // orig: 0x002656c0
// the replies' callbacks (989snd's (reply, data))
void stream_read_done(int32_t reply, uint64_t data);         // orig: 0x00266168
void stream_started_reply(int32_t reply, uint64_t data);     // orig: 0x002661a8
void dialog_play_reply(int32_t reply, uint64_t data);        // orig: 0x002661d8
void music_preload_reply(int32_t reply, uint64_t data);      // orig: 0x00266238
void music2_play_reply(int32_t reply, uint64_t data);        // orig: 0x00266298
void music_queue_reply(int32_t reply, uint64_t data);        // orig: 0x002662e8
void music_play_reply(int32_t reply, uint64_t data);         // orig: 0x00266340
void stream_playing_reply(int32_t reply, uint64_t data);     // orig: 0x00266380
void stream_remaining_reply(int32_t reply, uint64_t data);   // orig: 0x002663d8
void music_streams_callbacks_register();  // orig: (none) — the callbacks above by address

}  // namespace rac1

#endif
