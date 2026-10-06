#include "StdInc.h"

#include "TaskComplexFollowLeaderInFormation.h"
#include "TaskComplexLeaveCar.h"
#include "TaskComplexEnterCarAsPassenger.h"
#include "TaskSimpleCarDrive.h"
#include "TaskSimplePause.h"
#include "TaskSimpleStandStill.h"
#include "TaskSimpleAchieveHeading.h"
#include "SeekEntity/PosCalculators/EntitySeekPosCalculatorXYOffset.h"
#include "SeekEntity/TaskComplexSeekEntityXYOffset.h"
#include "General.h"
#include "ModelInfo.h"
#include "PedGroups.h"

void CTaskComplexFollowLeaderInFormation::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexFollowLeaderInFormation, 0x870c3c, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6949A0);
    RH_ScopedInstall(Destructor, 0x694A40);

    RH_ScopedInstall(CreateSubTask, 0x6962A6);

    RH_ScopedVMTInstall(Clone, 0x695740);
    RH_ScopedVMTInstall(GetTaskType, 0x694A30);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x696820);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x6968E0);
    RH_ScopedVMTInstall(ControlSubTask, 0x696940);
}

// 0x6949A0 (body at 0x6949A7; 0x6949A0 is a lock-prefix thunk jumping there)
CTaskComplexFollowLeaderInFormation::CTaskComplexFollowLeaderInFormation(CPedGroup* pedGroup, CPed* ped, const CVector& posn, float dist) : CTaskComplex() {
    m_Group = pedGroup;
    m_Leader = ped;
    m_Pos = posn;
    m_Int = 4;
    m_Dist = dist;
    CEntity::SafeRegisterRef(m_Leader);
}

// For 0x695740
CTaskComplexFollowLeaderInFormation::CTaskComplexFollowLeaderInFormation(const CTaskComplexFollowLeaderInFormation& o) :
    CTaskComplexFollowLeaderInFormation{
        o.m_Group,
        o.m_Leader,
        o.m_Pos,
        o.m_Dist
    }
{
}

// 0x694A40
CTaskComplexFollowLeaderInFormation::~CTaskComplexFollowLeaderInFormation() {
    CEntity::SafeCleanUpRef(m_Leader);
}

// 0x6962A6 (reached via thunks at 0x6962A0 and 0x401913)
CTask* CTaskComplexFollowLeaderInFormation::CreateSubTask(eTaskType taskType, CPed* ped) {
    switch (taskType) {
    case TASK_SIMPLE_STAND_STILL:
        return new CTaskSimpleStandStill{ 500, false, false, 8.f };
    case TASK_SIMPLE_PAUSE:
        return new CTaskSimplePause{ 2000 };
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER:
        return new CTaskComplexEnterCarAsPassenger{ m_Leader->m_pVehicle, 0, false };
    case TASK_COMPLEX_LEAVE_CAR:
        return new CTaskComplexLeaveCar{ ped->m_pVehicle, 0, 0, true, false };
    case TASK_SIMPLE_CAR_DRIVE:
        return new CTaskSimpleCarDrive{ ped->m_pVehicle, nullptr, false };
    case TASK_COMPLEX_SEEK_ENTITY: {
        const auto task = new CTaskComplexSeekEntityXYOffset{ m_Leader, 50'000, 1'000, 1.f, 2.f, 2.f, false, true };
        task->GetSeekPosCalculator().SetOffset(m_Pos);
        task->SetMoveState(PEDMOVE_SPRINT);
        return task;
    }
    case TASK_SIMPLE_ACHIEVE_HEADING: {
        auto& group = m_Group->GetMembership();
        auto* member = group.GetMember(CGeneral::GetRandomNumberInRange(0, (int32)group.CountMembers()));
        if (member == ped) {
            member = group.GetLeader();
        }
        if (!member || CGeneral::GetRandomNumberInRange(0, 50) != 20) {
            return new CTaskSimpleAchieveHeading{ ped->m_fCurrentRotation, 0.5f, 0.2f };
        }
        const auto angle = CGeneral::GetRadianAngleBetweenPoints(
            CVector2D{ member->GetPosition() - ped->GetPosition() },
            CVector2D{ 0.f, 0.f }
        );
        return new CTaskSimpleAchieveHeading{ CGeneral::LimitRadianAngle(angle), 0.5f, 0.2f };
    }
    default:
        return nullptr;
    }
}

