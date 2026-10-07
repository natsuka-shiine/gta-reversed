/*
* Inverted loader - The DLL part
*
* Normally the original game executable is started, and `gta_reversed.asi` is loaded into it (by an ASI loader).
* With the inverted loader it's the other way around: Our own (tiny) executable is started (See `Launcher.cpp`),
* it loads this DLL, and this DLL then:
*   1. Maps the original game image to where it would normally be (The launcher reserves that address range)
*   2. Fills in the game's import table
*   3. Runs the game's entry point, which initializes the game's C runtime and static objects
*   4. Right before the game's `WinMain` would run, loads `gta_reversed.asi` (just like an ASI loader would), then lets `WinMain` run
*
* So all the addresses used by the hooks (and by `StaticRef`) stay valid, and the ASI needs no changes.
*
* This DLL must not depend on anything of the game or of the ASI.
*/

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <format>
#include <span>
#include <string>
#include <string_view>

namespace {
constexpr uintptr_t GAME_IMAGE_BASE  = 0x400000;
constexpr uint32_t  GAME_IMAGE_SIZE  = 0x8B1000;
constexpr uint32_t  GAME_ENTRY_POINT = 0x824570; // 1.0 US (compact) - `WinMainCRTStartup`
constexpr uint32_t  GAME_WIN_MAIN    = 0x748710; // Called by the above, once the game's C runtime is up

//! Environment variable to override the path of the original executable
constexpr auto ENV_ORIGINAL_EXE = "GTA_REVERSED_ORIGINAL_EXE";

//! Name of the environment variable that can be used to load another module in place of `gta_reversed.asi` (For trying other builds of it)
constexpr auto ENV_ASI = "GTA_REVERSED_ASI";

//! Tried in this order (relative to the current directory, which is the launcher's)
constexpr const char* ORIGINAL_EXE_CANDIDATES[]{
    "gta_sa_compact.exe",
    "gta_sa.exe",
    "gta-sa.exe",
};

//! Tried in this order
constexpr const char* ASI_CANDIDATES[]{
    "scripts\\gta_reversed.asi",
    "gta_reversed.asi",
};

//! Written next to the launcher, started over on each run. For when the game dies before (or without) its own logging
constexpr auto TRACE_FILE_NAME = "gta_reversed_loader.log";

HANDLE s_TraceFile = INVALID_HANDLE_VALUE;

void Trace(const std::string& msg) {
    if (s_TraceFile == INVALID_HANDLE_VALUE) {
        s_TraceFile = CreateFileA(TRACE_FILE_NAME, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (s_TraceFile == INVALID_HANDLE_VALUE) {
            return;
        }
    }
    const auto line = std::format("[{:>8}] {}\r\n", GetTickCount(), msg);
    DWORD      written{};
    WriteFile(s_TraceFile, line.data(), static_cast<DWORD>(line.size()), &written, nullptr);
    FlushFileBuffers(s_TraceFile);
}

//! `module+offset` of an address
std::string DescribeAddress(uintptr_t addr) {
    if (addr >= GAME_IMAGE_BASE && addr < GAME_IMAGE_BASE + GAME_IMAGE_SIZE) {
        return std::format("0x{:08X} (original game code)", addr);
    }
    HMODULE mod{};
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(addr), &mod) && mod) {
        char path[MAX_PATH]{};
        GetModuleFileNameA(mod, path, sizeof(path));
        const std::string_view sv{ path };
        const auto             slash = sv.find_last_of('\\');
        return std::format("0x{:08X} ({}+0x{:X})", addr, slash == sv.npos ? sv : sv.substr(slash + 1), addr - reinterpret_cast<uintptr_t>(mod));
    }
    return std::format("0x{:08X}", addr);
}

/*!
* List everything on the stack that points into code: that's the return addresses, and some stale or unrelated values.
* For where there are no frame pointers to walk (the original game's code), so a regular stack trace stops short.
*/
void TraceCodePointersOnStack(uintptr_t esp, int maxListed) {
    auto       numListed = 0;
    const auto sp        = reinterpret_cast<const uintptr_t*>(esp);
    for (auto i = 0; i < 2048 && numListed < maxListed; i++) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(&sp[i], &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))) {
            break; // End of the stack
        }
        const auto value = sp[i];
        if (!VirtualQuery(reinterpret_cast<void*>(value), &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT) {
            continue;
        }
        if (!(mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) {
            continue;
        }
        Trace(std::format("    [esp+0x{:04X}] {}", i * sizeof(uintptr_t), DescribeAddress(value)));
        numListed++;
    }
}

