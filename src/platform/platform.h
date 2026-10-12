// The host's operating system, behind one small interface (docs/platforms.md): the only place
// with Linux (POSIX) or Windows calls. Each function has a POSIX implementation (*_posix.cpp, Linux)
// and a Windows one (*_win32.cpp); CMake compiles one set. The rest of the code (runtime/,
// src/game/, src/host/...) calls these and stays standard C++.
//
// Host code: no game decision here.
#ifndef RAC1_PLATFORM_PLATFORM_H
#define RAC1_PLATFORM_PLATFORM_H

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <string>
#include <vector>

namespace rac1::platform {

// ---- The game frame's coroutine (docs/decisions.md, 12) ----
// A second stack on the calling thread, switched to and from by hand. One per process, used by one
// thread (runtime/ee/ee_thread.cpp). POSIX: ucontext. Windows: a fiber.

// Reserves the coroutine's stack, `bytes` (committed as it is touched, a guard below it). False when
// it cannot be had.
bool coroutine_create(std::size_t bytes);
// The next coroutine_resume runs `entry` from its start, with the calling thread's FPU modes (the
// rounding of docs/decisions.md, 11). Only when the coroutine is not suspended in coroutine_yield.
void coroutine_start(void (*entry)());
// Runs the coroutine until it yields or `entry` returns; then returns here.
void coroutine_resume();
// On the coroutine: back to coroutine_resume's caller; the next resume continues from here.
void coroutine_yield();

// ---- Files ----

// std::fopen on a path (on Windows a wide path, so any name works).
std::FILE* open_file(const std::filesystem::path& path, const char* mode);
// Reads up to `n` bytes at `offset` without moving a shared position (several threads read the same
// file at once). Returns the bytes read, 0 at the end, -1 on an error.
std::int64_t read_at(std::FILE* f, std::uint64_t offset, void* dst, std::size_t n);
// A hint that these bytes are read soon (nothing where the system has no such hint).
void will_need(std::FILE* f, std::uint64_t offset, std::uint64_t bytes);
// The whole file mapped read-only into memory. False when it cannot be opened, is empty, or cannot be mapped.
bool map_file(const std::filesystem::path& path, const std::byte** data, std::size_t* size);
void unmap_file(const std::byte* data, std::size_t size);

// ---- The player's folders ----

// The settings' folder (rac1-native's own): $XDG_CONFIG_HOME/rac1-native, else ~/.config/rac1-native;
// Windows: %APPDATA%\rac1-native. Empty when there is none (the callers then use the working directory).
std::filesystem::path config_dir();
// The data's folder (the memory card): $XDG_DATA_HOME/rac1-native, else ~/.local/share/rac1-native;
// Windows: %APPDATA%\rac1-native. Empty when there is none.
std::filesystem::path data_dir();
// This program's own file (empty when unknown).
std::filesystem::path executable_path();

// ---- Processes and the environment ----

// Runs a program (looked up in PATH) with `args` (args[0] the program), its standard output
// discarded, and waits for it. Its exit code; -1 when it could not be started or did not exit.
int run_quiet(const std::vector<std::string>& args);
// The Python interpreter's name for run_quiet: python3; Windows: $RAC1_PYTHON (the setup sets it), else python.
std::string python_program();
// The names of the environment's variables that start with `prefix`.
std::vector<std::string> environment_names(const char* prefix);
// Removes a variable from the environment.
void unset_environment(const char* name);

// ---- Threads, time ----

// The system's id of the calling thread (Linux: gettid; Windows: GetCurrentThreadId).
std::uint64_t thread_id();
// The calling thread's CPU time, in milliseconds.
double thread_cpu_ms();
// `t` as the local time (localtime_r). False on an error.
bool local_time(std::time_t t, std::tm* out);

// ---- Debugging ----

// The calling thread's call stack (up to `frames` return addresses) printed on stderr; nothing where
// it is not supported.
void print_backtrace(int frames);

}  // namespace rac1::platform

#endif
