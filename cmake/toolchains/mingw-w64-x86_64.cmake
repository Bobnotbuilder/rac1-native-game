# Cross-compiling for Windows (x86-64) from Linux with GCC for MinGW-w64.
#   Debian / Ubuntu: sudo apt install mingw-w64
#   cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake
# The posix thread model's compiler (std::thread through winpthreads), when there is one.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(_rac1_mingw x86_64-w64-mingw32)

find_program(_rac1_mingw_cxx NAMES ${_rac1_mingw}-g++-posix ${_rac1_mingw}-g++ REQUIRED)
find_program(_rac1_mingw_cc NAMES ${_rac1_mingw}-gcc-posix ${_rac1_mingw}-gcc REQUIRED)
set(CMAKE_C_COMPILER ${_rac1_mingw_cc})
set(CMAKE_CXX_COMPILER ${_rac1_mingw_cxx})
set(CMAKE_RC_COMPILER ${_rac1_mingw}-windres)

set(CMAKE_FIND_ROOT_PATH /usr/${_rac1_mingw})
# pkg-config (FFmpeg, optional) looks in the Windows libraries only, never in the Linux system's
set(ENV{PKG_CONFIG_LIBDIR} /usr/${_rac1_mingw}/lib/pkgconfig)
set(ENV{PKG_CONFIG_PATH} "")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
