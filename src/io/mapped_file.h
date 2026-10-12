#pragma once
// Read-only memory-mapped file. Used by tools that previously read the whole
// 4 GB ISO into a std::vector (needed > 4 GB RAM).

#include <cstddef>
#include <filesystem>
#include <span>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

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
        int fd = ::open(path.c_str(), O_RDONLY);
        if (fd < 0) return false;
        struct stat st {};
        if (::fstat(fd, &st) != 0 || st.st_size <= 0) { ::close(fd); return false; }
        void* p = ::mmap(nullptr, static_cast<std::size_t>(st.st_size), PROT_READ, MAP_PRIVATE, fd, 0);
        ::close(fd);
        if (p == MAP_FAILED) return false;
        data_ = static_cast<const std::byte*>(p);
        size_ = static_cast<std::size_t>(st.st_size);
        return true;
    }

    void close() {
        if (data_) ::munmap(const_cast<std::byte*>(data_), size_);
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
