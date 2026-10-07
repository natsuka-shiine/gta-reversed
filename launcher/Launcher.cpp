/*
* Inverted loader - The executable part
*
* See `LoaderDll.cpp` for the other half, and `CMakeLists.txt` for the constraints.
*
* NOTE: No C runtime here! That means: no globals with constructors, no big stack frames, no C/C++ library calls.
*/

#include <Windows.h>

#pragma runtime_checks("", off)

//! Size of the original game image (`SizeOfImage`). This executable must cover at least `[0x400000, 0x400000 + this)`.
#define GTA_IMAGE_SIZE 0x8B1000

//! Address space reservation for the original image.
//! It's an uninitialized section, so it takes no space in the file.
#pragma bss_seg(".gtaimg")
extern "C" char g_GtaImageReserve[GTA_IMAGE_SIZE];
char g_GtaImageReserve[GTA_IMAGE_SIZE];
#pragma bss_seg()
#pragma comment(linker, "/SECTION:.gtaimg,ERW")
#pragma comment(linker, "/INCLUDE:_g_GtaImageReserve")

static const char* const DLL_PATHS[] = {
    "gta_reversed_loader.dll",
    "scripts\\gta_reversed_loader.dll",
};

static void Fail(const char* msg) {
    MessageBoxA(nullptr, msg, "gta_reversed launcher", MB_ICONERROR | MB_OK);
    ExitProcess(1);
}

extern "C" void __cdecl LauncherEntry() {
    // Make the directory of this executable the current one (The game expects that)
    static char path[MAX_PATH];
    const auto  len = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        Fail("Couldn't get the path of the executable");
    }
    for (auto i = len; i-- > 0;) {
        if (path[i] == '\\' || path[i] == '/') {
            path[i] = '\0';
            break;
        }
    }
    SetCurrentDirectoryA(path);

    HMODULE dll = nullptr;
    for (const auto dllPath : DLL_PATHS) {
        if ((dll = LoadLibraryA(dllPath)) != nullptr) {
            break;
        }
    }
    if (!dll) {
        Fail("Couldn't load `gta_reversed_loader.dll` (Looked next to the executable, and in `scripts\\`)");
    }

    const auto run = reinterpret_cast<void(__cdecl*)()>(GetProcAddress(dll, "NotsaInvertedLoaderRun"));
    if (!run) {
        Fail("`gta_reversed_loader.dll` doesn't export `NotsaInvertedLoaderRun` (Is it up to date?)");
    }

    // Never returns. From here on this executable's image is replaced by the original game's.
    run();

    ExitProcess(0);
}
