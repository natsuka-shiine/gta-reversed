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

#--------------------------------------------------------------------
# Local changes to librw's sources (kept minimal, each is a one-line replacement that's only done once)
#
# `@SC@` stands for a semicolon in the arguments (a real one would split the argument, lists being what they are in CMake)
#--------------------------------------------------------------------
function(librw_patch_line file marker)
    file(READ "${librw_SOURCE_DIR}/${file}" content)
    string(FIND "${content}" "${marker}" has_marker)
    if (NOT has_marker EQUAL -1)
        return() # Already done
    endif()
    string(ASCII 59 semicolon)
    set(args ${ARGN})
    while (args)
        list(POP_FRONT args from to)
        string(REPLACE "@SC@" "${semicolon}" from "${from}")
        string(REPLACE "@SC@" "${semicolon}" to "${to}")
        string(FIND "${content}" "${from}" found)
        if (found EQUAL -1)
            message(FATAL_ERROR "librw: `${from}` not found in ${file}, the local changes have to be updated for this version of librw")
        endif()
        string(REPLACE "${from}" "${to}" content "${content}")
    endwhile()
    file(WRITE "${librw_SOURCE_DIR}/${file}" "${content}")
    message(STATUS "librw: applied local changes to ${file}")
endfunction()

# An earlier version of the change below also hit two lines it shouldn't have, undo that where it happened
file(READ "${librw_SOURCE_DIR}/src/d3d/d3d.cpp" content)
string(FIND "${content}" "*ibuf = nil = (" found)
if (NOT found EQUAL -1)
    string(REPLACE "*ibuf = nil = (" "*ibuf = (" content "${content}")
    file(WRITE "${librw_SOURCE_DIR}/src/d3d/d3d.cpp" "${content}")
    message(STATUS "librw: repaired src/d3d/d3d.cpp")
endif()

# A failed buffer creation (e.g.: after a failed device reset) returned an uninitialized pointer
librw_patch_line("src/d3d/d3d.cpp" "*vbuf = nil"
    "IDirect3DVertexBuffer9 *vbuf@SC@" "IDirect3DVertexBuffer9 *vbuf = nil@SC@"
    "IDirect3DIndexBuffer9 *ibuf@SC@"  "IDirect3DIndexBuffer9 *ibuf = nil@SC@"
)

# Callbacks around the reset of the Direct3D 9 device, for what's created on the device behind librw's back
# (it has to be released for the reset to succeed), and to hear about a reset that failed.
librw_patch_line("src/d3d/d3ddevice.cpp" "deviceReleaseCB"
    "static DynamicVB *dynamicVBs@SC@"
    "static DynamicVB *dynamicVBs@SC@ void (*deviceReleaseCB)(void)@SC@ void (*deviceRestoreCB)(void)@SC@ void (*deviceResetFailedCB)(long)@SC@"

    "releaseVidmemRasters()@SC@"
    "if(deviceReleaseCB) deviceReleaseCB()@SC@ releaseVidmemRasters()@SC@"

    "restoreD3d9Device()@SC@"
    "restoreD3d9Device()@SC@ if(deviceRestoreCB) deviceRestoreCB()@SC@"

    "d3d::d3ddevice->Reset(&d3d9Globals.present)@SC@"
    "{ HRESULT resetResult = d3d::d3ddevice->Reset(&d3d9Globals.present)@SC@ if(FAILED(resetResult) && deviceResetFailedCB) deviceResetFailedCB(resetResult)@SC@ }"
)

# librw's CMake build leaves this file out of the skeleton library (its premake build has it),
# so nothing using the skeleton links without this
if (TARGET librw_skeleton)
    target_sources(librw_skeleton PRIVATE "${librw_SOURCE_DIR}/skeleton/camera.cpp")
endif()
