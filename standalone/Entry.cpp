/*
* The game as an executable of its own: no launcher, no loader, no original executable next to it.
*
* What's left of the original is its data, for as long as the game's code refers to it by address (see `ImageBlock.cpp`).
* It's put in place here, in the entry point: before the C runtime starts, because the constructors of our own globals
* use it already. So no C runtime in `NotsaExeEntry`, and nothing that needs it (security cookie, runtime checks).
*/
#include <Windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "Image.h"

extern "C" int WinMainCRTStartup();
BOOL APIENTRY  DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);
INT WINAPI     NOTSA_WinMain(HINSTANCE instance, HINSTANCE hPrevInstance, LPSTR cmdLine, INT nCmdShow);

#pragma runtime_checks("", off)
#pragma strict_gs_check(off)
#pragma optimize("", off)

namespace {
[[noreturn]] void Fail(const char* msg) {
    MessageBoxA(nullptr, msg, "gta_reversed", MB_ICONERROR | MB_OK);
    ExitProcess(1);
}
};

extern "C" __declspec(safebuffers) int NotsaExeEntry() {
    auto* const image = reinterpret_cast<volatile unsigned char*>(NOTSA_IMAGE_BEGIN);
    if (static_cast<void*>(g_NotsaOriginalImage) != reinterpret_cast<void*>(NOTSA_IMAGE_BEGIN)) {
        Fail("The block for the original's data isn't at 0x401000 (The linker has put something before it, see standalone/ImageBlock.cpp)");
    }

    // There's no code of the original: anything that goes there stops at once (`int3`), instead of running whatever is there
    for (auto i = 0u; i < NOTSA_IMAGE_DATA_BEGIN - NOTSA_IMAGE_BEGIN; i++) {
        image[i] = 0xCC;
    }

    const auto res  = FindResourceA(nullptr, NOTSA_IMAGE_RESOURCE_NAME, MAKEINTRESOURCEA(10) /*RT_RCDATA*/);
    const auto size = res ? SizeofResource(nullptr, res) : 0;
    const auto data = res ? static_cast<const unsigned char*>(LockResource(LoadResource(nullptr, res))) : nullptr;
    if (!data || size < 8) {
        Fail("The data of the game is missing from the executable");
    }
    const auto begin = *reinterpret_cast<const uint32_t*>(data), end = *reinterpret_cast<const uint32_t*>(data + 4);
    if (begin < NOTSA_IMAGE_BEGIN || end > NOTSA_IMAGE_END || begin > end || size != 8 + (end - begin)) {
        Fail("The data of the game in the executable isn't what it should be");
    }
    auto* const dst = reinterpret_cast<volatile unsigned char*>(begin);
    for (auto i = 0u; i < end - begin; i++) {
        dst[i] = data[8 + i];
    }

    return WinMainCRTStartup();
}

#pragma optimize("", on)

namespace {
//! A call to code of the original: there's none here. Say so, the game's own handler then logs where it came from.
LONG WINAPI OnException(EXCEPTION_POINTERS* info) {
    const auto* const rec  = info->ExceptionRecord;
    const auto        addr = reinterpret_cast<uintptr_t>(rec->ExceptionAddress);
    if (rec->ExceptionCode == EXCEPTION_BREAKPOINT && addr >= NOTSA_IMAGE_BEGIN && addr < NOTSA_IMAGE_DATA_BEGIN) {
        static bool s_Reported{};
        if (!s_Reported) {
            s_Reported = true;
            char msg[256];
            sprintf_s(
                msg, "The game called the function at 0x%08X of the original executable, called from 0x%08X.\nThere's no such code here: it has to be reversed.",
                static_cast<unsigned>(addr), *reinterpret_cast<const unsigned*>(info->ContextRecord->Esp)
            );
            MessageBoxA(nullptr, msg, "gta_reversed", MB_ICONERROR | MB_OK);
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
};

INT WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, INT cmdShow) {
    // The game expects the directory of the executable to be the current one
    if (char path[MAX_PATH]{}; GetModuleFileNameA(nullptr, path, sizeof(path))) {
        if (auto* const slash = strrchr(path, '\\')) {
            *slash = '\0';
            SetCurrentDirectoryA(path);
        }
    }
    AddVectoredExceptionHandler(1, &OnException);

    // Same start as when this code is a module of the original: logging, configuration, and the hooks.
    // The hooks have no code to redirect here, but the data has pointers to functions of the original in it,
    // and a hook is what makes those end up in ours.
    if (!DllMain(instance, DLL_PROCESS_ATTACH, nullptr)) {
        return 1;
    }
    return NOTSA_WinMain(instance, prevInstance, cmdLine, cmdShow);
}
