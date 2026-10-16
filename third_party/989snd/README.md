# 989snd (OpenGOAL)

The IOP sound driver 989snd reimplemented by the OpenGOAL project
(<https://github.com/open-goal/jak-project>, `game/sound/989snd` and `game/sound/common`,
commit efb21c3e8c5a40a74f2e86510f915da55eaeba34), ISC license (see `LICENSE`). Ratchet &
Clank's banks are the same 989snd format (SBlk version 1 sound blocks, the samples in SPU2
ADPCM). The host's 989snd servers (`runtime/iop/snd989_iop.cpp`) run on it.

Changes from upstream:

- `player.h` / `player.cpp`: the cubeb output stream removed; `Tick` is public, the host's
  audio device pulls the output. `SetSoundPBend` added (the handler's pitch bend), `ActiveSounds` (a debug count).
- `sfxblock.h` / `sfxblock.cpp`, `sfxgrain.cpp`, `musicbank.cpp`: a sound id past the bank's
  sounds plays nothing (reported once) instead of indexing past `Sounds`, as the IRX's
  `snd_PlaySFX` does (`sound < 0 || sound >= count`: handle 0). The game plays such ids: on
  Quartu the hero's low-air beep is sound -1.
- `common/common_types.h`: only the integer types kept.
- Not taken: `fakeplayer`, `sndplay` (a command-line player), `common/flava.h` (Jak 3).
- Shims for what the code used from the rest of OpenGOAL: `common/util/Assert.h`,
  `common/log/log.h`, `fmt/format.h` (the few `fmt::format` / `fmt::print` calls),
  `third-party/magic_enum.hpp` (`enum_name` in a debug print). `common/util/BinaryReader.h` is
  OpenGOAL's.
