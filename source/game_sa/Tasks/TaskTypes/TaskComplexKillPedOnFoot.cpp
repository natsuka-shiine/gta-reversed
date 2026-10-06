#include "StdInc.h"

#include "TaskComplexKillPedOnFoot.h"
#include "TaskComplexKillPedOnFootMelee.h"
#include "TaskComplexKillPedOnFootArmed.h"
#include "TaskComplexDestroyCar.h"
#include "TaskComplexDragPedFromCar.h"
#include "TaskComplexLeaveCar.h"
#include "TaskComplexSignalAtPed.h"
#include "TaskSimpleCarDriveTimed.h"
#include "TaskSimpleLeaveGroup.h"
#include "TaskSimplePause.h"
#include "TaskSimpleStandStill.h"
#include "Events/EventAreaCodes.h"
#include "CarEnterExit.h"
#include "CullZones.h"
#include "CopPed.h"
#include "Wanted.h"
#include "PedGroups.h"

void CTaskComplexKillPedOnFoot::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexKillPedOnFoot, 0x86D894, 11);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(Constructor, 0x620E30);
    RH_ScopedInstall(CreateSubTask, 0x625E70);
    RH_ScopedVMTInstall(MakeAbortable, 0x625E40);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x62B150);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x62B490);
    RH_ScopedVMTInstall(ControlSubTask, 0x626260);
}

CTaskComplexKillPedOnFoot::CTaskComplexKillPedOnFoot(
    CPed* target,
    int32 time,
    int32 pedFlags,
    int32 delay,
    int32 chance,
    uint8 nCompetence,
    bool bWaitForPlayerToBeSafe,
    bool bWaitingForPlayerToBeSafe
) :
    m_bWaitForPlayerToBeSafe{ bWaitForPlayerToBeSafe },
    m_bWaitingForPlayerToBeSafe{ bWaitingForPlayerToBeSafe },
    m_target{ target },
    m_pedFlags{ pedFlags },
    m_actionDelay{ delay },
    m_actionChance{ chance },
    m_nCompetence{ nCompetence },
    m_time{ time },
    m_startTime{ CTimer::GetTimeInMS() }
{
    CEntity::SafeRegisterRef(m_target);
}

CTaskComplexKillPedOnFoot::~CTaskComplexKillPedOnFoot() {
    CEntity::SafeCleanUpRef(m_target);
}

CTaskComplexKillPedOnFoot* CTaskComplexKillPedOnFoot::Constructor(CPed* target, int32 time, int32 pedFlags, int32 delay, int32 chance, int8 a7) {
    this->CTaskComplexKillPedOnFoot::CTaskComplexKillPedOnFoot(target, time, pedFlags, delay, chance, a7);
    return this;
}

// 0x65E9A0 - `CTaskComplexGangLeader::DoGangAttackSpeech` (Not present in the codebase [yet], so a local copy is used)
static void DoGangAttackSpeech(CPed* ped, CPed* target) {
    if (!ped || !target) {
        return;
    }
    if (!IsPedTypeGang(ped->m_nPedType)) {
        return;
    }
    if (!IsPedTypeGang(target->m_nPedType) && target != FindPlayerPed(0)) {
        return;
    }
    switch (target->m_nPedType) {
    case PED_TYPE_GANG1: ped->Say(CTX_GLOBAL_ATTACK_GANG_BALLAS); break;
    case PED_TYPE_GANG3: ped->Say(CTX_GLOBAL_ATTACK_GANG_LSV);    break;
    case PED_TYPE_GANG8: ped->Say(CTX_GLOBAL_ATTACK_GANG_VLA);    break;
    default:             break;
    }
}

// NOTSA - Speed (2D) of the vehicle is low enough to try dragging someone out
static bool IsVehicleSlowEnoughToDragPedOut(const CVehicle* veh) {
    return std::sqrt(sq(veh->m_vecMoveSpeed.x) + sq(veh->m_vecMoveSpeed.y)) <= 0.1f;
}

