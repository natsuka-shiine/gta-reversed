#include "StdInc.h"

#include "TaskComplexLeaveCar.h"
#include "TaskSimpleDie.h"
#include "TaskSimplePause.h"
#include "TaskComplexGetUpAndStandStill.h"
#include "TaskComplexLeaveBoat.h"
#include "TaskSimpleCarDriveTimed.h"
#include "TaskSimpleCarCloseDoorFromOutside.h"
#include "TaskSimpleCarWaitToSlowDown.h"
#include "TaskSimpleCarWaitForDoorNotToBeInUse.h"
#include "TaskSimpleCarGetOut.h"
#include "TaskSimpleCarJumpOut.h"
#include "TaskSimpleCarSetPedOut.h"
#include "TaskSimpleCarForcePedOut.h"
#include "TaskComplexCarSlowBeDraggedOut.h"
#include "EventDamage.h"
#include "EventGroupEvent.h"
#include "EventLeaderExitedCarAsDriver.h"
#include "CarEnterExit.h"
#include "PedGroups.h"

void CTaskComplexLeaveCar::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexLeaveCar, 0x86E828, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedVMTInstall(MakeAbortable, 0x641100);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x6419F0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x641FC0);
    RH_ScopedInstall(CreateSubTask, 0x641530);
}

// 0x646CF0 - `CTaskSimpleCarWaitForDoorNotToBeInUse::CheckDoorsFreeOfPeds` (Not present in the codebase yet)
static void CheckDoorsFreeOfPeds(const CVehicle& veh, int32 door, bool& outQuit, bool& outWait) {
    uint8 doorFlag;
    switch (door) {
    case TARGET_DOOR_FRONT_RIGHT: doorFlag = 4; break;
    case TARGET_DOOR_REAR_RIGHT:  doorFlag = 8; break;
    case TARGET_DOOR_DRIVER:      doorFlag = 1; break;
    case TARGET_DOOR_REAR_LEFT:   doorFlag = 2; break;
    default:                      return;
    }
    if (veh.m_nGettingInFlags & doorFlag) {
        outQuit = true;
    }
    if (veh.m_nGettingOutFlags & doorFlag) {
        outWait = true;
    }
}

// 0x63BA00
void CTaskComplexLeaveCar::PrepareVehicleForPedExit(CPed* ped) {
    auto* const veh = m_pTargetVehicle;

    m_nDoorFlagsSet = (uint8)CCarEnterExit::ComputeDoorFlag(veh, m_nTargetDoor, true);
    veh->SetGettingOutFlags(m_nDoorFlagsSet);
    m_nNumGettingInSet = 1;
    veh->m_nNumGettingIn++;

    if (veh->m_pDriver && !veh->m_pDriver->IsPlayer()) {
        if (ped == veh->m_pDriver && m_bSensibleLeaveCar) {
            veh->m_autoPilot.m_nCruiseSpeed = 0;
            veh->m_autoPilot.m_nCarMission  = MISSION_NONE;
        }
    }
    if (ped->IsPlayer() && ped == veh->m_pDriver) {
        veh->SetStatus(STATUS_FORCED_STOP);
    }
}

// 0x63BAB0
void CTaskComplexLeaveCar::ComputeTargetDoor(CPed* ped) {
    if (m_nTargetDoor == 0) {
        m_nTargetDoor = CCarEnterExit::ComputeTargetDoorToExit(m_pTargetVehicle, ped);
    }
}

// 0x63BAE0
void CTaskComplexLeaveCar::CreateTaskUtilityLineUpPedWithCar(CPed* ped) {
    m_pTaskUtilityLineUpPedWithCar = new CTaskUtilityLineUpPedWithCar{ CVector{}, 0, 0, m_nTargetDoor };
}

// NOTSA
void CTaskComplexLeaveCar::ClearVehicleFlagsSet() {
    m_pTargetVehicle->ClearGettingOutFlags(m_nDoorFlagsSet);
    m_nDoorFlagsSet = 0;
    m_pTargetVehicle->m_nNumGettingIn -= m_nNumGettingInSet;
    m_nNumGettingInSet = 0;
}

