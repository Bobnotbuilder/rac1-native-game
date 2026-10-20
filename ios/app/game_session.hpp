// UIKit adapter to the original boot/frame/level-exit state machines, not new game logic.
#pragma once
#include <game/render/metal/gs_scene.hpp>
#include <platform/apple/game_data.hpp>
#include <host/host_input.h>
namespace rac1::ios {
class GameSession {
public:
    bool configure(const platform::apple::GameDataPaths& data,std::string& error);
    bool step(std::string& error);
    void suspend();
    bool resume(std::string& error);
    bool scene(metal::SceneFrame& result,bool& clear,std::string& error);
private:
    void initialize();
    bool enter_level(bool files_read);
    void update();
    void vsync();
    static void blocking_vsync(int kind);
    bool configured_=false,initialized_=false,exiting_=false,running_=true,paused_=true,audio_=false;
    uint32_t kicked_first_=0,kicked_end_=0;
    bool initialized_before_step_=false;
    host_input::Bindings bindings_=host_input::default_bindings();
    static GameSession* active_;
};
}
