cmake_minimum_required(VERSION 4.2.0 FATAL_ERROR)

#--------------------------------------------------------------------
# Inverted loader
#
# A tiny executable that takes the place of the original game executable in memory:
# it sits at the same base address (0x400000) and reserves (at least) the same address range.
# All it does is load `gta_reversed_loader.dll` (below), which then maps the original game image into that range,
# runs the game's entry point, and loads `gta_reversed.asi` right before the game's `WinMain` (like an ASI loader would).
#
# It must not depend on the C runtime (No TLS directory, no load config, no static initializers),
# because its own image is overwritten by the original one.
#--------------------------------------------------------------------

if (NOT MSVC)
    return()
endif()

#
# The loader DLL (A regular DLL, with the C runtime)
#
set(RE_PROJECT_LOADER_DLL_NAME gta_reversed_loader)

add_library(${RE_PROJECT_LOADER_DLL_NAME} MODULE LoaderDll.cpp)

set_target_properties(${RE_PROJECT_LOADER_DLL_NAME}
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/"
    UNITY_BUILD OFF
)

#
# The launcher executable (No C runtime)
#

# Runtime checks need the CRT (This only affects this directory)
string(REGEX REPLACE "/RTC[1csu]+" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

set(RE_PROJECT_LAUNCHER_NAME gta_reversed_launcher)

add_executable(${RE_PROJECT_LAUNCHER_NAME} WIN32 Launcher.cpp)

set_target_properties(${RE_PROJECT_LAUNCHER_NAME}
    PROPERTIES
    OUTPUT_NAME "gta_sa_reversed"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/"
    UNITY_BUILD OFF
)

target_compile_options(${RE_PROJECT_LAUNCHER_NAME} PRIVATE
    /GS- # No security cookie (needs the CRT)
    /Zl  # No default library names in the object file
)

target_link_options(${RE_PROJECT_LAUNCHER_NAME} PRIVATE
    /NODEFAULTLIB
    /ENTRY:LauncherEntry
    # The subsystem version decides how Windows treats the process. The original is 4.0, the lowest the linker accepts is 5.01.
    # It must be below 6.0: from that on Windows terminates the process when it detects heap corruption (0xC0000374),
    # and the game does things to its heap that only the old behavior tolerates.
    /SUBSYSTEM:WINDOWS,5.01
    /BASE:0x400000         # Same as the original
    /FIXED                 # No relocations - Must load at the base above
    /DYNAMICBASE:NO
    /NXCOMPAT:NO           # Same as the original
    /SAFESEH:NO            # The original's exception handlers must be accepted for this address range
    /STACK:0x200000,0x1000 # Same as the original
    /HEAP:0x200000,0x1000  # Same as the original
    /TSAWARE:NO            # Same as the original
    /MANIFEST:NO
)

target_link_libraries(${RE_PROJECT_LAUNCHER_NAME} PRIVATE
    kernel32
    user32
)
