#include "StdInc.h"

#include "StaticData.h"
#include "dllmain.h"

/*
* Format of the config file (`CONFIG_FILE_NAME`, next to the game executable):
*
*   # Comment
*   own 0x8CD4F4 0x8CD782 ModelIndices    <- Own the range [begin, end) as well (copied from the original), the name is only used in messages
*   clear                                  <- Forget all ranges so far (including the generated ones)
*   nofill                                 <- Don't overwrite the original location of the owned ranges
*   map                                    <- Write `static_data_map.txt`: the address and size of every global declared by `StaticRef`
*/

namespace notsa::StaticData {
#if defined(NOTSA_OWN_STATIC_DATA) && !defined(NOTSA_STANDALONE)
namespace {
constexpr uint8 FILL_BYTE = 0xCD;

struct Range {
    uintptr      begin, end; //!< [begin, end) in the original executable
    char         name[48];
    uint8*       storage;    //!< Where it lives now
    bool         isGenerated; //!< Has initial values of its own (`init`), otherwise it's copied from the original location
    const uint8* init;        //!< Initial values, everything after `initSize` is zero
    uint32       initSize;
};

//! Ranges that are always owned, with their initial values. See `tools/static-data/generate.py`
struct GeneratedRange {
    uintptr      begin, end;
    const char*  name;
    const uint8* init;
    uint32       initSize;
};
#include "StaticDataInit.inc"

// NOTE: Everything here must be constant-initialized (See `Resolve`), and nothing may allocate from the heap
constexpr auto MAX_RANGES      = 1024u;
constexpr auto MAX_MAP_ENTRIES = 16'384u;

Range    s_Ranges[MAX_RANGES]{};
uint32   s_NumRanges{};
struct MapEntry {
    uintptr addr;
    uint32  size;
};
MapEntry s_Map[MAX_MAP_ENTRIES]{}; //!< Every global resolved so far
uint32   s_NumMapEntries{};
bool     s_IsConfigLoaded{};
bool     s_DoFill{ true };
bool     s_DoWriteMap{};
bool     s_IsConfigFileFound{};
uint32   s_NumInitMismatches{};
char     s_ConfigPath[MAX_PATH]{};
uint32   s_NumViolationsReported{};

//! Messages from before logging was available
char   s_EarlyLog[8192]{};
uint32 s_EarlyLogLen{};

void EarlyLog(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const auto n = vsnprintf(s_EarlyLog + s_EarlyLogLen, sizeof(s_EarlyLog) - s_EarlyLogLen, fmt, args);
    va_end(args);
    if (n > 0) {
        s_EarlyLogLen = std::min<uint32>(s_EarlyLogLen + n + 1, sizeof(s_EarlyLog) - 1); // Messages are separated by a `\0`
    }
}

void AddRange(uintptr begin, uintptr end, const char* name, bool isGenerated = false, const uint8* init = nullptr, uint32 initSize = 0) {
    if (begin >= end || begin < 0x401000 || end > 0xCB1000) {
        EarlyLog("Invalid range [%#x, %#x) `%s`", begin, end, name);
        return;
    }
    for (auto i = 0u; i < s_NumRanges; i++) {
        if (begin < s_Ranges[i].end && s_Ranges[i].begin < end) {
            EarlyLog("Range [%#x, %#x) `%s` overlaps `%s`, ignored", begin, end, name, s_Ranges[i].name);
            return;
        }
    }
    if (s_NumRanges == MAX_RANGES) {
        EarlyLog("Too many ranges, `%s` ignored", name);
        return;
    }
    auto& r = s_Ranges[s_NumRanges++];
    r.begin       = begin;
    r.end         = end;
    r.isGenerated = isGenerated;
    r.init        = init;
    r.initSize    = initSize;
    strncpy_s(r.name, name, _TRUNCATE);
}

//! Path of a file next to the game executable (The current directory can't be relied on: some ASI loaders change it while loading us)
const char* GetPathNextToExecutable(const char* fileName, char (&out)[MAX_PATH]) {
    const auto len = GetModuleFileNameA(nullptr, out, MAX_PATH);
    auto       end = len && len < MAX_PATH ? out + len : out;
    while (end != out && end[-1] != '\\' && end[-1] != '/') {
        end--;
    }
    *end = '\0';
    strcat_s(out, fileName);
    return out;
}

void LoadConfigFile() {
    GetPathNextToExecutable(CONFIG_FILE_NAME, s_ConfigPath);
    const auto h = CreateFileA(s_ConfigPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        return;
    }
    s_IsConfigFileFound = true;
    static char text[256 * 1024];
    DWORD       len{};
    ReadFile(h, text, sizeof(text) - 1, &len, nullptr);
    CloseHandle(h);
    text[len] = '\0';

    char* ctx{};
    for (auto* line = strtok_s(text, "\r\n", &ctx); line; line = strtok_s(nullptr, "\r\n", &ctx)) {
        while (*line == ' ' || *line == '\t') {
            line++;
        }
        unsigned begin{}, end{};
        char     name[48]{ "?" };
        if (!*line || *line == '#') {
            continue;
        } else if (sscanf_s(line, "own %x %x %47s", &begin, &end, name, static_cast<unsigned>(sizeof(name))) >= 2) {
            AddRange(begin, end, name);
        } else if (!strncmp(line, "clear", 5)) {
            s_NumRanges = 0;
        } else if (!strncmp(line, "nofill", 6)) {
            s_DoFill = false;
        } else if (!strncmp(line, "map", 3)) {
            s_DoWriteMap = true;
        } else {
            EarlyLog("Config: Can't make sense of the line `%s`", line);
        }
    }
}

const Range* FindRange(uintptr addr) {
    for (auto i = 0u; i < s_NumRanges; i++) {
        if (addr >= s_Ranges[i].begin && addr < s_Ranges[i].end) {
            return &s_Ranges[i];
        }
    }
    return nullptr;
}

//! Decide what's owned, and move it. Happens on the first use of any global, which is before any game code of ours runs.
void LoadConfig() {
    s_IsConfigLoaded = true;

    // Another build of this module is going to run the game: own nothing, and above all don't overwrite the original
    if (notsa::IsAnotherBuildWanted()) {
        return;
    }

    for (const auto& r : GENERATED_RANGES) {
        AddRange(r.begin, r.end, r.name, true, r.init, r.initSize);
    }
    LoadConfigFile();

    // One block for everything. Each range keeps the alignment it had (relative to 16 bytes).
    size_t total{};
    const auto Place = [&total](const Range& r) {
        const auto offset = ((total + 15) & ~size_t{ 15 }) + (r.begin & 15);
        total = offset + (r.end - r.begin);
        return offset;
    };
    for (auto i = 0u; i < s_NumRanges; i++) {
        Place(s_Ranges[i]);
    }
    auto* const block = static_cast<uint8*>(VirtualAlloc(nullptr, total ? total : 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)); // Zero-filled
    total = 0;

    for (auto i = 0u; i < s_NumRanges; i++) {
        auto&       r        = s_Ranges[i];
        const auto  size     = r.end - r.begin;
        auto* const original = reinterpret_cast<uint8*>(r.begin);

        r.storage = block + Place(r);
        if (r.isGenerated) {
            if (r.init) {
                memcpy(r.storage, r.init, std::min<size_t>(r.initSize, size));
            }

            // As long as the original is around, check that the generated values are what it has at this point.
            // If not, something has changed them already (or the generator is wrong), go with what's there.
            if (memcmp(r.storage, original, size) != 0) {
                auto k = 0u;
                while (r.storage[k] == original[k]) {
                    k++;
                }
                EarlyLog("The initial values of range `%s` differ from the original at %#x (ours: %#04x, original: %#04x), using the original's", r.name, r.begin + k, r.storage[k], original[k]);
                s_NumInitMismatches++;
                memcpy(r.storage, original, size);
            }
        } else {
            memcpy(r.storage, original, size);
        }

        if (s_DoFill) {
            DWORD oldProtect{};
            VirtualProtect(original, size, PAGE_READWRITE, &oldProtect);
            memset(original, FILL_BYTE, size);
        }
    }
}
};

void* Resolve(uintptr addr, size_t size) {
    if (!s_IsConfigLoaded) {
        LoadConfig();
    }

    if (s_NumMapEntries < MAX_MAP_ENTRIES) {
        s_Map[s_NumMapEntries++] = { addr, static_cast<uint32>(size) };
    }

    const auto* const r = FindRange(addr);
    if (!r) {
        return reinterpret_cast<void*>(addr);
    }
    if (size == 0) {
        EarlyLog("The global at %#x is in the owned range `%s`, but its size isn't known, so it can't be checked that the range covers it", addr, r->name);
    } else if (addr + size > r->end) {
        EarlyLog("The global at [%#x, %#x) starts in the owned range `%s`, but ends after it (at %#x). The range is wrong!", addr, addr + size, r->name, r->end);
    }
    return r->storage + (addr - r->begin);
}

void Init() {
    if (!s_IsConfigLoaded) {
        LoadConfig();
    }

    for (const auto* msg = s_EarlyLog; msg < s_EarlyLog + s_EarlyLogLen; msg += strlen(msg) + 1) {
        NOTSA_LOG_ERR("StaticData: {}", msg);
    }

    NOTSA_LOG_INFO("StaticData: Config file `{}` {}", s_ConfigPath, s_IsConfigFileFound ? "loaded" : "not found (that's fine, it's optional)");

    uint32 totalBytes{};
    for (auto i = 0u; i < s_NumRanges; i++) {
        const auto& r = s_Ranges[i];
        uint32 numGlobals{};
        for (auto k = 0u; k < s_NumMapEntries; k++) {
            numGlobals += s_Map[k].addr >= r.begin && s_Map[k].addr < r.end;
        }
        totalBytes += r.end - r.begin;
        NOTSA_LOG_DEBUG("StaticData: Range `{}` [{:#x}, {:#x}) is owned ({} bytes, {} globals declared in it so far)", r.name, r.begin, r.end, r.end - r.begin, numGlobals);
    }
    uint32 numGenerated{};
    for (auto i = 0u; i < s_NumRanges; i++) {
        numGenerated += s_Ranges[i].isGenerated;
    }
    NOTSA_LOG_INFO(
        "StaticData: {} ranges ({} bytes) of the original executable's data are owned{}. {} of them have generated initial values, {} of those didn't match the original",
        s_NumRanges, totalBytes, s_DoFill ? "" : " (original location not overwritten)", numGenerated, s_NumInitMismatches
    );

    if (s_DoWriteMap) {
        // NOTE: Globals that are only declared inside functions show up once that function has run
        std::sort(s_Map, s_Map + s_NumMapEntries, [](const MapEntry& a, const MapEntry& b) { return a.addr != b.addr ? a.addr < b.addr : a.size > b.size; });
        char mapPath[MAX_PATH];
        GetPathNextToExecutable("static_data_map.txt", mapPath);
        if (FILE* f{}; fopen_s(&f, mapPath, "w") == 0 && f) {
            fprintf(f, "# address size (0 = unknown) owned\n");
            for (auto k = 0u; k < s_NumMapEntries; k++) {
                if (k && s_Map[k].addr == s_Map[k - 1].addr && s_Map[k].size == s_Map[k - 1].size) {
                    continue;
                }
                fprintf(f, "0x%06X %u %d\n", s_Map[k].addr, s_Map[k].size, FindRange(s_Map[k].addr) != nullptr);
            }
            fclose(f);
            NOTSA_LOG_INFO("StaticData: Wrote `{}` ({} entries)", mapPath, s_NumMapEntries);
        } else {
            NOTSA_LOG_ERR("StaticData: Couldn't write `{}`", mapPath);
        }
        s_DoWriteMap = false;
    }
}

void Verify() {
    if (!s_DoFill || s_NumViolationsReported >= 64) {
        return;
    }
    for (auto i = 0u; i < s_NumRanges; i++) {
        const auto& r     = s_Ranges[i];
        auto* const begin = reinterpret_cast<uint8*>(r.begin);
        auto* const end   = reinterpret_cast<uint8*>(r.end);
        for (auto* p = begin; p != end; p++) {
            if (*p == FILL_BYTE) {
                continue;
            }
            // Report the whole run of changed bytes at once
            auto* runEnd = p + 1;
            while (runEnd != end && *runEnd != FILL_BYTE) {
                runEnd++;
            }
            NOTSA_LOG_ERR(
                "StaticData: Something wrote to the original location of owned data: [{:#x}, {:#x}) in range `{}` (first byte is now {:#04x})",
                reinterpret_cast<uintptr>(p), reinterpret_cast<uintptr>(runEnd), r.name, static_cast<uint32>(*p)
            );
            memset(p, FILL_BYTE, runEnd - p);
            p = runEnd - 1;
            if (++s_NumViolationsReported >= 64) {
                NOTSA_LOG_ERR("StaticData: Too many violations, not reporting any more");
                return;
            }
        }
    }
}
#else
void* Resolve(uintptr addr, size_t) { return reinterpret_cast<void*>(addr); }
void Init() {}
void Verify() {}
#endif
};
