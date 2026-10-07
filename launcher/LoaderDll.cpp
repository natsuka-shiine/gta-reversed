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

/*!
* If this environment variable is set, the original's code is made non-executable before the game starts (its data stays).
* Every function of it that still gets called is recorded in the trace file (once, with who called it), then allowed to run.
* What's listed there is what's left to do before the original executable isn't needed for its code.
*/
constexpr auto ENV_TRAP_ORIGINAL_CODE = "GTA_REVERSED_TRAP_ORIGINAL_CODE";

/*!
* If this environment variable is set, none of the original's code is run at all: not its start-up code (C runtime, the constructors
* of its global objects), and anything else that calls into it is stopped with a message. Only its data is used.
* This is what the game is with the original executable there for nothing but its data.
*/
constexpr auto ENV_STANDALONE = "GTA_REVERSED_STANDALONE";

/*!
* If this environment variable is set, the data of the original is saved right before its `WinMain` would run: that is with
* everything its start-up code has done to it (its 1667 initializers of global variables, and its C runtime's own).
* The difference to the data in the executable's file is what a standalone game has to do itself, see `tools/startup-data`.
* Set it to the name of the file to write. Use it with `GTA_REVERSED_ASI` set, so that the asi an ASI loader has loaded by
* then has stayed out of the way.
*/
constexpr auto ENV_DUMP_STARTUP_DATA = "GTA_REVERSED_DUMP_STARTUP_DATA";

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

/*
* Trapping calls into the original's code
*/
struct CodeRange {
    uintptr_t begin, end;
};
CodeRange         s_OriginalCode[8]{};
uint32_t          s_NumOriginalCodeRanges{};
bool              s_IsTrappingOriginalCode{};
bool              s_IsStandalone{}; //!< No code of the original may run (`ENV_STANDALONE`)
uintptr_t         s_TrappedEntries[2048]{};
volatile LONG     s_NumTrappedEntries{};

bool IsOriginalCode(uintptr_t addr) {
    for (auto i = 0u; i < s_NumOriginalCodeRanges; i++) {
        if (addr >= s_OriginalCode[i].begin && addr < s_OriginalCode[i].end) {
            return true;
        }
    }
    return false;
}

void ProtectOriginalCode(bool executable) {
    for (auto i = 0u; i < s_NumOriginalCodeRanges; i++) {
        DWORD old{};
        VirtualProtect(reinterpret_cast<void*>(s_OriginalCode[i].begin), s_OriginalCode[i].end - s_OriginalCode[i].begin, executable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE, &old);
    }
}

//! A call into the original's code was caught. Record where to and from, and let it run (The whole page, until `RearmTrapPeriodically` comes by)
bool OnOriginalCodeExecuted(EXCEPTION_POINTERS* info) {
    const auto eip = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);

    // A hook of ours (`jmp rel32` to outside of the original): no code of the original runs, go where it leads.
    // Still of interest: somebody got the address of the original function from somewhere (a vtable or a table of the original's data),
    // which won't be there without the original. Recorded as `VIA HOOK`.
    auto viaHook = false;
    if (const auto* const code = reinterpret_cast<const uint8_t*>(eip); code[0] == 0xE9) {
        int32_t rel{};
        memcpy(&rel, code + 1, sizeof(rel));
        if (const auto target = eip + 5 + rel; !IsOriginalCode(target)) {
            info->ContextRecord->Eip = target;
            viaHook                  = true;
        }
    }

    auto isNew = true;
    const LONG numTrapped = s_NumTrappedEntries; // (A copy: it's volatile)
    const auto n          = std::min(numTrapped, static_cast<LONG>(std::size(s_TrappedEntries)));
    for (auto i = 0; i < n; i++) {
        if (s_TrappedEntries[i] == eip) {
            isNew = false;
            break;
        }
    }
    if (isNew) {
        if (const auto idx = InterlockedIncrement(&s_NumTrappedEntries) - 1; idx < static_cast<LONG>(std::size(s_TrappedEntries))) {
            s_TrappedEntries[idx] = eip;
        }
        // At the entry of a function the return address is on the top of the stack, and the frame pointer is still the caller's
        Trace(std::format("{} 0x{:08X} called from {}", viaHook ? "VIA HOOK" : "ORIGINAL CODE", eip, DescribeAddress(*reinterpret_cast<const uintptr_t*>(info->ContextRecord->Esp))));
        if (!viaHook) {
            void*      frames[10]{};
            const auto numFrames = CaptureStackBackTrace(0, static_cast<DWORD>(std::size(frames)), frames, nullptr);
            for (auto i = 4u; i < numFrames; i++) { // (The first few are the exception dispatching)
                Trace("    " + DescribeAddress(reinterpret_cast<uintptr_t>(frames[i])));
            }
        }
    }
    if (viaHook) {
        return true; // (The page stays as it is)
    }
    if (s_IsStandalone) {
        // Nothing of the original is set up to run (no C runtime, no constructed globals): this is the end
        const auto msg = std::format(
            "The game called a function of the original executable (at 0x{:08X}), which isn't available in standalone mode.\n\nSee `{}` for who called it.",
            eip, TRACE_FILE_NAME
        );
        MessageBoxA(nullptr, msg.c_str(), "gta_reversed (standalone)", MB_ICONERROR | MB_OK);
        ExitProcess(3);
    }

    DWORD old{};
    VirtualProtect(reinterpret_cast<void*>(eip & ~uintptr_t{ 0xFFF }), 0x1000, PAGE_EXECUTE_READWRITE, &old);
    return true;
}

