cmake_minimum_required(VERSION 3.20)
project(rac1-native LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# (a Debug build runs many times slower than the game needs)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)
# the game's code runs with the host FPU rounding toward zero (runtime/ee/ps2_fpu.h): no folding or
# moving of float operations that assumes round-to-nearest
add_compile_options(-frounding-math)

find_package(OpenGL REQUIRED)

# ---- third-party libraries (THIRD_PARTY_NOTICES.md) ----
# kept in third_party/: glad (a generated loader), 989snd (OpenGOAL's, adapted), gamecontrollerdb (data)
add_subdirectory(third_party/glad EXCLUDE_FROM_ALL)
add_subdirectory(third_party/989snd)
# downloaded at configure time: GLFW, GLM, Dear ImGui, TinyGLTF, miniaudio
include(cmake/dependencies.cmake)

# ---- src/formats/ : the game's file formats (our own decoders) ----
add_library(rac1_io INTERFACE)
target_include_directories(rac1_io INTERFACE src)

add_library(rac1_formats STATIC
    src/formats/scene_wad.cpp
    src/formats/level_textures.cpp
    src/formats/level_mesh.cpp
    src/formats/ratchet_executable.cpp
    src/formats/iso_toc.cpp
    src/formats/level_gameplay.cpp
    src/formats/level_collision.cpp
    src/formats/level_core.cpp
    src/formats/moby_model.cpp
    src/formats/moby_mrp.cpp
    src/formats/world_mrp.cpp
    src/formats/tie_model.cpp
    src/formats/shrub_model.cpp
    src/formats/sky_model.cpp
    src/formats/vag.cpp
    src/formats/sound_bank.cpp
    src/formats/animation.cpp
    src/formats/vif_stream.cpp
    src/formats/tie_packet.cpp
)
target_link_libraries(rac1_formats PUBLIC rac1_io)
target_include_directories(rac1_formats PUBLIC src)
target_link_libraries(rac1_formats PUBLIC tinygltf_headers)

# ---- runtime/ : the PS2 hardware layer only (EE / VU / GS / SIF / IOP / SPU2), no game logic ----
add_library(rac1_runtime STATIC
    runtime/kernel/ee_memory.cpp
    runtime/kernel/disc.cpp
    runtime/kernel/bootstrap.cpp
    runtime/kernel/vu0.cpp
    runtime/vu/vu_interpreter.cpp
    runtime/ee/ee_interpreter.cpp
    runtime/ee/ee_thread.cpp
    runtime/ee/ee_timer.cpp
    runtime/vu/vif_unpack.cpp
    runtime/vu/vu_xgkick.cpp
    runtime/vu/vu0.cpp
    runtime/vu/vu1.cpp
    runtime/gs/gs_state.cpp
    runtime/gs/gs_emu.cpp
    runtime/gs/gs_draw.cpp
    runtime/gs/gs_renderer.cpp
    runtime/gs/static_geom_cache.cpp
    src/game/render/texture_decode.cpp
    runtime/sif/sif_rpc.cpp
    runtime/iop/snd989_iop.cpp
    runtime/iop/snd989_stream.cpp
    runtime/iop/iop_ram.cpp
    runtime/iop/stash_iop.cpp
    runtime/ipu/movie_hw.cpp
    # file I/O (hardware-adjacent, no game decisions)
    runtime/level/level_loader.cpp
)
target_include_directories(rac1_runtime PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(rac1_runtime PUBLIC rac1_formats glad glfw glm_headers OpenGL::GL imgui opengoal_989snd)

# The movies' MPEG-2 pictures (runtime/ipu/movie_hw.cpp, the IPU's stand-in): FFmpeg's libavcodec
# when pkg-config finds it (optional). Without it the game runs and its movies are skipped (logged).
option(RAC1_MOVIES "Decode the game's movies with libavcodec when found" ON)
if(RAC1_MOVIES)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(RAC1_AVCODEC QUIET IMPORTED_TARGET libavcodec libavutil)
        # (a Homebrew FFmpeg when the system has no development files for it)
        if(NOT RAC1_AVCODEC_FOUND AND EXISTS "/home/linuxbrew/.linuxbrew/lib/pkgconfig/libavcodec.pc")
            set(ENV{PKG_CONFIG_PATH} "/home/linuxbrew/.linuxbrew/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
            pkg_check_modules(RAC1_AVCODEC QUIET IMPORTED_TARGET libavcodec libavutil)
        endif()
    endif()
    if(RAC1_AVCODEC_FOUND)
        message(STATUS "Movies: libavcodec ${RAC1_AVCODEC_libavcodec_VERSION}")
        target_link_libraries(rac1_runtime PUBLIC PkgConfig::RAC1_AVCODEC)
        target_compile_definitions(rac1_runtime PRIVATE RAC1_HAVE_AVCODEC=1)
    else()
        message(STATUS "Movies: libavcodec not found (pkg-config): the game's movies are skipped")
    endif()
endif()

# ---- src/game/ : the game's code, ported (`// orig: 0xADDR`) ----
add_subdirectory(src/game)

# ---- src/_devtools/ : the debug menu (host code, behind --debug-tools) ----
option(RAC1_DEVTOOLS "Compile src/_devtools/ (the debug menu)" ON)
add_subdirectory(src/_devtools)

# ---- src/setup/ : setup & asset extraction ----
add_library(rac1_setup STATIC
    src/setup/kick_trace.cpp
    src/setup/world_trace.cpp
    src/setup/mrp_export.cpp
    src/setup/scene_export.cpp
    src/setup/iso_extract.cpp
    src/setup/setup_manager.cpp
)
target_include_directories(rac1_setup PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(rac1_setup PUBLIC rac1_formats rac1_game rac1_runtime)

# ---- the game: one executable ----
add_executable(rac1-native
    src/host/orig_main.cpp
    src/host/audio_out.cpp
    src/host/sample_prof.cpp
    src/host/host_priority.cpp
    src/host/host_input.cpp
    src/host/sim_script.cpp
    src/host/sim_autopilot.cpp
)
target_link_libraries(rac1-native PRIVATE rac1_runtime rac1_game rac1_setup miniaudio_headers)
if(RAC1_DEVTOOLS)
    target_link_libraries(rac1-native PRIVATE rac1_devtools)
    target_compile_definitions(rac1-native PRIVATE RAC1_DEVTOOLS=1)
endif()

