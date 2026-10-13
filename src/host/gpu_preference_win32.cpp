// The dedicated GPU on Windows laptops with two (an integrated Intel / AMD one and an NVIDIA or AMD one): the
// program asks the drivers for the dedicated one by exporting these two variables, which NVIDIA's Optimus and AMD's
// PowerXpress / switchable graphics drivers look for when it starts. Without them an unknown program starts on the
// integrated GPU (Windows' "power saving"). The player's own choice (Windows Settings > System > Display > Graphics,
// or the driver's control panel) still wins. In the executable itself, not in a library: the drivers read the
// program's own exports, and nothing references these, so a library's object would not be linked.
#include <cstdint>

extern "C" {
__declspec(dllexport) std::uint32_t NvOptimusEnablement = 1;
__declspec(dllexport) std::int32_t AmdPowerXpressRequestHighPerformance = 1;
}