// 0x696820
CTask* CTaskComplexFollowLeaderInFormation::CreateNextSubTask(CPed* ped) {
    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_LEAVE_CAR:
    case TASK_SIMPLE_CAR_DRIVE:
        return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
    case TASK_SIMPLE_PAUSE:
        return CreateSubTask(TASK_SIMPLE_PAUSE, ped);
    case TASK_SIMPLE_STAND_STILL:
        if (m_Leader->m_nMoveState == PEDMOVE_STILL || !m_Leader->m_nMoveState) {
            return CreateSubTask(TASK_SIMPLE_STAND_STILL, ped);
        }
        return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER:
        return ControlSubTask(ped);
    default:
        return nullptr;
    }
}

// 0x6968E0
CTask* CTaskComplexFollowLeaderInFormation::CreateFirstSubTask(CPed* ped) {
    if (!ped->bInVehicle || !ped->m_pVehicle) {
        return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
    }
    if (m_Leader->bInVehicle && m_Leader->m_pVehicle == ped->m_pVehicle) {
        return CreateSubTask(TASK_SIMPLE_CAR_DRIVE, ped);
    }
    return CreateSubTask(TASK_COMPLEX_LEAVE_CAR, ped);
}

// 0x696940
CTask* CTaskComplexFollowLeaderInFormation::ControlSubTask(CPed* ped) {
    if (!m_Leader) {
        return nullptr;
    }

    if (m_pSubTask->GetTaskType() == TASK_SIMPLE_CAR_DRIVE) {
        return m_pSubTask;
    }

    const auto GetLeaderToPedDistSq = [&] {
        return (ped->GetPosition() - m_Leader->GetPosition()).SquaredMagnitude();
    };

    if (m_Leader->bInVehicle && m_Leader->m_pVehicle) {
        if (m_pSubTask->GetTaskType() == TASK_COMPLEX_SEEK_ENTITY) {
            // Created by `CreateSubTask`, so it's always the XYOffset variant
            static_cast<CTaskComplexSeekEntityXYOffset*>(m_pSubTask)->SetEntityMaxDist2D(
                m_Leader->m_pVehicle->GetModelInfo()->GetColModel()->GetBoundRadius()
            );
            if (m_Dist > 0.f) {
                if (sq(m_Dist) < GetLeaderToPedDistSq() && m_pSubTask->MakeAbortable(ped)) {
                    CTaskSimpleStandStill{ 0, false, false, 8.f }.ProcessPed(ped);
                    return new CTaskSimplePause{ 2000 };
                }
            }
        } else if (m_pSubTask->GetTaskType() == TASK_SIMPLE_STAND_STILL) {
            if (!CPedGroups::GetPedsGroup(ped)) {
                const auto* const leaderVeh   = m_Leader->m_pVehicle;
                const auto        boundRadius = leaderVeh->GetModelInfo()->GetColModel()->GetBoundRadius();
                const auto        distSq      = GetLeaderToPedDistSq();
                if (distSq < sq(boundRadius)) {
                    if (leaderVeh->m_nNumPassengers < leaderVeh->m_nMaxPassengers && m_pSubTask->MakeAbortable(ped)) {
                        return new CTaskComplexEnterCarAsPassenger{ m_Leader->m_pVehicle, 0, false };
                    }
                } else if (distSq < sq(m_Dist) && m_pSubTask->MakeAbortable(ped)) {
                    return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
                }
            }
        }
        return m_pSubTask;
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER: {
        if (m_pSubTask->MakeAbortable(ped)) {
            return CreateFirstSubTask(ped); // Virtual call (vtbl + 0x24)
        }
        break;
    }
    case TASK_SIMPLE_PAUSE:
    case TASK_SIMPLE_STAND_STILL: {
        if (sq(m_Dist) > GetLeaderToPedDistSq()) {
            return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
        }
        break;
    }
    }
    return m_pSubTask;
}