//! Records the exceptions that usually mean the end (They may still be handled by somebody, so only the first few are recorded)
LONG CALLBACK TraceExceptions(EXCEPTION_POINTERS* info) {
    static LONG s_NumRecorded = 0;

    const auto* const rec = info->ExceptionRecord;
    switch (rec->ExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_STACK_OVERFLOW:
    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_PRIV_INSTRUCTION:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_BREAKPOINT:
    case 0xC0000374: // Heap corruption
    case 0xC0000409: // Fail fast (e.g.: `abort`, `std::terminate`, stack cookie)
    case 0xE06D7363: // C++ exception
        break;
    default:
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (rec->ExceptionCode == 0xE06D7363) { // These are usually caught, don't let them use up the limit below
        static LONG s_NumCppRecorded = 0;
        if (InterlockedIncrement(&s_NumCppRecorded) <= 3) {
            Trace(std::format("C++ exception thrown at {} (Only the first 3 are recorded)", DescribeAddress(reinterpret_cast<uintptr_t>(rec->ExceptionAddress))));
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (InterlockedIncrement(&s_NumRecorded) > 16) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    auto msg = std::format("Exception 0x{:08X} at {}", rec->ExceptionCode, DescribeAddress(reinterpret_cast<uintptr_t>(rec->ExceptionAddress)));
    if (rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && rec->NumberParameters >= 2) {
        msg += std::format(", {} 0x{:08X}", rec->ExceptionInformation[0] ? "writing" : "reading", rec->ExceptionInformation[1]);
    }
    Trace(msg);

    if (rec->ExceptionCode != EXCEPTION_STACK_OVERFLOW) { // (No stack left to do this with)
        void*      frames[24]{};
        const auto n = CaptureStackBackTrace(0, static_cast<DWORD>(std::size(frames)), frames, nullptr);
        for (auto i = 0u; i < n; i++) {
            Trace("    " + DescribeAddress(reinterpret_cast<uintptr_t>(frames[i])));
        }

        // The above walks frame pointers, which the original game's code doesn't keep
        const auto eip = reinterpret_cast<uintptr_t>(rec->ExceptionAddress);
        if (eip >= GAME_IMAGE_BASE && eip < GAME_IMAGE_BASE + GAME_IMAGE_SIZE) {
            Trace("  In the original game's code, code pointers on the stack at the time:");
            TraceCodePointersOnStack(info->ContextRecord->Esp, 32);
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

HANDLE s_MainThread{};

//! Where the main thread is (and roughly how it got there): for when the game hangs without a word
void TraceMainThread(const char* when) {
    if (SuspendThread(s_MainThread) == static_cast<DWORD>(-1)) {
        return;
    }
    CONTEXT ctx{};
    ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
    if (GetThreadContext(s_MainThread, &ctx)) {
        Trace(std::format("Main thread {}: at {}", when, DescribeAddress(ctx.Eip)));

        TraceCodePointersOnStack(ctx.Esp, 48);
    }
    ResumeThread(s_MainThread);
}

DWORD WINAPI TraceMainThreadLater(void*) {
    Sleep(8'000);
    TraceMainThread("after 8 seconds");
    Sleep(22'000);
    TraceMainThread("after 30 seconds");
    return 0;
}

[[noreturn]] void LoaderFail(const std::string& msg) {
    Trace("FAILED: " + msg);
    MessageBoxA(nullptr, msg.c_str(), "gta_reversed inverted loader", MB_ICONERROR | MB_OK);
    ExitProcess(1);
}

std::span<const uint8_t> ReadWholeFile(const std::string& path) {
    const auto h = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        return {};
    }
    const auto size = GetFileSize(h, nullptr);
    auto*      data = size != INVALID_FILE_SIZE && size ? static_cast<uint8_t*>(VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)) : nullptr;
    DWORD      read{};
    const auto ok   = data && ReadFile(h, data, size, &read, nullptr) && read == size;
    CloseHandle(h);
    if (!ok) {
        if (data) {
            VirtualFree(data, 0, MEM_RELEASE);
        }
        return {};
    }
    return { data, size };
}

//! Check that the file is the executable all the addresses in the source are for
const IMAGE_NT_HEADERS32* GetValidatedHeaders(std::span<const uint8_t> file) {
    if (file.size() < sizeof(IMAGE_DOS_HEADER)) {
        return nullptr;
    }
    const auto* const dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || static_cast<size_t>(dos->e_lfanew) + sizeof(IMAGE_NT_HEADERS32) > file.size()) {
        return nullptr;
    }
    const auto* const nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data() + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        return nullptr;
    }
    const auto& opt = nt->OptionalHeader;
    if (opt.ImageBase != GAME_IMAGE_BASE || opt.SizeOfImage != GAME_IMAGE_SIZE || opt.ImageBase + opt.AddressOfEntryPoint != GAME_ENTRY_POINT) {
        return nullptr;
    }
    return nt;
}

std::span<const uint8_t> FindOriginalExecutable() {
    std::string tried;
    const auto Try = [&](const std::string& path) -> std::span<const uint8_t> {
        tried += "\n  " + path;
        const auto file = ReadWholeFile(path);
        if (file.empty()) {
            tried += " (not found)";
            return {};
        }
        if (!GetValidatedHeaders(file)) {
            tried += " (not the 1.0 US compact executable)";
            VirtualFree(const_cast<uint8_t*>(file.data()), 0, MEM_RELEASE);
            return {};
        }
        return file;
    };

    char envPath[MAX_PATH]{};
    if (const auto n = GetEnvironmentVariableA(ENV_ORIGINAL_EXE, envPath, sizeof(envPath)); n > 0 && n < sizeof(envPath)) {
        if (const auto file = Try(envPath); !file.empty()) {
            return file;
        }
    }
    for (const auto candidate : ORIGINAL_EXE_CANDIDATES) {
        if (const auto file = Try(candidate); !file.empty()) {
            return file;
        }
    }
    LoaderFail(std::format(
        "Couldn't find the original game executable.\n"
        "It must be the 1.0 US \"compact\" one (image size {:#x}, entry point {:#x}).\n"
        "Set the `{}` environment variable to its path, or put it next to the launcher.\n\nTried:{}",
        GAME_IMAGE_SIZE, GAME_ENTRY_POINT, ENV_ORIGINAL_EXE, tried
    ));
}

//! Copy the headers and the sections to where the Windows loader would've put them
void MapImage(std::span<const uint8_t> file, const IMAGE_NT_HEADERS32& nt) {
    auto* const base = reinterpret_cast<uint8_t*>(GAME_IMAGE_BASE);

    // The launcher must own the whole range
    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(base, &mbi, sizeof(mbi)) || mbi.AllocationBase != base || mbi.Type != MEM_IMAGE) {
        LoaderFail("The address range of the game isn't owned by the launcher executable (Was this DLL loaded by something else?)");
    }
    if (const auto* const launcherNt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + reinterpret_cast<const IMAGE_DOS_HEADER*>(base)->e_lfanew);
        launcherNt->OptionalHeader.SizeOfImage < GAME_IMAGE_SIZE) {
        LoaderFail("The launcher executable is too small to hold the game image (Rebuild the launcher)");
    }

    DWORD oldProtect{};
    if (!VirtualProtect(base, GAME_IMAGE_SIZE, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        LoaderFail(std::format("Couldn't make the game's address range writable (error {})", GetLastError()));
    }

    // NOTE: This wipes the launcher's own code and data. Nothing of it is used after this point.
    memset(base, 0, GAME_IMAGE_SIZE);
    memcpy(base, file.data(), std::min<size_t>(nt.OptionalHeader.SizeOfHeaders, file.size()));

    const auto* const sections = IMAGE_FIRST_SECTION(&nt);
    for (auto i = 0u; i < nt.FileHeader.NumberOfSections; i++) {
        const auto& s    = sections[i];
        const auto  size = s.Misc.VirtualSize ? std::min(s.SizeOfRawData, s.Misc.VirtualSize) : s.SizeOfRawData;
        if (!size) {
            continue;
        }
        if (static_cast<size_t>(s.PointerToRawData) + size > file.size() || s.VirtualAddress + size > GAME_IMAGE_SIZE) {
            LoaderFail("The original game executable is truncated or malformed");
        }
        memcpy(base + s.VirtualAddress, file.data() + s.PointerToRawData, size);
    }
}

//! Fill in the import address table (The delay-loaded imports are resolved by the game itself)
void ResolveImports() {
    auto* const       base = reinterpret_cast<uint8_t*>(GAME_IMAGE_BASE);
    const auto* const nt   = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + reinterpret_cast<const IMAGE_DOS_HEADER*>(base)->e_lfanew);
    const auto&       dir  = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!dir.VirtualAddress) {
        return;
    }
    for (auto* desc = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(base + dir.VirtualAddress); desc->Name; desc++) {
        const auto* const dllName = reinterpret_cast<const char*>(base + desc->Name);
        const auto        dll     = LoadLibraryA(dllName);
        if (!dll) {
            LoaderFail(std::format("Couldn't load `{}`, which the game needs (error {})", dllName, GetLastError()));
        }
        const auto* lookup = reinterpret_cast<const IMAGE_THUNK_DATA32*>(base + (desc->OriginalFirstThunk ? desc->OriginalFirstThunk : desc->FirstThunk));
        auto*       iat    = reinterpret_cast<IMAGE_THUNK_DATA32*>(base + desc->FirstThunk);
        for (; lookup->u1.AddressOfData; lookup++, iat++) {
            FARPROC     fn{};
            std::string fnName;
            if (IMAGE_SNAP_BY_ORDINAL32(lookup->u1.Ordinal)) {
                const auto ordinal = static_cast<WORD>(IMAGE_ORDINAL32(lookup->u1.Ordinal));
                fn     = GetProcAddress(dll, MAKEINTRESOURCEA(ordinal));
                fnName = std::format("#{}", ordinal);
            } else {
                const auto* const byName = reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(base + lookup->u1.AddressOfData);
                fn     = GetProcAddress(dll, byName->Name);
                fnName = byName->Name;
            }
            if (!fn) {
                LoaderFail(std::format("`{}` doesn't have `{}`, which the game needs", dllName, fnName));
            }
            iat->u1.Function = reinterpret_cast<DWORD>(fn);
        }
    }
}

std::array<uint8_t, 5> s_WinMainOriginalBytes{};

/*!
* The game's entry point calls this instead of its `WinMain`. By now the game's C runtime (and with that, its heap) is initialized,
* which is the state an ASI loader would load the ASI in. So that's what's done here.
*/
int WINAPI OnGameWinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int cmdShow) {
    memcpy(reinterpret_cast<void*>(GAME_WIN_MAIN), s_WinMainOriginalBytes.data(), s_WinMainOriginalBytes.size());
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(GAME_WIN_MAIN), s_WinMainOriginalBytes.size());

    // Might have been loaded already (if there's an ASI loader among the game's DLLs), that's fine
    HMODULE     asi{};
    std::string errors;
    char        envPath[MAX_PATH]{};
    if (const auto n = GetEnvironmentVariableA(ENV_ASI, envPath, sizeof(envPath)); n > 0 && n < sizeof(envPath)) {
        Trace(std::format("`{}` is set, loading `{}`", ENV_ASI, envPath));
        if ((asi = LoadLibraryA(envPath)) == nullptr) {
            LoaderFail(std::format("Couldn't load `{}` (set in `{}`), error {}", envPath, ENV_ASI, GetLastError()));
        }
    }
    for (const auto path : ASI_CANDIDATES) {
        if (asi || (asi = LoadLibraryA(path)) != nullptr) {
            break;
        }
        errors += std::format("\n  {} (error {})", path, GetLastError());
    }
    if (!asi) {
        LoaderFail("Couldn't load `gta_reversed.asi`. Tried:" + errors);
    }

    {
        char path[MAX_PATH]{};
        GetModuleFileNameA(asi, path, sizeof(path));
        Trace(std::format("Loaded `{}` at 0x{:08X}, calling `WinMain`", path, reinterpret_cast<uintptr_t>(asi)));
    }

    // `WinMain` is hooked by the ASI now, so this ends up in ours
    const auto ret = reinterpret_cast<int(WINAPI*)(HINSTANCE, HINSTANCE, LPSTR, int)>(GAME_WIN_MAIN)(instance, prevInstance, cmdLine, cmdShow);
    Trace(std::format("`WinMain` returned {}", ret));
    return ret;
}

