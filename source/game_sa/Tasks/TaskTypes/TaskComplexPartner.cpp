#include "StdInc.h"

#include "TaskComplexPartner.h"
#include "TaskComplexPartnerChat.h"
#include "TaskComplexGoToPointAndStandStill.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskComplexWander.h"
#include "TaskComplexWanderStandard.h"
#include "TaskSimpleStandStill.h"
#include "Ragdoll/IKChainManager.h"

void CTaskComplexPartner::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexPartner, 0x870664, 14);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(Constructor, 0x681E70);
    RH_ScopedInstall(CalcTargetPositions, 0x681FE0);
    RH_ScopedInstall(GetPartnerState, 0x6822B0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x683AD0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x681F20);
    RH_ScopedVMTInstall(ControlSubTask, 0x6840D0);
    RH_ScopedVMTInstall(StreamRequiredAnims, 0x682310);
    RH_ScopedVMTInstall(RemoveStreamedAnims, 0x682370);
}

// 0x681E70
CTaskComplexPartner::CTaskComplexPartner(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, bool makePedAlwaysFacePartner, int8 updateDirectionCount, CVector point)
    : CTaskComplex()
{
    m_leadSpeaker              = leadSpeaker;
    m_makePedAlwaysFacePartner = makePedAlwaysFacePartner;
    m_distanceMultiplier       = distanceMultiplier;
    m_updateDirectionCount     = updateDirectionCount;
    m_point                    = point;
    m_partner                  = partner;
    m_partnerState             = PARTNER_STATE_UNK_1;
    m_taskCompleted            = false;
    m_firstToTargetFlag        = -1;
    m_requiredAnimsStreamedIn  = false;
    m_animBlockName[0]         = '\0';
    CEntity::SafeRegisterRef(partner);
}

/*!
 * @addr 0x683A40
 */
CTaskComplexPartner::~CTaskComplexPartner() {
    CEntity::SafeCleanUpRef(m_partner);

    if (m_requiredAnimsStreamedIn) {
        if (strcmp(m_animBlockName, "") != 0) {
            CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(m_animBlockName));
        }
        m_requiredAnimsStreamedIn = false;
    }
}

CTaskComplexPartner* CTaskComplexPartner::Constructor(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, bool makePedAlwaysFacePartner, int8 updateDirectionCount, CVector point) {
    this->CTaskComplexPartner::CTaskComplexPartner(commandName, partner, leadSpeaker, distanceMultiplier, makePedAlwaysFacePartner, updateDirectionCount, point);
    return this;
}

// 0x681FE0
void CTaskComplexPartner::CalcTargetPositions(CPed* ped, CVector& outPoint, CVector& outPartnerPoint) {
    const CVector pedPos     = ped->GetPosition();
    const CVector partnerPos = m_partner->GetPosition();

    CVector dir = partnerPos - pedPos;
    CVector mid = pedPos + dir * m_distanceMultiplier;
    dir.Normalise();

    if (GetTaskType() != TASK_COMPLEX_PARTNER_CHAT) {
        // Make sure the meeting point isn't too close to either of the peds
        for (auto iter = 0;; iter++) {
            if (iter > 10) {
                m_taskCompleted = true;
                return;
            }

            bool bMoved = false;

            if (const auto dist = std::hypot(mid.x - pedPos.x, mid.y - pedPos.y); dist < 0.7f) {
                mid += dir * (0.75f - dist);
                bMoved = true;
            }

            if (const auto dist = std::hypot(mid.x - partnerPos.x, mid.y - partnerPos.y); dist < 0.7f) {
                mid -= dir * (0.75f - dist);
            } else if (!bMoved) {
                break;
            }
        }
    }

    const CVector half = dir * 0.5f;
    outPoint        = mid - half;
    outPartnerPoint = mid + half;
}

// 0x6822B0
int8 CTaskComplexPartner::GetPartnerState() {
    const auto partnerTask = static_cast<CTaskComplexPartner*>(m_partner->GetTaskManager().FindActiveTaskByType(static_cast<eTaskType>(m_taskId)));
    return partnerTask
        ? (int8)partnerTask->m_partnerState
        : 0;
}

