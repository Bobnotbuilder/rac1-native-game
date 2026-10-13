# tests/oracle/ — the oracle: the ported game code run against the original, executed by the EE interpreter (ADR 6,
# docs/oracle.md). For developers: built only with -DRAC1_ORACLE=ON, never by ./setup.sh for players. It needs the
# generated sources (made from the NTSC-U disc) and, for most batches, RAM dumps that are never in the repository.

# the interpreter's calls of the original, the comparison of the RAM (shared by the two programs)
add_library(oracle_env STATIC oracle_env.cpp)
target_link_libraries(oracle_env PUBLIC rac1_runtime)
target_include_directories(oracle_env PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR})

# the programs land in build/tests/ (build/tests/ovl_oracle --batch N), run from the repository's root (they read assets/)
set(RAC1_ORACLE_BIN_DIR ${CMAKE_BINARY_DIR}/tests)

# the vector math library alone (the first port proved this way)
add_executable(vecmath_oracle vecmath_oracle.cpp)
target_link_libraries(vecmath_oracle PRIVATE oracle_env rac1_game)
set_target_properties(vecmath_oracle PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${RAC1_ORACLE_BIN_DIR})

# every port's batch: one batch_<N>.cpp each (docs/oracle.md: adding a batch)
file(GLOB OVL_ORACLE_BATCHES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/batch_*.cpp)
add_executable(ovl_oracle ovl_oracle.cpp ovl_oracle_unit.cpp ${OVL_ORACLE_BATCHES})
target_link_libraries(ovl_oracle PRIVATE oracle_env rac1_game)
set_target_properties(ovl_oracle PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${RAC1_ORACLE_BIN_DIR})
if(WIN32)
    # (the game's code runs on the main thread's stack here too: 64 MB, as the game's)
    target_link_options(ovl_oracle PRIVATE -Wl,--stack,67108864)
    target_link_options(vecmath_oracle PRIVATE -Wl,--stack,67108864)
endif()
