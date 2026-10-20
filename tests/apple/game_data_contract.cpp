#ifdef NDEBUG
#error Runtime-path assertions must remain enabled
#endif
#include <platform/apple/game_data.hpp>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <cstdio>
#include <unistd.h>
int main() {
    namespace fs=std::filesystem;
    using namespace rac1::platform::apple;
    char pattern[]="/tmp/rac1-data-contract-XXXXXX";
    const auto temporary=mkdtemp(pattern); assert(temporary);
    const fs::path root=fs::path(temporary)/"Documents/RAC1NativeData";
    auto put=[&](const fs::path& name){fs::create_directories((root/name).parent_path());std::ofstream(root/name)<<"synthetic fixture, NOT game input";};
    GameDataPaths result; result.root="unchanged"; std::string error;
    assert(!resolve_game_data(root,result,error) && error=="GAME_DATA_ROOT_REQUIRED");
    put("assets/dumps/SCUS_971.99");
    assert(!resolve_game_data(root,result,error) && error=="OWNED_NTSC_U_ISO_REQUIRED");
    assert(result.root=="unchanged");
    put("assets/iso/rac1-ntsc-u.iso"); put("assets/ios-runtime-assets.json");
    for(int i=0;i<21;++i) {
        auto name=i<19 ? "level"+std::to_string(i) : i==19 ? "title" : "flight";
        put(fs::path("assets/generated/mrp/moby")/name/"textures.mrpt");
        put(fs::path("assets/generated/mrp/world")/name/"world.mrpw");
    }
    assert(resolve_game_data(root,result,error) && error.empty());
    assert(result.iso==fs::canonical(root/"assets/iso/rac1-ntsc-u.iso"));
    fs::remove(root/"assets/generated/mrp/world/level3/world.mrpw");
    const auto before=result.root;
    assert(!resolve_game_data(root,result,error) && error=="MISSING_RUNTIME_WORLD_GROUP" && result.root==before);
    put("assets/generated/mrp/world/level3/world.mrpw");
    fs::remove(root/"assets/iso/rac1-ntsc-u.iso");
    const auto outside=fs::path(temporary)/"outside.iso";std::ofstream(outside)<<"not a disc";
    fs::create_symlink(outside,root/"assets/iso/rac1-ntsc-u.iso");
    assert(!resolve_game_data(root,result,error) && error=="GAME_DATA_PATH_ESCAPES_SANDBOX_PACKAGE");
    // Delete ONLY the unique temporary fixture directory created by this test.
    fs::remove_all(temporary);
    std::puts("PASS: app-owned path mapping, 21 runtime groups, missing/escaping paths fail closed; ISO bytes/identity not certified by this fixture");
}
