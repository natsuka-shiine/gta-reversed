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

/*!
* Whether the code of the original executable can be called. Not if the inverted loader runs the game standalone
* (the `GTA_REVERSED_STANDALONE` environment variable): the original is only there for its data then.
*/
bool IsOriginalCodeAvailable();
};