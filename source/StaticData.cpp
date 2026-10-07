#include "StdInc.h"

#include "StaticData.h"

namespace notsa::StaticData {
#if defined(NOTSA_OWN_STATIC_DATA) && !defined(NOTSA_STANDALONE)
namespace {
constexpr uint8 FILL_BYTE = 0xCD;

struct Entry {
    uintptr addr;
    uint32  size;
    void*   storage;
};

// NOTE: Everything here must be constant-initialized (See `Adopt`)
constexpr auto NUM_RANGES  = std::size(OWNED_RANGES);
constexpr auto MAX_ENTRIES = 16'384u;

Entry  s_Entries[MAX_ENTRIES]{};
uint32 s_NumEntries{};

Entry  s_Conflicts[64][2]{}; //!< Globals whose range overlaps another's (but isn't the same storage)
uint32 s_NumConflicts{};
uint32 s_NumOutOfRange{};

uint8* s_Snapshots[NUM_RANGES]{}; //!< The original bytes of each range, as they were before they got overwritten
bool   s_IsInitialized{};
uint32 s_NumViolationsReported{};

int32 FindRange(uintptr addr, size_t size) {
    for (auto i = 0u; i < NUM_RANGES; i++) {
        if (addr >= OWNED_RANGES[i].begin && addr + size <= OWNED_RANGES[i].end) {
            return static_cast<int32>(i);
        }
    }
    return -1;
}
};

void Adopt(uintptr addr, void* storage, size_t size) {
    const auto range = FindRange(addr, size);
    if (range == -1) { // Starts inside an owned range, but doesn't end in it
        s_NumOutOfRange++;
        memcpy(storage, reinterpret_cast<void*>(addr), size);
        return;
    }

    // Two declarations for the same memory (e.g. with different types) would each get their own storage, and drift apart
    for (auto i = 0u; i < s_NumEntries; i++) {
        const auto& e = s_Entries[i];
        if (addr < e.addr + e.size && e.addr < addr + size && s_NumConflicts < std::size(s_Conflicts)) {
            s_Conflicts[s_NumConflicts][0] = e;
            s_Conflicts[s_NumConflicts][1] = { addr, static_cast<uint32>(size), storage };
            s_NumConflicts++;
        }
    }

    // Once the original location has been overwritten the bytes have to come from the snapshot
    const auto* const src = s_IsInitialized
        ? s_Snapshots[range] + (addr - OWNED_RANGES[range].begin)
        : reinterpret_cast<const uint8*>(addr);
    memcpy(storage, src, size);

    if (s_NumEntries < MAX_ENTRIES) {
        s_Entries[s_NumEntries++] = { addr, static_cast<uint32>(size), storage };
    }
}

void Init() {
    assert(!s_IsInitialized);

    for (auto i = 0u; i < NUM_RANGES; i++) {
        const auto& r    = OWNED_RANGES[i];
        const auto  size = r.end - r.begin;

        s_Snapshots[i] = static_cast<uint8*>(VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        memcpy(s_Snapshots[i], reinterpret_cast<void*>(r.begin), size);

        DWORD oldProtect{};
        VirtualProtect(reinterpret_cast<void*>(r.begin), size, PAGE_READWRITE, &oldProtect);
        memset(reinterpret_cast<void*>(r.begin), FILL_BYTE, size);

        // How much of the range is covered by declarations
        uint32 numGlobals{}, numBytes{};
        for (auto k = 0u; k < s_NumEntries; k++) {
            if (s_Entries[k].addr >= r.begin && s_Entries[k].addr < r.end) {
                numGlobals++;
                numBytes += s_Entries[k].size;
            }
        }
        NOTSA_LOG_INFO("StaticData: Range `{}` [{:#x}, {:#x}) is owned: {} globals, {} of {} bytes declared", r.name, r.begin, r.end, numGlobals, numBytes, size);
    }
    s_IsInitialized = true;

    for (auto i = 0u; i < s_NumConflicts; i++) {
        const auto& [a, b] = s_Conflicts[i];
        NOTSA_LOG_ERR("StaticData: Overlapping declarations: [{:#x}, {:#x}) and [{:#x}, {:#x}). They don't share storage anymore!", a.addr, a.addr + a.size, b.addr, b.addr + b.size);
    }
    if (s_NumOutOfRange) {
        NOTSA_LOG_ERR("StaticData: {} globals start inside an owned range but don't end in it, the ranges are wrong", s_NumOutOfRange);
    }
    if (s_NumEntries == MAX_ENTRIES) {
        NOTSA_LOG_ERR("StaticData: Too many owned globals, increase `MAX_ENTRIES`");
    }
}

void Verify() {
    if (!s_IsInitialized || s_NumViolationsReported >= 64) {
        return;
    }
    for (const auto& r : OWNED_RANGES) {
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
                "StaticData: Something wrote to the original location of an owned global: [{:#x}, {:#x}) in range `{}` (first byte is now {:#04x})",
                reinterpret_cast<uintptr>(p), reinterpret_cast<uintptr>(runEnd), r.name, *p
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
void Adopt(uintptr, void*, size_t) {}
void Init() {}
void Verify() {}
#endif
};
