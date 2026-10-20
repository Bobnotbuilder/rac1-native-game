// App-owned runtime data paths. Resolving paths is NOT ISO identity/hash validation.
#pragma once
#include <filesystem>
#include <string>
namespace rac1::platform::apple {
struct GameDataPaths {
    std::filesystem::path root, iso, boot, converted;
};
// root is the prepared package ROOT containing assets/, not the assets directory itself.
// Missing/escaping paths fail without publishing a partial result or changing process state.
bool resolve_game_data(const std::filesystem::path& root, GameDataPaths& result, std::string& error);
} // namespace rac1::platform::apple
