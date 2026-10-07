#pragma once

#include <cstdint>
#include <cstddef>
#include <array>

/*
* Ownership of the game's global variables
*
* The game's globals live in the data sections of the original executable, and are accessed by address (See `StaticRef` in `Base.h`).
* To build a standalone executable all of them have to live in our own memory instead.
*
* This is the mechanism to get there gradually, while the game stays runnable:
*  - Address ranges of the original executable's data can be "owned". An owned range is copied (as a whole, keeping its layout)
*    into memory of our own, and every `StaticRef<T, 0xADDRESS>()` whose address is in it refers to the copy instead.
*  - The original location of an owned range is then overwritten with a fill pattern (and checked regularly),
*    so anything still using it (code that wasn't reversed, the engine, etc.) shows up.
*
* Which ranges are owned is decided at startup:
*  - There's a generated list (`StaticDataInit.inc`, made by `tools/static-data/generate.py`), which also has the initial
*    values of those ranges. Those don't depend on the original executable's data anymore.
*  - For experimenting, a text file next to the game executable can add more (those are copied from the original),
*    see `CONFIG_FILE_NAME`, and `StaticData.cpp` for the format.
*
* The original lays its globals out by source file, so a subsystem's globals are (mostly) one contiguous range.
*/
namespace notsa::StaticData {
inline constexpr auto CONFIG_FILE_NAME = "gta-reversed-static-data.txt";

/*!
* Where the global at the given address of the original executable lives now. Called once per global (by `StaticRef`).
* NOTE: This runs during static initialization, so it can't rely on anything that isn't constant-initialized.
*
* @param addr Address in the original executable
* @param size Size of the global, or 0 if it isn't known (See `detail::SizeOf`)
*/
void* Resolve(std::uintptr_t addr, std::size_t size);

//! Call once logging is available (Reports what happened so far)
void Init();

//! Check that nothing has written to the original location of the owned ranges. Cheap enough to call every frame.
void Verify();

namespace detail {
/*!
* Size of a type, or 0 if it's incomplete (Globals are often declared where their type is only forward declared)
* `sizeof(std::array<Incomplete, N>)` isn't something that can be tested for (it's a hard error), so arrays are taken apart by hand.
*/
template<typename T>
struct SizeOf {
    static constexpr std::size_t Get() {
        if constexpr (requires { sizeof(T); }) {
            return sizeof(T);
        } else {
            return 0;
        }
    }
};
template<typename T, std::size_t N>
struct SizeOf<std::array<T, N>> {
    static constexpr std::size_t Get() { return SizeOf<T>::Get() * N; }
};
template<typename T, std::size_t N>
struct SizeOf<T[N]> {
    static constexpr std::size_t Get() { return SizeOf<T>::Get() * N; }
};
};
};
