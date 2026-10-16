# The installed version, for the game (src/host/update_check.cpp): run at every build (cmake -P), it writes
# OUT (rac1_version.h) only when something changed, so that only what includes it is compiled again.
#   VERSION  the release's version (project(... VERSION) in CMakeLists.txt, bumped at each release)
#   SRC      the repository; when it is a git clone (and git is there): `git describe --tags --always --dirty`,
#            the commit and the branch (a zip download has no .git: they stay empty)
#   GIT      the git program (empty: none)
#   PYTHON   the Python interpreter the build was configured with (empty: none)
set(describe "")
set(commit "")
set(branch "")
# (a git clone that tools/setup.py --update updated from the source zip, without git: its git commit is stale)
set(over_git FALSE)
if(EXISTS "${SRC}/assets/update-version.txt")
    file(STRINGS "${SRC}/assets/update-version.txt" _uv_over REGEX "^over-git 1$")
    if(_uv_over)
        set(over_git TRUE)
    endif()
endif()
if(GIT AND EXISTS "${SRC}/.git" AND NOT over_git)
    execute_process(COMMAND "${GIT}" -C "${SRC}" describe --tags --always --dirty
                    OUTPUT_VARIABLE describe OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE r1)
    execute_process(COMMAND "${GIT}" -C "${SRC}" rev-parse HEAD
                    OUTPUT_VARIABLE commit OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE r2)
    execute_process(COMMAND "${GIT}" -C "${SRC}" rev-parse --abbrev-ref HEAD
                    OUTPUT_VARIABLE branch OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE r3)
    if(NOT r1 EQUAL 0)
        set(describe "")
    endif()
    if(NOT r2 EQUAL 0)
        set(commit "")
    endif()
    if(NOT r3 EQUAL 0)
        set(branch "")
    endif()
elseif(EXISTS "${SRC}/assets/update-version.txt")
    # (a zip download, or a clone without git, updated by tools/setup.py --update: the commit / tag and channel it
    # unpacked)
    file(STRINGS "${SRC}/assets/update-version.txt" _uv_lines)
    foreach(_line IN LISTS _uv_lines)
        if(_line MATCHES "^describe (.*)$")
            set(describe "${CMAKE_MATCH_1}")
        elseif(_line MATCHES "^commit (.*)$")
            set(commit "${CMAKE_MATCH_1}")
        elseif(_line MATCHES "^branch (.*)$")
            set(branch "${CMAKE_MATCH_1}")
        endif()
    endforeach()
endif()
set(text "// Made by cmake/version.cmake at each build: do not edit.
#define RAC1_VERSION \"${VERSION}\"
#define RAC1_GIT_DESCRIBE \"${describe}\"
#define RAC1_GIT_COMMIT \"${commit}\"
#define RAC1_GIT_BRANCH \"${branch}\"
#define RAC1_BUILD_PYTHON \"${PYTHON}\"
")
set(old "")
if(EXISTS "${OUT}")
    file(READ "${OUT}" old)
endif()
if(NOT old STREQUAL text)
    file(WRITE "${OUT}" "${text}")
endif()
