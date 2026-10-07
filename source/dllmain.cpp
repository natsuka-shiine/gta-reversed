#include "StdInc.h"
#include <cstdlib>
#include <app_debug.h>
#include "extensions/CommandLine.h"
#include "extensions/debug.hpp"
#include "extensions/Configuration.hpp"
#include "InjectHooksMain.h"

static constexpr auto DEFAULT_INI_FILENAME = "gta-reversed.ini";

#include "extensions/Configs/FastLoader.hpp"
#include "extensions/Configs/Miscellaneous.hpp"
#include "dllmain.h"

HMODULE s_HandleOfDLL{};

//! Another build of this module was asked for (`GTA_REVERSED_ASI`): this one stays loaded, but does nothing
static bool s_IsInert{};

void LoadConfigurations() {
    // Firstly load the INI into the memory.
    g_ConfigurationMgr.Load(DEFAULT_INI_FILENAME);

    // Then load all specific configurations.
    g_FastLoaderConfig.Load();
    g_MiscConfig.Load();
    // ...
}

bool notsa::IsOriginalCodeAvailable() {
    static const bool s_IsAvailable = [] {
        char buf[8]{};
        return GetEnvironmentVariableA("GTA_REVERSED_STANDALONE", buf, sizeof(buf)) == 0;
    }();
    return s_IsAvailable;
}

bool notsa::IsAnotherBuildWanted() {
    // No C++ runtime here (strings, statics with constructors): this is used before it's up
    char wanted[MAX_PATH]{}, self[MAX_PATH]{};
    if (const auto n = GetEnvironmentVariableA("GTA_REVERSED_ASI", wanted, sizeof(wanted)); n == 0 || n >= sizeof(wanted)) {
        return false;
    }
    HMODULE mod{};
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(&notsa::IsAnotherBuildWanted), &mod)) {
        return false;
    }
    GetModuleFileNameA(mod, self, sizeof(self));
    const auto FileNameOf = [](const char* path) {
        const char* name = path;
        for (const char* p = path; *p; p++) {
            if (*p == '\\' || *p == '/') {
                name = p + 1;
            }
        }
        return name;
    };
    return _stricmp(FileNameOf(wanted), FileNameOf(self)) != 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH: {
        s_HandleOfDLL = hModule;

        if (notsa::IsAnotherBuildWanted()) {
            s_IsInert = true;
            return TRUE; // (Not `FALSE`: that runs our static destructors under the loader lock, and whoever loaded us might make a fuss too)
        }

        // Fail if RenderWare has already been started
        if (*(RwCamera**)0xC1703C) {
            MessageBox(NULL, "gta_reversed failed to load (RenderWare has already been started)", "Error", MB_ICONERROR | MB_OK);
            return FALSE;
        }

        std::setlocale(LC_ALL, "en_US.UTF-8");

        notsa::debug::DisplayConsole();
        notsa::debug::LoadSymbols(); // Used by logging
        notsa::Logging::CreateInstance();

        CommandLine::Load(__argc, __argv);
        if (CommandLine::s_WaitForDebugger) {
            notsa::debug::WaitForDebugger();
        }
        LoadConfigurations();

        notsa::StaticData::Init();

        ReversibleHooks::RHManager::CreateInstance();
        InjectHooksMain();

        break;
    }
    case DLL_PROCESS_DETACH: {
        if (s_IsInert) {
            break;
        }
        if (lpReserved == nullptr) {
            NOTSA_LOG_INFO("DLL_PROCESS_DETACH: Shutting down normally...");

            ReversibleHooks::RHManager::DestroyInstance();
            notsa::Logging::DestroyInstance();
            notsa::debug::UnloadSymbols();
        } else { // This is pretty much the only thing that's ever reached
            NOTSA_LOG_INFO("DLL_PROCESS_DETACH: Process is terminating, shutting down only what's necessary");

            notsa::Logging::DestroyInstance();
        }
        break;
    }
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

HMODULE notsa::GetDLLHandle() {
    return s_HandleOfDLL;
}
