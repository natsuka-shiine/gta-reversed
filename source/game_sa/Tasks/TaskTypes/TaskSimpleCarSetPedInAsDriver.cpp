#include "StdInc.h"

#include "TaskSimpleCarSetPedInAsDriver.h"
#include "TaskSimpleCarSetPedOut.h"
#include "TaskSimpleCarDrive.h"
#include "CarEnterExit.h"
#include "Crime.h"
#include "PedGroups.h"
#include "EventGroupEvent.h"
#include "EventLeaderEnteredCarAsDriver.h"
#include "EventCopCarBeingStolen.h"

void CTaskSimpleCarSetPedInAsDriver::InjectHooks() {
    RH_ScopedClass(CTaskSimpleCarSetPedInAsDriver);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(ProcessPed, 0x64B950);
}

// 0x6470E0
CTaskSimpleCarSetPedInAsDriver::CTaskSimpleCarSetPedInAsDriver(CVehicle* targetVehicle, CTaskUtilityLineUpPedWithCar* utility) : CTaskSimple() {
    m_bIsFinished = 0;
    m_pAnim = 0;
    m_pTargetVehicle = targetVehicle;
    m_pUtility = utility;
    m_bWarpingInToCar = 0;
    m_nDoorFlagsToClear = 0;
    m_nNumGettingInToClear = 0;
    CEntity::SafeRegisterRef(m_pTargetVehicle);
}

CTaskSimpleCarSetPedInAsDriver::CTaskSimpleCarSetPedInAsDriver(CVehicle* targetVehicle, bool warpingInToCar, CTaskUtilityLineUpPedWithCar* utility) : // NOTSA
    CTaskSimpleCarSetPedInAsDriver{ targetVehicle, utility }
{
    m_bWarpingInToCar = warpingInToCar;
}

CTaskSimpleCarSetPedInAsDriver::~CTaskSimpleCarSetPedInAsDriver() {
    CEntity::SafeCleanUpRef(m_pTargetVehicle);
}

// 0x649E00
CTask* CTaskSimpleCarSetPedInAsDriver::Clone() const {
    auto task = new CTaskSimpleCarSetPedInAsDriver(m_pTargetVehicle, m_pUtility);
    task->m_bWarpingInToCar = m_bWarpingInToCar;
    task->m_nDoorFlagsToClear = m_nDoorFlagsToClear;
    task->m_nNumGettingInToClear = m_nNumGettingInToClear;
    return task;
}

// 0x64B950
bool CTaskSimpleCarSetPedInAsDriver::ProcessPed(CPed* ped) {
    // NOTE: Unlike `ChangeEntityReference` the original code doesn't check if `m_pTargetVehicle` is null
    if (ped->m_pVehicle) {
        ped->m_pVehicle->CleanUpOldReference(reinterpret_cast<CEntity**>(&ped->m_pVehicle));
    }
    ped->m_pVehicle = m_pTargetVehicle;
    m_pTargetVehicle->RegisterReference(reinterpret_cast<CEntity**>(&ped->m_pVehicle));

    // If there's somebody (else) driving already kick them out (unless they're already leaving)
    if (auto* const driver = m_pTargetVehicle->m_pDriver; driver != ped && driver) {
        auto& driverTaskMgr = driver->GetTaskManager();
        if (   !driverTaskMgr.FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR)
            && !driverTaskMgr.FindActiveTaskByType(TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT_AND_STAND_UP)
            && !driverTaskMgr.FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR_AND_DIE)
            && !driverTaskMgr.FindActiveTaskByType(TASK_SIMPLE_BIKE_JACKED)
        ) {
            CTaskSimpleCarSetPedOut setPedOut{
                m_pTargetVehicle,
                (eTargetDoor)CCarEnterExit::ComputeTargetDoorToExit(m_pTargetVehicle, m_pTargetVehicle->m_pDriver),
                true
            };
            setPedOut.m_bWarpingOutOfCar = true;
            setPedOut.ProcessPed(m_pTargetVehicle->m_pDriver);
        }
    }

    ped->m_fAimingRotation = ped->m_fCurrentRotation;

    if (m_bWarpingInToCar) {
        ped->GetPosition() = m_pTargetVehicle->GetPosition(); // NOTE: Not using `SetPosn`, the original code writes the position directly too
    }

    if (auto* const group = CPedGroups::GetPedsGroup(ped)) {
        if (group->GetMembership().IsLeader(ped) && !group->GetIntelligence().GetCurrentEvent()) {
            CEventGroupEvent groupEvent{ ped, new CEventLeaderEnteredCarAsDriver{ m_pTargetVehicle } };
            group->GetIntelligence().AddEvent(&groupEvent);
        }
    }

    m_pTargetVehicle->SetDriver(ped);
    ped->bInVehicle = true;

    if (ped->IsPlayer()) {
        ped->GetPlayerData()->m_bPlayersGangActive = true;
        ped->AsPlayer()->ClearAdrenaline();
    }

    if (ped->IsPlayer() && !m_pTargetVehicle->vehicleFlags.bHasBeenOwnedByPlayer) {
        m_pTargetVehicle->vehicleFlags.bHasBeenOwnedByPlayer = true;
        CCrime::ReportCrime(CRIME_CAR_STEAL, m_pTargetVehicle, FindPlayerPed());
    }

    ped->UpdateStatEnteringVehicle();
    ped->SetMoveState(PEDMOVE_NONE);
    ped->SetMoveAnim();
    ped->m_bUsesCollision = false;

    if (m_pTargetVehicle->m_nAlarmState == (uint16)-1) {
        m_pTargetVehicle->m_nAlarmState = 15'000;
    }

    m_pTargetVehicle->vehicleFlags.bEngineOn = !m_pTargetVehicle->vehicleFlags.bEngineBroken;

    ped->SetPedState(PEDSTATE_DRIVING);

    if (ped->IsPlayer()) {
        m_pTargetVehicle->SetStatus(STATUS_PLAYER);
    } else if (m_pTargetVehicle->GetStatus() != STATUS_SIMPLE || ped->IsCreatedByMission()) {
        m_pTargetVehicle->SetStatus(
            m_pTargetVehicle->m_nVehicleSubType == VEHICLE_TYPE_TRAIN
                ? STATUS_TRAIN_NOT_MOVING
                : STATUS_PHYSICS
        );
    }

    if (m_nDoorFlagsToClear) {
        m_pTargetVehicle->ClearGettingInFlags(m_nDoorFlagsToClear);
    }
    if (m_nNumGettingInToClear) {
        m_pTargetVehicle->m_nNumGettingIn -= m_nNumGettingInToClear;
    }

    CCarEnterExit::RemoveGetInAnims(ped);
    CCarEnterExit::AddInCarAnim(m_pTargetVehicle, ped, true);
    ped->RemoveWeaponWhenEnteringVehicle(0);

    ped->bRenderPedInCar = !m_pTargetVehicle->vehicleFlags.bIsBus || m_pTargetVehicle->m_nModelIndex == MODEL_BUS;

    ped->GetTaskManager().SetTask(
        new CTaskSimpleCarDrive{ m_pTargetVehicle, m_bWarpingInToCar ? nullptr : m_pUtility, false },
        TASK_PRIMARY_DEFAULT
    );

    if (ped->IsPlayer() && m_pTargetVehicle && m_pTargetVehicle->IsLawEnforcementVehicle()) {
        CEventCopCarBeingStolen event{ ped, m_pTargetVehicle };
        GetEventGlobalGroup()->Add(&event, false);
    }

    return true;
}
