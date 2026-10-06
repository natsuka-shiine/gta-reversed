#include "StdInc.h"

#include "TaskSimpleCarSetPedOut.h"
#include "TaskSimplePlayerOnFoot.h"
#include "TaskSimpleStandStill.h"
#include "TaskComplexWander.h"
#include "CarEnterExit.h"
#include "Garages.h"
#include "Bike.h"

void CTaskSimpleCarSetPedOut::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleCarSetPedOut, 0x86EEB8, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(PositionPedOutOfCollision, 0x6479B0);

    RH_ScopedVMTInstall(Clone, 0x649F50);
    RH_ScopedVMTInstall(ProcessPed, 0x647D10);
}

// 0x6478B0
CTaskSimpleCarSetPedOut::CTaskSimpleCarSetPedOut(CVehicle* targetVehicle, eTargetDoor nTargetDoor, bool bSwitchOffEngine, bool warpingOutOfCar) :
    m_nTargetDoor{ nTargetDoor },
    m_pTargetVehicle{ targetVehicle },
    m_bSwitchOffEngine{ bSwitchOffEngine },
    m_bWarpingOutOfCar{ warpingOutOfCar }
{
    CEntity::SafeRegisterRef(m_pTargetVehicle);
}

CTaskSimpleCarSetPedOut::~CTaskSimpleCarSetPedOut() {
    CEntity::SafeCleanUpRef(m_pTargetVehicle);
}

// 0x6479B0
void CTaskSimpleCarSetPedOut::PositionPedOutOfCollision(CPed* ped, CVehicle* veh, int32 door) {
    if (!veh) {
        veh = ped->m_pVehicle;
        if (!veh) {
            return;
        }
    }

    const auto heading = veh->GetHeading();
    ped->m_fAimingRotation  = heading;
    ped->m_fCurrentRotation = heading;
    ped->SetHeading(heading);

    if (veh->m_nVehicleType != VEHICLE_TYPE_BOAT) {
        CWorld::pIgnoreEntity = veh;

        // Check if the ped is inside something (other than the thing the vehicle is attached to)
        auto       isColliding = false;
        const auto TestSphere  = [&](float zOffset) {
            const auto& pedPos = ped->GetPosition();
            const auto  hit    = CWorld::TestSphereAgainstWorld(CVector{ pedPos.x, pedPos.y, pedPos.z + zOffset }, 0.4f, veh, true, true, false, false, false, false);
            if (hit && hit != veh->m_pAttachedTo) {
                isColliding = true;
            }
        };
        TestSphere(-0.2f);
        TestSphere(+0.2f);

        if (!CWorld::GetIsLineOfSightClear(veh->GetPosition(), ped->GetPosition(), true, false, false, true, false, false, false) || isColliding) {
            ped->PositionPedOutOfCollision(door, veh, true);
        }

        CWorld::pIgnoreEntity = nullptr;
        return;
    }

    // Boats
    auto& pedMat = *ped->m_matrix; // NOTE: Original code accesses the matrix directly too (no null check)

    if (veh->m_pHandlingData->m_bSitInBoat) {
        pedMat.GetPosition() += veh->m_matrix->GetUp() * 0.5f;
    }

    // 0x647B60 - Ped isn't colliding with anything, so just leave them standing on the boat
    const auto StandOnBoat = [&] {
        ped->m_vecMoveSpeed = veh->m_vecMoveSpeed * 0.9f;
        ped->m_vecMoveSpeed.z -= 0.1f;
        ped->bIsStanding = true;
        if (!ped->m_standingOnEntity) {
            ped->m_standingOnEntity = veh;
            veh->RegisterReference(&ped->m_standingOnEntity);
        }
    };

    if (!ped->TestCollision(false)) {
        StandOnBoat();
        return;
    }

    if (!veh->vehicleFlags.bIsDrowning) { // 0x647ADB - `*(veh + 0x42B) & 0x40`
        pedMat.GetPosition() -= pedMat.GetForward() * 0.3f;
        if (!ped->TestCollision(false)) {
            StandOnBoat();
            return;
        }
        pedMat.GetPosition() += pedMat.GetForward() * 0.3f;
        ped->PositionPedOutOfCollision(door, veh, true);
    } else {
        pedMat.GetPosition().z -= 0.3f;
        if (ped->TestCollision(false)) {
            pedMat.GetPosition().z += 0.3f;
            ped->PositionPedOutOfCollision(door, veh, true);
        }
    }
}

