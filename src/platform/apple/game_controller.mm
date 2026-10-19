// MIT. Polling a GameController snapshot preserves the existing remapping and PS2 packet path.
#include "game_controller.h"
#import <GameController/GameController.h>

namespace rac1::platform::apple {
host_input::HostState sample_controller() {
    host_input::HostState state{}; // disconnect means a neutral packet, never stale held controls
    @autoreleasepool {
        GCExtendedGamepad* pad = nil;
        for (GCController* controller in [GCController controllers]) {
            if (controller.extendedGamepad) { pad = [controller.extendedGamepad capture]; break; }
        }
        if (!pad) return state;
        state.pad_present = true;
        // GLFW's public gamepad positions, the indices expected by HostState (not GLFW calls).
        state.pad_buttons[0] = pad.buttonA.pressed; // Cross
        state.pad_buttons[1] = pad.buttonB.pressed; // Circle
        state.pad_buttons[2] = pad.buttonX.pressed; // Square
        state.pad_buttons[3] = pad.buttonY.pressed; // Triangle
        state.pad_buttons[4] = pad.leftShoulder.pressed;
        state.pad_buttons[5] = pad.rightShoulder.pressed;
        state.pad_buttons[6] = pad.buttonOptions.pressed;
        state.pad_buttons[7] = pad.buttonMenu.pressed;
        state.pad_buttons[8] = pad.buttonHome.pressed;
        state.pad_buttons[9] = pad.leftThumbstickButton.pressed;
        state.pad_buttons[10] = pad.rightThumbstickButton.pressed;
        state.pad_buttons[11] = pad.dpad.up.pressed;
        state.pad_buttons[12] = pad.dpad.right.pressed;
        state.pad_buttons[13] = pad.dpad.down.pressed;
        state.pad_buttons[14] = pad.dpad.left.pressed;
        state.pad_axes[0] = pad.leftThumbstick.xAxis.value;
        state.pad_axes[1] = -pad.leftThumbstick.yAxis.value; // upstream expects up negative
        state.pad_axes[2] = pad.rightThumbstick.xAxis.value;
        state.pad_axes[3] = -pad.rightThumbstick.yAxis.value;
        state.pad_axes[4] = 2.0f * pad.leftTrigger.value - 1.0f;
        state.pad_axes[5] = 2.0f * pad.rightTrigger.value - 1.0f;
    }
    return state;
}
} // namespace rac1::platform::apple