// 0x62F1A0
CTaskComplexLeaveCar::CTaskComplexLeaveCar(CVehicle* targetVehicle, int32 nTargetDoor, int32 nDelayTime) : CTaskComplexLeaveCar(targetVehicle, nTargetDoor, nDelayTime, false, true) {
    m_bDie = true;
}

// 0x63B8C0
CTaskComplexLeaveCar::CTaskComplexLeaveCar(CVehicle* targetVehicle, int32 nTargetDoor, int32 nDelayTime, bool bSensibleLeaveCar, bool bForceGetOut) : CTaskComplex() {
    m_nTargetDoor                  = nTargetDoor;
    m_nDelayTime                   = nDelayTime;
    m_bSensibleLeaveCar            = bSensibleLeaveCar;
    m_pTargetVehicle               = targetVehicle;
    m_bForceGetOut                 = bForceGetOut;
    m_bDie                         = false;
    m_pTaskUtilityLineUpPedWithCar = nullptr;
    m_nDoorFlagsSet                = 0;
    m_nNumGettingInSet             = 0;
    m_nDieAnimID                   = ANIM_ID_KO_SHOT_FRONT_0;
    m_fDieAnimBlendDelta           = 4.0f;
    m_fDieAnimSpeed                = 1.0f;
    m_bIsInAir                     = false;

    CEntity::SafeRegisterRef(m_pTargetVehicle);
}

// 0x63B970
CTaskComplexLeaveCar::~CTaskComplexLeaveCar() {
    if (m_pTargetVehicle) {
        m_pTargetVehicle->ClearGettingOutFlags(m_nDoorFlagsSet);
        m_pTargetVehicle->m_nNumGettingIn -= m_nNumGettingInSet;
        m_pTargetVehicle->CleanUpOldReference(reinterpret_cast<CEntity**>(&m_pTargetVehicle));
    }

    delete m_pTaskUtilityLineUpPedWithCar;
}

CTaskComplexLeaveCar::CTaskComplexLeaveCar(const CTaskComplexLeaveCar& o) :
    CTaskComplexLeaveCar{ o.m_pTargetVehicle, o.m_nTargetDoor, o.m_nDelayTime, o.m_bSensibleLeaveCar, o.m_bForceGetOut }
{
}

