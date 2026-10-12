#pragma once
// Read-only memory-mapped file. Used by tools that previously read the whole
// 4 GB ISO into a std::vector (needed > 4 GB RAM).

#include <cstddef>
#include <filesystem>
#include <span>

#include <platform/platform.h>

namespace rac1 {

class MappedFile {
public:
    MappedFile() = default;
    explicit MappedFile(const std::filesystem::path& path) { open(path); }
    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;
    ~MappedFile() { close(); }

    bool open(const std::filesystem::path& path) {
        close();
        return platform::map_file(path, &data_, &size_);  // (mmap; MapViewOfFile on Windows)
    }

    void close() {
        if (data_) platform::unmap_file(data_, size_);
        data_ = nullptr;
        size_ = 0;
    }

    const std::byte* data() const { return data_; }
    std::size_t size() const { return size_; }
    std::span<const std::byte> span() const { return {data_, size_}; }
    explicit operator bool() const { return data_ != nullptr; }

private:
    const std::byte* data_ = nullptr;
    std::size_t size_ = 0;
};

}  // namespace rac1
