#include "game_session.hpp"
#include <runtime/ee/ps2_fpu.h>
#include <runtime/ee/ee_thread.h>
#include <runtime/ee/ee_timer.h>
#include <runtime/kernel/disc.h>
#include <runtime/kernel/ee_memory.h>
#include <runtime/iop/snd989_iop.h>
#include <runtime/iop/snd989_stream.h>
#include <runtime/iop/stash_iop.h>
#include <runtime/ipu/movie_hw.h>
#include <runtime/level/level_loader.h>
#include <game/boot/front_end.h>
#include <game/boot/front_image.h>
#include <game/boot/level_read.h>
#include <game/lib/libgraph.h>
#include <game/lib/libmc.h>
#include <game/lib/snd989.h>
#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/level_change.h>
#include <game/ovl/memory_card_ops.h>
#include <game/ovl/vsync_callback.h>
#include <game/render/render_toggles.h>
#include <game/render/render_view.h>
#include <host/host_language.h>
#include <platform/apple/game_controller.h>
#include <setup/iso_extract.h>
#include <cfenv>
#include <cstdlib>
#include <stdexcept>
bool audio_out_start();void audio_out_stop();void audio_out_push(const int16_t*,int);
void audio_out_step(bool);void audio_out_set_alone(bool);
namespace rac1::ios {
GameSession* GameSession::active_=nullptr;
namespace {
// The original host starts these same display environments before boot_start.
void display_environment() {
    sw(0x15ee8c,NOT_ADDR(0x280000));sw(0x15ee84,0xe0000);sw(0x15ee88,NOT_ADDR(0x1b0000));sw(0x15ee80,0);
    constexpr uint32_t g=0x151780;sw(0x15eeb8,g);
    sh(g+0x156,lwu(0x15ee84)>>13);sh(g+0x16e,lwu(0x15ee88)>>13);
    sh(g+0x150,512);sh(g+0x152,416);sh(g+0x16c,0x31);
    sh(g+0x15e,0);sh(g+0x158,512);sh(g+0x15a,448);sh(g+0x168,0);sh(g+0x16a,0);sw(g+0x170,0);
    sh(g+0x15c,0);sh(g+0x154,0);sh(g+0x164,0);
    gs_set_def_draw_env(g+0x40,0,512,416,3,0x31);
    sd(g+0x40,(ld(g+0x40)&~0x1ffull)|uint64_t(lh(g+0x156)&0x1ff));
    sd(g+0x50,uint64_t(int64_t(lh(g+0x16e)))|(uint64_t(lhu(g+0x16c)&0xf)<<24));
    gs_set_def_draw_env(g+0xd0,0,512,448,0,0);
    sd(g+0xd0,ld(g+0xd0)&~0x1ffull);
    for(uint32_t t:{g+0x30,g+0xc0}){sd(t,0x1000000000008008ull);sd(t+8,0xe);}
}
// Keep UIKit's FP environment intact; game code runs toward-zero/FTZ on its coroutine.
struct HostFpEnvironment { fenv_t env;HostFpEnvironment(){std::fegetenv(&env);}~HostFpEnvironment(){std::fesetenv(&env);} };
}
bool GameSession::configure(const platform::apple::GameDataPaths& data,std::string& error) {
    if(configured_ || active_) {error="GAME_SESSION_ALREADY_CONFIGURED";return false;}
    const auto check=setup::validate_iso(data.iso.string());
    if(!check.valid || check.serial!="SCUS_971.99"){error="VERIFIED_NTSC_U_INPUT_REQUIRED";return false;}
    // Called BEFORE any game, disc-cache or audio thread starts. Every unchanged assets/... reader
    // now resolves within the imported app-owned prepared package, never within the signed bundle.
    std::error_code ec;std::filesystem::current_path(data.root,ec);
    if(ec || setenv("RAC1_ISO",data.iso.c_str(),1)!=0){error="GAME_DATA_ROOT_ACTIVATION_FAILED";return false;}
    configured_=true;active_=this;paused_=false;error.clear();return true;
}
void GameSession::initialize() {
    ps2fpu::enter_ee_rounding();init_memory();g_ee_null_is_ram=true;g_legacy_host_writes=false;
    g_view_from_game=true;g_world_faithful=true;g_world_mrp=false;
    g_moby_renderer=MobyRenderer::kFaithful;g_moby_vu1_pass=true;
    sw(0x15ed88,host_language::console_language(true));
    const auto cnf=ee_stack_alloc(0x800);
    if(disc_read_ee(cnf,0x121,1))save_names_init(cnf);
    ee_stack_free(0x800);card_lib_init();mc_card_insert(mc_default_card_dir());
    snd989_iop_start();stash_iop_start();sound_callbacks_register();
    snd989_mirror_enable();audio_=audio_out_start();
    if(!audio_)throw std::runtime_error("IOS_AUDIO_DEVICE_START_FAILED");
    ee_write<uint16_t>(0x1518d0,512);ee_write<uint16_t>(0x1518d2,416);display_environment();
    ee_thread_set_vsync_hook([](EeVsyncFrame kind){blocking_vsync(int(kind));});
    if(!front_image_load())throw std::runtime_error("MATCHING_BOOT_IMAGE_LOAD_FAILED");
    ovl_reloc_set_level(OVL_FRONT);boot_start();initialized_=true;
}
void GameSession::vsync() {
    ee_t1_vsync();vsync_callback();int16_t pcm[800*2],movie[800*2];
    snd989_iop_render(pcm,800);float left=1,right=1;snd989_movie_gain(left,right);
    const int n=movie_hw_audio_take(movie,800);movie_hw_audio_add(pcm,movie,n,left,right);
    if(snd989_mirror_enabled())snd989_mirror_vsync(movie,n,left,right,pcm);else audio_out_push(pcm,800);
}
void GameSession::blocking_vsync(int kind) {
    if(!active_)return;
    if(kind==int(EeVsyncFrame::Kicked)) {
        const auto i=lwu(OV(0x1611d0));active_->kicked_first_=lwu(OV(0x1611b8)+(i&1)*4);active_->kicked_end_=lwu(OV(0x1611c0));
    }
    active_->vsync();
}
bool GameSession::enter_level(bool files_read) {
    const int level=lw(0x15ed84);
    if(!disc_has_level(level))return false;
    if(!files_read){const int previous=ovl_reloc_level();ovl_reloc_set_level(OVL_NONE);
        bool ok=level_header_read(level)&&level_files_read();
        if(ok){ee_write<uint16_t>(0x15ee48,3);while(level_bank_step()==0){}}
        ovl_reloc_set_level(previous);if(!ok)return false;}
    // The existing faithful VU/GS path reads its own native game packets; never initialize GL caches.
    LevelLoader::load_level_overlay(level);ovl_reloc_set_level(level);overlay_entry_start();return true;
}
void GameSession::update() {
    if(exiting_) {if(level_exit_frame()){level_exit_finish();running_=enter_level(level_exit_files_read());exiting_=false;}return;}
    if(boot_active()?boot_frame():overlay_frame())return;
    switch(level_exit_start()) {
        case LevelExit::Load:case LevelExit::Movie:exiting_=true;break;
        case LevelExit::Quit:
            if(!front_image_load())throw std::runtime_error("BOOT_RETURN_IMAGE_LOAD_FAILED");
            ovl_reloc_set_level(OVL_FRONT);boot_return_start();break;
    }
}
bool GameSession::step(std::string& error) {
    if(!configured_ || paused_ || !running_){error="GAME_SESSION_NOT_RUNNING";return false;}
    HostFpEnvironment restore;
    try {
        const auto state=platform::apple::sample_controller();
        const auto packet=host_input::compute(bindings_,state);
        host_input::to_packet(packet,g_host_pad_packet);
        if(initialized_)host_language::note(lwu(0x15ed88));
        initialized_before_step_=initialized_;audio_out_step(true);
        const bool complete=ee_thread_run([this]{ps2fpu::enter_ee_rounding();if(!initialized_)initialize();else update();});
        audio_out_step(false);
        if(complete && initialized_before_step_){ps2fpu::enter_ee_rounding();vsync();}
        if(!running_)throw std::runtime_error("ORIGINAL_LEVEL_TRANSITION_FAILED");
        error.clear();return true;
    }catch(const std::exception& e){audio_out_step(false);error=e.what();suspend();return false;}
}
bool GameSession::scene(metal::SceneFrame& result,bool& clear,std::string& error) {
    HostFpEnvironment restore;ps2fpu::enter_ee_rounding();
    gs::chain_join();auto& gs=gs::gs_global();gs.reset_frame();
    const bool suspended=ee_thread_suspended();
    const bool held=suspended && ee_thread_frame()==EeVsyncFrame::Held;
    clear=!suspended || ee_thread_frame()==EeVsyncFrame::Scene;
    uint32_t start=kicked_first_,end=kicked_end_;
    if(!suspended || ee_thread_frame()==EeVsyncFrame::Scene){const auto index=lwu(OV(0x1611d0));start=lwu(OV(0x1611b8)+(index&1)*4);end=lwu(OV(0x1611c0));}
    metal::SceneView view;view.width=512;view.height=416;
    view.screen_fbp=clear?uint32_t(lhu(0x151780+0x156)):0;
    gs.set_screen_area(view.screen_fbp*32,8,416);
    if(!held && start && end>start)gs.vif1_chain(start,end);
    view.depth_valid=render_gs_depth(view.depth_a,view.depth_b);
    return metal::prepare_gs_scene(gs.primitives(),view,result,error);
}
void GameSession::suspend(){paused_=true;audio_out_set_alone(false);if(audio_){audio_out_stop();audio_=false;}}
bool GameSession::resume(std::string& error){if(initialized_ && !audio_){audio_=audio_out_start();if(!audio_){error="IOS_AUDIO_RESUME_FAILED";return false;}}paused_=false;error.clear();return true;}
} // namespace rac1::ios
