#include "StdInc.h"

#include "TaskComplexPartnerDeal.h"
#include "TaskComplexGangLeader.h"
#include "TaskComplexSequence.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskSimpleRunAnim.h"

void CTaskComplexPartnerDeal::InjectHooks()
{
    RH_ScopedVirtualClass(CTaskComplexPartnerDeal, 0x870754, 14);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(Constructor, 0x684190);

    RH_ScopedVMTInstall(CreateFirstSubTask, 0x6823B0);
    RH_ScopedVMTInstall(StreamRequiredAnims, 0x6823C0);
    RH_ScopedVMTInstall(GetPartnerSequence, 0x682440);
}

CTaskComplexPartnerDeal::CTaskComplexPartnerDeal(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, CVector point) :
    CTaskComplexPartner(commandName, partner, leadSpeaker, distanceMultiplier, true, 1, point)
{
    m_taskId = TASK_COMPLEX_PARTNER_DEAL;
    strcpy_s(m_animBlockName, "gangs");
}

CTaskComplexPartnerDeal* CTaskComplexPartnerDeal::Constructor(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, CVector point)
{
    this->CTaskComplexPartnerDeal::CTaskComplexPartnerDeal(commandName, partner, leadSpeaker, distanceMultiplier, point);
    return this;
}

// 0x6823B0
CTask* CTaskComplexPartnerDeal::CreateFirstSubTask(CPed* ped)
{
    return CTaskComplexPartner::CreateFirstSubTask(ped);
}

// 0x6823C0
void CTaskComplexPartnerDeal::StreamRequiredAnims()
{
    // NOTE: Not using `CAnimManager::StreamAnimBlock` here on purpose, this is what the game does.
    if (m_requiredAnimsStreamedIn) {
        if (!CTaskComplexGangLeader::ShouldLoadGangAnims()) {
            CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(m_animBlockName));
            m_requiredAnimsStreamedIn = false;
        }
    } else if (CTaskComplexGangLeader::ShouldLoadGangAnims()) {
        const auto blockIdx = CAnimManager::GetAnimationBlockIndex(m_animBlockName);
        if (CAnimManager::GetAnimationBlock(m_animBlockName)->IsLoaded) {
            CAnimManager::AddAnimBlockRef(blockIdx);
            m_requiredAnimsStreamedIn = true;
        } else {
            CStreaming::RequestModel(IFPToModelId(blockIdx), STREAMING_KEEP_IN_MEMORY);
        }
    }
}

// 0x682440
CTaskComplexSequence* CTaskComplexPartnerDeal::GetPartnerSequence()
{
    const auto sequence = new CTaskComplexSequence{};
    sequence->AddTask(new CTaskComplexTurnToFaceEntityOrCoord{ m_partner, 0.5f, 0.02f });
    sequence->AddTask(new CTaskSimpleRunAnim{ ANIM_GROUP_GANGS, m_leadSpeaker ? ANIM_ID_DEALER_DEAL : ANIM_ID_DRUGS_BUY, 4.0f, false });
    return sequence;
}
