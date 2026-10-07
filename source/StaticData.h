#pragma once

#include <cstdint>
#include <cstddef>

/*
* Ownership of the game's global variables
*
* The game's globals live in the data sections of the original executable, and are accessed by address (See `StaticRef` in `Base.h`).
* To build a standalone executable all of them have to live in our own memory instead.
*
* This is the mechanism to get there gradually, while the game stays runnable:
*  - A global declared with `StaticRef<T, 0xADDRESS>()` whose address is in one of the ranges below is "owned":
*    it gets storage of its own, which starts out as a copy of the original bytes.
*  - On startup the original bytes of every owned range are overwritten with a fill pattern (and checked regularly),
*    so anything still using the original location (code that wasn't reversed, a global nobody declared, etc.) shows up.
*
* The original lays its globals out by source file, so a subsystem's globals are (mostly) one contiguous range.
*/
namespace notsa::StaticData {
struct Range {
    std::uintptr_t begin, end; //!< [begin, end)
    const char*    name;
};

//! Ranges of the original executable's data that are owned by us. Add to this as subsystems are migrated.
inline constexpr Range OWNED_RANGES[]{
    { 0x8CD4F4, 0x8CD782, "ModelIndices" },
};

//! Whenever the global at the address lives in our own memory
constexpr bool IsOwned(std::uintptr_t addr) {
#ifdef NOTSA_OWN_STATIC_DATA
    for (const auto& r : OWNED_RANGES) {
        if (addr >= r.begin && addr < r.end) {
            return true;
        }
    }
#endif
    return false;
}

/*!
* Initialize the storage of an owned global from the original bytes, and take note of it. Called once per global (by `StaticRef`).
* NOTE: This runs during static initialization, so it can't rely on anything that isn't constant-initialized.
*/
void Adopt(std::uintptr_t addr, void* storage, std::size_t size);

//! Call once, before the game starts running (but after the game image is in memory)
void Init();

//! Check that nothing has written to the original location of the owned ranges. Cheap enough to call every frame.
void Verify();
};
