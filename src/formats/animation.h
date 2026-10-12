#ifndef RAC1_FORMATS_ANIMATION_H
#define RAC1_FORMATS_ANIMATION_H

// A moby class's skeleton and animation sequences, as stored. The
// fields whose meaning is not known yet are named by their offset ("word_4").

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace rac1 {

struct Matrix4 {
    float m[4][4];
};

// A joint's bind translation and its parent (16 bytes as stored): the parent's index times 0x40
// (the size of a joint matrix), 0 for the root.
struct JointBind {
    float x, y, z;
    std::uint16_t parent_offset;
    std::uint16_t word_e;
};
static_assert(sizeof(JointBind) == 16);

// A sequence's trigger block (32 bytes, kept as stored).
struct TriggerBlock {
    std::uint32_t words[8];
};
static_assert(sizeof(TriggerBlock) == 0x20);

// One keyframe. A plain sequence's frames fill `plain`, a packed sequence's frames `packed`.
struct Keyframe {
    struct Plain {
        float word_0 = 0.0f;
        std::uint16_t word_4 = 0;
        std::uint16_t word_c = 0;
        std::vector<std::uint64_t> rotations;        // one per joint: a quaternion, 4 x s16 / 32767 (x, y, z, w)
        std::vector<std::uint64_t> extra_tracks;
        std::vector<std::uint64_t> position_tracks;  // 4 x s16: x, y, z, the joint
    } plain;
    struct Packed {
        std::uint16_t word_0 = 0;
        std::uint16_t word_2 = 0;
        std::vector<std::uint8_t> block_a;           // the frame's three blocks of the sizes the sequence gives
        std::vector<std::uint8_t> block_b;
        std::vector<std::uint8_t> block_c;
        std::vector<std::uint8_t> joint_mask;        // one bit per joint
        std::vector<std::uint8_t> extra_stream;      // one variable-length entry per extra track
        std::vector<std::uint8_t> position_stream;   // ... per position track
    } packed;
};

struct AnimSequence {
    std::array<float, 4> bounding_sphere = {0.f, 0.f, 0.f, 0.f};
    std::vector<Keyframe> frames;
    std::vector<std::uint32_t> triggers;
    std::optional<TriggerBlock> trigger_block;
    std::int32_t info = 0;
    std::uint8_t sound_count = 0xff;
    std::uint8_t byte_13 = 0;
    bool packed = false;                             // the frames are packed (a flag in the frame table)
    // A packed sequence's base pose and tracks, shared by its frames.
    struct PackedBase {
        std::vector<std::uint16_t> rotations;        // three halfwords per joint
        std::vector<std::uint64_t> extra_tracks;
        std::vector<std::uint64_t> position_tracks;
    } packed_base;
};

// One of the class's joint lists (two byte lists, as stored).
struct JointLists {
    std::vector<std::uint8_t> first;
    std::vector<std::uint8_t> second;
};

struct DecodedMobyAnimationSection {
    std::uint8_t joint_count = 0;
    std::uint8_t sequence_count = 0;
    std::uint8_t shadow_count = 0;
    std::vector<Matrix4> skeleton;                   // the joints' inverse bind matrices
    std::vector<JointBind> joint_binds;
    std::vector<JointLists> joint_lists;
    std::vector<std::optional<AnimSequence>> sequences;
};

// The skeleton and sequences of a moby class (its decompressed bytes, from its 0x48-byte header).
DecodedMobyAnimationSection decode_moby_animations(
    std::span<const std::byte> moby_class_data
);

} // namespace rac1

#endif // RAC1_FORMATS_ANIMATION_H
