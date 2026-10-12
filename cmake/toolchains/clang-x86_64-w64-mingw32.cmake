# Cross-compiling for Windows (x86-64) from Linux with clang, against MinGW-w64's headers and
# libraries (the recommended compiler: the same as on Windows with MSYS2's CLANG64 environment).
#   Debian / Ubuntu: sudo apt install clang lld mingw-w64
#   cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/clang-x86_64-w64-mingw32.cmake
# clang uses the MinGW-w64 GCC's libstdc++ and runtime (the posix thread model's, when there is one).
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(_rac1_triple x86_64-w64-windows-gnu)
set(_rac1_mingw x86_64-w64-mingw32)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_COMPILER_TARGET ${_rac1_triple})
set(CMAKE_CXX_COMPILER_TARGET ${_rac1_triple})
set(CMAKE_RC_COMPILER ${_rac1_mingw}-windres)
set(CMAKE_SYSROOT /usr/${_rac1_mingw})

# MinGW-w64 GCC's own directory (libstdc++'s headers and libgcc): the posix thread model's when installed
file(GLOB _rac1_gcc_dirs LIST_DIRECTORIES true /usr/lib/gcc/${_rac1_mingw}/*-posix)
if(NOT _rac1_gcc_dirs)
    file(GLOB _rac1_gcc_dirs LIST_DIRECTORIES true /usr/lib/gcc/${_rac1_mingw}/*)
endif()
list(SORT _rac1_gcc_dirs)
list(GET _rac1_gcc_dirs -1 _rac1_gcc_dir)
get_filename_component(_rac1_gcc_ver "${_rac1_gcc_dir}" NAME)
string(REGEX REPLACE "-(posix|win32)$" "" _rac1_gcc_base "${_rac1_gcc_ver}")
set(_rac1_cxx_inc /usr/lib/gcc/${_rac1_mingw}/${_rac1_gcc_ver}/include/c++)
# (-femulated-tls: GCC's libstdc++ for MinGW keeps its thread-local variables in emulated TLS)
set(CMAKE_C_FLAGS_INIT "-femulated-tls")
set(CMAKE_CXX_FLAGS_INIT "-femulated-tls -isystem ${_rac1_cxx_inc} -isystem ${_rac1_cxx_inc}/${_rac1_mingw} -isystem ${_rac1_cxx_inc}/backward")
set(_rac1_link "-fuse-ld=lld -L${_rac1_gcc_dir} -rtlib=libgcc -unwindlib=libgcc -stdlib=libstdc++")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_rac1_link}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_rac1_link}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_rac1_link}")

set(CMAKE_FIND_ROOT_PATH /usr/${_rac1_mingw})
# pkg-config (FFmpeg, optional) looks in the Windows libraries only, never in the Linux system's
set(ENV{PKG_CONFIG_LIBDIR} /usr/${_rac1_mingw}/lib/pkgconfig)
set(ENV{PKG_CONFIG_PATH} "")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
