#include "StdInc.h"

#include "TaskComplexFollowPedFootsteps.h"
#include "PedGeometryAnalyser.h" // CPointRoute
#include "TaskSimpleGoToPoint.h"
#include "TaskSimpleStandStill.h"
#include "TaskSimpleHitHead.h"
#include "SeekEntity/TaskComplexSeekEntityStandard.h"

void CTaskComplexFollowPedFootsteps::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexFollowPedFootsteps, 0x870CC0, 12);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x694E20);
    RH_ScopedVMTInstall(MakeAbortable, 0x694ED0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x694EE0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x695000);
    RH_ScopedVMTInstall(ControlSubTask, 0x695090);
    RH_ScopedVMTInstall(CreateSubTask, 0x695E40);
}

// 0x694E20
CTaskComplexFollowPedFootsteps::CTaskComplexFollowPedFootsteps(CPed* ped) : CTaskComplex() {
    m_targetPed               = ped;
    m_updateGoToPoint         = false;
    m_subTaskCreateCheckTimer = CTimer::GetTimeInMS();
    m_lineOfSightCheckTimer   = 0;
    m_pointRoute              = nullptr;
    m_moveState               = PEDMOVE_WALK;

    CEntity::SafeRegisterRef(m_targetPed);

    m_pointRoute = new CPointRoute();
}

CTaskComplexFollowPedFootsteps* CTaskComplexFollowPedFootsteps::Constructor(CPed* ped) {
    this->CTaskComplexFollowPedFootsteps::CTaskComplexFollowPedFootsteps(ped);
    return this;
}

CTaskComplexFollowPedFootsteps::~CTaskComplexFollowPedFootsteps() {
    CEntity::SafeCleanUpRef(m_targetPed);

    delete m_pointRoute;
    m_pointRoute = nullptr;
}

// 0x694ED0
bool CTaskComplexFollowPedFootsteps::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    return m_pSubTask->MakeAbortable(ped, priority, event);
}

// 0x694EE0
CTask* CTaskComplexFollowPedFootsteps::CreateNextSubTask(CPed* ped) {
    if (!m_targetPed) {
        return CreateSubTask(TASK_FINISHED, ped);
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_HIT_HEAD:
        return CreateSubTask(TASK_SIMPLE_STAND_STILL, ped);
    case TASK_SIMPLE_STAND_STILL:
    case TASK_SIMPLE_GO_TO_POINT:
        return CreateSubTask(m_pointRoute && !m_pointRoute->IsEmpty() ? TASK_SIMPLE_GO_TO_POINT : TASK_SIMPLE_STAND_STILL, ped);
    case TASK_COMPLEX_SEEK_ENTITY: {
        // NOTE: Squared distance is compared against the non-squared constant (Unlike in `ControlSubTask`)
        const auto distSq = (m_targetPed->GetPosition() - ped->GetPosition()).SquaredMagnitude();
        return CreateSubTask(distSq <= 1.4f ? TASK_SIMPLE_STAND_STILL : TASK_COMPLEX_SEEK_ENTITY, ped);
    }
    default:
        return CreateSubTask(TASK_FINISHED, ped);
    }
}

// 0x695000
CTask* CTaskComplexFollowPedFootsteps::CreateFirstSubTask(CPed* ped) {
    if (!m_targetPed) {
        return CreateSubTask(TASK_FINISHED, ped);
    }
    // NOTE: Squared distance is compared against the non-squared constant (Unlike in `ControlSubTask`)
    const auto distSq = (m_targetPed->GetPosition() - ped->GetPosition()).SquaredMagnitude();
    return CreateSubTask(distSq <= 1.4f ? TASK_SIMPLE_STAND_STILL : TASK_COMPLEX_SEEK_ENTITY, ped);
}

