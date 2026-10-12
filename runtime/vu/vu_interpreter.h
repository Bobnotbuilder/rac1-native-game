#ifndef RAC1_RUNTIME_VU_INTERPRETER_H
#define RAC1_RUNTIME_VU_INTERPRETER_H

#include <cstdint>
#include <array>
#include <cstddef>

#include <functional>
#include <vector>

namespace rac1 {

struct GsPrimitive {
    float x, y, z;
    uint8_t r, g, b, a;
    float u, v;
    uint64_t prim;
    bool adc;
};

struct alignas(16) VUVector {
    union {
        float f[4];
        uint32_t u[4];
        int32_t s[4];
        float v[4];  // (the ported macro-mode code's name for the lanes)
    };
};

struct VUState {
    std::array<VUVector, 32> vf;      // vf00 to vf31
    std::array<int16_t, 16> vi;        // vi00 to vi15
    VUVector acc;                      // Accumulator
    float q;                           // Q register
    float i;                           // I register
    float r;                           // R register
    float p;                           // P register
    uint32_t macflag;                  // MAC status flags (16-bit)
    uint32_t statusflag;               // Status flags (12-bit)
    uint32_t clipflag;                 // Clip flags (24-bit)
    uint32_t mac_visible;              // MAC flags as seen by FMxxx (4-instruction latency), set by execute_microprogram
    uint32_t status_visible = 0;       // the status flag as FSxxx see it (the same latency)
    uint32_t clip_visible = 0;         // the clip flag as FCxxx see it (the same latency)
    alignas(16) std::array<uint8_t, 16384> mem; // VU data memory (16KB; covers VU0 4KB + VU1 16KB)
    std::vector<GsPrimitive> xgkick_primitives; // Extracted primitives from XGKICK
    std::function<void(uint32_t)> xgkick_hook;    // when set, XGKICK hands it the packet's quadword address instead
    uint32_t xtop = 0;                            // VIF1's TOP as XTOP reads it (set at the program's start)
};

// Represents a 128-bit general purpose Emotion Engine CPU register
struct u128 {
    uint64_t lo;
    uint64_t hi;
};

/**
 * execute_cop2_instruction
 * Decodes and executes a single 32-bit COP2 MIPS instruction.
 * Moves data between registers and modifies VUState accordingly.
 */
void execute_cop2_instruction(VUState& state, uint32_t opcode, u128* ee_gprs, bool is_macro_mode = false);

// The pipeline state of a running micro program: its pc, the branch in its delay slot, the E bit's
// countdown, the MAC / status / clip flags' 4-instruction delay, the EFU's Q in flight, the pending XGKICK.
struct VuPipe {
    uint32_t pc = 0;
    size_t size = 0;
    bool branch_pending = false;
    uint32_t branch_target = 0;
    bool halt_pending = false;
    int halt_delay = 0;
    bool halted = true;
    uint32_t mac_ring[4] = {};
    uint32_t status_ring[4] = {};
    uint32_t clip_ring[4] = {};
    uint64_t issued = 0;
    bool q_pending = false;
    float q_next = 0.0f;
    uint64_t q_ready = 0;
    int kick_delay = 0;
    uint32_t kick_addr = 0;
    // (fast ports, the fast form: one call runs many instructions) set when the call ended on
    // a taken backward jump, the step the caller's spin detection looks at (pc after < pc before)
    bool back_jump = false;
    // (fast ports) the instruction count a caller will not run past: a call stops before a block that would
    // pass it (the exact step function then runs the last instructions one by one)
    uint64_t issue_limit = ~0ull;
    // (RAC1_VU_HZ, the interpreter's integer branch hazard) the integer register the previous instruction wrote and its value before
    int vi_hz_reg = -1;
    int16_t vi_hz_old = 0;
    uint32_t pc_mask = 0xffffffffu;  // (VuMicroRun::set_pc_mask: the interpreter's wrap of branch targets)
};
// A ported micro program: runs the instruction at pipe.pc as the interpreter
// would; false once halted or past the memory.
using VuNativeStep = bool (*)(VUState& state, VuPipe& pipe);

// A micro program run one instruction at a time (the VU0 beside the EE: the EE interpreter
// steps it as it goes); execute_microprogram runs one to its end. With a native step set, the
// ported program runs instead of the interpreter.
class VuMicroRun {
public:
    void start(VUState& state, const uint64_t* micromem, size_t micromem_size, uint32_t start_pc, VuNativeStep native = nullptr);
    bool step();    // one instruction; false once halted (E bit) or past the memory
    void finish();  // the pending XGKICK / Q landed
    bool running() const { return st_ != nullptr && !p_.halted; }
    uint32_t pc() const { return p_.pc; }
    const VuPipe& pipe() const { return p_; }
    void limit_issue(uint64_t n) { p_.issue_limit = n; }  // (a fast port does not run past n instructions in all)
    // branch / jump targets wrap with this mask (VU1's 2048-instruction micro memory: 0x7ff; default none: a target past the program stops it)
    void set_pc_mask(uint32_t m) { p_.pc_mask = m; }

private:
    VUState* st_ = nullptr;
    const uint64_t* mem_ = nullptr;
    VuNativeStep native_ = nullptr;
    VuPipe p_;
};

/**
 * execute_microprogram
 * Executes a VU microprogram from the given micromemory buffer starting at start_pc.
 * Halts execution when the E-bit is encountered and its delay slot is executed.
 */
void execute_microprogram(VUState& state, const uint64_t* micromem, size_t micromem_size, uint32_t start_pc,
                          uint64_t max_instructions = 2000000, VuNativeStep native = nullptr);

} // namespace rac1

#endif // RAC1_RUNTIME_VU_INTERPRETER_H
