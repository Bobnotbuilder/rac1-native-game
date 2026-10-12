// The movie player module linked into the overlay (0x2ee130..0x2f0cd0, ~70 functions: Sony's
// streaming MPEG sample player with the game's own skip test): a disc stream read into a ring
// buffer, demultiplexed (libmpeg's sceMpegDemuxPss), the pictures decoded by a thread on the IPU
// (sceMpegGetPicture), the sound sent to the IOP, the pictures shown at vblank by an interrupt
// handler. The host stands in for it at its three entry points: its threads, the IPU
// and the SPU2 stream are runtime/ipu/movie_hw; the game's skip test (movie_skip_test, in
// src/game/ovl/movies.cpp) and the loop's end are ported.
#ifndef RAC1_GAME_LIB_MOVIE_PLAYER_H
#define RAC1_GAME_LIB_MOVIE_PLAYER_H

#include <cstdint>

namespace rac1 {

// orig: (host) stands in for 0x002ee130 — the player opened on a PSS at sector `lsn`, `size`
// bytes, sound track `lang`. False when it cannot be played (no decoder, a read error): the
// original then prints "Can't Open movie" and skips the loop.
bool movie_player_open(uint32_t lsn, uint32_t size, int32_t lang);
// orig: (host) stands in for 0x002eddd0 (its start) — the loop on the player's block `player`.
void movie_player_loop_start(uint32_t player, uint32_t buf, uint32_t aux);
// orig: (host) stands in for 0x002eddd0 (one pass of its loop, then its end) — a game frame of
// the movie: the skip test, the picture of this frame. True at the end (the sound stopped, the
// game's master volume 5 back from 0x13e550 + 0x5c).
bool movie_player_loop_frame();
// orig: (host) stands in for 0x002ee3d8 — the player closed.
void movie_player_close();
// orig: (host) the debug menu's skip: the player's stop flag set (as the game's skip test does,
// 0x2f05e0), the movie ends at its next frame.
void movie_player_stop();

}  // namespace rac1

#endif
