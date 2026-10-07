/*
* The address range of the original executable, as the first thing in this one.
*
* The game's code refers to the global variables of the original by their address (`StaticRef<T, 0xADDR>()`): until each of
* them is defined in the source instead, the data has to be where the original has it. This block is that place,
* `[0x401000, 0xCB1000)`, and `Entry.cpp` fills it before anything else runs.
*
* It ends up at the very start of the image because its section is named `.text` plainly: the linker sorts the contributions
* to a section by what follows the `$` in their name, and the compiler's own code is in `.text$mn` and the like.
* (The compiler warns that it ignores the attributes for a standard section, which is the point: C4325)
* An uninitialized section, or one with a name of its own, is placed after the code by the linker, which is too late.
*/
#include "Image.h"

#pragma warning(disable : 4325)
#pragma section(".text", execute, read)
extern "C" __declspec(allocate(".text")) unsigned char g_NotsaOriginalImage[NOTSA_IMAGE_END - NOTSA_IMAGE_BEGIN] = { 0xCC };
