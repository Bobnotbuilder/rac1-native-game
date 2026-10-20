// Narrow original-game oracle using the upstream R5900 reference interpreter, never an app runtime.
// Inputs are the user's local NTSC-U overlay. No success stubs/hooks or skipped failed comparisons.
#ifdef NDEBUG
#error Oracle assertions must remain enabled
#endif
#include <runtime/ee/ee_interpreter.h>
#include <runtime/ee/ps2_fpu.h>
#include <runtime/kernel/ee_memory.h>
#include <game/math/vecmath.h>
#include <platform/bit_cast.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>
using namespace rac1;
static bool load_overlay() {
    std::ifstream stream("assets/dumps/ios-fp-oracle-overlay.bin",std::ios::binary);
    char magic[8]; uint32_t count=0;
    if(!stream.read(magic,8) || std::memcmp(magic,"R1FPORC1",8) || !stream.read(reinterpret_cast<char*>(&count),4) || count>128) return false;
    init_memory();
    for(uint32_t i=0;i<count;++i) {
        uint32_t address=0,size=0;
        if(!stream.read(reinterpret_cast<char*>(&address),4) || !stream.read(reinterpret_cast<char*>(&size),4)) return false;
        if(!address || uint64_t(address)+size>0x04000000) return false;
        if(!stream.read(static_cast<char*>(ee_ram_ptr(address)),size)) return false;
    }
    return true;
}
int main() {
    ps2fpu::enter_ee_rounding();
    if(!load_overlay()) { std::fputs("FAIL: audited local original overlay input required\n",stderr); return 1; }
    EeInterpreter original;
    unsigned checked=0;
    struct Function { const char* name; uint32_t address; float (*native)(float,float); };
    const Function functions[]={
        {"frame_scale_f",0x001fef10,[](float a,float){ return frame_scale_f(a); }},
        {"abs_f",0x001ff218,[](float a,float){ return abs_f(a); }},
        {"max_f",0x001ff220,[](float a,float b){ return max_f(a,b); }},
        {"min_f",0x001ff228,[](float a,float b){ return min_f(a,b); }},
    };
    // Deterministic finite input families, including rounding boundaries and signed zero.
    const float values[]={0.f,-0.f,1.f,-1.f,.1f,-.1f,0x1.000002p0f,-0x1.000002p0f,
                          .25f,-.25f,127.5f,-127.5f,1e-10f,-1e-10f,12345.125f,-12345.125f};
    for(const auto& fn:functions) {
        unsigned function_cases=0;
        for(float a:values) for(float b:values) {
            ee_write<float>(0x0015ed68,b);
            original.cpu().fpr[12]=a;
            original.cpu().fpr[13]=b;
            if(!original.call(fn.address,{},0x00166c00,10000)) {
                std::fprintf(stderr,"FAIL: original %s: %s\n",fn.name,original.error().c_str()); return 1;
            }
            const float want=original.cpu().fpr[0],got=fn.native(a,b);
            if(portable_bit_cast<uint32_t>(want)!=portable_bit_cast<uint32_t>(got)) {
                std::fprintf(stderr,"FAIL: %s case=%u original=%08x native=%08x\n",fn.name,function_cases,
                    portable_bit_cast<uint32_t>(want),portable_bit_cast<uint32_t>(got)); return 1;
            }
            ++checked; ++function_cases;
        }
        std::printf("PASS_ORIGINAL_GAME_MATH function=%s cases=%u\n",fn.name,function_cases);
    }
    std::printf("PASS: %u original-game function invocations, 4 specifically selected math routines\n",checked);
    std::puts("ARM64_RUNTIME=NOT_TESTED; GAME_BOOT_MENU_PLAYABLE=NOT_TESTED; WHOLE_GAME_COVERAGE=UNKNOWN");
}