// 0x641100
bool CTaskComplexLeaveCar::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (!m_pTargetVehicle) {
        return true;
    }

    switch (priority) {
    case ABORT_PRIORITY_IMMEDIATE: {
        m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_IMMEDIATE, event);
        if (ped->bInVehicle) {
            ComputeTargetDoor(ped);

            CTaskSimpleCarCloseDoorFromOutside closeDoor{ m_pTargetVehicle, (uint32)m_nTargetDoor, nullptr };
            closeDoor.MakeAbortable(ped, ABORT_PRIORITY_IMMEDIATE, event);

            CTaskSimpleCarSetPedOut setPedOut{ m_pTargetVehicle, (eTargetDoor)m_nTargetDoor, m_bSensibleLeaveCar };
            setPedOut.ProcessPed(ped);
        }
        ClearVehicleFlagsSet();
        return true;
    }
    case ABORT_PRIORITY_URGENT: {
        const auto AbortSubTaskAndClearFlags = [&] {
            if (!m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, event)) {
                return false;
            }
            ClearVehicleFlagsSet();
            return true;
        };
        const auto IsKilledByDamageEvent = [&] {
            return event && event->GetEventType() == EVENT_DAMAGE && static_cast<const CEventDamage*>(event)->HasKilledPed(); // 0x4ABCA0
        };

        if (m_pSubTask->GetTaskType() == TASK_SIMPLE_DIE) {
            if (m_pSubTask->GetTaskType() == TASK_SIMPLE_CAR_DRIVE_TIMED) { // Never true (Original code is like this)
                return false;
            }
            return AbortSubTaskAndClearFlags();
        }

        if (m_pSubTask->GetTaskType() == TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN && IsKilledByDamageEvent()) {
            return AbortSubTaskAndClearFlags();
        }
        if (m_pSubTask->GetTaskType() == TASK_COMPLEX_GET_UP_AND_STAND_STILL && IsKilledByDamageEvent()) {
            return AbortSubTaskAndClearFlags();
        }

        if (m_pSubTask->GetTaskType() == TASK_SIMPLE_CAR_JUMP_OUT) {
            if (!event) {
                return false;
            }
            if (event->GetEventType() == EVENT_IN_AIR || event->GetEventType() == EVENT_IN_WATER) {
                if (event->GetEventType() == EVENT_IN_AIR) {
                    m_bIsInAir = true;
                }
                if (!m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, event)) {
                    return false;
                }

                const auto DoInWaterFall = [&] {
                    if (event->GetEventType() == EVENT_IN_WATER && ped->m_vecMoveSpeed.z < -0.3f) {
                        CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, ANIM_ID_FALL_FRONT, 16.0f);
                    }
                };

                if (ped->GetActiveWeapon().m_Type != WEAPON_PARACHUTE) {
                    DoInWaterFall();
                } else if (event->GetEventType() != EVENT_IN_AIR) {
                    DoInWaterFall();
                } else if (m_pTargetVehicle->GetNumContactWheels() != 0) {
                    DoInWaterFall();
                } else if (m_pTargetVehicle->m_nVehicleSubType == VEHICLE_TYPE_PLANE && m_pTargetVehicle->m_vecMoveSpeed.Magnitude() > 0.2f) {
                    CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, ANIM_ID_FALL_FRONT, 8.0f);
                } else if (ped->IsPlayer()) {
                    CColPoint colPoint;
                    CEntity*  colEntity;
                    if (!CWorld::ProcessVerticalLine(ped->GetPosition(), -10.0f, colPoint, colEntity, true, false, false, false, true, false, nullptr)) {
                        CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, ANIM_ID_FALL_FRONT, 8.0f);
                    }
                }
                return false; // Yes, even though the subtask was aborted
            }
        }

        if (event && m_pSubTask->GetTaskType() == TASK_SIMPLE_CAR_DRIVE_TIMED) {
            return m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, event);
        }
        return false;
    }
    default:
        return false;
    }
}

