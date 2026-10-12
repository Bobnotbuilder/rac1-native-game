#ifndef RAC1_FORMATS_LEVEL_GAMEPLAY_H
#define RAC1_FORMATS_LEVEL_GAMEPLAY_H

// A level's gameplay file: its block table summarised, and the blocks
// the tools read (moby placements, pvars, paths, cameras, volumes) decoded.

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace rac1 {

struct Vec3f {
    float x, y, z;
};

struct Vec4f {
    float x, y, z, w;
};

struct Rgb96 {
    std::int32_t r, g, b;
};

// A moby placed in the level (from its 0x78-byte record; the fields not known are named by their
// offset in it).
struct MobyPlacement {
    std::int32_t id;                 // its index in the block
    std::int32_t class_id;           // +0x18
    float scale;                     // +0x1c
    float draw_distance;             // +0x20
    std::int32_t update_distance;    // +0x24
    Vec3f position;                  // +0x30
    Vec3f rotation;                  // +0x3c
    std::int32_t group;              // +0x48
    std::int32_t rooted;             // +0x4c
    float rooted_distance;           // +0x50
    std::int32_t pvar_index;         // +0x58
    std::int32_t occlusion;          // +0x5c
    std::int32_t mode_bits;          // +0x60
    Rgb96 colour;                    // +0x64
    std::int32_t light;              // +0x70
    std::int32_t word_4;
    std::int32_t word_8;
    std::int32_t word_c;
    std::int32_t word_10;
    std::int32_t word_14;
    std::int32_t word_54;
    std::int32_t word_74;
};

// A moby's pvars: where in the pvar data they start and how many bytes they are.
struct PvarSpan {
    std::int32_t offset;
    std::int32_t size;
};

// A path: its points in world space (w is 1).
struct GameplayPath {
    std::vector<Vec4f> points;
};

// A camera placement (32 bytes as stored).
struct CameraPlacement {
    std::int32_t type;
    Vec3f position;
    Vec3f rotation;                  // Euler angles
    std::int32_t pvar_index;
};

// A volume (cuboid, sphere, cylinder or pill), 128 bytes as stored.
struct VolumeShape {
    float matrix[16];                // model to world
    float inverse_matrix[12];        // world to model, 3 x 4
    float rotation[3];               // Euler angles
    float word_7c;
};

struct GameplayBlockSummary {
    std::string name;
    bool present = false;
    std::uint32_t offset = 0;
    std::uint32_t size = 0;
    std::uint32_t row_count = 0;
};

struct DecodedGameplay {
    std::vector<MobyPlacement> mobys;
    std::vector<PvarSpan> pvar_spans;
    std::vector<std::uint8_t> pvar_data;
    std::vector<GameplayPath> paths;
    std::vector<CameraPlacement> cameras;
    std::vector<VolumeShape> cuboids;
    std::vector<VolumeShape> spheres;
    std::vector<VolumeShape> cylinders;
    std::vector<VolumeShape> pills;
    std::vector<GameplayBlockSummary> blocks;
};

// Decodes a level's gameplay file, WAD-compressed or not.
DecodedGameplay decode_level_gameplay(std::span<const std::byte> data);

} // namespace rac1

#endif // RAC1_FORMATS_LEVEL_GAMEPLAY_H
