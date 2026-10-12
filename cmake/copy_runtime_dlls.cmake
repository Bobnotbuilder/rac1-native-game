# Run after the build on Windows (cmake -P), when the program uses DLLs that are not Windows' own (FFmpeg's, from
# MSYS2): copies them beside the program, so that it starts on its own (a double click, without MSYS2's PATH).
# -DEXE=<the program> -DSEARCH_DIR=<where the DLLs are: the compiler's bin folder> -DOBJDUMP=<objdump>
set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM windows+pe)
set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL objdump)
set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND "${OBJDUMP}")
file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${EXE}"
    DIRECTORIES "${SEARCH_DIR}"
    RESOLVED_DEPENDENCIES_VAR dlls
    UNRESOLVED_DEPENDENCIES_VAR unresolved
    PRE_EXCLUDE_REGEXES "^api-ms-.*" "^ext-ms-.*"
    POST_EXCLUDE_REGEXES ".*[/\\\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\].*")  # (Windows' own: System32 and the like)
get_filename_component(exe_dir "${EXE}" DIRECTORY)
foreach(dll IN LISTS dlls)
    file(COPY "${dll}" DESTINATION "${exe_dir}")
endforeach()