// 0x695090
CTask* CTaskComplexFollowPedFootsteps::ControlSubTask(CPed* ped) {
    const auto TryChangeSubTask = [&](eTaskType taskType) -> CTask* {
        return m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)
            ? CreateSubTask(taskType, ped)
            : m_pSubTask;
    };

    if (!m_targetPed || !m_pointRoute || !m_targetPed->IsAlive()) {
        return TryChangeSubTask(TASK_FINISHED);
    }

    if (m_pSubTask->GetTaskType() == TASK_COMPLEX_SEEK_ENTITY) {
        return m_pSubTask;
    }

    if (m_targetPed->bInVehicle) {
        return TryChangeSubTask(TASK_COMPLEX_SEEK_ENTITY);
    }

    if (ped->GetIntelligence()->m_AnotherStaticCounter > 30) { // Stuck
        return TryChangeSubTask(TASK_SIMPLE_HIT_HEAD);
    }

    if (CTimer::GetTimeInMS() - m_lineOfSightCheckTimer > 500) {
        m_lineOfSightCheckTimer = CTimer::GetTimeInMS();
        m_updateGoToPoint       = CWorld::GetIsLineOfSightClear(ped->GetPosition(), m_targetPed->GetPosition(), true, false, false, true, false, false, false);
    }

    const CVector targetPos   = m_targetPed->GetPosition();
    const CVector pedToTarget = targetPos - ped->GetPosition();
    const auto    distSq      = pedToTarget.SquaredMagnitude();
    const auto    dist2D      = std::sqrt(sq(pedToTarget.x) + sq(pedToTarget.y));

    if (distSq < sq(1.4f) && m_pSubTask->GetTaskType() != TASK_SIMPLE_STAND_STILL) {
        if (m_updateGoToPoint) {
            m_pointRoute->Clear();
            return TryChangeSubTask(TASK_SIMPLE_STAND_STILL);
        }
        return m_pSubTask;
    }

    if (distSq > sq(8.0f)) { // Too far
        return TryChangeSubTask(TASK_COMPLEX_SEEK_ENTITY);
    }

    if (dist2D < 1.0f && std::abs(pedToTarget.z) > 2.0f) { // Target is above/below us
        return TryChangeSubTask(TASK_COMPLEX_SEEK_ENTITY);
    }

    if (m_updateGoToPoint) { // Target is visible, so go straight to them
        m_pointRoute->Clear();
        m_pointRoute->AddUnlessFull(m_targetPed->GetPosition());
        m_subTaskCreateCheckTimer = CTimer::GetTimeInMS() - 332;

        const auto simplest = ped->GetTaskManager().GetSimplestActiveTask();
        if (CTask::IsGoToTask(simplest)) {
            static_cast<CTaskSimpleGoToPoint*>(simplest)->UpdatePoint(m_targetPed->GetPosition(), 0.5f, false);
        }
    }

    if (CTimer::GetTimeInMS() - m_subTaskCreateCheckTimer < 166 || m_pointRoute->GetSize() >= 8) {
        if (m_pSubTask->GetTaskType() == TASK_SIMPLE_STAND_STILL && m_pointRoute->GetSize() > 1) {
            return TryChangeSubTask(TASK_SIMPLE_GO_TO_POINT);
        }
        return m_pSubTask;
    }

    m_subTaskCreateCheckTimer = CTimer::GetTimeInMS();

    // Record the target's footsteps
    if (m_pointRoute->IsEmpty()) {
        m_pointRoute->AddUnlessFull(targetPos);
    } else {
        const auto    numPoints = m_pointRoute->GetSize();
        const CVector last      = m_pointRoute->m_Entries[numPoints - 1];
        CVector       lastToTarget = targetPos - last;
        if (lastToTarget.SquaredMagnitude() > sq(0.35f)) {
            if (numPoints >= 2) {
                CVector prevToLast = last - m_pointRoute->m_Entries[numPoints - 2];
                prevToLast.Normalise();
                lastToTarget.Normalise();
                if (prevToLast.Dot(lastToTarget) >= 0.95f) { // Still going in (roughly) the same direction, so the last point is redundant
                    m_pointRoute->PopBack();
                }
            }
            m_pointRoute->AddUnlessFull(targetPos);
        }
    }

    // NOTE: This isn't the length of the route, but the sum of the squared distances from the first point - It's how the original code does it.
    auto        total = 0.0f;
    const auto& first = m_pointRoute->m_Entries[0];
    for (auto i = 1u; i < m_pointRoute->GetSize(); i++) {
        total += (m_pointRoute->m_Entries[i] - first).SquaredMagnitude();
    }
    total += (ped->GetPosition() - m_targetPed->GetPosition()).SquaredMagnitude(); // NOTE: 2D on Android

    m_moveState = total > sq(3.5f) ? PEDMOVE_RUN : PEDMOVE_WALK;
    if (m_pSubTask->GetTaskType() == TASK_SIMPLE_GO_TO_POINT) {
        static_cast<CTaskSimpleGoToPoint*>(m_pSubTask)->m_moveState = static_cast<eMoveState>(m_moveState);
    }

    return m_pSubTask;
}

// 0x695E40
CTask* CTaskComplexFollowPedFootsteps::CreateSubTask(eTaskType taskType, CPed* ped) {
    if (!m_targetPed || !m_pointRoute) {
        return nullptr;
    }

    switch (taskType) {
    case TASK_COMPLEX_SEEK_ENTITY:
        return new CTaskComplexSeekEntityStandard{
            m_targetPed,
            50'000,
            1'000,
            m_targetPed->bInVehicle ? 4.0f : 1.0f,
            2.0f,
            2.0f,
            true,
            true
        };
    case TASK_SIMPLE_GO_TO_POINT: {
        const auto point = m_pointRoute->PopFront();
        return new CTaskSimpleGoToPoint{ static_cast<eMoveState>(m_moveState), point, 0.01f, false, false };
    }
    case TASK_SIMPLE_STAND_STILL:
        return new CTaskSimpleStandStill{ 10'000, false, false, 8.0f };
    case TASK_SIMPLE_HIT_HEAD:
        return new CTaskSimpleHitHead{};
    default:
        return nullptr;
    }
}