// 0x6419F0
CTask* CTaskComplexLeaveCar::CreateNextSubTask(CPed* ped) {
    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_PAUSE:
    case TASK_SIMPLE_DIE:
        return CreateSubTask(TASK_FINISHED, ped);
    case TASK_COMPLEX_GET_UP_AND_STAND_STILL:
        return CreateSubTask(ped->bInVehicle ? TASK_SIMPLE_CAR_SET_PED_OUT : TASK_FINISHED, ped);
    case TASK_COMPLEX_LEAVE_BOAT:
        return CreateSubTask(m_bDie ? TASK_SIMPLE_DIE : TASK_FINISHED, ped);
    case TASK_SIMPLE_CAR_DRIVE_TIMED: {
        if (ped->bInVehicle) {
            if (ped->m_pVehicle->IsPassenger(ped) || ped->m_pVehicle->IsDriver(ped)) {
                ComputeTargetDoor(ped);

                bool quit = false, wait = false;
                CheckDoorsFreeOfPeds(*m_pTargetVehicle, m_nTargetDoor, quit, wait);
                if (m_bForceGetOut) {
                    return CreateSubTask(TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN, ped);
                }
                if (wait) {
                    return CreateSubTask(TASK_SIMPLE_CAR_WAIT_FOR_DOOR_NOT_TO_BE_IN_USE, ped);
                }
                if (!quit) {
                    return CreateSubTask(TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN, ped);
                }
            } else {
                ped->bInVehicle = false;
            }
        }
        return CreateSubTask(TASK_FINISHED, ped);
    }
    case TASK_SIMPLE_CAR_CLOSE_DOOR_FROM_OUTSIDE: {
        m_nDieAnimID = ANIM_ID_KO_SHOT_FRONT_0;
        return CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped);
    }
    case TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN: {
        if (!m_pTargetVehicle->CanPedStepOutCar(false)) {
            PrepareVehicleForPedExit(ped);
            ped->SetPedState(PEDSTATE_NONE);
            return CreateSubTask(TASK_SIMPLE_CAR_JUMP_OUT, ped);
        }

        const auto ForcePedOut = [&] {
            PrepareVehicleForPedExit(ped);
            ped->SetPedState(PEDSTATE_NONE);
            CreateTaskUtilityLineUpPedWithCar(ped);
            return CreateSubTask(TASK_SIMPLE_CAR_FORCE_PED_OUT, ped);
        };

        if (!CCarEnterExit::IsRoomForPedToLeaveCar(m_pTargetVehicle, m_nTargetDoor, nullptr)) {
            // No room at the current door, try using the one on the other side
            auto* const veh         = m_pTargetVehicle;
            const auto  isBikeLike  = veh->IsBike() || veh->m_pHandlingData->m_bTandemSeats;
            int32       otherDoor   = 0;
            switch (m_nTargetDoor) {
            case TARGET_DOOR_FRONT_RIGHT: {
                if (veh->m_pDriver || (veh->m_nGettingInFlags & 1)) {
                    return ForcePedOut();
                }
                otherDoor = TARGET_DOOR_DRIVER;
                break;
            }
            case TARGET_DOOR_REAR_RIGHT: {
                if (veh->m_apPassengers[1] || (veh->m_nGettingInFlags & 2)) {
                    return ForcePedOut();
                }
                otherDoor = TARGET_DOOR_REAR_LEFT;
                break;
            }
            case TARGET_DOOR_DRIVER: {
                if (!isBikeLike && (veh->m_apPassengers[0] || (veh->m_nGettingInFlags & 4))) {
                    return ForcePedOut();
                }
                otherDoor = TARGET_DOOR_FRONT_RIGHT;
                break;
            }
            case TARGET_DOOR_REAR_LEFT: {
                if (!isBikeLike && (veh->m_apPassengers[2] || (veh->m_nGettingInFlags & 8))) {
                    return ForcePedOut();
                }
                otherDoor = TARGET_DOOR_REAR_RIGHT;
                break;
            }
            default:
                break;
            }
            if (!CCarEnterExit::IsRoomForPedToLeaveCar(veh, otherDoor, nullptr)) {
                return ForcePedOut();
            }
            if (veh->m_pHandlingData->m_bForceDoorCheck && veh->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE && !veh->AsAutomobile()->m_aCarNodes[otherDoor]) {
                return ForcePedOut();
            }
            m_nTargetDoor = otherDoor;
        }

        PrepareVehicleForPedExit(ped);
        ped->SetPedState(PEDSTATE_NONE);
        if (m_bDie) {
            return CreateSubTask(TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT, ped);
        }
        CreateTaskUtilityLineUpPedWithCar(ped);
        return CreateSubTask(TASK_SIMPLE_CAR_GET_OUT, ped);
    }
    case TASK_SIMPLE_CAR_WAIT_FOR_DOOR_NOT_TO_BE_IN_USE:
        return CreateSubTask(TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN, ped);
    case TASK_SIMPLE_CAR_GET_OUT: {
        m_pTaskUtilityLineUpPedWithCar->m_nDoorOpenPosType = 2;
        if (!m_bForceGetOut || m_pTargetVehicle->m_nDoorLock != CARLOCK_UNLOCKED) {
            if (CCarEnterExit::CarHasDoorToClose(m_pTargetVehicle, m_nTargetDoor)) {
                return CreateSubTask(TASK_SIMPLE_CAR_CLOSE_DOOR_FROM_OUTSIDE, ped);
            }
        }
        if (m_pTargetVehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE && CCarEnterExit::CarHasDoorToClose(m_pTargetVehicle, m_nTargetDoor)) {
            // Leave the door open
            auto&      dmgMgr = m_pTargetVehicle->AsAutomobile()->m_damageManager;
            const auto door   = static_cast<tComponent>(m_nTargetDoor);
            const auto status = (uint32)dmgMgr.GetDoorStatus_Component(door);
            if (status == 0 || status == 2) { // DAMSTATE_OK, DAMSTATE_DAMAGED
                dmgMgr.SetDoorStatus_Component(door, static_cast<eDoorStatus>(status + 1));
            }
        }
        return CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped);
    }
    case TASK_SIMPLE_CAR_JUMP_OUT: {
        if (m_bDie) {
            m_nDieAnimID         = ANIM_ID_KO_SHOT_FRONT_0;
            m_fDieAnimBlendDelta = 1000.0f;
            m_fDieAnimSpeed      = 0.5f;
            return CreateSubTask(TASK_SIMPLE_DIE, ped);
        }
        if (!m_pTargetVehicle || m_pTargetVehicle->m_nVehicleType == VEHICLE_TYPE_BIKE || m_pTargetVehicle->m_nVehicleSubType == VEHICLE_TYPE_QUAD) {
            return CreateSubTask(TASK_FINISHED, ped);
        }
        if (m_bIsInAir || ped->bIsDrowning) {
            return CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped);
        }
        ClearVehicleFlagsSet();
        return CreateSubTask(TASK_COMPLEX_GET_UP_AND_STAND_STILL, ped);
    }
    case TASK_SIMPLE_CAR_FORCE_PED_OUT:
        return CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped);
    case TASK_SIMPLE_CAR_SET_PED_OUT:
        return CreateSubTask(m_bDie ? TASK_SIMPLE_DIE : TASK_FINISHED, ped);
    case TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT: {
        m_nDieAnimID         = ANIM_ID_FLOOR_HIT;
        m_fDieAnimBlendDelta = 1000.0f;
        m_fDieAnimSpeed      = 0.5f;
        if (m_pTargetVehicle && m_pTargetVehicle->m_nDoorLock == CARLOCK_COP_CAR) {
            m_pTargetVehicle->m_nDoorLock = CARLOCK_UNLOCKED;
        }
        return CreateSubTask(TASK_SIMPLE_DIE, ped);
    }
    default:
        return nullptr;
    }
}