// 0x683AD0
CTask* CTaskComplexPartner::CreateNextSubTask(CPed* ped) {
    if (!m_partner) {
        return nullptr;
    }

    const auto GetPartnerTask = [this] {
        return static_cast<CTaskComplexPartner*>(m_partner->GetTaskManager().FindActiveTaskByType(static_cast<eTaskType>(m_taskId)));
    };

    {
        const auto partnerTask = GetPartnerTask();
        if (!partnerTask || partnerTask->m_partner != ped) {
            return nullptr;
        }
    }

    const auto CreateStandStill = [] {
        return new CTaskSimpleStandStill{ 50, false, false, 8.0f };
    };

    switch (m_partnerState) {
    case PARTNER_STATE_UNK_1: {
        if (m_pSubTask->GetTaskType() != TASK_SIMPLE_STAND_STILL) {
            return nullptr;
        }
        if (m_leadSpeaker) {
            CalcTargetPositions(ped, m_point, m_targetPoint);
            GetPartnerTask()->m_point = m_targetPoint;
        }
        m_partnerState = PARTNER_STATE_GOT_TARGET_POS;
        return CreateStandStill();
    }
    case PARTNER_STATE_GOT_TARGET_POS: {
        if (m_pSubTask->GetTaskType() != TASK_SIMPLE_STAND_STILL) {
            return nullptr;
        }
        if (m_point.x == 0.0f && m_point.y == 0.0f) { // Not set yet
            return CreateStandStill();
        }
        m_partnerState = PARTNER_STATE_GOING_TO_POINT;
        GetTimeout()   = 0;
        return new CTaskComplexGoToPointAndStandStill{ PEDMOVE_WALK, m_point, 0.1f, 0.0f, false, true };
    }
    case PARTNER_STATE_GOING_TO_POINT: {
        switch (m_pSubTask->GetTaskType()) {
        case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL:
            return new CTaskComplexTurnToFaceEntityOrCoord{ m_partner, 0.5f, 0.2f };
        case TASK_COMPLEX_TURN_TO_FACE_ENTITY:
            m_partnerState = PARTNER_STATE_AT_POINT;
            return CreateStandStill();
        default:
            return nullptr;
        }
    }
    case PARTNER_STATE_AT_POINT: {
        const auto partnerState = GetPartnerState();
        if (partnerState == PARTNER_STATE_AT_POINT || partnerState == PARTNER_STATE_FINE_TUNING) { // Partner got there first
            if (m_firstToTargetFlag == -1) {
                m_firstToTargetFlag = 0;
            }
            const auto partnerTask = GetPartnerTask();
            if (partnerTask->m_firstToTargetFlag == -1) {
                partnerTask->m_firstToTargetFlag = 1;
            }
            m_partnerState = PARTNER_STATE_FINE_TUNING;
        } else {
            if (m_firstToTargetFlag == -1) {
                m_firstToTargetFlag = 1;
            }
            const auto partnerTask = GetPartnerTask();
            if (partnerTask->m_firstToTargetFlag == -1) {
                partnerTask->m_firstToTargetFlag = 0;
            }
            partnerTask->m_point = m_targetPoint;
        }
        return CreateStandStill();
    }
    case PARTNER_STATE_FINE_TUNING: {
        if (!m_makePedAlwaysFacePartner) { // No fine tuning required
            if (GetPartnerState() >= PARTNER_STATE_FINE_TUNING) {
                // Make sure the ped wanders off in the opposite direction after we're done
                const auto defaultTask = ped->GetTaskManager().GetTaskPrimary(TASK_PRIMARY_DEFAULT);
                if (defaultTask && defaultTask->GetTaskType() == TASK_COMPLEX_WANDER && static_cast<CTaskComplexWander*>(defaultTask)->GetWanderType() == WANDER_TYPE_STANDARD) {
                    const auto fwd    = ped->GetForwardVector();
                    const auto dir    = CGeneral::GetNodeHeadingFromVector(-fwd.x, -fwd.y);
                    const auto wander = new CTaskComplexWanderStandard{ PEDMOVE_WALK, (uint8)dir, true };
                    wander->m_nMinNextScanTime = CTimer::GetTimeInMS() + 100'000;
                    ped->GetTaskManager().SetTask(wander, TASK_PRIMARY_DEFAULT, false);
                }
                m_partnerState = PARTNER_STATE_READY;
            }
            return CreateStandStill();
        }

        if (m_firstToTargetFlag == 1) { // We were first, so it's the partner who does the fine tuning
            if (GetPartnerState() == PARTNER_STATE_READY) {
                m_partnerState = PARTNER_STATE_READY;
            }
            return CreateStandStill();
        }

        // Move so that we're exactly 1 unit away from the partner
        const auto& pedPos     = ped->GetPosition();
        const auto& partnerPos = m_partner->GetPosition();

        CVector2D partnerToPed{ pedPos.x - partnerPos.x, pedPos.y - partnerPos.y };
        const auto dist = partnerToPed.Magnitude();
        if (dist > 0.99f && dist < 1.01f) { // Good enough
            m_partnerState = PARTNER_STATE_READY;
            return CreateStandStill();
        }

        partnerToPed.Normalise();
        CVector2D shift{
            partnerToPed.x + partnerPos.x - pedPos.x,
            partnerToPed.y + partnerPos.y - pedPos.y
        };
        if (shift.Magnitude() > 0.02f) {
            shift.Normalise();
            shift.x *= 0.02f;
            shift.y *= 0.02f;
        }

        const auto& mat = ped->GetMatrix();
        ped->m_vecAnimMovingShiftLocal.x = mat.GetRight().x * shift.x + mat.GetRight().y * shift.y;
        ped->m_vecAnimMovingShiftLocal.y = mat.GetForward().x * shift.x + mat.GetForward().y * shift.y;
        return CreateStandStill();
    }
    case PARTNER_STATE_READY: {
        if (!m_requiredAnimsStreamedIn) {
            return nullptr;
        }
        if (m_updateDirectionCount == 0) {
            return nullptr;
        }
        m_updateDirectionCount--;
        return GetPartnerSequence();
    }
    default:
        return nullptr;
    }
}

