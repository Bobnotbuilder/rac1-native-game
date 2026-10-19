// MIT: sandbox-compatible implementation of the existing host interface, Apple ARM64.
#include <platform/platform.h>
#import <Foundation/Foundation.h>
#include <TargetConditionals.h>
#include <mach-o/dyld.h>
#include <mach/mach.h>
#include <mach/thread_info.h>
#include <execinfo.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace rac1::platform {
std::FILE* open_file(const std::filesystem::path& path, const char* mode) { return std::fopen(path.c_str(), mode); }
std::int64_t read_at(std::FILE* f, uint64_t offset, void* dst, size_t n) {
    if (!f || offset > static_cast<uint64_t>(std::numeric_limits<off_t>::max())) { errno = EINVAL; return -1; }
    ssize_t count;
    do { count = pread(fileno(f), dst, n, static_cast<off_t>(offset)); } while (count < 0 && errno == EINTR);
    return count;
}
void will_need(std::FILE*, uint64_t, uint64_t) { /* no blocking prefetch on the render/game thread */ }
bool map_file(const std::filesystem::path& path, const std::byte** data, size_t* size) {
    if (!data || !size) return false;
    *data = nullptr;
    *size = 0;
    const int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) return false;
    struct stat st{};
    if (fstat(fd, &st) || st.st_size <= 0) { close(fd); return false; }
    void* p = mmap(nullptr, static_cast<size_t>(st.st_size), PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (p == MAP_FAILED) return false;
    *data = static_cast<const std::byte*>(p);
    *size = static_cast<size_t>(st.st_size);
    return true;
}
void unmap_file(const std::byte* data, size_t size) {
    if (data && size) munmap(const_cast<std::byte*>(data), size);
}
namespace {
std::filesystem::path folder(NSSearchPathDirectory directory, const char* component) {
    @autoreleasepool {
        NSURL* base = [[[NSFileManager defaultManager] URLsForDirectory:directory inDomains:NSUserDomainMask] firstObject];
        return base ? std::filesystem::path(base.fileSystemRepresentation) / component : std::filesystem::path{};
    }
}
}
std::filesystem::path config_dir() { return folder(NSApplicationSupportDirectory, "RAC1Native"); }
std::filesystem::path data_dir() { return folder(NSDocumentDirectory, "RAC1NativeSaves"); }
std::filesystem::path executable_path() {
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    return _NSGetExecutablePath(buffer.data(), &size) == 0 ? std::filesystem::path(buffer.data()) : std::filesystem::path{};
}
std::string system_language() {
    @autoreleasepool {
        NSString* language = [[NSLocale preferredLanguages] firstObject];
        NSString* code = language ? [NSLocale componentsFromLocaleIdentifier:language][NSLocaleLanguageCode] : nil;
        return code ? std::string(code.UTF8String) : std::string{};
    }
}
// iOS cannot run setup, Python or an updater subprocess. Return an explicit failure, not success.
int run_quiet(const std::vector<std::string>&) { errno = ENOTSUP; return -1; }
void launch_updater(const std::vector<std::string>&, const std::filesystem::path&) {
    std::fprintf(stderr, "[ios] external updater unavailable; install an app update instead\n");
}
std::string python_program() { return {}; }
std::vector<std::string> environment_names(const char* prefix) {
    std::vector<std::string> names;
    if (!prefix) return names;
    @autoreleasepool {
        for (NSString* name in [[NSProcessInfo processInfo] environment]) {
            const char* key = name.UTF8String;
            if (std::strncmp(key, prefix, std::strlen(prefix)) == 0) names.emplace_back(key);
        }
    }
    return names;
}
void unset_environment(const char* name) { if (name) unsetenv(name); }
uint64_t thread_id() { uint64_t id = 0; pthread_threadid_np(nullptr, &id); return id; }
double thread_cpu_ms() {
    thread_basic_info_data_t info{};
    mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;
    const thread_t thread = mach_thread_self();
    const kern_return_t result = thread_info(thread, THREAD_BASIC_INFO, reinterpret_cast<thread_info_t>(&info), &count);
    mach_port_deallocate(mach_task_self(), thread);
    if (result != KERN_SUCCESS) return -1;
    return 1e3 * (static_cast<double>(info.user_time.seconds) + info.system_time.seconds)
         + 1e-3 * (static_cast<double>(info.user_time.microseconds) + info.system_time.microseconds);
}
bool local_time(std::time_t t, std::tm* out) { return localtime_r(&t, out) != nullptr; }
void process_begin() {}
void process_end() {}
void print_backtrace(int frames) {
    void* addresses[64];
    if (frames > 0) backtrace_symbols_fd(addresses, backtrace(addresses, frames < 64 ? frames : 64), STDERR_FILENO);
}
} // namespace rac1::platform