// NOTSA
static bool IsPedInVehicleSeat(CPed* ped) {
    return ped->bInVehicle
        && ped->m_pVehicle
        && (ped->m_pVehicle->IsDriver(ped) || ped->m_pVehicle->IsPassenger(ped));
}

// NOTSA - Code shared by `CreateFirstSubTask` and `CreateNextSubTask`
// Cops and gang members get a pistol if the player (who has stopped moving) is somewhere they can't easily be reached
static void MaybeGivePistolToAttackPlayer(CPed* ped, CPed* target, CPed* playerTarget, const CVector& pedToTarget) {
    const auto IsTargetOutOfReach = [&] {
        if (CCullZones::NoPolice()) {
            return true;
        }
        const auto targetStandingOn = target->m_standingOnEntity;
        return targetStandingOn
            && targetStandingOn != ped->m_standingOnEntity
            && pedToTarget.Magnitude() < 5.0f;
    };
    if (!IsTargetOutOfReach()) {
        return;
    }
    if (!playerTarget || !playerTarget->GetPlayerData()->m_bStoppedMoving) {
        return;
    }
    if ((IsPedTypeGang(ped->m_nPedType) || ped->m_nPedType == PED_TYPE_COP) && !ped->IsCreatedByMission()) {
        ped->GiveWeapon(WEAPON_PISTOL, 1000, true);
        ped->SetCurrentWeapon(WEAPON_PISTOL);
        ped->SetMoveState(PEDMOVE_STILL);
    }
}

// 0x625E40
bool CTaskComplexKillPedOnFoot::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    ped->bDontAcceptIKLookAts = false; // 0x474, bit 22
    return m_pSubTask
        ? m_pSubTask->MakeAbortable(ped, priority, event)
        : true;
}

// 0x62B150
CTask* CTaskComplexKillPedOnFoot::CreateNextSubTask(CPed* ped) {
    if (!m_target) {
        return CreateSubTask(TASK_FINISHED, ped);
    }

    const auto GetAttackTaskType = [&] {
        return ped->GetActiveWeapon().IsTypeMelee()
            ? TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE
            : TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED;
    };

    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_DESTROY_CAR:
    case TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE:
    case TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED:
    case TASK_SIMPLE_STAND_STILL:
    case TASK_NONE:
        return CreateSubTask(TASK_FINISHED, ped);
    case TASK_SIMPLE_CAR_DRIVE_TIMED:
        return CreateSubTask(TASK_COMPLEX_LEAVE_CAR, ped);
    case TASK_COMPLEX_DRAG_PED_FROM_CAR: {
        if (static_cast<CTaskComplexDragPedFromCar*>(m_pSubTask)->ShouldQuitAfterDraggingPedOut()) { // (subTask + 0x10) & 4
            return CreateSubTask(TASK_COMPLEX_DESTROY_CAR, ped);
        }
        return CreateSubTask(GetAttackTaskType(), ped);
    }
    case TASK_SIMPLE_PAUSE: {
        if (ped->m_nPedType == PED_TYPE_COP && static_cast<CCopPed*>(ped)->m_bDontPursuit) {
            return CreateSubTask(
                FindPlayerWanted()->m_ChanceOnRoadBlock == 0 // word @ CWanted + 0x1C
                    ? TASK_SIMPLE_PAUSE
                    : TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED,
                ped
            );
        }
        if (m_bWaitForPlayerToBeSafe && m_bWaitingForPlayerToBeSafe && !FindPlayerWanted()->m_bEverybodyBackOff) {
            m_bWaitingForPlayerToBeSafe = false;
            return CreateFirstSubTask(ped);
        }
        return CreateSubTask(TASK_FINISHED, ped);
    }
    case TASK_COMPLEX_LEAVE_CAR: {
        const auto playerTarget = m_target->IsPlayer() ? m_target : nullptr;
        if (ped->bInVehicle) {
            return CreateSubTask(TASK_SIMPLE_CAR_DRIVE_TIMED, ped);
        }
        MaybeGivePistolToAttackPlayer(ped, m_target, playerTarget, m_target->GetPosition() - ped->GetPosition());
        return CreateSubTask(GetAttackTaskType(), ped);
    }
    default:
        return nullptr;
    }
}

