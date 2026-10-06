#include "StdInc.h"

#include "TaskSimpleSlideToCoord.h"
#include "TaskSimpleStandStill.h"

void CTaskSimpleSlideToCoord::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleSlideToCoord, 0x86FFEC, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    // + RH_ScopedOverloadedInstall(Constructor, "NoAnim", 0x66C3E0, CTaskSimpleSlideToCoord*(CTaskSimpleSlideToCoord::*)(CVector const&, float, float));
    // + RH_ScopedOverloadedInstall(Constructor, "Anim", 0x66C450, CTaskSimpleSlideToCoord*(CTaskSimpleSlideToCoord::*)(CVector const&, float, float, char const*, char const*, int32, float, bool, int32));
    // + RH_ScopedVmtInstall(MakeAbortable, 0x66C4D0);
    RH_ScopedVMTInstall(ProcessPed, 0x66C4E0);
}

// 0x66C3E0
CTaskSimpleSlideToCoord::CTaskSimpleSlideToCoord(const CVector& slideToPos, float aimingRotation, float speed) :
    CTaskSimpleRunNamedAnim(),
    m_SlideToPos{ slideToPos },
    m_fAimingRotation{ aimingRotation },
    m_Speed{ speed },
    m_bFirstTime{ true },
    m_bRunningAnim{ false }
{
    // m_Timer not initialized
}

// 0x66C450
CTaskSimpleSlideToCoord::CTaskSimpleSlideToCoord(const CVector& slideToPos, float aimingRotation, float speed, const char* animBlockName, const char* animGroupName, uint32 animFlags, float animBlendDelta, bool bRunInSequence, uint32 endTime) :
    CTaskSimpleRunNamedAnim{ animBlockName, animGroupName, animFlags, animBlendDelta, endTime, false, bRunInSequence, false, false },
    m_SlideToPos{ slideToPos },
    m_fAimingRotation{ aimingRotation },
    m_Speed{ speed },
    m_bFirstTime{ true },
    m_bRunningAnim{ false },
    m_Timer{ -1 }
{
}

// 0x66D300
CTask* CTaskSimpleSlideToCoord::Clone() const {
    return m_bRunningAnim
        ? new CTaskSimpleSlideToCoord(m_SlideToPos, m_fAimingRotation, m_Speed, m_animName, m_animGroupName, m_animFlags, m_fBlendDelta, !!m_bRunInSequence, m_Time)
        : new CTaskSimpleSlideToCoord(m_SlideToPos, m_fAimingRotation, m_Speed);
}

// 0x66C4D0
bool CTaskSimpleSlideToCoord::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    return m_bRunningAnim ? CTaskSimpleAnim::MakeAbortable(ped, priority, event) : true;
}

// 0x66C4E0
bool CTaskSimpleSlideToCoord::ProcessPed(CPed* ped) {
    const bool bAnimFinished = m_bRunningAnim
        ? CTaskSimpleRunNamedAnim::ProcessPed(ped)
        : true;

    if (m_Timer == -1) {
        if (!m_bRunningAnim) {
            m_Timer = CTimer::GetTimeInMS() + 2000;
        } else if (bAnimFinished) {
            m_Timer = CTimer::GetTimeInMS() + 500;
        }
    }

    if (m_bFirstTime) {
        CTaskSimpleStandStill standStillTask{ STAND_STILL_TIME, false, false, 8.0f };
        standStillTask.ProcessPed(ped);
        if (ped->IsPlayer()) {
            ped->GetTaskManager().GetTaskPrimary(TASK_PRIMARY_DEFAULT)->MakeAbortable(ped, ABORT_PRIORITY_IMMEDIATE, nullptr);
        }
        ped->m_fAimingRotation = m_fAimingRotation;
        m_bFirstTime = false;
    }

    const auto& pedPos   = ped->GetPosition();
    const auto  distSq2D = sq(pedPos.x - m_SlideToPos.x) + sq(pedPos.y - m_SlideToPos.y);
    const auto  bArrived = distSq2D < sq(0.05f);
    if (bArrived) {
        ped->m_vecAnimMovingShiftLocal = CVector2D{ 0.0f, 0.0f };
    } else {
        const auto  slide = (m_SlideToPos - pedPos) * m_Speed;
        const auto& mat   = ped->GetMatrix();
        ped->m_vecAnimMovingShiftLocal = CVector2D{
            slide.Dot(mat.GetRight()),
            slide.Dot(mat.GetForward())
        };
    }

    if ((uint32)m_Timer < CTimer::GetTimeInMS()) {
        return true;
    }

    return bAnimFinished
        && bArrived
        && std::abs(CGeneral::LimitRadianAngle(ped->m_fCurrentRotation - ped->m_fAimingRotation)) < 0.1f;
}
