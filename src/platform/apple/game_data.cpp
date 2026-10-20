#include "game_data.hpp"
#include <system_error>
namespace rac1::platform::apple {
namespace {
bool inside(const std::filesystem::path& root,const std::filesystem::path& path) {
    auto r=root.begin(),p=path.begin();
    for(;r!=root.end();++r,++p) if(p==path.end() || *r!=*p) return false;
    return true;
}
}
bool resolve_game_data(const std::filesystem::path& requested, GameDataPaths& out, std::string& error) {
    namespace fs=std::filesystem;
    std::error_code ec;
    const auto root=fs::canonical(requested,ec);
    if(ec || !fs::is_directory(root,ec)) { error="GAME_DATA_ROOT_REQUIRED"; return false; }
    auto required=[&](const fs::path& relative,fs::path& result,const char* missing,bool directory=false) {
        const auto path=fs::canonical(root/relative,ec);
        if(ec) { error=missing; return false; }
        if(!inside(root,path)) { error="GAME_DATA_PATH_ESCAPES_SANDBOX_PACKAGE"; return false; }
        const bool valid=directory ? fs::is_directory(path,ec) : fs::is_regular_file(path,ec) && fs::file_size(path,ec)>0;
        if(ec || !valid) { error=missing; return false; }
        result=path; return true;
    };
    GameDataPaths next; next.root=root;
    if(!required("assets/iso/rac1-ntsc-u.iso",next.iso,"OWNED_NTSC_U_ISO_REQUIRED") ||
       !required("assets/dumps/SCUS_971.99",next.boot,"MATCHING_BOOT_EXECUTABLE_REQUIRED") ||
       !required("assets/generated/mrp",next.converted,"CONVERTED_RUNTIME_ASSETS_REQUIRED",true)) return false;
    fs::path ignored;
    if(!required("assets/ios-runtime-assets.json",ignored,"VERIFIED_ASSET_RECEIPT_REQUIRED")) return false;
    for(int i=0;i<21;++i) {
        const auto level=i<19 ? "level"+std::to_string(i) : i==19 ? "title" : "flight";
        if(!required(fs::path("assets/generated/mrp/moby")/level/"textures.mrpt",ignored,"MISSING_RUNTIME_TEXTURE_GROUP") ||
           !required(fs::path("assets/generated/mrp/world")/level/"world.mrpw",ignored,"MISSING_RUNTIME_WORLD_GROUP")) return false;
    }
    out=std::move(next); error.clear(); return true;
}
} // namespace rac1::platform::apple