// 0x62B490
CTask* CTaskComplexKillPedOnFoot::CreateFirstSubTask(CPed* ped) {
    m_bNewTarget             = false;
    m_bRoomToDragPedOutOfCar = true;

    if (!m_target) {
        ped->bDontAcceptIKLookAts = false; // 0x474, bit 22
        return nullptr;
    }

    if (m_target->m_fHealth <= 0.0f) {
        m_bTargetKilled = true;
    }

    const CVector pedToTarget  = m_target->GetPosition() - ped->GetPosition();
    const auto    playerTarget = m_target->IsPlayer() ? m_target : nullptr;

    const auto nextTaskType = [&]() -> eTaskType {
        if (ped->m_pVehicle && ped->bInVehicle) {
            return TASK_COMPLEX_LEAVE_CAR;
        }

        if (playerTarget) {
            MaybeGivePistolToAttackPlayer(ped, m_target, playerTarget, pedToTarget);
        }

        if (!IsPedInVehicleSeat(m_target)) {
            return ped->GetActiveWeapon().IsTypeMelee()
                ? TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE
                : TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED;
        }

        const auto veh = m_target->m_pVehicle;
        m_bRoomToDragPedOutOfCar = true;

        if (veh->m_nVehicleSubType == VEHICLE_TYPE_HELI || veh->m_nVehicleSubType == VEHICLE_TYPE_PLANE || ped->bStayInSamePlace) {
            return TASK_COMPLEX_DESTROY_CAR;
        }
        if (!IsVehicleSlowEnoughToDragPedOut(veh)) {
            return TASK_COMPLEX_DESTROY_CAR;
        }
        if (!veh->CanPedOpenLocks(ped)) {
            return TASK_COMPLEX_DESTROY_CAR;
        }
        if (veh->m_nVehicleType == VEHICLE_TYPE_BIKE && !ped->GetActiveWeapon().IsTypeMelee()) {
            return TASK_COMPLEX_DESTROY_CAR;
        }
        if (veh->m_nVehicleType == VEHICLE_TYPE_BOAT) {
            return TASK_COMPLEX_DESTROY_CAR;
        }

        m_bRoomToDragPedOutOfCar = CCarEnterExit::IsRoomForPedToLeaveCar(veh, CCarEnterExit::ComputeTargetDoorToExit(veh, m_target), nullptr);
        if (!m_bRoomToDragPedOutOfCar) {
            return TASK_COMPLEX_DESTROY_CAR;
        }
        return m_target->bDontDragMeOutCar
            ? TASK_COMPLEX_DESTROY_CAR
            : TASK_COMPLEX_DRAG_PED_FROM_CAR;
    }();

    const auto subTask = CreateSubTask(nextTaskType, ped);
    ped->DropEntityThatThisPedIsHolding(true);
    return subTask;
}

