// Structural readback through actual upstream runtime format readers. No rendering claim.
#include <formats/moby_mrp.h>
#include <formats/world_mrp.h>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
bool verify_private_assets() {
    namespace fs=std::filesystem;
    const fs::path base="assets/generated/mrp";
    for(int i=0;i<21;++i) {
        const std::string name=i<19 ? "level"+std::to_string(i) : i==19 ? "title" : "flight";
        if(!fs::is_regular_file(base/"moby"/name/"textures.mrpt") ||
           !fs::is_regular_file(base/"world"/name/"world.mrpw")) {
            std::fprintf(stderr,"MISSING_REQUIRED_ASSET_GROUP=%s\n",name.c_str()); return false;
        }
    }
    unsigned classes=0,textures=0,worlds=0;
    for(const auto& file:fs::recursive_directory_iterator(base)) {
        if(file.is_symlink()) { std::fputs("Unexpected converted-asset symlink\n",stderr); return false; }
        if(!file.is_regular_file()) continue;
        const auto path=file.path();
        if(!file.file_size()) return false;
        bool ok=false;
        if(path.extension()==".mrpm") {
            rac1::mrp::MobyClass c;
            ok=rac1::mrp::read_class(path.string(),c); ++classes;
        } else if(path.extension()==".mrpt") {
            std::vector<rac1::mrp::Texture> t;
            ok=rac1::mrp::read_textures(path.string(),t); ++textures;
        } else if(path.extension()==".mrpw") {
            rac1::mrpw::World w;
            ok=rac1::mrpw::read_world(path.string(),w); ++worlds;
        }
        if(!ok) { std::fprintf(stderr,"ASSET_READBACK_FAILED=%s\n",path.filename().string().c_str()); return false; }
    }
    if(classes==0 || textures<21 || worlds!=21) return false;
    std::printf("ASSET_READBACK=PASS CLASSES=%u TEXTURE_FILES=%u WORLDS=%u REQUIRED_GROUPS=21\n",classes,textures,worlds);
    std::puts("VISUAL_GAME_ASSET_ACCEPTANCE=NOT_TESTED");
    return true;
}
