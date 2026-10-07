#pragma once

#include <Windows.h>

namespace notsa {
HMODULE GetDLLHandle();

/*!
* The inverted loader can be told to use another build of this module (with the `GTA_REVERSED_ASI` environment variable).
* If that's not this one, it has to stay out of the way: it's still loaded if there's an ASI loader in the game's directory.
* @note Usable before `DllMain` (in static initializers) too.
*/
bool IsAnotherBuildWanted();
};