//! Redirect the game's `WinMain` to `OnGameWinMain`
void HookGameWinMain() {
    auto* const fn = reinterpret_cast<uint8_t*>(GAME_WIN_MAIN);
    memcpy(s_WinMainOriginalBytes.data(), fn, s_WinMainOriginalBytes.size());
    fn[0] = 0xE9; // jmp rel32
    const auto rel = reinterpret_cast<uintptr_t>(&OnGameWinMain) - (GAME_WIN_MAIN + 5);
    memcpy(fn + 1, &rel, sizeof(rel));
}
}; // namespace

/*!
* Called by the launcher (on the main thread, right after this DLL was loaded). Never returns.
*/
extern "C" __declspec(dllexport) void __cdecl NotsaInvertedLoaderRun() {
    Trace("Inverted loader started");
    AddVectoredExceptionHandler(1, &TraceExceptions);
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &s_MainThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
    CloseHandle(CreateThread(nullptr, 0, &TraceMainThreadLater, nullptr, 0, nullptr));
    {
        const auto file = FindOriginalExecutable();
        MapImage(file, *GetValidatedHeaders(file));
        VirtualFree(const_cast<uint8_t*>(file.data()), 0, MEM_RELEASE);
    }
    Trace("Original executable mapped, resolving its imports (This loads the game's DLLs, and with them an ASI loader if there's one)");
    ResolveImports();
    HookGameWinMain();
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(GAME_IMAGE_BASE), GAME_IMAGE_SIZE);

    // Run the original entry point: it initializes the game's C runtime, runs its static constructors,
    // and then calls `WinMain` (0x748710), which goes to `OnGameWinMain` (above)
    Trace("Running the original entry point");
    reinterpret_cast<void(__cdecl*)()>(GAME_ENTRY_POINT)();

    Trace("The original entry point returned");
    ExitProcess(0);
}
