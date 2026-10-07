cmake_minimum_required(VERSION 4.2.0 FATAL_ERROR)

include(FetchContent)

#--------------------------------------------------------------------
# Game features
#--------------------------------------------------------------------
option(${RE_PROJECT}_WITH_OPENAL "Use OpenAL" OFF)
option(${RE_PROJECT}_USE_SDL3 "Use SDL3" OFF)
option(${RE_PROJECT}_STANDALONE "Build standalone executable" OFF)
option(${RE_PROJECT}_OWN_STATIC_DATA "Keep the game's global variables that have been migrated in our own memory (See StaticData.h)" ON)
option(${RE_PROJECT}_STANDALONE_DUMP_HOOKS_ONLY "Dump hooks only" OFF)

if (${RE_PROJECT}_USE_SDL3)
    add_compile_definitions(NOTSA_USE_SDL3)
endif()
if (${RE_PROJECT}_OWN_STATIC_DATA)
    add_compile_definitions(NOTSA_OWN_STATIC_DATA)
endif()
if (${RE_PROJECT}_STANDALONE)
    add_compile_definitions(NOTSA_STANDALONE)
endif()
if (${RE_PROJECT}_STANDALONE_DUMP_HOOKS_ONLY)
    if (NOT ${RE_PROJECT}_STANDALONE)
        message(FATAL_ERROR "${RE_PROJECT}_STANDALONE_DUMP_HOOKS_ONLY requires ${RE_PROJECT}_STANDALONE=ON")
    endif()
    add_compile_definitions(NOTSA_STANDALONE_DUMP_HOOKS_ONLY)
endif()

#--------------------------------------------------------------------
# Build options
#--------------------------------------------------------------------
option(${RE_PROJECT}_UNITY_BUILD "Use Unity/Jumbo build" OFF)
option(${RE_PROJECT}_WITH_SANITIZERS "Use Sanitizers (better crash log)" OFF)
option(${RE_PROJECT}_WITH_LTO "Use Link Time Optimization" OFF)
option(${RE_PROJECT}_WITH_CLEO_COMMANDS "Enable CLEO script commands" OFF)
option(${RE_PROJECT}_WITH_SCRIPT_COMMAND_HOOKS "Create script command hooks in the UI (Command hooks are installed regardless)" OFF)

if(${RE_PROJECT}_WITH_SCRIPT_COMMAND_HOOKS)
    add_compile_definitions(NOTSA_WITH_SCRIPT_COMMAND_HOOKS)
endif()

if (NOT WIN32 AND NOT ${RE_PROJECT}_WITH_OPENAL)
    set(${RE_PROJECT}_WITH_OPENAL ON)
endif()

#--------------------------------------------------------------------
# Gather source files, and create the target
#--------------------------------------------------------------------

#
# Regular source files
# Module scanning is disabled for these, so we can unity build
# If any of them contain module imports the compiler should error out
#
file(
    GLOB_RECURSE SOURCE_FILES_WITHOUT_MODULES_LIST
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h*"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c*"
)
list(APPEND SOURCE_FILES_WITHOUT_MODULES_LIST StdInc.cpp)
if (${RE_PROJECT}_WITH_OPENAL)
    # TODO: Remove dsound specific sources
