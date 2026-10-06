#include "StdInc.h"

#include "TaskComplexBeInCouple.h"
#include "TaskComplexWanderStandard.h"
#include "TaskComplexWalkAlongsidePed.h"
#include "TaskComplexWaitForPed.h"
#include "Ragdoll/IKChainManager.h"

void CTaskComplexBeInCouple::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexBeInCouple, 0x870724, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedVMTInstall(CreateNextSubTask, 0x684840);
    RH_ScopedVMTInstall(ControlSubTask, 0x684930);
}

// 0x6836F0
CTaskComplexBeInCouple::CTaskComplexBeInCouple(
    CPed* ped,
    bool  isLeader,
    bool  holdHands,
    bool  lookAtEachOther,
    float giveUpDist
) :
    CTaskComplex(),
    m_partner{ped},
    m_isLeader{isLeader},
    m_holdHands{holdHands},
    m_lookAtEachOther{lookAtEachOther},
    m_giveUpDist{giveUpDist}
{
    CEntity::SafeRegisterRef(m_partner);
}

// 0x683780
CTaskComplexBeInCouple::~CTaskComplexBeInCouple() {
    CEntity::SafeCleanUpRef(m_partner);
}

// 0x6837E0
CTask* CTaskComplexBeInCouple::CreateFirstSubTask(CPed* ped) {
    return CreateNextSubTask(ped);
}

// 0x6837F0
void CTaskComplexBeInCouple::AbortArmIK(CPed* ped) {
    const auto DoAbortArmIK = [ped](eIKArm arm) {
        if (IKChainManager_c::IsArmPointing(arm, ped)) {
            IKChainManager_c::AbortPointArm(arm, ped, 250);
        }
    };
    DoAbortArmIK(eIKArm::IK_ARM_RIGHT);
    DoAbortArmIK(eIKArm::IK_ARM_LEFT);
}

// 0x6847C0
bool CTaskComplexBeInCouple::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (m_partner && event && event->HasEditableResponse()) {
        auto editable = static_cast<CEventEditableResponse*>(const_cast<CEvent*>(event)); // Okay let's go!

        bool bAddToEventGroup = editable->m_bAddToEventGroup;
        editable->m_bAddToEventGroup = false;
        switch (event->GetEventType()) {
        case EVENT_DAMAGE:
        case EVENT_GUN_AIMED_AT:
        case EVENT_SHOT_FIRED:
            m_partner->GetEventGroup().Add(editable, false);
            break;
        }
        editable->m_bAddToEventGroup = bAddToEventGroup;
    }
    AbortArmIK(ped);
    return true;
}

// 0x684840
CTask* CTaskComplexBeInCouple::CreateNextSubTask(CPed* ped) {
    AbortArmIK(ped);

    if (!m_partner) {
        return nullptr;
    }

    if (m_isLeader) {
        return new CTaskComplexWanderStandard{ PEDMOVE_WALK, (uint8)CGeneral::GetRandomNumberInRange(0, 8), true };
    }
    return new CTaskComplexWalkAlongsidePed{ m_partner, m_giveUpDist };
}

// 0x683840
CTaskComplexBeInCouple::WalkSide CTaskComplexBeInCouple::GetWalkSide(CPed* ped) const {
    const auto right = m_partner->GetMatrix().GetRight(); // NOTE: Original code accesses the matrix directly (without null check)
    return DotProduct(right, ped->GetPosition()) - DotProduct(right, m_partner->GetPosition()) < 0.0f
        ? WalkSide::LEFT
        : WalkSide::RIGHT;
}

// 0x684930
CTask* CTaskComplexBeInCouple::ControlSubTask(CPed* ped) {
    const auto Finish = [&]() -> CTask* {
        AbortArmIK(ped);
        return nullptr;
    };

    if (!m_partner || !m_partner->IsAlive()) {
        return Finish();
    }

    // 0x684966 - Partner must be in a couple with us, and exactly one of us must be the leader
    {
        const auto tPartnerCouple = static_cast<CTaskComplexBeInCouple*>(m_partner->GetIntelligence()->FindTaskByType(TASK_COMPLEX_BE_IN_COUPLE));
        if (!tPartnerCouple || tPartnerCouple->m_isLeader == m_isLeader || tPartnerCouple->m_partner != ped) {
            return Finish();
        }
    }

    // 0x6849B3
    const auto tOurActiveCouple     = ped->GetIntelligence()->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_BE_IN_COUPLE);
    const auto tPartnerActiveCouple = m_partner->GetIntelligence()->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_BE_IN_COUPLE);

    const auto side = GetWalkSide(ped);

    // 0x6849EC - Update the offset we should walk at
    if (!m_isLeader) {
        if (const auto tWalkAlongside = static_cast<CTaskComplexWalkAlongsidePed*>(ped->GetIntelligence()->FindTaskByType(TASK_COMPLEX_WALK_ALONGSIDE_PED))) {
            tWalkAlongside->SetOffset(CVector{ side == WalkSide::LEFT ? -1.05f : 1.05f, 0.0f, 0.0f });
        }
    }

    // 0x684A5D
    const auto pedToPartner   = CVector2D{ m_partner->GetPosition() - ped->GetPosition() };
    const auto pedToPartnerSq = pedToPartner.SquaredMagnitude();
    if (pedToPartnerSq > sq(m_giveUpDist) || !tOurActiveCouple || !tPartnerActiveCouple) {
        return Finish();
    }

    // 0x684ACA - Partner got too far, wait for them
    if (pedToPartnerSq > sq(2.0f) && m_isLeader) {
        AbortArmIK(ped);
        if (!ped->GetIntelligence()->FindTaskByType(TASK_COMPLEX_WAIT_FOR_PED)) {
            return new CTaskComplexWaitForPed{ m_partner, 0.75f, 20'000, false };
        }
    }

    // 0x684B38
    if (m_holdHands) {
        auto handPos = ped->GetPosition() + CVector{ pedToPartner.x * 0.5f, pedToPartner.y * 0.5f, 0.0f };
        if (pedToPartnerSq < sq(1.5f)) {
            if (side != (WalkSide)m_prevSide) {
                AbortArmIK(ped);
            }
            switch (side) {
            case WalkSide::LEFT:
            case WalkSide::RIGHT: {
                g_ikChainMan.PointArm(
                    "CTaskComplexBeInCouple",
                    side == WalkSide::LEFT ? eIKArm::IK_ARM_RIGHT : eIKArm::IK_ARM_LEFT,
                    ped,
                    nullptr,
                    eBoneTag32{ (eBoneTag)-1 },
                    &handPos,
                    0.5f,
                    250,
                    30.0f
                );
                m_prevSide = (int8)side;
                break;
            }
            default:
                break;
            }
        } else {
            AbortArmIK(ped);
        }
    }

    // 0x684C1D
    if (m_lookAtEachOther) {
        const auto DoLookAt = [](CPed* looker, CPed* at) {
            if (g_ikChainMan.IsLooking(looker)) {
                return;
            }
            if (CGeneral::GetRandomNumberInRange(0, 100) <= 80) {
                return;
            }
            g_ikChainMan.LookAt(
                "TaskBeInCouple",
                looker,
                at,
                CGeneral::GetRandomNumberInRange(2000, 4000),
                BONE_HEAD,
                nullptr,
                false,
                0.25f,
                500,
                3,
                false
            );
        };
        DoLookAt(ped, m_partner);
        DoLookAt(m_partner, ped);
    }

    return m_pSubTask;
}