// 0x641FC0
CTask* CTaskComplexLeaveCar::CreateFirstSubTask(CPed* ped) {
    if ((ped->m_nPedState == PEDSTATE_ARRESTED || ped->bIsBeingArrested) && ped->IsPlayer()) {
        return new CTaskSimplePause{ -1 };
    }
    if (!ped->bInVehicle) {
        NOTSA_LOG_DEBUG("CTaskComplexLeaveCar - ped not in car");
        return new CTaskSimplePause{ -1 };
    }

    if (m_pTargetVehicle->m_pDriver == ped) {
        if (auto* const group = CPedGroups::GetPedsGroup(ped)) {
            if (group->GetMembership().IsLeader(ped)) {
                CEventGroupEvent groupEvent{ ped, new CEventLeaderExitedCarAsDriver{} };
                group->GetIntelligence().AddEvent(&groupEvent);
            }
        }
        if (ped->IsPlayer()) {
            if (ped->m_pVehicle) {
                ped->m_pVehicle->m_vehicleAudio.PlayerAboutToExitVehicleAsDriver();
            }
        } else {
            ped->SetRadioStation();
        }
    }

    if (m_pTargetVehicle->m_nVehicleType == VEHICLE_TYPE_BOAT) {
        return new CTaskComplexLeaveBoat{ m_pTargetVehicle, 0 };
    }
    return new CTaskSimpleCarDriveTimed{ m_pTargetVehicle, m_nDelayTime };
}

