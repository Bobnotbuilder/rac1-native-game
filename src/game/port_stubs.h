#ifndef RAC1_GAME_PORT_STUBS_H
#define RAC1_GAME_PORT_STUBS_H

#include <iostream>
#include <unordered_set>
#include <string>
#include <cstdint>
#include <utility>
#include <type_traits>

namespace rac1 {

struct PairHash {
    std::size_t operator()(const std::pair<std::uint32_t, std::string>& p) const {
        return std::hash<std::uint32_t>{}(p.first) ^ std::hash<std::string>{}(p.second);
    }
};

// Global set to keep track of unported call logs across all translation units.
inline std::unordered_set<std::pair<std::uint32_t, std::string>, PairHash> g_logged_port_stubs;
// the call sites already logged, by the caller name's address (__func__ is one array per function): a stub called
// every frame no longer builds a string to find it was logged
struct PtrPairHash {
    std::size_t operator()(const std::pair<std::uint32_t, const char*>& p) const {
        return std::hash<std::uint32_t>{}(p.first) ^ std::hash<const void*>{}(p.second);
    }
};
inline std::unordered_set<std::pair<std::uint32_t, const char*>, PtrPairHash> g_logged_port_stub_sites;

struct PortStubHelper {
    std::uint32_t addr;
    const char* name;
    const char* caller;

    PortStubHelper(std::uint32_t addr, const char* name, const char* caller)
        : addr(addr), name(name), caller(caller) {
        log_once();
    }

    void log_once() {
        if (g_logged_port_stub_sites.count({addr, caller}) != 0) return;
        g_logged_port_stub_sites.insert({addr, caller});
        std::pair<std::uint32_t, std::string> key = {addr, caller};
        if (g_logged_port_stubs.find(key) == g_logged_port_stubs.end()) {
            g_logged_port_stubs.insert(key);
            std::cout << "[TODO_PORT] Unported call to 0x" << std::hex << addr << std::dec 
                      << " (" << name << ") from caller: " << caller << std::endl;
        }
    }

    // templated conversion to any type
    template <typename T>
    operator T() const {
        // If T is a function pointer type, return a dummy function matching that type signature!
        // In C++, we can return nullptr or cast. But to prevent direct crash if called, we can return
        // a dummy lambda converted to function pointer for void(void).
        // For other function pointer types, we just return nullptr, but since the only unported function
        // pointer we call right now is void(*)(), we support it specifically.
        if constexpr (std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>) {
            if constexpr (std::is_same_v<T, void(*)()>) {
                return []() {
                    // Dummy no-op function for void() functions
                };
            }
        }
        return T{};
    }

    // Support direct function call semantics with any arguments
    template <typename... Args>
    PortStubHelper operator()(Args&&...) const {
        return *this;
    }
};

} // namespace rac1

#define TODO_PORT(addr, name) (::rac1::PortStubHelper(addr, name, __func__))

#endif // RAC1_GAME_PORT_STUBS_H
