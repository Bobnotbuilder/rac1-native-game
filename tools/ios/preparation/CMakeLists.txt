cmake_minimum_required(VERSION 3.20)
# HOST preparation only: the unchanged upstream exporters, no GL renderer/game window.
# Does not generate code, link an iPhone game, or claim iPhone playability.
project(rac1_private_asset_preparation LANGUAGES CXX OBJCXX)
if(NOT APPLE)
    message(FATAL_ERROR "This adaptation currently uses the existing Apple platform file adapter")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_OBJCXX_STANDARD 20)
get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
include(${ROOT}/assets/generated/src/generated_sources.cmake)
execute_process(COMMAND python3 ${ROOT}/tools/setup.py --code-inputs
                WORKING_DIRECTORY ${ROOT} OUTPUT_VARIABLE INPUTS RESULT_VARIABLE RESULT)
string(REGEX MATCH "^[^\n]+" FINGERPRINT "${INPUTS}")
if(NOT RESULT EQUAL 0 OR NOT FINGERPRINT STREQUAL RAC1_CODEGEN_FINGERPRINT)
    message(FATAL_ERROR "Completed private generation is missing or stale; do not substitute a no-generated build")
endif()
add_compile_options(-Wall -Wextra -frounding-math -ffp-contract=off -fno-strict-aliasing)
add_library(rac1_preparation_vu STATIC)
foreach(SOURCE IN LISTS RAC1_GENERATED_SOURCES)
    if(SOURCE MATCHES "/(moby_vu1|world_vu1_)[^/]*\\.cpp$")
        target_sources(rac1_preparation_vu PRIVATE ${ROOT}/assets/generated/src/${SOURCE})
    endif()
endforeach()
target_include_directories(rac1_preparation_vu PRIVATE ${ROOT} ${ROOT}/src)
add_executable(rac1-private-asset-prep
    ${ROOT}/tools/ios/preparation/main.cpp
    ${ROOT}/tools/ios/preparation/verify_assets.cpp
    ${ROOT}/src/platform/apple/platform_apple.mm
    ${ROOT}/src/setup/iso_extract.cpp ${ROOT}/src/setup/mrp_export.cpp ${ROOT}/src/setup/setup_manager.cpp
    ${ROOT}/src/setup/kick_trace.cpp ${ROOT}/src/setup/world_trace.cpp ${ROOT}/src/setup/scene_export.cpp
    ${ROOT}/runtime/kernel/disc.cpp ${ROOT}/runtime/kernel/ee_memory.cpp
    ${ROOT}/src/game/render/mrp_world_assets.cpp
    ${ROOT}/runtime/vu/vu_interpreter.cpp ${ROOT}/runtime/vu/vu_xgkick.cpp
    ${ROOT}/runtime/vu/vif_unpack.cpp ${ROOT}/runtime/vu/vu1.cpp
    ${ROOT}/src/formats/iso_toc.cpp ${ROOT}/src/formats/scene_wad.cpp
    ${ROOT}/src/formats/level_core.cpp ${ROOT}/src/formats/level_textures.cpp
    ${ROOT}/src/formats/moby_model.cpp ${ROOT}/src/formats/moby_mrp.cpp
    ${ROOT}/src/formats/world_mrp.cpp ${ROOT}/src/formats/sky_model.cpp
    ${ROOT}/src/formats/tie_model.cpp ${ROOT}/src/formats/tie_packet.cpp
    ${ROOT}/src/formats/shrub_model.cpp ${ROOT}/src/formats/vif_stream.cpp
    ${ROOT}/src/formats/animation.cpp)
target_include_directories(rac1-private-asset-prep PRIVATE ${ROOT} ${ROOT}/src)
target_link_libraries(rac1-private-asset-prep PRIVATE rac1_preparation_vu "-framework Foundation")
add_executable(rac1-private-math-oracle
    ${ROOT}/tests/apple/private_original_math_oracle.cpp
    ${ROOT}/src/game/math/vecmath.cpp ${ROOT}/src/game/ovl/ovl_reloc.cpp
    ${ROOT}/runtime/ee/ee_interpreter.cpp ${ROOT}/runtime/kernel/ee_memory.cpp
    ${ROOT}/runtime/kernel/disc.cpp
    ${ROOT}/runtime/vu/vu_interpreter.cpp ${ROOT}/runtime/vu/vu_xgkick.cpp
    ${ROOT}/src/platform/apple/platform_apple.mm)
target_include_directories(rac1-private-math-oracle PRIVATE ${ROOT} ${ROOT}/src)
target_compile_options(rac1-private-math-oracle PRIVATE -UNDEBUG)
target_link_libraries(rac1-private-math-oracle PRIVATE "-framework Foundation")
# This selected-function oracle never calls level switching; discard its unused variant-table
# graph rather than providing fake variant functions or allowing unresolved symbols.
target_link_options(rac1-private-math-oracle PRIVATE -Wl,-dead_strip)