// 0x649F50
CTask* CTaskSimpleCarSetPedOut::Clone() const {
    const auto task = new CTaskSimpleCarSetPedOut{ m_pTargetVehicle, m_nTargetDoor, m_bSwitchOffEngine };
    task->m_bWarpingOutOfCar     = m_bWarpingOutOfCar;
    task->m_bFallingOutOfCar     = m_bFallingOutOfCar;
    task->m_bKnockedOffBike      = m_bKnockedOffBike;
    task->m_nDoorFlagsToClear    = m_nDoorFlagsToClear;
    task->m_nNumGettingInToClear = m_nNumGettingInToClear;
    return task;
}

// 0x647D10
bool CTaskSimpleCarSetPedOut::ProcessPed(CPed* ped) {
    ped->bInVehicle = false;
    ped->SetUsesCollision(true);
    ped->UpdateStatLeavingVehicle();

    if (!m_bKnockedOffBike) {
        PositionPedOutOfCollision(ped, nullptr, m_nTargetDoor);
    }

    CCarEnterExit::RemoveCarSitAnim(ped);
    ped->RestartNonPartialAnims();

    if (!m_bKnockedOffBike && !m_bFallingOutOfCar && m_pTargetVehicle->m_nVehicleSubType != VEHICLE_TYPE_BOAT) {
        ped->m_vecMoveSpeed = CVector{ 0.0f, 0.0f, 0.0f };
    }

    if (const auto veh = ped->m_pVehicle) {
        if (m_nDoorFlagsToClear) {
            m_pTargetVehicle->ClearGettingOutFlags(m_nDoorFlagsToClear);
        }
        if (m_nNumGettingInToClear) {
            m_pTargetVehicle->m_nNumGettingIn -= m_nNumGettingInToClear;
        }

        if (veh->m_pDriver == ped) {
            veh->RemoveDriver(!m_bSwitchOffEngine);
            veh->SetStatus(STATUS_ABANDONED);
            if (veh->m_nDoorLock == CARLOCK_COP_CAR) {
                veh->m_nDoorLock = CARLOCK_UNLOCKED;
            }
            if (ped->m_nPedType == PED_TYPE_COP && veh->IsLawEnforcementVehicle()) {
                veh->ChangeLawEnforcerState(false);
            }
        } else {
            veh->RemovePassenger(ped);
        }

        auto pedPos = ped->GetPosition();
        if (CGarages::IsPointWithinAnyGarage(pedPos)) {
            veh->m_nOverrideLights      = NO_CAR_LIGHT_OVERRIDE;
            veh->vehicleFlags.bLightsOn = false;
        }
    }

    if (!m_bFallingOutOfCar && !m_bKnockedOffBike) {
        // Put (almost) stationary bikes on their side-stand
        if (const auto veh = ped->m_pVehicle; veh && veh->m_nVehicleType == VEHICLE_TYPE_BIKE) {
            if (std::fabs(veh->m_vecMoveSpeed.x) < 0.1f && std::fabs(veh->m_vecMoveSpeed.y) < 0.1f) {
                static_cast<CBike*>(veh)->bikeFlags.bOnSideStand = true;
            }
        }
    }

    // Set default task
    {
        CTask* task;
        if (ped->IsPlayer()) {
            task = new CTaskSimplePlayerOnFoot{};
        } else if (ped->GetCreatedBy() == PED_MISSION) {
            task = new CTaskSimpleStandStill{ 999'999, true, false, 8.0f };
        } else {
            task = CTaskComplexWander::GetWanderTaskByPedType(ped);
        }
        ped->GetTaskManager().SetTask(task, TASK_PRIMARY_DEFAULT, false);
    }

    ped->ReplaceWeaponWhenExitingVehicle();
    ped->bDonePositionOutOfCollision = true;
    ped->m_nPedState = PEDSTATE_IDLE; // Android calls `SetPedState(PEDSTATE_IDLE)` here

    if (m_pTargetVehicle && m_pTargetVehicle->physicalFlags.bTouchingWater) {
        ped->physicalFlags.bTouchingWater = true;
    }

    return true;
}
