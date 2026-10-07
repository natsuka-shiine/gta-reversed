cmake_minimum_required(VERSION 4.2.0 FATAL_ERROR)

#--------------------------------------------------------------------
# librw - A re-implementation of the RenderWare Graphics engine (https://github.com/aap/librw, MIT)
#
# The standalone executable is going to use this in place of the RenderWare code of the original game.
# See `docs/roadmap_to_exe.md`.
#--------------------------------------------------------------------

include(FetchContent)

option(${RE_PROJECT}_LIBRW_EXAMPLES "Build librw's own tools and examples (to check that it works with our toolchain)" ON)

# librw is plain old C++, don't build it as C++23 (This only affects this directory)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)

set(LIBRW_PLATFORM "D3D9" CACHE STRING "" FORCE)
set(LIBRW_INSTALL OFF CACHE BOOL "" FORCE)
set(LIBRW_TOOLS ${${RE_PROJECT}_LIBRW_EXAMPLES} CACHE BOOL "" FORCE)

FetchContent_Declare(
    librw
    GIT_REPOSITORY https://github.com/aap/librw
    GIT_TAG        18532c2e13efbc1aa43f0b00b4459831f9414de0 # master as of 2026-08-26
)
FetchContent_MakeAvailable(librw)

# librw's CMake build leaves this file out of the skeleton library (its premake build has it),
# so nothing using the skeleton links without this
if (TARGET librw_skeleton)
    target_sources(librw_skeleton PRIVATE "${librw_SOURCE_DIR}/skeleton/camera.cpp")
endif()
