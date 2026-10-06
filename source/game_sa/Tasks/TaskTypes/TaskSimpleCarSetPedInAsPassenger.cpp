#include "StdInc.h"

#include "TaskSimpleCarSetPedInAsPassenger.h"
#include "TaskUtilityLineUpPedWithCar.h"
#include "TaskSimpleCarSetPedOut.h"
#include "TaskSimpleCarDrive.h"
#include "CarEnterExit.h"
#include "Crime.h"

void CTaskSimpleCarSetPedInAsPassenger::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleCarSetPedInAsPassenger, 0x86EE04, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedVMTInstall(ProcessPed, 0x64B5D0);
}

// OG constructor was at 0x646FE0
CTaskSimpleCarSetPedInAsPassenger::CTaskSimpleCarSetPedInAsPassenger(CVehicle* targetVehicle, eTargetDoor nTargetDoor, bool warpingInToCar, CTaskUtilityLineUpPedWithCar* utility) :
    m_nTargetDoor{ nTargetDoor },
    m_pTargetVehicle{ targetVehicle },
    m_pUtility{ utility },
    m_bWarpingInToCar{warpingInToCar}
{
    CEntity::SafeRegisterRef(m_pTargetVehicle);
}

// For 0x649D90
CTaskSimpleCarSetPedInAsPassenger::CTaskSimpleCarSetPedInAsPassenger(const CTaskSimpleCarSetPedInAsPassenger& o) :
    CTaskSimpleCarSetPedInAsPassenger{
        o.m_pTargetVehicle,
        o.m_nTargetDoor,
        o.m_bWarpingInToCar,
        o.m_pUtility
    }
{
    m_nNumGettingInToClear = o.m_nNumGettingInToClear;
}

// 0x647080
CTaskSimpleCarSetPedInAsPassenger::~CTaskSimpleCarSetPedInAsPassenger() {
    CEntity::SafeCleanUpRef(m_pTargetVehicle);
}

// 0x64B5D0
bool CTaskSimpleCarSetPedInAsPassenger::ProcessPed(CPed* ped) {
    // NOTE: The game calls `m_pParentTask->GetTaskType()` here if the ped is already in a vehicle, but discards the result (leftover debug code)

    CEntity::ChangeEntityReference(ped->m_pVehicle, m_pTargetVehicle);
    ped->bInVehicle        = true;
    ped->m_fAimingRotation = ped->m_fCurrentRotation;

    if (ped->IsPlayer()) {
        ped->m_pPlayerData->m_bPlayersGangActive = true;
        ped->AsPlayer()->ClearAdrenaline();
    }

    if (ped->IsPlayer() && !m_pTargetVehicle->vehicleFlags.bHasBeenOwnedByPlayer) {
        m_pTargetVehicle->vehicleFlags.bHasBeenOwnedByPlayer = true;
        CCrime::ReportCrime(CRIME_CAR_STEAL, m_pTargetVehicle, FindPlayerPed());
    }

    auto passengerIdx = -1;
    if (!m_pTargetVehicle->vehicleFlags.bIsBus) {
        passengerIdx = CCarEnterExit::ComputePassengerIndexFromCarDoor(m_pTargetVehicle, (int32)m_nTargetDoor);
        if (passengerIdx != -1) {
            // If there's somebody (else) sitting on that seat already kick them out (unless they're already leaving)
            const auto occupant = m_pTargetVehicle->m_apPassengers[passengerIdx];
            if (occupant != ped && occupant) {
                auto& occupantTaskMgr = occupant->GetTaskManager();
                if (   !occupantTaskMgr.FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR)
                    && !occupantTaskMgr.FindActiveTaskByType(TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT_AND_STAND_UP)
                    && !occupantTaskMgr.FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR_AND_DIE)
                    && !occupantTaskMgr.FindActiveTaskByType(TASK_SIMPLE_BIKE_JACKED)
                ) {
                    CTaskSimpleCarSetPedOut setPedOut{
                        m_pTargetVehicle,
                        (eTargetDoor)CCarEnterExit::ComputeTargetDoorToExit(m_pTargetVehicle, occupant),
                        true
                    };
                    setPedOut.m_bWarpingOutOfCar = true;
                    setPedOut.ProcessPed(occupant);
                }
            }
        }
    }

    if (m_bWarpingInToCar) {
        ped->SetPosn(m_pTargetVehicle->GetPosition());
    }

    if (passengerIdx == -1) {
        m_pTargetVehicle->AddPassenger(ped);
    } else {
        m_pTargetVehicle->AddPassenger(ped, (uint8)passengerIdx);
    }

    ped->UpdateStatEnteringVehicle();
    ped->SetMoveState(PEDMOVE_NONE);
    ped->SetMoveAnim();
    ped->m_bUsesCollision = false;

    if (m_pTargetVehicle->m_nAlarmState == (uint16)-1) {
        m_pTargetVehicle->m_nAlarmState = 15'000;
    }

    ped->SetPedState(PEDSTATE_DRIVING);

    if (m_nDoorFlagsToClear) {
        m_pTargetVehicle->ClearGettingInFlags(m_nDoorFlagsToClear);
    }
    if (m_nNumGettingInToClear) {
        m_pTargetVehicle->m_nNumGettingIn -= m_nNumGettingInToClear;
    }

    ped->RemoveWeaponWhenEnteringVehicle(0);
    ped->bRenderPedInCar = !m_pTargetVehicle->vehicleFlags.bIsBus;

    CCarEnterExit::RemoveGetInAnims(ped);
    CCarEnterExit::AddInCarAnim(m_pTargetVehicle, ped, false);

    if (!m_bWarpingInToCar) {
        m_pUtility->ProcessPed(ped, m_pTargetVehicle, nullptr);
    }

    ped->GetTaskManager().SetTask(new CTaskSimpleCarDrive{ m_pTargetVehicle, m_pUtility, false }, TASK_PRIMARY_DEFAULT);

    return true;
}
