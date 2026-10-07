#include <StdInc.h>

#include "Commands.hpp"
#include <CommandParser/Parser.hpp>

#include <Radar.h>
#include <World.h>

using namespace notsa::script;

/*!
* Various radar blip commands
*/

namespace {
// The original also looks up the blip currently stored in the output variable
// (`CollectNextParameterWithoutIncreasingPC` + `CRadar::GetActualBlipArrayIndex`) before
// creating a new one, but the result is unused, so the handlers below don't do it.

// 0x47CEE6 - Common tail of the entity blip commands
tBlipHandle AddBlipForEntity(eBlipType type, int32 entityHandle, uint32 arg2) {
    const auto handle = CRadar::SetEntityBlip(type, entityHandle, arg2, BLIP_DISPLAY_BOTH); // The original also passes the script's name (unused)
    CRadar::ChangeBlipScale(handle, 3);
    return handle;
}

// COMMAND_ADD_BLIP_FOR_CAR - 0x47CEBF
tBlipHandle AddBlipForCar(int32 vehicleHandle) { // The handle isn't checked
    return AddBlipForEntity(BLIP_CAR, vehicleHandle, 0);
}

// COMMAND_ADD_BLIP_FOR_CHAR - 0x47CEFD
tBlipHandle AddBlipForChar(int32 pedHandle) { // The handle isn't checked
    return AddBlipForEntity(BLIP_CHAR, pedHandle, 1);
}

// COMMAND_ADD_BLIP_FOR_OBJECT - 0x47CF25
tBlipHandle AddBlipForObject(int32 objectHandle) { // The handle isn't checked
    return AddBlipForEntity(BLIP_OBJECT, objectHandle, 6);
}

// COMMAND_ADD_BLIP_FOR_COORD - 0x47CF4E
tBlipHandle AddBlipForCoord(CRunningScript& S, CVector pos) {
    if (pos.z <= -100.f) {
        pos.z = CWorld::FindGroundZForCoord(pos.x, pos.y);
    }
    const auto handle = CRadar::SetCoordBlip(BLIP_COORD, pos, BLIP_COLOUR_REDCOPY, BLIP_DISPLAY_BOTH, S.m_szName);
    CRadar::ChangeBlipScale(handle, 3);
    return handle;
}

// COMMAND_REMOVE_BLIP - 0x47C660
void RemoveBlip(int32 handle) {
    CRadar::ClearBlip(static_cast<tBlipHandle>(handle));
}

// COMMAND_CHANGE_BLIP_SCALE - 0x47C764
void ChangeBlipScale(int32 handle, int32 size) {
    CRadar::ChangeBlipScale(static_cast<tBlipHandle>(handle), size);
}

// COMMAND_CHANGE_BLIP_DISPLAY - 0x47D00B
void ChangeBlipDisplay(int32 handle, int32 display) {
    CRadar::ChangeBlipDisplay(static_cast<tBlipHandle>(handle), static_cast<eBlipDisplay>(display));
}

// COMMAND_DOES_BLIP_EXIST - 0x46EB03
bool DoesBlipExist(int32 handle) {
    return CRadar::GetActualBlipArrayIndex(static_cast<tBlipHandle>(handle)) != -1;
}

// COMMAND_SET_BLIP_AS_FRIENDLY - 0x472845
void SetBlipAsFriendly(int32 handle, int32 friendly) {
    CRadar::SetBlipFriendly(static_cast<tBlipHandle>(handle), static_cast<uint8>(friendly) != 0); // Only the low byte is passed
}
};

void notsa::script::commands::blip::RegisterHandlers() {
    REGISTER_COMMAND_HANDLER_BEGIN("Blip");

    REGISTER_COMMAND_HANDLER(COMMAND_ADD_BLIP_FOR_CAR, AddBlipForCar);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_BLIP_FOR_CHAR, AddBlipForChar);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_BLIP_FOR_OBJECT, AddBlipForObject);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_BLIP_FOR_COORD, AddBlipForCoord);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_BLIP, RemoveBlip);
    REGISTER_COMMAND_HANDLER(COMMAND_CHANGE_BLIP_SCALE, ChangeBlipScale);
    REGISTER_COMMAND_HANDLER(COMMAND_CHANGE_BLIP_DISPLAY, ChangeBlipDisplay);
    REGISTER_COMMAND_HANDLER(COMMAND_DOES_BLIP_EXIST, DoesBlipExist);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_BLIP_AS_FRIENDLY, SetBlipAsFriendly);
}
