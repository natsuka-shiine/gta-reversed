#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <new>
#include <type_traits>

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

/*!
* Construct the global objects the original's start-up code would have, if that hasn't run (See `detail::Construct`).
* Call it where the original has them constructed by: right before `WinMain` starts its work. Globals referred to
* before this are constructed here (not when they're first referred to, which is during our own start-up:
* far too early for a constructor of the game to run), the ones first referred to later are constructed then.
*/
void ConstructGlobals();

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

/*
* Constructing the globals
*
* The original constructs its global objects in its start-up code, before `WinMain`. Without that code having run
* (when the original executable is only there for its data), a global is what the executable's file has for it:
* its statically initialized values, or zeros. No vtable pointers, nothing a constructor would have done.
* So then we construct it, when it's first referred to. Which globals: the original doesn't say which ones it has
* constructors for, so this is by what our type of it needs:
*  - a type with virtual functions always (it's unusable without its vtable pointer)
*  - any other type with a (non-trivial) default constructor only if all of its memory is zero: then there are no
*    statically initialized values our constructor could overwrite with something else.
* Arrays are done element by element.
*/
bool ShouldConstructGlobals();
//! @return If the global at `[addr, addr + size)` is to be constructed now (not if it, or something overlapping it, was already)
bool ClaimForConstruction(std::uintptr_t addr, std::size_t size);
bool IsAllZero(const void* p, std::size_t size);
//! @return If it's too early to construct anything: `fn(p)` is called by `ConstructGlobals` then. Otherwise the caller has to.
bool DeferConstruction(void (*fn)(void*), void* p);

template<typename T>
struct Construct {
    //! If there's anything to construct at all (Not for incomplete types: those can only be left alone)
    static constexpr bool IsNeeded() {
        if constexpr (requires { sizeof(T); }) {
            return std::is_default_constructible_v<T> && !std::is_trivially_default_constructible_v<T>;
        } else {
            return false;
        }
    }
    static void Do(void* p) {
        if constexpr (IsNeeded()) {
            if (std::is_polymorphic_v<T> || IsAllZero(p, sizeof(T))) {
                ::new (p) T;
            }
        }
    }
};
template<typename T, std::size_t N>
struct Construct<std::array<T, N>> {
    static constexpr bool IsNeeded() { return Construct<T>::IsNeeded(); }
    static void Do(void* p) {
        for (std::size_t i = 0, size = SizeOf<T>::Get(); size && i < N; i++) {
            Construct<T>::Do(static_cast<unsigned char*>(p) + i * size);
        }
    }
};
template<typename T, std::size_t N>
struct Construct<T[N]> {
    static constexpr bool IsNeeded() { return Construct<T>::IsNeeded(); }
    static void Do(void* p) {
        for (std::size_t i = 0, size = SizeOf<T>::Get(); size && i < N; i++) {
            Construct<T>::Do(static_cast<unsigned char*>(p) + i * size);
        }
    }
};

//! Construct the global of type `T` at `addr` of the original (which lives at `p` now) if the original didn't. See above.
template<typename T>
void ConstructIfOriginalDidnt(std::uintptr_t addr, void* p) {
    if constexpr (Construct<std::remove_cv_t<T>>::IsNeeded()) {
        if (ShouldConstructGlobals() && ClaimForConstruction(addr, SizeOf<T>::Get())) {
            if (!DeferConstruction(&Construct<std::remove_cv_t<T>>::Do, p)) {
                Construct<std::remove_cv_t<T>>::Do(p);
            }
        }
    }
}
};
};