else()
    file(
        GLOB_RECURSE OPENAL_FILES
        LIST_DIRECTORIES false
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/game_sa/Audio/OpenAL/*.c*"
        "${CMAKE_CURRENT_SOURCE_DIR}/game_sa/Audio/OpenAL/*.h*"
    )
    list(REMOVE_ITEM SOURCE_FILES_WITHOUT_MODULES_LIST ${OPENAL_FILES})
endif()

#
# Source files that should be scanned for modules
#
set(SOURCE_FILES_WITH_MODULES_LIST) # Empty for now, we dont use modules yet

#
# Module source files
#
file(
    GLOB_RECURSE CXX_MODULE_FILES_LIST
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ixx"
)

if (${RE_PROJECT}_STANDALONE)
    add_executable(${RE_PROJECT_LIB_NAME} WIN32)
else()
    add_library(${RE_PROJECT_LIB_NAME} MODULE)
endif()
if (CXX_MODULE_FILES_LIST)
    target_sources(${RE_PROJECT_LIB_NAME} PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES ${CXX_MODULE_FILES_LIST})
endif()
if (SOURCE_FILES_WITH_MODULES_LIST)
    set_source_files_properties(${SOURCE_FILES_WITH_MODULES_LIST} PROPERTIES CXX_SCAN_FOR_MODULES ON)
    target_sources(${RE_PROJECT_LIB_NAME} PRIVATE FILE_SET cxx_modules FILES ${SOURCE_FILES_WITH_MODULES_LIST})
endif()
if (SOURCE_FILES_WITHOUT_MODULES_LIST)
    set_source_files_properties(${SOURCE_FILES_WITHOUT_MODULES_LIST} PROPERTIES CXX_SCAN_FOR_MODULES OFF)
    target_sources(${RE_PROJECT_LIB_NAME} PRIVATE ${SOURCE_FILES_WITHOUT_MODULES_LIST})
endif()
target_link_options(${RE_PROJECT_LIB_NAME} PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/INCREMENTAL>"
)
target_compile_options(${RE_PROJECT_LIB_NAME} PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/MP;/Zc:preprocessor;/W3;/wd4200;/wd4251;/wd4275;/wd4996;/wd26495;/wd26812;/wd28159>"
    "$<$<OR:$<CXX_COMPILER_ID:Clang,GCC>,$<CXX_COMPILER_ID:GNU>>:-Wall;-Wno-switch>"
)

if (${RE_PROJECT}_STANDALONE) 
    set(TARGET_SUFFIX ".exe")
else()
    set(TARGET_SUFFIX ".asi")
endif()
set_target_properties(${RE_PROJECT_LIB_NAME}
    PROPERTIES
    SUFFIX "${TARGET_SUFFIX}"
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/"
    COMPILE_WARNING_AS_ERROR ON
)
target_link_options(${RE_PROJECT_LIB_NAME} PRIVATE "/SUBSYSTEM:WINDOWS")

#--------------------------------------------------------------------
# Add dependencies
#--------------------------------------------------------------------

# Adding this here as it has no Conan-center package, and it's header-only
FetchContent_Declare(
    imfilebrowser
    GIT_REPOSITORY https://github.com/AirGuanZ/imgui-filebrowser
    GIT_TAG        47a1884
)
FetchContent_MakeAvailable(imfilebrowser)

macro(add_dependency)
    foreach (target ${ARGV})
        find_package(${target} CONFIG REQUIRED)

        # Disable warnings of dependencies
        add_compile_options(${target} PRIVATE
            "$<$<CXX_COMPILER_ID:MSVC>:-w>"
            "$<$<OR:$<CXX_COMPILER_ID:Clang,GCC>,$<CXX_COMPILER_ID:GNU>>:-Wno-everything>"
        )
    endforeach()
endmacro()
add_dependency(
    nlohmann_json
    Ogg
    spdlog
    Tracy
    Vorbis
    imgui
    libjpeg-turbo
    Boost
)
if (${RE_PROJECT}_USE_SDL3)
    add_dependency(SDL3)
endif()

target_link_libraries(${RE_PROJECT_LIB_NAME} PRIVATE
    nlohmann_json::nlohmann_json
    Ogg::ogg
    spdlog::spdlog
    Tracy::TracyClient
    vorbis::vorbis
    imgui::imgui
    libjpeg-turbo::libjpeg-turbo
    ddraw
    Winmm
    dxguid
    strmiids
    d3d9
    Boost::headers
)
if (${RE_PROJECT}_USE_SDL3)
    target_link_libraries(${RE_PROJECT_LIB_NAME} PRIVATE
        SDL3::SDL3
    )
endif()

target_include_directories(${RE_PROJECT_LIB_NAME} PRIVATE
    .
    /
    toolsmenu/
    game_sa/
    game_sa/Debug/
    game_sa/Animation/
    game_sa/Attractors/

    game_sa/Audio/
    game_sa/Audio/Config
    game_sa/Audio/Entities
    game_sa/Audio/Hardware
    game_sa/Audio/Loaders
    game_sa/Audio/Managers
    game_sa/Audio/Utility

    game_sa/Collision/
    game_sa/Conversations/
    game_sa/Core/
    game_sa/Entity/
    game_sa/Entity/Dummy/
    game_sa/Entity/Object/
    game_sa/Entity/Ped/
    game_sa/Entity/Vehicle/
    game_sa/Enums/
    game_sa/Events/
    game_sa/Frontend/
    game_sa/Fx/
    game_sa/Fx/Blueprints/
    game_sa/Fx/Blueprints/Interp
    game_sa/Interior/
    game_sa/Memory/
    game_sa/Models/
    game_sa/Plant/
    game_sa/Plugins/
    game_sa/Pipelines/
    game_sa/Pipelines/CustomBuilding
    game_sa/Pipelines/CustomCarEnvMap
    game_sa/RenderWare/
    game_sa/RenderWare/rw/
    game_sa/Scripts/
    game_sa/Tasks/
    game_sa/Tasks/TaskTypes/
    game_sa/Tasks/TaskTypes/SeekEntity/

    app/
    app/imgui
    app/platform/
    app/platform/android/
    app/platform/win/
    app/platform/win/VideoPlayer/
    app/platform/linux/
    app/securom/

    oswrapper

    libs/imgui
    ${imfilebrowser_SOURCE_DIR}
)

target_sources(${RE_PROJECT_LIB_NAME} PRIVATE
    notsa.natvis
)

target_compile_definitions(${RE_PROJECT_LIB_NAME} PRIVATE
    _CRT_SECURE_NO_WARNINGS
    NOMINMAX
    USE_GTASA_ALLOCATOR
    EXTRA_DEBUG_FEATURES
    FIX_BUGS
    NOTSA_WINDOWED_MODE # NOTE: This is a workaround for the game to run in windowed mode, and it must be enabled, otherwise shit's gonna crash
    SA_SKINNED_PEDS

    USE_D3D9 # TODO
)
target_compile_features(${RE_PROJECT_LIB_NAME} PRIVATE cxx_std_23)
target_precompile_headers(${RE_PROJECT_LIB_NAME} PRIVATE StdInc.cpp)

#--------------------------------------------------------------------
# Configure build options
#--------------------------------------------------------------------
if (${RE_PROJECT}_UNITY_BUILD)
    message(STATUS "Configuring Unity/Jumbo build")
    target_compile_definitions(${RE_PROJECT_LIB_NAME} PRIVATE NOTSA_UNITY_BUILD)
    set_target_properties(${RE_PROJECT_LIB_NAME}
        PROPERTIES
        UNITY_BUILD TRUE
        UNITY_BUILD_MODE BATCH
        UNITY_BUILD_BATCH_SIZE 16
    )
endif()

if (${RE_PROJECT}_WITH_LTO)
    message(STATUS "Configuring Link Time Optimization")
    set_target_properties(${RE_PROJECT_LIB_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if (${RE_PROJECT}_WITH_SANITIZERS)
    message(STATUS "Configuring sanitizers")
   #add_sanitizers(${RE_PROJECT_LIB_NAME}) # TODO
    message(FATAL_ERROR "${RE_PROJECT}_WITH_SANITIZERS is not implemented yet; keep it OFF for now (add_sanitizers() is currently missing).")
endif()

#--------------------------------------------------------------------
# Configure the Audio API
#--------------------------------------------------------------------
if (${RE_PROJECT}_WITH_OPENAL)
    target_compile_definitions(${RE_PROJECT_LIB_NAME} PRIVATE USE_OPENAL)

    target_include_directories(${RE_PROJECT_LIB_NAME} PRIVATE
        game_sa/Audio/OpenAL
    )
else()
    target_compile_definitions(${RE_PROJECT_LIB_NAME} PRIVATE USE_DSOUND)

    target_link_libraries(${RE_PROJECT_LIB_NAME} PRIVATE dsound)
endif()


#--------------------------------------------------------------------
# Configure misc. dependencies for Windows
#--------------------------------------------------------------------
if(WIN32)
    if(MSVC)
        # MSVC knows how to find dbghelp on it's own
        target_link_libraries(${RE_PROJECT_LIB_NAME} PRIVATE
            dbghelp
        )

        target_compile_definitions(${RE_PROJECT_LIB_NAME} PRIVATE DIRECTINPUT_VERSION=0x0800)
    else()
        message(FATAL_ERROR "Need to link dbghelp lib for win32")
    endif()
endif()


#--------------------------------------------------------------------
# Install
#--------------------------------------------------------------------
if (${RE_PROJECT}_INSTALL)
    install(
        TARGETS ${RE_PROJECT_LIB_NAME}
        EXPORT ${RE_PROJECT_LIB_NAME}-targets
        RUNTIME DESTINATION "."
    )
    if (MSVC)
        install(FILES $<TARGET_PDB_FILE:${RE_PROJECT_LIB_NAME}> DESTINATION "." OPTIONAL)
    endif()
endif()

#--------------------------------------------------------------------
# librw adapter check
#
# Compiles (but doesn't link) all the game code against the RenderWare adapter for librw (`game_sa/RenderWare/librw`),
# to see how far that is from building. Not part of the default build:
#   cmake --build build --target gta_reversed_librw_check
#--------------------------------------------------------------------
if (${RE_PROJECT}_WITH_LIBRW)
    set(RE_PROJECT_LIBRW_CHECK_NAME gta_reversed_librw_check)

    add_library(${RE_PROJECT_LIBRW_CHECK_NAME} OBJECT EXCLUDE_FROM_ALL ${SOURCE_FILES_WITHOUT_MODULES_LIST})
    set_source_files_properties(${SOURCE_FILES_WITHOUT_MODULES_LIST} PROPERTIES CXX_SCAN_FOR_MODULES OFF)

    target_include_directories(${RE_PROJECT_LIBRW_CHECK_NAME} PRIVATE
        $<TARGET_PROPERTY:${RE_PROJECT_LIB_NAME},INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(${RE_PROJECT_LIBRW_CHECK_NAME} PRIVATE
        $<TARGET_PROPERTY:${RE_PROJECT_LIB_NAME},COMPILE_DEFINITIONS>
        NOTSA_LIBRW
        NOTSA_NO_ORIGINAL_CODE # Nothing may forward to code of the original executable on purpose (see `docs/roadmap_to_exe.md`)
    )
    # Exactly the options of the main target (Not the directory's: `add_dependency` above adds things to those that aren't options)
    set_target_properties(${RE_PROJECT_LIBRW_CHECK_NAME} PROPERTIES
        COMPILE_OPTIONS "$<TARGET_PROPERTY:${RE_PROJECT_LIB_NAME},COMPILE_OPTIONS>"
    )
    target_link_libraries(${RE_PROJECT_LIBRW_CHECK_NAME} PRIVATE
        $<TARGET_PROPERTY:${RE_PROJECT_LIB_NAME},LINK_LIBRARIES>
        librw
    )
    target_compile_features(${RE_PROJECT_LIBRW_CHECK_NAME} PRIVATE cxx_std_23)
    target_precompile_headers(${RE_PROJECT_LIBRW_CHECK_NAME} PRIVATE StdInc.cpp)

    if (${RE_PROJECT}_UNITY_BUILD)
        target_compile_definitions(${RE_PROJECT_LIBRW_CHECK_NAME} PRIVATE NOTSA_UNITY_BUILD)
        set_target_properties(${RE_PROJECT_LIBRW_CHECK_NAME}
            PROPERTIES
            UNITY_BUILD TRUE
            UNITY_BUILD_MODE BATCH
            UNITY_BUILD_BATCH_SIZE 16
        )
    endif()

    # Links the objects of the check above with librw into a module, to see what's still missing from the adapter
    # (Every unresolved symbol is a function the adapter declares, but doesn't implement). Not part of the default build:
    #   cmake --build build --target gta_reversed_librw_link
    set(RE_PROJECT_LIBRW_LINK_NAME gta_reversed_librw_link)

    add_library(${RE_PROJECT_LIBRW_LINK_NAME} MODULE EXCLUDE_FROM_ALL $<TARGET_OBJECTS:${RE_PROJECT_LIBRW_CHECK_NAME}>)
    set_target_properties(${RE_PROJECT_LIBRW_LINK_NAME} PROPERTIES LINKER_LANGUAGE CXX)
    target_link_libraries(${RE_PROJECT_LIBRW_LINK_NAME} PRIVATE
        $<TARGET_PROPERTY:${RE_PROJECT_LIB_NAME},LINK_LIBRARIES>
        librw
        xinput
    )
    target_link_options(${RE_PROJECT_LIBRW_LINK_NAME} PRIVATE "/SUBSYSTEM:WINDOWS" "/INCREMENTAL:NO")

    #--------------------------------------------------------------------
    # The game as an executable of its own
    #
    # The same objects again, linked into an executable that needs nothing of the original at run time: no launcher,
    # no loader, no original executable. What it still has of the original is its data (as a resource), because the game's
    # code refers to the original's global variables by address: see `standalone/ImageBlock.cpp`.
    # That data isn't in the repository, make it from your own executable first:
    #   python tools/startup-data/make_image.py <gta_sa.exe (1.0 US, compact)>
    # Not part of the default build:
    #   cmake --build build --target gta_reversed_exe
    #--------------------------------------------------------------------
    set(RE_PROJECT_STANDALONE_DIR "${PROJECT_SOURCE_DIR}/standalone")
    if (MSVC AND EXISTS "${RE_PROJECT_STANDALONE_DIR}/gta_image.bin")
        set(RE_PROJECT_EXE_NAME gta_reversed_exe)

        # `ImageBlock.cpp` first: the block has to be what the image starts with
        add_executable(${RE_PROJECT_EXE_NAME} WIN32 EXCLUDE_FROM_ALL
            "${RE_PROJECT_STANDALONE_DIR}/ImageBlock.cpp"
            "${RE_PROJECT_STANDALONE_DIR}/Entry.cpp"
            "${RE_PROJECT_STANDALONE_DIR}/Image.rc"
            $<TARGET_OBJECTS:${RE_PROJECT_LIBRW_CHECK_NAME}>
        )
        set_source_files_properties("${RE_PROJECT_STANDALONE_DIR}/Image.rc" PROPERTIES OBJECT_DEPENDS "${RE_PROJECT_STANDALONE_DIR}/gta_image.bin")
        set_target_properties(${RE_PROJECT_EXE_NAME} PROPERTIES
            LINKER_LANGUAGE CXX
            OUTPUT_NAME "gta_reversed"
            UNITY_BUILD OFF
        )
        target_link_libraries(${RE_PROJECT_EXE_NAME} PRIVATE
            $<TARGET_PROPERTY:${RE_PROJECT_LIB_NAME},LINK_LIBRARIES>
            librw
            xinput
        )
        target_link_options(${RE_PROJECT_EXE_NAME} PRIVATE
            /ENTRY:NotsaExeEntry   # Puts the data in place, then starts the C runtime
            /BASE:0x400000         # Same as the original: the data is at its addresses
            /FIXED
            /DYNAMICBASE:NO
            /SECTION:.text,ERW     # The block for the original's data is in it
            /INCREMENTAL:NO        # (Would put its own things before the block)
            /NXCOMPAT:NO           # Same as the original
            /SAFESEH:NO
            /SUBSYSTEM:WINDOWS,5.01 # Same as the launcher, see there
            /STACK:0x200000,0x1000 # Same as the original
        )
    else()
        message(STATUS "No `standalone/gta_image.bin`: target `gta_reversed_exe` isn't available (see tools/startup-data/make_image.py)")
    endif()
endif()
