// A narrow adapter to the EXISTING upstream setup exporters, not new conversion algorithms.
// The original setup enters here before gameplay's enter_ee_rounding(); preserve that environment.
#include <setup/iso_extract.h>
#include <setup/mrp_export.h>
#include <setup/scene_export.h>
#include <setup/setup_manager.h>
#include <io/mapped_file.h>
#include <cstdio>
#include <filesystem>
#include <string>
bool verify_private_assets();
int main(int argc, char** argv) {
    if(argc==2 && std::string(argv[1])=="--verify-assets") return verify_private_assets()?0:1;
    if(argc>1 && std::string(argv[1])=="--setup") return rac1::setup::run_setup(argc,argv);
    if(argc!=3) { std::fputs("usage: rac1-private-asset-prep <owned ISO> <moby|world|flight>\n",stderr); return 2; }
    const std::string stage=argv[2];
    if(stage!="moby" && stage!="world" && stage!="flight") return 2;
    const auto check=rac1::setup::validate_iso(argv[1]);
    if(!check.valid || check.serial!="SCUS_971.99") { std::fputs("Validated SCUS-97199 required\n",stderr); return 1; }
    if(!std::filesystem::exists("assets/generated/src/generated_sources.cmake")) return 1;
    // Run the actual upstream CLI/controller too: ISO linkage, stage handling and status stay canonical.
    // No force argument: existing level outputs are preserved.
    char setup[]="--setup", iso[]="--iso", step[]="--step";
    char* adapted[]={argv[0],setup,iso,argv[1],step,argv[2]};
    const int result=rac1::setup::run_setup(6,adapted);
    std::printf("\nUPSTREAM_EXPORT_STAGE=%s RESULT=%s\n",stage.c_str(),result==0?"PASS":"FAIL");
    return result;
}
