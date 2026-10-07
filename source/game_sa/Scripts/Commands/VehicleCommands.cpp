#include <StdInc.h>

#include "Commands.hpp"
#include <CommandParser/Parser.hpp>
#include <cassert>

#include "PlayerInfo.h"
#include "World.h"
#include "CarGenerator.h"
#include "TheCarGenerators.h"
#include "CarCtrl.h"
#include "MissionCleanup.h"
#include "ModelInfo.h"
#include "VehicleModelInfo.h"
#include "VehicleRecording.h"
#include "CommandParser/Parser.hpp"
using namespace notsa::script;

/*!
* Various vehicle commands
*/

namespace {
void ClearHeliOrientation(CHeli& heli) {
    heli.ClearHeliOrientation();
}

void RemoveRCBuggy() {
    FindPlayerInfo().BlowUpRCBuggy(false);
}

void SetCarProofs(CVehicle& veh, bool bullet, bool fire, bool explosion, bool collision, bool melee) {
    auto& flags           = veh.physicalFlags;
    flags.bBulletProof    = bullet;
    flags.bFireProof      = fire;
    flags.bExplosionProof = explosion;
    flags.bCollisionProof = collision;
    flags.bMeleeProof     = melee;
}

/// CREATE_CAR_GENERATOR(014B)
int32 CreateCarGenerator(CVector pos, float heading, int32 modelId, int32 color1, int32 color2, int32 forceSpawn, int32 alarmChance, int32 doorLockChance, int32 minDelay, int32 maxDelay) {
    if (pos.z > -100.0f) {
        pos.z += 0.015f;
    }
    return CTheCarGenerators::CreateCarGenerator(
        pos,
        heading,
        modelId,
        (int16)color1,
        (int16)color2,
        (uint8)forceSpawn,
        (uint8)alarmChance,
        (uint8)doorLockChance,
        (uint16)minDelay,
        (uint16)maxDelay,
        0,
        true
    );
}

void SwitchCarGenerator(int32 generatorId, int32 count) {
    const auto generator = CTheCarGenerators::Get(generatorId);
    if (count) {
        generator->SwitchOn();
        if (count <= 100) {
            generator->m_nGenerateCount = count;
        }
    } else {
        generator->SwitchOff();
    }
}

void SetHasBeenOwnedForCarGenerator(int32 generatorId, bool alreadyOwned) {
    CTheCarGenerators::Get(generatorId)->bPlayerHasAlreadyOwnedCar = alreadyOwned;
}

/// CREATE_CAR(00A5)
CVehicle& CreateCar(CRunningScript& S, int32 modelId, CVector pos) {
    return *CCarCtrl::CreateCarForScript(modelId, pos, S.m_UsesMissionCleanup);
}

/// DELETE_CAR(00A6)
void DeleteCar(CRunningScript& S, int32 handle) {
    if (const auto vehicle = GetVehiclePool()->GetAtRef(handle)) {
        CWorld::Remove(vehicle);
        CWorld::RemoveReferencesToDeletedObject(vehicle);
        delete vehicle;
    }
    if (S.m_UsesMissionCleanup) {
        CTheScripts::MissionCleanUp.RemoveEntityFromList(handle, MISSION_CLEANUP_ENTITY_TYPE_VEHICLE);
    }
}

/// MARK_CAR_AS_NO_LONGER_NEEDED(01C3)
void MarkCarAsNoLongerNeeded(CRunningScript& S, int32 handle) {
    CTheScripts::CleanUpThisVehicle(GetVehiclePool()->GetAtRef(handle)); // Handles null
    if (S.m_UsesMissionCleanup) {
        CTheScripts::MissionCleanUp.RemoveEntityFromList(handle, MISSION_CLEANUP_ENTITY_TYPE_VEHICLE);
    }
}

/// IS_CAR_DEAD(0119)
bool IsCarDead(CVehicle* vehicle) {
    return !vehicle || vehicle->GetStatus() == STATUS_WRECKED || vehicle->vehicleFlags.bIsDrowning;
}

/// GET_CAR_COORDINATES(00AA)
CVector GetCarCoordinates(CVehicle& vehicle) {
    return vehicle.GetPosition();
}

/// SET_CAR_COORDINATES(00AB)
void SetCarCoordinates(CVehicle& vehicle, CVector pos) {
    CCarCtrl::SetCoordsOfScriptCar(&vehicle, pos.x, pos.y, pos.z, false, true);
}

/// GET_OFFSET_FROM_CAR_IN_WORLD_COORDS(0407)
CVector GetOffsetFromCarInWorldCoords(CVehicle& vehicle, CVector offset) {
    return vehicle.GetMatrix().TransformVector(offset) + vehicle.GetPosition();
}

/// SET_CAR_HEADING(0175)
void SetCarHeading(CVehicle& vehicle, float heading) {
    vehicle.SetHeading(DegreesToRadians(FixAngleDegrees(heading)));
    vehicle.UpdateRwMatrix();
}

/// FREEZE_CAR_POSITION(0519)
void FreezeCarPosition(CVehicle& vehicle, bool freeze) {
    auto& flags = vehicle.physicalFlags;
    flags.bDisableCollisionForce = freeze;
    flags.bCollidable            = freeze;
    flags.bDontApplySpeed        = freeze;
    if (freeze) {
        vehicle.SkipPhysics();
        vehicle.SetVelocity(CVector{});
        vehicle.m_vecTurnSpeed = CVector{};
    }
}

/// SET_CAR_HEALTH(0224)
void SetCarHealth(CVehicle& vehicle, int32 health) {
    vehicle.m_fHealth = static_cast<float>(health);
}

/// LOCK_CAR_DOORS(020A)
void LockCarDoors(CVehicle* vehicle, eCarLock lockStatus) {
    if (vehicle) {
        vehicle->m_nDoorLock = lockStatus;
    }
}

float GetCarSpeed(CVehicle& veh) {
    return veh.m_vecMoveSpeed.Magnitude() * 50.f;
}

/// SET_CAR_FORWARD_SPEED(04BA)
void SetCarForwardSpeed(CVehicle& veh, float speed) {
    veh.m_vecMoveSpeed = veh.m_matrix->GetForward() * (speed * (1.0f / 60.0f));
    if (veh.m_pHandlingData->m_bIsHeli && veh.IsAutomobile()) {
        static_cast<CAutomobile&>(veh).m_fHeliRotorSpeed = 0.22f;
    }
}

/// SET_CAR_CRUISE_SPEED(00AD)
void SetCarCruiseSpeed(CVehicle& veh, float speed) {
    // The original stores the truncated value first, and limits that
    veh.m_autoPilot.m_nCruiseSpeed = (uint8)(int32)speed;
    veh.m_autoPilot.m_nCruiseSpeed = (uint8)(int32)std::min((float)veh.m_autoPilot.m_nCruiseSpeed, veh.m_pHandlingData->m_transmissionData.m_MaxFlatVelocity * 60.0f);
}

/// SET_CAR_DENSITY_MULTIPLIER(01EB)
void SetCarDensityMultiplier(float multiplier) {
    CCarCtrl::CarDensityMultiplier = multiplier;
}

void SetCarDrivingStyle(CVehicle& veh, eCarDrivingStyle style) {
    veh.m_autoPilot.m_nCarDrivingStyle = style;
}

/// CHANGE_CAR_COLOUR(0229)
void ChangeCarColour(CVehicle& veh, int32 primary, int32 secondary) {
    veh.m_nPrimaryColor   = (uint8)primary;
    veh.m_nSecondaryColor = (uint8)secondary;
}

/// CUSTOM_PLATE_FOR_NEXT_CAR(0674)
void CustomPlateForNextCar(int32 modelId, std::string_view text) {
    char plate[9]{};
    text.copy(plate, std::min<size_t>(text.size(), 8));
    for (auto i = 0; i < 8; i++) {
        if (plate[i] == '_' || plate[i] == '\0') {
            plate[i] = ' ';
        }
    }
    plate[8] = '\0';

    const auto mi = CModelInfo::GetModelInfo(modelId);
    if (!mi || mi->GetModelType() != MODEL_INFO_VEHICLE) {
        return;
    }
    const auto vmi = mi->AsVehicleModelInfoPtr();
    if (vmi->m_pPlateMaterial) {
        vmi->SetCustomCarPlateText(plate);
    }
}

bool IsFirstCarColor(CVehicle& veh, int32 color) {
    return veh.m_nPrimaryColor == color;
}

bool IsSecondCarColor(CVehicle& veh, int32 color) {
    return veh.m_nSecondaryColor == color;
}

MultiRet<uint8, uint8> GetExtraCarColors(CVehicle& veh) {
    return { veh.m_nTertiaryColor, veh.m_nQuaternaryColor };
}

void PopCarBootUsingPhysics(CAutomobile& automobile) {
    automobile.PopBootUsingPhysics();
}

void SkipToNextAllowedStation(CTrain& train) {
    CTrain::SkipToNextAllowedStation(&train);
}

/// REQUEST_CAR_RECORDING(07C0)
void RequestCarRecording(int32 fileNumber) {
    CVehicleRecording::RequestRecordingFile(fileNumber);
}

/// HAS_CAR_RECORDING_BEEN_LOADED(07C1)
bool HasCarRecordingBeenLoaded(int32 fileNumber) {
    return CVehicleRecording::HasRecordingFileBeenLoaded(fileNumber);
}

/// START_PLAYBACK_RECORDED_CAR(05EB)
void StartPlaybackRecordedCar(CVehicle& vehicle, int32 fileNumber) {
    CVehicleRecording::StartPlaybackRecordedCar(&vehicle, fileNumber, false, false);
}

/// STOP_PLAYBACK_RECORDED_CAR(05EC)
void StopPlaybackRecordedCar(CVehicle* vehicle) {
    if (vehicle) {
        CVehicleRecording::StopPlaybackRecordedCar(vehicle);
    }
}

/// IS_PLAYBACK_GOING_ON_FOR_CAR(060E)
bool IsPlaybackGoingOnForCar(CVehicle* vehicle) {
    return CVehicleRecording::IsPlaybackGoingOnForCar(vehicle);
}

void SetRailTrackResistanceMult(float value) {
    CVehicle::ms_fRailTrackResistance = CVehicle::ms_fRailTrackResistanceDefault * (value > 0.0f ? value : 1.0f);
}

void DisableHeliAudio(CVehicle& vehicle, bool enable) {
    if (enable) {
        vehicle.m_vehicleAudio.EnableHelicoptor();
    } else {
        vehicle.m_vehicleAudio.DisableHelicoptor();
    }
}

/// LOCATE_CAR_2D(01AD)
bool LocateCar2D(CRunningScript& S, CVehicle& vehicle, CVector2D pos, CVector2D radius, bool highlightArea) {
    const auto& vehPos = vehicle.GetPosition();
    const auto  lo = pos - radius, hi = pos + radius;
    const auto  isInside = lo.x <= vehPos.x && vehPos.x <= hi.x
                        && lo.y <= vehPos.y && vehPos.y <= hi.y;
    if (highlightArea) {
        S.HighlightImportantArea(lo, hi, -100.0f);
    }
    if (CTheScripts::DbgFlag) {
        CTheScripts::DrawDebugSquare(lo.x, lo.y, hi.x, hi.y);
    }
    return isInside;
}

/// LOCATE_CAR_3D(01AF)
bool LocateCar3D(CRunningScript& S, CVehicle& vehicle, CVector pos, CVector radius, bool highlightArea) {
    const auto& vehPos = vehicle.GetPosition();
    const auto  lo = pos - radius, hi = pos + radius;
    const auto  isInside = lo.x <= vehPos.x && vehPos.x <= hi.x
                        && lo.y <= vehPos.y && vehPos.y <= hi.y
                        && lo.z <= vehPos.z && vehPos.z <= hi.z;
    if (highlightArea) {
        S.HighlightImportantArea(CVector2D{ lo.x, lo.y }, CVector2D{ hi.x, hi.y }, pos.z);
    }
    return isInside;
}

bool IsCarInAirProper(CRunningScript& S, CVehicle& vehicle) {
    for (auto* const e : vehicle.GetCollidingEntities()) {
        if (e && (e->GetIsTypeBuilding() || e->GetIsTypeVehicle())) {
            return false;
        }
    }
    return true;
}

bool IsCarStuck(CVehicle& vehicle) {
    return CTheScripts::StuckCars.HasCarBeenStuckForAWhile(GetVehiclePool()->GetRef(&vehicle));
}

void AddStuckCarCheck(CVehicle& vehicle, float stuckRadius, uint32 time) {
    CTheScripts::StuckCars.AddCarToCheck(GetVehiclePool()->GetRef(&vehicle), stuckRadius, time, false, false, false, false, 0);
}

void RemoveStuckCarCheck(CVehicle& vehicle) {
    CTheScripts::StuckCars.RemoveCarFromCheck(GetVehiclePool()->GetRef(&vehicle));
}

void AddStuckCarCheckWithWarp(CVehicle& vehicle, float stuckRadius, uint32 time, bool stuck, bool flipped, bool inWater, int8 numberOfNodesToCheck) {
    CTheScripts::StuckCars.AddCarToCheck(GetVehiclePool()->GetRef(&vehicle), stuckRadius, time, true, stuck, flipped, inWater, numberOfNodesToCheck);
}

void PlaneAttackPlayerUsingDogFight(CPlane& plane, CPlayerPed& player, float altitude) {
    if (plane.m_autoPilot.m_nCarMission != eCarMission::MISSION_PLANE_CRASH_AND_BURN && plane.m_autoPilot.m_nCarMission != eCarMission::MISSION_HELI_CRASH_AND_BURN) {
        plane.m_autoPilot.SetCarMission(eCarMission::MISSION_PLANE_DOG_FIGHT_PLAYER);
    }
    plane.m_minAltitude = altitude;
}

/// SET_CAR_ALWAYS_CREATE_SKIDS(07EE)
void SetCarAlwaysCreateSkids(CVehicle& vehicle, bool enable) {
    vehicle.vehicleFlags.bAlwaysSkidMarks = enable;
}

/// SET_CAR_AS_MISSION_CAR(0763)
void SetCarAsMissionCar(CRunningScript& S, CVehicle& vehicle) {
    if (S.m_UsesMissionCleanup && (vehicle.IsCreatedBy(eVehicleCreatedBy::RANDOM_VEHICLE) || vehicle.IsCreatedBy(eVehicleCreatedBy::PARKED_VEHICLE))) {
        vehicle.SetVehicleCreatedBy(eVehicleCreatedBy::MISSION_VEHICLE);
        CTheScripts::MissionCleanUp.AddEntityToList(vehicle);
    }
}

}