DWORD WINAPI RearmTrapPeriodically(void*) {
    for (;;) {
        Sleep(250);
        ProtectOriginalCode(false);
    }
}

//! Records the exceptions that usually mean the end (They may still be handled by somebody, so only the first few are recorded)
LONG CALLBACK TraceExceptions(EXCEPTION_POINTERS* info) {
    static LONG s_NumRecorded = 0;

    const auto* const rec = info->ExceptionRecord;
    if (s_IsTrappingOriginalCode && rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && rec->NumberParameters >= 2 && rec->ExceptionInformation[0] == 8 /* execute */
        && IsOriginalCode(rec->ExceptionInformation[1])) {
        if (rec->ExceptionInformation[1] != reinterpret_cast<uintptr_t>(rec->ExceptionAddress)) {
            // An instruction that started on a page that's allowed to run by now, and continues on the next one, which isn't: just let it
            DWORD old{};
            VirtualProtect(reinterpret_cast<void*>(rec->ExceptionInformation[1] & ~uintptr_t{ 0xFFF }), 0x1000, PAGE_EXECUTE_READWRITE, &old);
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        return OnOriginalCodeExecuted(info) ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
    }
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
        msg += std::format(", {} 0x{:08X}", rec->ExceptionInformation[0] == 8 ? "executing" : rec->ExceptionInformation[0] ? "writing" : "reading", rec->ExceptionInformation[1]);
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

struct StartupDataRun {
    uint32_t addr, size;
};
#include "StartupData.inc"

/*!
* Do to the original's data what its start-up code would have: the constructors of its global objects, and the initial values
* that are computed. In a standalone game that code doesn't run, and plenty relies on what it leaves behind (For example
* `gpCamColVars`, which it points at an entry of `gCamColVars`).
* The game module then constructs, on top of this, the globals that have to be its own (The ones with a virtual table).
*/
void ApplyStartupData() {
    const auto* src = STARTUP_DATA;
    for (const auto& run : STARTUP_DATA_RUNS) {
        memcpy(reinterpret_cast<void*>(run.addr), src, run.size);
        src += run.size;
    }
    Trace(std::format("Standalone: applied what the original's start-up code does to its data ({} runs, {} bytes)", std::size(STARTUP_DATA_RUNS), std::size(STARTUP_DATA)));
}

/*!
* The game's entry point calls this instead of its `WinMain`. By now the game's C runtime (and with that, its heap) is initialized,
* which is the state an ASI loader would load the ASI in. So that's what's done here.
*/
int WINAPI OnGameWinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int cmdShow) {
    if (!s_IsStandalone) { // (Otherwise it was never hooked: nothing was going to call it)
        memcpy(reinterpret_cast<void*>(GAME_WIN_MAIN), s_WinMainOriginalBytes.data(), s_WinMainOriginalBytes.size());
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(GAME_WIN_MAIN), s_WinMainOriginalBytes.size());
    }

    if (char dumpPath[MAX_PATH]{}; GetEnvironmentVariableA(ENV_DUMP_STARTUP_DATA, dumpPath, sizeof(dumpPath))) {
        // Everything of the image that isn't code, as one block (its address in the file's first 8 bytes)
        const auto* const base     = reinterpret_cast<const uint8_t*>(GAME_IMAGE_BASE);
        const auto* const nt       = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + reinterpret_cast<const IMAGE_DOS_HEADER*>(base)->e_lfanew);
        const auto* const sections = IMAGE_FIRST_SECTION(nt);
        uint32_t          begin = GAME_IMAGE_BASE + GAME_IMAGE_SIZE, end = 0;
        for (auto i = 0u; i < nt->FileHeader.NumberOfSections; i++) {
            if (!(sections[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)) {
                begin = std::min<uint32_t>(begin, GAME_IMAGE_BASE + sections[i].VirtualAddress);
                end   = std::max<uint32_t>(end, GAME_IMAGE_BASE + sections[i].VirtualAddress + std::max(sections[i].Misc.VirtualSize, sections[i].SizeOfRawData));
            }
        }
        end = std::min<uint32_t>(end, GAME_IMAGE_BASE + GAME_IMAGE_SIZE);
        if (const auto h = CreateFileA(dumpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr); h != INVALID_HANDLE_VALUE) {
            const uint32_t header[2]{ begin, end };
            DWORD          written{};
            WriteFile(h, header, sizeof(header), &written, nullptr);
            WriteFile(h, reinterpret_cast<const void*>(begin), end - begin, &written, nullptr);
            CloseHandle(h);
            Trace(std::format("Saved the original's data [0x{:08X}, 0x{:08X}) as its start-up code left it to `{}`", begin, end, dumpPath));
        } else {
            Trace(std::format("Couldn't write `{}` (error {})", dumpPath, GetLastError()));
        }
    }

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
    auto winMain = reinterpret_cast<int(WINAPI*)(HINSTANCE, HINSTANCE, LPSTR, int)>(GAME_WIN_MAIN);

    if (char buf[8]{}; s_IsStandalone || GetEnvironmentVariableA(ENV_TRAP_ORIGINAL_CODE, buf, sizeof(buf))) {
        // Go to ours directly (It's a `jmp rel32` there now), the original is about to be off limits
        if (const auto* const fn = reinterpret_cast<const uint8_t*>(GAME_WIN_MAIN); fn[0] == 0xE9) {
            int32_t rel{};
            memcpy(&rel, fn + 1, sizeof(rel));
            winMain = reinterpret_cast<decltype(winMain)>(GAME_WIN_MAIN + 5 + rel);
        } else if (s_IsStandalone) {
            LoaderFail("The module didn't put its own `WinMain` in place of the original's, there's nothing to run");
        }

        const auto* const base     = reinterpret_cast<const uint8_t*>(GAME_IMAGE_BASE);
        const auto* const nt       = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + reinterpret_cast<const IMAGE_DOS_HEADER*>(base)->e_lfanew);
        const auto* const sections = IMAGE_FIRST_SECTION(nt);
        for (auto i = 0u; i < nt->FileHeader.NumberOfSections && s_NumOriginalCodeRanges < std::size(s_OriginalCode); i++) {
            if (sections[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) {
                const auto begin = GAME_IMAGE_BASE + sections[i].VirtualAddress;
                s_OriginalCode[s_NumOriginalCodeRanges++] = { begin, begin + ((std::max(sections[i].Misc.VirtualSize, sections[i].SizeOfRawData) + 0xFFF) & ~0xFFFu) };
                Trace(std::format("Original code: section `{:.8}` [0x{:08X}, 0x{:08X})", reinterpret_cast<const char*>(sections[i].Name), begin, s_OriginalCode[s_NumOriginalCodeRanges - 1].end));
            }
        }

        // Data isn't executable only if this is on (The launcher isn't linked as compatible with it, the original game isn't)
        if (!SetProcessDEPPolicy(PROCESS_DEP_ENABLE)) {
            Trace(std::format("Couldn't turn on data execution prevention (error {}), calls into the original's code can't be trapped", GetLastError()));
            if (s_IsStandalone) {
                LoaderFail("Data execution prevention couldn't be turned on, which standalone mode needs to keep the original's code from running");
            }
        } else {
            s_IsTrappingOriginalCode = true;
            ProtectOriginalCode(false);
            if (s_IsStandalone) {
                Trace("Standalone: the original's code is off limits from here on, a call into it ends the game (`ORIGINAL CODE` line)");
            } else {
                CloseHandle(CreateThread(nullptr, 0, &RearmTrapPeriodically, nullptr, 0, nullptr));
                Trace("Trapping calls into the original's code from here on (`ORIGINAL CODE` lines)");
            }
        }
    }

    const auto ret = winMain(instance, prevInstance, cmdLine, cmdShow);
    Trace(std::format("`WinMain` returned {} ({} functions of the original's code were called)", ret, static_cast<long>(s_NumTrappedEntries)));
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
    if (char buf[8]{}; GetEnvironmentVariableA(ENV_STANDALONE, buf, sizeof(buf))) {
        // Only the data of the original is going to be used. Its imports aren't resolved (so none of its DLLs, and no ASI loader,
        // get loaded), and its entry point isn't run: no C runtime of its own, and its global objects aren't constructed.
        s_IsStandalone = true;
        Trace("Standalone: original executable mapped for its data only, starting our `WinMain` directly");
        ApplyStartupData();
        ExitProcess(OnGameWinMain(GetModuleHandleA(nullptr), nullptr, GetCommandLineA(), SW_SHOWDEFAULT));
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
