// The story's pictures and movies: the picture cards shown between levels
// (0x2929f0, their images 0x292690, a sprite 0x292500), the movies of the story (0x292420 by
// their number in the table of contents) and the movie player's entry (0x2edd28). Their loops run
// one game frame per call (the host's frame), as the level exit's. See

#ifndef RAC1_GAME_OVL_MOVIES_H
#define RAC1_GAME_OVL_MOVIES_H

#include <cstdint>

namespace rac1 {

// orig: 0x00292690 — the card file of the language (table of contents 0x137b80 + 0x1388 +
// lang * 8) read and unpacked into *(0x173e14), its background (image 1) and the two cards
// (images first + 2, last + 2) uploaded to the GS from 0x15ee8c; their TEX0s to the three EE
// doublewords `out0` (background), `out1`, `out2`.
void card_images_load(int32_t lang, int32_t first, int32_t last, uint32_t out0, uint32_t out1, uint32_t out2);
// orig: 0x00292500 — a textured sprite (x, y, w, h in screen pixels; colour; TEX0; texture
// coordinates s0..s1, t0..t1 as floats) into the frame's packets *(0x1611c0).
void card_sprite(int32_t x, int32_t y, int32_t w, int32_t h, uint64_t colour, uint64_t tex0, float s0, float s1, float t0, float t1);

// orig: 0x002929f0 — a card shown for `frames` frames (two cards crossfaded when first != last;
// the background scrolling), the next level's files loaded meanwhile when `load` (the card stays
// until they are in); a fade at the end. Split for the host's frame loop: card_start, then
// card_frame until it returns true (the end included).
void card_start(int32_t lang, int32_t first, int32_t last, int32_t frames, int32_t load);
bool card_frame();
// Whether the last card's load (`load`) finished (the level's files and bank are in).
bool card_load_done();

// orig: 0x00292420 — movie `k` of the story (table of contents 0x137b80 + 0x1938 + k * 8,
// PAL + 0x1998), played by the movie player in the buffers *(0x173e14), the sound in the game's
// language; the HUD flags 0x13e550 + 0x6b. Split: movie_start, movie_frame until true.
void movie_start(int32_t k);
bool movie_frame();

// orig: 0x002edd28 — the movie player's entry: a PSS at sector `lsn`, `size` bytes, its buffers
// `buf` / `buf2`, sound track `lang`. Split: movie_stream_start, movie_stream_frame until true.
void movie_stream_start(uint32_t lsn, uint32_t size, uint32_t buf, uint32_t buf2, int32_t lang);
bool movie_stream_frame();

// The skip test of the player's loop (0x2eddd0): the pad read, then Start (or any button when
// *(0x15eed8) == 2) skips; when the game has not been finished and the level is above 0, only
// L1 + L2 + R1 + R2 held with Start pressed. -1 in 0x15eed8: no skip. True to stop (the player's
// stop flag set in its block at `player` + 0xa8).
bool movie_skip_test(uint32_t player);

}  // namespace rac1

#endif