void notsa::script::commands::vehicle::RegisterHandlers() {
    REGISTER_COMMAND_HANDLER_BEGIN("Vehicle");

    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_HELI_ORIENTATION, ClearHeliOrientation);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_RC_BUGGY, RemoveRCBuggy);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_PROOFS, SetCarProofs);
    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_CAR_GENERATOR, CreateCarGenerator);
    REGISTER_COMMAND_HANDLER(COMMAND_SWITCH_CAR_GENERATOR, SwitchCarGenerator);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_HAS_BEEN_OWNED_FOR_CAR_GENERATOR, SetHasBeenOwnedForCarGenerator);
    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_CAR, CreateCar);
    REGISTER_COMMAND_HANDLER(COMMAND_DELETE_CAR, DeleteCar);
    REGISTER_COMMAND_HANDLER(COMMAND_MARK_CAR_AS_NO_LONGER_NEEDED, MarkCarAsNoLongerNeeded);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_DEAD, IsCarDead);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_COORDINATES, GetCarCoordinates);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_COORDINATES, SetCarCoordinates);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_OFFSET_FROM_CAR_IN_WORLD_COORDS, GetOffsetFromCarInWorldCoords);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_HEADING, SetCarHeading);
    REGISTER_COMMAND_HANDLER(COMMAND_FREEZE_CAR_POSITION, FreezeCarPosition);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_HEALTH, SetCarHealth);
    REGISTER_COMMAND_HANDLER(COMMAND_LOCK_CAR_DOORS, LockCarDoors);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_SPEED, GetCarSpeed);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_FORWARD_SPEED, SetCarForwardSpeed);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_CRUISE_SPEED, SetCarCruiseSpeed);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_DENSITY_MULTIPLIER, SetCarDensityMultiplier);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_DRIVING_STYLE, SetCarDrivingStyle);
    REGISTER_COMMAND_HANDLER(COMMAND_CHANGE_CAR_COLOUR, ChangeCarColour);
    REGISTER_COMMAND_HANDLER(COMMAND_CUSTOM_PLATE_FOR_NEXT_CAR, CustomPlateForNextCar);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_FIRST_CAR_COLOUR, IsFirstCarColor);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_SECOND_CAR_COLOUR, IsSecondCarColor);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_EXTRA_CAR_COLOURS, GetExtraCarColors);
    REGISTER_COMMAND_HANDLER(COMMAND_POP_CAR_BOOT_USING_PHYSICS, PopCarBootUsingPhysics);
    REGISTER_COMMAND_HANDLER(COMMAND_SKIP_TO_NEXT_ALLOWED_STATION, SkipToNextAllowedStation);
    REGISTER_COMMAND_HANDLER(COMMAND_REQUEST_CAR_RECORDING, RequestCarRecording);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_CAR_RECORDING_BEEN_LOADED, HasCarRecordingBeenLoaded);
    REGISTER_COMMAND_HANDLER(COMMAND_START_PLAYBACK_RECORDED_CAR, StartPlaybackRecordedCar);
    REGISTER_COMMAND_HANDLER(COMMAND_STOP_PLAYBACK_RECORDED_CAR, StopPlaybackRecordedCar);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_PLAYBACK_GOING_ON_FOR_CAR, IsPlaybackGoingOnForCar);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_RAILTRACK_RESISTANCE_MULT, SetRailTrackResistanceMult);
    REGISTER_COMMAND_HANDLER(COMMAND_DISABLE_HELI_AUDIO, DisableHeliAudio);
    REGISTER_COMMAND_HANDLER(COMMAND_LOCATE_CAR_2D, LocateCar2D);
    REGISTER_COMMAND_HANDLER(COMMAND_LOCATE_CAR_3D, LocateCar3D);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_IN_AIR_PROPER, IsCarInAirProper);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_STUCK, IsCarStuck);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_STUCK_CAR_CHECK, AddStuckCarCheck);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_STUCK_CAR_CHECK, RemoveStuckCarCheck);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_STUCK_CAR_CHECK_WITH_WARP, AddStuckCarCheckWithWarp);
    REGISTER_COMMAND_HANDLER(COMMAND_PLANE_ATTACK_PLAYER_USING_DOG_FIGHT, PlaneAttackPlayerUsingDogFight);

    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_ALWAYS_CREATE_SKIDS, SetCarAlwaysCreateSkids);


    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_TAXI);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SWITCH_TAXI_TIMER);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_BOAT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_GET_NUMBER_OF_CARS_COLLECTED_BY_GARAGE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_HAS_CAR_BEEN_TAKEN_TO_GARAGE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_GET_RANDOM_CAR_OF_TYPE_IN_ZONE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_HAS_RESPRAY_HAPPENED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CAR_RAM_CAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CAR_BLOCK_CAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CAR_FUNNY_SUSPENSION);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CAR_BIG_WHEELS);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SWITCH_CAR_RADIO);
}
