// Apple framework snapshots -> UNCHANGED upstream remapping -> real PS2 packet encoding.
// Synthetic inputs; not a claim of physical Backbone/device gameplay acceptance.
#ifdef NDEBUG
#error Controller contracts require assertions
#endif
#include <platform/apple/game_controller.h>
#import <Foundation/Foundation.h>
#import <GameController/GameController.h>
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace rac1;
int main() {
    @autoreleasepool {
        auto bindings=host_input::default_bindings();
        const auto neutral=platform::apple::decode_controller_snapshot(nil);
        assert(!neutral.pad_present && neutral.pad_axes[4]==-1 && neutral.pad_axes[5]==-1);
        uint8_t packet[6];
        host_input::to_packet(host_input::compute(bindings,neutral),packet);
        assert(packet[0]==255 && packet[1]==255);
        for(unsigned i=2;i<6;++i) assert(packet[i]==127);
        GCController* controller=[GCController controllerWithExtendedGamepad];
        GCExtendedGamepad* pad=controller.extendedGamepad;
        assert(pad);
        GCControllerButtonInput* buttons[]={pad.buttonA,pad.buttonB,pad.buttonX,pad.buttonY,pad.leftShoulder,pad.rightShoulder,pad.buttonMenu};
        const uint16_t held[]={0x40,0x20,0x80,0x10,0x04,0x08,0x0800};
        for(unsigned i=0;i<7;++i) {
            assert(buttons[i]);
            [buttons[i] setValue:1];
            const auto state=platform::apple::decode_controller_snapshot([pad capture]);
            assert(state.pad_present);
            const auto output=host_input::compute(bindings,state);
            assert(output.held==held[i]);
            host_input::to_packet(output,packet);
            assert((uint16_t(packet[0])<<8|packet[1])==uint16_t(~held[i]));
            [buttons[i] setValue:0];
        }
        [pad.leftThumbstick.xAxis setValue:.75f];
        [pad.leftThumbstick.yAxis setValue:.5f];
        [pad.rightThumbstick.xAxis setValue:-.5f];
        [pad.rightThumbstick.yAxis setValue:-.75f];
        [pad.leftTrigger setValue:1];
        [pad.rightTrigger setValue:1];
        auto state=platform::apple::decode_controller_snapshot([pad capture]);
        assert(std::fabs(state.pad_axes[0]-.75f)<1e-6 && std::fabs(state.pad_axes[1]+.5f)<1e-6);
        assert(std::fabs(state.pad_axes[2]+.5f)<1e-6 && std::fabs(state.pad_axes[3]-.75f)<1e-6);
        assert(state.pad_axes[4]==1 && state.pad_axes[5]==1);
        const auto output=host_input::compute(bindings,state);
        assert(output.lx>0 && output.ly<0 && output.rx<0 && output.ry>0);
        assert(output.held==(0x01|0x02));
        // A disconnect must not leave prior trigger or stick state in the next packet.
        host_input::to_packet(host_input::compute(bindings,platform::apple::decode_controller_snapshot(nil)),packet);
        assert(packet[0]==255 && packet[1]==255);
        for(unsigned i=2;i<6;++i) assert(packet[i]==127);
        std::puts("PASS: GameController snapshots -> upstream bindings -> active-low PS2 packets; sticks/triggers/disconnect");
        std::puts("PHYSICAL_BACKBONE_ACCEPTANCE=NOT_TESTED");
    }
}
