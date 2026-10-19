cmake_minimum_required(VERSION 3.20)
# Intentionally separate from the desktop OpenGL build. This is a runtime test target, NOT an app.
project(rac1-apple-runtime LANGUAGES CXX OBJCXX ASM)
if(NOT APPLE OR NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    message(FATAL_ERROR "This target requires Apple arm64, not arm64e or an Intel gameplay build")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_OBJCXX_STANDARD 20)
set(ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
add_compile_options("$<$<COMPILE_LANGUAGE:CXX,OBJCXX>:-Wall;-Wextra;-Werror;-frounding-math;-ffp-contract=off;-fno-strict-aliasing;-UNDEBUG>")
add_library(rac1_apple_runtime STATIC
    ${ROOT}/src/platform/apple/platform_apple.mm
    ${ROOT}/src/platform/apple/coroutine_apple.cpp
    ${ROOT}/src/platform/apple/arm64_context.S
    ${ROOT}/src/platform/apple/game_controller.mm
    ${ROOT}/runtime/ee/ee_thread.cpp)
target_include_directories(rac1_apple_runtime PUBLIC ${ROOT} ${ROOT}/src)
target_compile_options(rac1_apple_runtime PRIVATE "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>")
target_link_libraries(rac1_apple_runtime PUBLIC "-framework Foundation" "-framework GameController")
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    enable_testing()
    add_executable(rac1_apple_runtime_test ${ROOT}/tests/apple/runtime_test.cpp)
    target_link_libraries(rac1_apple_runtime_test PRIVATE rac1_apple_runtime)
    add_test(NAME apple_runtime COMMAND rac1_apple_runtime_test)
    add_executable(rac1_metal_contract ${ROOT}/tests/apple/metal_contract.mm)
    target_compile_options(rac1_metal_contract PRIVATE -fobjc-arc)
    target_link_libraries(rac1_metal_contract PRIVATE "-framework Foundation" "-framework Metal")
endif()
