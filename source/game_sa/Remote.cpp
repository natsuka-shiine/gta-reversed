#include "StdInc.h"

#include "Remote.h"
#include "CarCtrl.h"
#include "MissionCleanup.h"
#include "TheScripts.h"

void CRemote::InjectHooks() {
    RH_ScopedClass(CRemote);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(GivePlayerRemoteControlledCar, 0x45AB10);
    RH_ScopedInstall(TakeRemoteControlledCarFromPlayer, 0x45AE80);
}

// 0x45AE80
void CRemote::TakeRemoteControlledCarFromPlayer(bool bCreateRemoteVehicleExplosion) {
    auto& playerInfo = CWorld::Players[CWorld::PlayerInFocus];

    if (playerInfo.m_pRemoteVehicle->m_nCreatedBy == MISSION_VEHICLE) {
        playerInfo.m_pRemoteVehicle->SetVehicleCreatedBy(RANDOM_VEHICLE);
        CTheScripts::MissionCleanUp.RemoveEntityFromList(
            GetVehiclePool()->GetRef(playerInfo.m_pRemoteVehicle),
            MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_VEHICLE
        );
    }

    playerInfo.m_pRemoteVehicle->vehicleFlags.bIsLocked   = false;
    playerInfo.m_nTimeOfRemoteVehicleExplosion            = CTimer::GetTimeInMS();
    playerInfo.m_bAfterRemoteVehicleExplosion             = true;
    playerInfo.m_bCreateRemoteVehicleExplosion            = bCreateRemoteVehicleExplosion;
    playerInfo.m_bFadeAfterRemoteVehicleExplosion         = true;
}

// 0x45AB10
void CRemote::GivePlayerRemoteControlledCar(CVector pos, float rotation, int16 modelId) {
    // NOTE: The original passes the model index as an unsigned 16-bit value
    const auto modelIndex = static_cast<uint16>(modelId);

    CVehicle* vehicle;
    if (CModelInfo::IsHeliModel(modelIndex)) {
        vehicle = new CHeli(modelIndex, MISSION_VEHICLE);
    } else if (CModelInfo::IsPlaneModel(modelIndex)) {
        vehicle = new CPlane(modelIndex, MISSION_VEHICLE);
    } else {
        vehicle = new CAutomobile(modelIndex, MISSION_VEHICLE, true);
    }

    bool foundGround{};
    pos.z = CWorld::FindGroundZFor3DCoord({ pos.x, pos.y, pos.z + 2.0f }, &foundGround, nullptr);
    pos.z += vehicle->GetDistanceFromCentreOfMassToBaseOfModel();

    vehicle->m_matrix->SetRotateZOnly(rotation);
    vehicle->SetPosn(pos);

    vehicle->SetStatus(STATUS_REMOTE_CONTROLLED);
    vehicle->vehicleFlags.bIsLocked = true;

    CCarCtrl::JoinCarWithRoadSystem(vehicle);

    vehicle->m_autoPilot.m_nCarMission      = MISSION_NONE;
    vehicle->m_autoPilot.m_nTempAction      = TEMPACT_NONE;
    vehicle->m_autoPilot.m_nCarDrivingStyle = DRIVING_STYLE_STOP_FOR_CARS;
    vehicle->m_autoPilot.m_speed            = 9.0f;
    vehicle->m_autoPilot.m_nCruiseSpeed     = 9;
    vehicle->m_autoPilot.m_nCurrentLane     = 0;
    vehicle->m_autoPilot.m_nNextLane        = 0;
    vehicle->vehicleFlags.bEngineOn         = !vehicle->vehicleFlags.bEngineBroken;

    CWorld::Add(vehicle);

    if (FindPlayerVehicle()) {
        FindPlayerVehicle()->SetStatus(STATUS_FORCED_STOP);
    }

    auto& remoteVehicle = CWorld::Players[CWorld::PlayerInFocus].m_pRemoteVehicle;
    remoteVehicle = vehicle;
    vehicle->RegisterReference(reinterpret_cast<CEntity**>(&remoteVehicle));

    TheCamera.TakeControl(vehicle, MODE_CAM_ON_A_STRING, eSwitchType::INTERPOLATION, 1);
    TheCamera.SetZoomValueCamStringScript(1);
}