// 0x681F20
CTask* CTaskComplexPartner::CreateFirstSubTask(CPed* ped) {
    if (m_leadSpeaker && m_taskId == TASK_COMPLEX_PARTNER_CHAT) {
        // NOTE: Original code accesses the members of the derived class here too
        auto* const chat = static_cast<CTaskComplexPartnerChat*>(this);
        if (chat->m_conversationEnabled) {
            if (CAEPedSpeechAudioEntity::RequestPedConversation(ped, m_partner)) {
                chat->m_pedConversationLoaded = true;
            } else if (chat->field_75) {
                chat->m_conversationEnabled = false;
            } else {
                return nullptr;
            }
        }
    }
    ped->StopPlayingHandSignal();
    return new CTaskSimpleStandStill{ 50, false, false, 8.0f };
}

// 0x6840D0
CTask* CTaskComplexPartner::ControlSubTask(CPed* ped) {
    const auto ShouldAbort = [&] {
        if (m_partnerState <= PARTNER_STATE_UNK_1 && m_partner && !m_taskCompleted) {
            return false;
        }
        if (m_partner) {
            const auto partnerTask = static_cast<CTaskComplexPartner*>(m_partner->GetTaskManager().FindActiveTaskByType(static_cast<eTaskType>(m_taskId)));
            if (partnerTask && partnerTask->m_partner == ped && !m_taskCompleted) {
                return false;
            }
        }
        return true;
    };

    if (ShouldAbort() && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
        return nullptr;
    }

    if (m_pSubTask->GetTaskType() == TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL) {
        if (++GetTimeout() > 150 && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) { // Taking too long to get to the point
            return nullptr;
        }
    }

    ped->DropEntityThatThisPedIsHolding(true);
    StreamRequiredAnims();
    if (g_ikChainMan.IsLooking(ped)) {
        g_ikChainMan.AbortLookAt(ped, 500);
    }
    return m_pSubTask;
}

// 0x682310
void CTaskComplexPartner::StreamRequiredAnims() {
    if (m_requiredAnimsStreamedIn) {
        return;
    }
    if (m_animBlockName[0] != '\0') {
        const auto blockIdx = CAnimManager::GetAnimationBlockIndex(m_animBlockName);
        if (!CAnimManager::ms_aAnimBlocks[blockIdx].IsLoaded) {
            CStreaming::RequestModel(IFPToModelId(blockIdx), STREAMING_KEEP_IN_MEMORY);
            return;
        }
        CAnimManager::AddAnimBlockRef(blockIdx);
    }
    m_requiredAnimsStreamedIn = true;
}

// 0x682370
void CTaskComplexPartner::RemoveStreamedAnims() {
    if (!m_requiredAnimsStreamedIn) {
        return;
    }
    if (m_animBlockName[0] != '\0') {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(m_animBlockName));
    }
    m_requiredAnimsStreamedIn = false;
}