// 0x6421B0
CTask* CTaskComplexLeaveCar::ControlSubTask(CPed* ped) {
    if (!m_pTargetVehicle) {
        return nullptr;
    }

    const auto subTaskType = m_pSubTask->GetTaskType();
    if (!ped->bInVehicle
        && subTaskType != TASK_SIMPLE_CAR_SET_PED_OUT
        && subTaskType != TASK_SIMPLE_CAR_JUMP_OUT
        && subTaskType != TASK_COMPLEX_GET_UP_AND_STAND_STILL
        && subTaskType != TASK_SIMPLE_DIE
    ) {
        return CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped);
    }

    if (!m_bSensibleLeaveCar && subTaskType == TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN) {
        m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_LEISURE, nullptr);
    }

    return m_pSubTask;
}

// 0x641530
CTask* CTaskComplexLeaveCar::CreateSubTask(eTaskType taskType, CPed* ped) {
    switch (taskType) {
    case TASK_SIMPLE_PAUSE:
        return new CTaskSimplePause{ -1 };
    case TASK_COMPLEX_GET_UP_AND_STAND_STILL:
        return new CTaskComplexGetUpAndStandStill{};
    case TASK_SIMPLE_DIE:
        return new CTaskSimpleDie{ ANIM_GROUP_DEFAULT, static_cast<AnimationId>(m_nDieAnimID), m_fDieAnimBlendDelta, m_fDieAnimSpeed };
    case TASK_COMPLEX_LEAVE_BOAT:
        return new CTaskComplexLeaveBoat{ m_pTargetVehicle, 0 };
    case TASK_SIMPLE_CAR_DRIVE_TIMED:
        return new CTaskSimpleCarDriveTimed{ m_pTargetVehicle, m_nDelayTime };
    case TASK_SIMPLE_CAR_CLOSE_DOOR_FROM_OUTSIDE:
        return new CTaskSimpleCarCloseDoorFromOutside{ m_pTargetVehicle, (uint32)m_nTargetDoor, m_pTaskUtilityLineUpPedWithCar };
    case TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN: {
        using enum CTaskSimpleCarWaitToSlowDown::SlowDownType;
        if (m_bDie || !m_bSensibleLeaveCar) {
            return new CTaskSimpleCarWaitToSlowDown{ m_pTargetVehicle, DONT_WAIT };
        }
        return new CTaskSimpleCarWaitToSlowDown{ m_pTargetVehicle, ped->IsPlayer() ? SLOW_ENOUGH_TO_STEP_OR_JUMP : SLOW_ENOUGH_TO_STEP };
    }
    case TASK_SIMPLE_CAR_WAIT_FOR_DOOR_NOT_TO_BE_IN_USE:
        return new CTaskSimpleCarWaitForDoorNotToBeInUse{ m_pTargetVehicle, (uint32)m_nTargetDoor, 0 };
    case TASK_SIMPLE_CAR_GET_OUT:
        return new CTaskSimpleCarGetOut{ m_pTargetVehicle, (uint32)m_nTargetDoor, m_pTaskUtilityLineUpPedWithCar };
    case TASK_SIMPLE_CAR_JUMP_OUT:
        return new CTaskSimpleCarJumpOut{ m_pTargetVehicle, (uint32)m_nTargetDoor, m_pTaskUtilityLineUpPedWithCar };
    case TASK_SIMPLE_CAR_FORCE_PED_OUT:
        return new CTaskSimpleCarForcePedOut{ m_pTargetVehicle, m_nTargetDoor };
    case TASK_SIMPLE_CAR_SET_PED_OUT:
        return new CTaskSimpleCarSetPedOut{ m_pTargetVehicle, (eTargetDoor)m_nTargetDoor, m_bSensibleLeaveCar };
    case TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT:
        return new CTaskComplexCarSlowBeDraggedOut{ m_pTargetVehicle, (eTargetDoor)m_nTargetDoor, true };
    default:
        return nullptr;
    }
}
