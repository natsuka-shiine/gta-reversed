#include <StdInc.h>

#include "Commands.hpp"
#include <CommandParser/Parser.hpp>

#include <Pickups.h>
#include <World.h>

using namespace notsa::script;

/*!
* Various pickup commands
*/

namespace {
// The original also looks up the pickup currently stored in the output variable
// (`CollectNextParameterWithoutIncreasingPC` + `CPickups::GetActualPickupIndex`) before
// creating a new one, but the result is unused, so the handlers below don't do it.

int32 GeneratePickup(script::Model model, int32 pickupType, uint32 ammo, CVector pos) {
    if (pos.z <= -100.f) {
        pos.z = CWorld::FindGroundZForCoord(pos.x, pos.y) + 0.5f;
    }
    const auto ref = CPickups::GenerateNewOne(
        pos,
        static_cast<uint32>(static_cast<eModelID>(model)),
        static_cast<ePickupType>(static_cast<uint8>(pickupType)), // Only the low byte is passed
        ammo,
        0u,
        false,
        nullptr
    );
    return ref.num;
}

// COMMAND_CREATE_PICKUP - 0x47E57A
int32 CreatePickup(script::Model model, int32 pickupType, CVector pos) {
    return GeneratePickup(model, pickupType, 0u, pos);
}

// COMMAND_CREATE_PICKUP_WITH_AMMO - 0x481678
int32 CreatePickupWithAmmo(script::Model model, int32 pickupType, int32 ammo, CVector pos) {
    return GeneratePickup(model, pickupType, static_cast<uint32>(ammo), pos);
}

// COMMAND_REMOVE_PICKUP - 0x47E6AB
void RemovePickup(int32 handle) {
    CPickups::RemovePickUp(tPickupReference{ handle });
}
};

void notsa::script::commands::pickup::RegisterHandlers() {
    REGISTER_COMMAND_HANDLER_BEGIN("Pickup");

    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_PICKUP, CreatePickup);
    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_PICKUP_WITH_AMMO, CreatePickupWithAmmo);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_PICKUP, RemovePickup);
}
