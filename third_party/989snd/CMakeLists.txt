# OpenGOAL's 989snd (ISC, see LICENSE and README.md): the IOP sound driver's reimplementation the
# host's 989snd servers run on.
file(GLOB SND989_SOURCES game/sound/989snd/*.cpp game/sound/common/*.cpp)
add_library(opengoal_989snd STATIC ${SND989_SOURCES})
target_include_directories(opengoal_989snd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(opengoal_989snd PRIVATE -w)