// 0x626260
CTask* CTaskComplexKillPedOnFoot::ControlSubTask(CPed* ped) {
    ped->bDontAcceptIKLookAts = true; // 0x474, bit 22

    const auto AbortSubTask = [&] {
        return m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr);
    };

    const auto GetTargetWanted = [&]() -> CWanted* {
        return m_target->GetPlayerData()
            ? m_target->GetPlayerData()->m_pWanted
            : nullptr;
    };

    // Re-check whenever there's room to drag the target out of the vehicle
    const auto UpdateRoomToDragPedOutOfCar = [&] {
        const auto veh = m_target->m_pVehicle;
        m_bRoomToDragPedOutOfCar = CCarEnterExit::IsRoomForPedToLeaveCar(veh, CCarEnterExit::ComputeTargetDoorToExit(veh, m_target), nullptr);
        return m_bRoomToDragPedOutOfCar;
    };

    // 0x6269D3
    const auto Finish = [&](eTaskType nextTaskType) -> CTask* {
        if (m_target) {
            const auto targetContact = m_target->m_pContactEntity;
            const auto pedContact    = ped->m_pContactEntity;
            if (targetContact && pedContact && targetContact->m_AreaCode != pedContact->m_AreaCode) {
                CEventAreaCodes event{ m_target };
                ped->GetEventGroup().Add(&event);
            }
        }
        if (nextTaskType == TASK_NONE || !AbortSubTask()) {
            if (m_target) {
                DoGangAttackSpeech(ped, m_target);
            }
            return m_pSubTask;
        }
        return CreateSubTask(nextTaskType, ped);
    };

    if (!m_target) {
        return Finish(TASK_FINISHED);
    }

    if (m_pSubTask->GetTaskType() == TASK_SIMPLE_PAUSE) {
        if (m_bWaitForPlayerToBeSafe && m_bWaitingForPlayerToBeSafe && !FindPlayerWanted()->m_bEverybodyBackOff) {
            AbortSubTask();
        }
        return m_pSubTask;
    }

    if (m_time > 0 && CTimer::GetTimeInMS() > m_startTime + (uint32)m_time) { // Time is up
        if (!AbortSubTask()) {
            return Finish(TASK_NONE);
        }
        return new CTaskSimpleStandStill{ 0, false, false, 8.0f };
    }

    if (m_bNewTarget) {
        if (!AbortSubTask()) {
            return Finish(TASK_NONE);
        }
        return CreateFirstSubTask(ped);
    }

    if (!m_bTargetKilled) {
        if (m_target->m_fHealth <= 0.0f) {
            if (AbortSubTask()) {
                m_bTargetKilled = true;
                if (m_pSubTask->GetTaskType() == TASK_COMPLEX_SIGNAL_AT_PED) {
                    return m_pSubTask;
                }
                if (ped->m_nPedType == PED_TYPE_COP && !m_target->IsPlayer()) {
                    ped->Say(CTX_GLOBAL_ARREST_CRIM);
                }
                if (!ped->bSignalAfterKill) {
                    return new CTaskSimpleLeaveGroup{};
                }
                return CreateSubTask(TASK_COMPLEX_SIGNAL_AT_PED, ped);
            }
        } else if (m_target->IsPlayer()) {
            const auto ShouldPause = [&] {
                if (GetTargetWanted()->m_bEverybodyBackOff) {
                    return true;
                }
                return ped->m_nPedType == PED_TYPE_COP
                    && static_cast<CCopPed*>(ped)->m_bDontPursuit
                    && FindPlayerWanted()->m_ChanceOnRoadBlock == 0; // word @ CWanted + 0x1C
            };
            if (ShouldPause() && m_pSubTask->GetTaskType() != TASK_SIMPLE_PAUSE) {
                if (m_bWaitForPlayerToBeSafe && GetTargetWanted()->m_bEverybodyBackOff) {
                    m_bWaitingForPlayerToBeSafe = true;
                }
                m_bShotFiredByPlayerFlag = false;
                return Finish(TASK_SIMPLE_PAUSE);
            }
        }
    }

    // 0x6264D2
    auto nextTaskType = TASK_NONE;
    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_DESTROY_CAR: {
        if (!m_target->bInVehicle) {
            nextTaskType = ped->GetActiveWeapon().IsTypeMelee()
                ? TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE
                : TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED;
            break;
        }
        if (m_target->m_pVehicle && !CCarEnterExit::IsVehicleHealthy(m_target->m_pVehicle)) {
            nextTaskType = TASK_COMPLEX_SIGNAL_AT_PED;
            break;
        }
        if (m_target->bDontDragMeOutCar) {
            break;
        }
        const auto veh = m_target->m_pVehicle;
        if (veh->m_nVehicleSubType == VEHICLE_TYPE_PLANE || veh->m_nVehicleSubType == VEHICLE_TYPE_HELI) {
            break;
        }
        if (veh->m_nVehicleType == VEHICLE_TYPE_BIKE || veh->m_nVehicleType == VEHICLE_TYPE_BOAT || ped->bStayInSamePlace) {
            break;
        }
        if (!IsVehicleSlowEnoughToDragPedOut(veh) || !veh->CanPedOpenLocks(ped)) {
            break;
        }
        if (m_bRoomToDragPedOutOfCar) {
            nextTaskType = TASK_COMPLEX_DRAG_PED_FROM_CAR;
        } else if (m_timer.IsOutOfTime()) {
            if (UpdateRoomToDragPedOutOfCar()) {
                nextTaskType = TASK_COMPLEX_DRAG_PED_FROM_CAR;
            }
            m_timer.Start(2000);
        }
        break;
    }
    case TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED: {
        static_cast<CTaskComplexKillPedOnFootArmed*>(m_pSubTask)->m_bShotFiredByPlayer = m_bShotFiredByPlayerFlag;

        if (ped->GetActiveWeapon().IsTypeMelee() && !ped->IsPlayer()) {
            nextTaskType = TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE;
        } else if (ped->GetActiveWeapon().m_TotalAmmo == 0 && !ped->IsPlayer()) { // Out of ammo, find another weapon
            bool bFoundWeapon = false;
            for (int32 slot = 0; slot < (int32)NUM_WEAPON_SLOTS; slot++) {
                if ((int32)ped->GetWeaponInSlot((size_t)slot).m_TotalAmmo > 0) {
                    ped->SetCurrentWeapon(slot);
                    bFoundWeapon = true;
                    break;
                }
            }
            if (!bFoundWeapon) {
                ped->SetCurrentWeapon(WEAPON_UNARMED);
                nextTaskType = TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE;
            }
        }

        if (IsPedInVehicleSeat(m_target)) {
            const auto veh = m_target->m_pVehicle;
            if (veh->m_nVehicleSubType == VEHICLE_TYPE_PLANE || veh->m_nVehicleSubType == VEHICLE_TYPE_HELI) {
                nextTaskType = TASK_COMPLEX_DESTROY_CAR;
            } else if (veh->m_nVehicleType != VEHICLE_TYPE_BIKE && veh->m_nVehicleType != VEHICLE_TYPE_BOAT && !ped->bStayInSamePlace) {
                if (IsVehicleSlowEnoughToDragPedOut(veh) && veh->CanPedOpenLocks(ped)) {
                    nextTaskType = UpdateRoomToDragPedOutOfCar()
                        ? TASK_COMPLEX_DRAG_PED_FROM_CAR
                        : TASK_COMPLEX_DESTROY_CAR;
                }
            }
        }
        break;
    }
    case TASK_COMPLEX_DRAG_PED_FROM_CAR: {
        if (!IsPedInVehicleSeat(m_target)) {
            break;
        }
        if (!IsVehicleSlowEnoughToDragPedOut(m_target->m_pVehicle)) {
            nextTaskType = TASK_COMPLEX_DESTROY_CAR;
            break;
        }
        if (m_timer.IsOutOfTime()) {
            const auto veh = m_target->m_pVehicle;
            if (veh && (veh->IsPassenger(m_target) || veh->IsDriver(m_target))) {
                if (!UpdateRoomToDragPedOutOfCar()) {
                    nextTaskType = TASK_COMPLEX_DESTROY_CAR;
                }
                m_timer.Start(2000);
            }
        }
        break;
    }
    case TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE: {
        static_cast<CTaskComplexKillPedOnFootMelee*>(m_pSubTask)->m_bShotFiredByPlayerFlag = m_bShotFiredByPlayerFlag;

        if (!ped->GetActiveWeapon().IsTypeMelee()) {
            nextTaskType = TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED;
            break;
        }
        if (!m_target->bInVehicle) {
            break;
        }
        const auto veh = m_target->m_pVehicle;
        if (veh->m_nVehicleSubType == VEHICLE_TYPE_PLANE || veh->m_nVehicleSubType == VEHICLE_TYPE_HELI) { // NOTE: `veh` isn't null checked here in the original code either
            nextTaskType = TASK_COMPLEX_DESTROY_CAR;
        } else if (veh && veh->m_nVehicleType != VEHICLE_TYPE_BOAT && !ped->bStayInSamePlace && veh->CanPedOpenLocks(ped)) {
            nextTaskType = UpdateRoomToDragPedOutOfCar()
                ? TASK_COMPLEX_DRAG_PED_FROM_CAR
                : TASK_COMPLEX_DESTROY_CAR;
        }
        break;
    }
    default:
        break;
    }
    m_bShotFiredByPlayerFlag = false;

    return Finish(nextTaskType);
}

