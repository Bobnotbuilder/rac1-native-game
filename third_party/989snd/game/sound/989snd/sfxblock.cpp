#include "sfxblock.h"

#include "blocksound_handler.h"
#include "sfxgrain.h"

#include <mutex>
#include <set>
#include <utility>

#include "common/log/log.h"

#include "third-party/magic_enum.hpp"

namespace snd {

// rac1: an id past the block's sounds, reported once per (sound count, id). The IRX's snd_PlaySFX
// (989snd.irx 0x3df0) plays nothing for a sound < 0 or >= the block's count (its handle 0).
bool SFXBlock::ValidSound(u32 sound_id, const char* where) const {
  if (sound_id < Sounds.size()) {
    return true;
  }
  static std::mutex m;
  static std::set<std::pair<size_t, u32>> reported;
  std::scoped_lock lock(m);
  if (reported.emplace(Sounds.size(), sound_id).second) {
    lg::warn("{}: sound {} is not in the block ({} sounds), not played", where, static_cast<s32>(sound_id),
             Sounds.size());
  }
  return false;
}

std::optional<std::unique_ptr<SoundHandler>> SFXBlock::MakeHandler(VoiceManager& vm,
                                                                   u32 sound_id,
                                                                   s32 vol,
                                                                   s32 pan,
                                                                   SndPlayParams& params,
                                                                   s32 current_tick,
                                                                   u32 owner) {
  if (!ValidSound(sound_id, "play_sound")) {
    return std::nullopt;
  }
  auto& SFX = Sounds[sound_id];

  if (SFX.Grains.empty()) {
    return std::nullopt;
  }

  auto handler = std::make_unique<BlockSoundHandler>(*this, SFX, vm, vol, pan, params, sound_id,
                                                     current_tick, owner);
  return handler;
}

std::optional<u32> SFXBlock::GetSoundByName(const char* name) {
  auto sound = Names.find(name);
  if (sound != Names.end()) {
    return sound->second;
  }

  return std::nullopt;
}

void SFXBlock::DebugPrintAllSounds() {
  for (const auto& [name, id] : Names) {
    printf("%s : %d\n", name.c_str(), id);
    const auto& sound = Sounds.at(id);
    printf(" Vol: %d\n", sound.Vol);
    printf(" VolGroup: %d\n", sound.VolGroup);
    printf(" Pan: %d\n", sound.Pan);
    printf(" InstanceLimit: %d\n", sound.InstanceLimit);
    printf(" Flags: 0x%x\n", sound.Flags.flags);
    printf(" User: 0x%x 0x%x 0x%x 0x%x\n", sound.UserData.data[0], sound.UserData.data[1],
           sound.UserData.data[2], sound.UserData.data[3]);
    printf(" Grains\n");
    for (const auto& grain : sound.Grains) {
      fmt::print("  {} ({})\n", magic_enum::enum_name(grain.Type), (int)grain.Type);
    }
  }
}

}  // namespace snd
