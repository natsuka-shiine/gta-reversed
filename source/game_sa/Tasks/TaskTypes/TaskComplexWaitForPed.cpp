#include "StdInc.h"

#include "TaskComplexWaitForPed.h"
#include "TaskComplexWalkAlongsidePed.h"
#include "TaskSimpleStandStill.h"
#include "Events/EventDeadPed.h"

void CTaskComplexWaitForPed::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexWaitForPed, 0x8706F8, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x683340);
    RH_ScopedInstall(Destructor, 0x6833D0);

    RH_ScopedVMTInstall(Clone, 0x683950);
    RH_ScopedVMTInstall(GetTaskType, 0x6833C0);
    RH_ScopedVMTInstall(MakeAbortable, 0x683430);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x683440);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x683450);
    RH_ScopedVMTInstall(ControlSubTask, 0x6834E0);
}

// 0x683340
CTaskComplexWaitForPed::CTaskComplexWaitForPed(CPed* ped, float radius, uint32 timeInMs, bool bRotateOtherPedsToWaitingPed) :
    m_ped{ ped },
    m_radius{ radius },
    m_timeInMs{ timeInMs },
    m_bRotateOtherPedsToWaitingPed{ bRotateOtherPedsToWaitingPed }
{
    // NOTE: `m_framesToWaitForSettingRotation` is left uninitialized here (It's set in `CreateFirstSubTask`)
    CEntity::RegisterReference(m_ped); // No null-check in the original code either
}

// 0x6833D0
CTaskComplexWaitForPed::~CTaskComplexWaitForPed() {
    CEntity::SafeCleanUpRef(m_ped);
}

// 0x683450
CTask* CTaskComplexWaitForPed::CreateFirstSubTask(CPed* ped) {
    if (!m_ped) {
        return nullptr;
    }

    m_timer.m_nStartTime = CTimer::GetTimeInMS();
    m_timer.m_nInterval  = (int32)m_timeInMs;
    m_timer.m_bStarted   = true;

    m_framesToWaitForSettingRotation = 0;

    return new CTaskSimpleStandStill{ 999'999, false, false, 8.0f };
}

// 0x6834E0
CTask* CTaskComplexWaitForPed::ControlSubTask(CPed* ped) {
    if (!m_ped) {
        return nullptr;
    }

    if (!m_ped->IsAlive()) { // 0x68350B
        CEventDeadPed event{ m_ped, false, CTimer::GetTimeInMS() };
        event.m_TaskId = TASK_COMPLEX_INVESTIGATE_DEAD_PED;
        ped->GetEventGroup().Add(&event, false);
        return nullptr;
    }

    if (m_timer.m_bStarted && m_timer.IsOutOfTime()) { // 0x68356D
        return nullptr;
    }

    // 0x683580 - Vector from the ped we're waiting for to where we are (or where we should be walking alongside it)
    CVector waitingPedToPos;
    if (const auto* const tWalkAlongside = static_cast<CTaskComplexWalkAlongsidePed*>(m_ped->GetIntelligence()->FindTaskByType(TASK_COMPLEX_WALK_ALONGSIDE_PED))) {
        // Original uses the matrix directly here (No null-check)
        waitingPedToPos = ped->m_matrix->TransformPoint(tWalkAlongside->GetOffset()) - m_ped->GetPosition();
    } else {
        waitingPedToPos = ped->GetPosition() - m_ped->GetPosition();
    }

    if (waitingPedToPos.SquaredMagnitude() < sq(m_radius)) { // 0x683624
        return nullptr;
    }

    if (m_bRotateOtherPedsToWaitingPed && m_framesToWaitForSettingRotation == 0) { // 0x683665
        const CVector pedToWaitingPed = m_ped->GetPosition() - ped->GetPosition();
        ped->m_fAimingRotation = CGeneral::LimitRadianAngle(
            CGeneral::GetRadianAngleBetweenPoints(pedToWaitingPed.x, pedToWaitingPed.y, 0.0f, 0.0f)
        );
    }

    if (++m_framesToWaitForSettingRotation > 10) { // 0x6836C5
        m_framesToWaitForSettingRotation = 0;
    }

    return m_pSubTask;
}