// 0x625E70
CTask* CTaskComplexKillPedOnFoot::CreateSubTask(int32 taskId, CPed* ped) {
    switch (taskId) {
    case TASK_NONE:
        return new CTaskSimpleLeaveGroup{};
    case TASK_SIMPLE_PAUSE: {
        CTaskSimpleStandStill standStill{ 0, false, false, 8.0f };
        standStill.ProcessPed(ped);
        return new CTaskSimplePause{ m_bWaitForPlayerToBeSafe && m_bWaitingForPlayerToBeSafe ? 10'000 : 2'000 };
    }
    case TASK_SIMPLE_STAND_STILL:
        return new CTaskSimpleStandStill{ 0, false, false, 8.0f };
    case TASK_COMPLEX_DRAG_PED_FROM_CAR: {
        const auto task = new CTaskComplexDragPedFromCar{ m_target, 0 };
        m_timer.Start(2000);
        return task;
    }
    case TASK_COMPLEX_LEAVE_CAR:
        return new CTaskComplexLeaveCar{ ped->m_pVehicle, 0, 0, true, true };
    case TASK_SIMPLE_CAR_DRIVE_TIMED:
        return new CTaskSimpleCarDriveTimed{ ped->m_pVehicle, 2000 };
    case TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE: {
        const auto task = new CTaskComplexKillPedOnFootMelee{ m_target };
        m_timer.Start(2000);
        return task;
    }
    case TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED: {
        const auto task = new CTaskComplexKillPedOnFootArmed{
            m_target,
            (uint32)m_pedFlags,
            (uint32)m_actionDelay,
            (uint32)m_actionChance,
            (int8)m_nCompetence
        };
        task->m_aimImmediate = m_bAimImmediate;
        m_bAimImmediate      = false;
        m_timer.Start(2000);
        return task;
    }
    case TASK_COMPLEX_DESTROY_CAR:
        return new CTaskComplexDestroyCar{ m_target->m_pVehicle, 0, 0, 0 };
    case TASK_COMPLEX_SIGNAL_AT_PED: {
        if (m_target == FindPlayerPed(0)) {
            ped->Say(CTX_GLOBAL_PLAYER_WASTED);
        } else {
            const auto group = CPedGroups::GetPedsGroup(m_target);
            if (!group || group->GetMembership().GetLeader() != FindPlayerPed(0)) {
                ped->Say(CTX_GLOBAL_ENEMY_GANG_WASTED);
            }
        }
        return new CTaskComplexSignalAtPed{ m_target, CGeneral::GetRandomNumberInRange(0, 1500), true };
    }
    case TASK_FINISHED:
        ped->bDontAcceptIKLookAts = false; // 0x474, bit 22
        return nullptr;
    default:
        return nullptr;
    }
}
