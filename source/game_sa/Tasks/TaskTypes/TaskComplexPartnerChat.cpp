#include "StdInc.h"

#include "TaskComplexPartnerChat.h"
#include "TaskComplexSequence.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskComplexChat.h"
#include "TaskSimpleChat.h"
#include "TaskSimpleStandStill.h"

void CTaskComplexPartnerChat::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexPartnerChat, 0x8707C4, 14);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(Constructor, 0x684290);
    RH_ScopedVMTInstall(MakeAbortable, 0x682C60);
    RH_ScopedVMTInstall(GetPartnerSequence, 0x684380);
}

// 0x684290
CTaskComplexPartnerChat::CTaskComplexPartnerChat(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, int8 updateDirectionCount, bool conversationEnabled, bool a8, CVector point) :
    CTaskComplexPartner(commandName, partner, leadSpeaker, distanceMultiplier, false, updateDirectionCount, point)
{
    m_taskId = TASK_COMPLEX_PARTNER_CHAT;
    m_pedConversationLoaded = 0;
    m_conversationEnabled = conversationEnabled;
    if (conversationEnabled) {
        m_updateDirectionCount = 4;
    }
    field_75 = a8;
    strcpy_s(m_commandName, commandName);
}

// 0x684320
CTaskComplexPartnerChat::~CTaskComplexPartnerChat() {
    if (m_conversationEnabled && m_pedConversationLoaded) {
        CAEPedSpeechAudioEntity::ReleasePedConversation();
    }
}

CTaskComplexPartnerChat* CTaskComplexPartnerChat::Constructor(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, int8 updateDirectionCount, bool conversationEnabled, bool a8, CVector point)
{
    this->CTaskComplexPartnerChat::CTaskComplexPartnerChat(commandName, partner, leadSpeaker, distanceMultiplier, updateDirectionCount, conversationEnabled, a8, point);
    return this;
}

// 0x682C60
bool CTaskComplexPartnerChat::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (!m_pSubTask->MakeAbortable(ped, priority, event)) {
        return false;
    }
    if (m_conversationEnabled && m_pedConversationLoaded) {
        CAEPedSpeechAudioEntity::ReleasePedConversation();
        m_pedConversationLoaded = false;
    }
    return true;
}

// 0x684380
CTaskComplexSequence* CTaskComplexPartnerChat::GetPartnerSequence() {
    auto* const seq = new CTaskComplexSequence{};
    seq->AddTask(new CTaskComplexTurnToFaceEntityOrCoord{ m_partner, 0.5f, 0.001f });

    // Pseudo-random value, in a way that it's the same for both peds
    const auto rnd      = (float)((int32)((float)((int32)m_updateDirectionCount << 4) + m_point.z + m_point.y + m_point.x) & 0x7F) / 128.0f;
    const auto timeA    = (int32)(rnd * 3000.0f + 3500.0f);
    const auto timeB    = (int32)((1.0f - rnd) * 3000.0f + 3500.0f);

    if (!m_leadSpeaker) { // Use whatever the lead speaker uses
        const auto partnerTask = static_cast<CTaskComplexPartnerChat*>(m_partner->GetIntelligence()->FindTaskByType(TASK_COMPLEX_PARTNER_CHAT));
        m_conversationEnabled  = partnerTask->m_conversationEnabled;
    }

    if (m_conversationEnabled) {
        // `m_updateDirectionCount` counts down from 3, each stage has 2 lines (One for each ped)
        const auto stage     = 3 - (int32)m_updateDirectionCount;
        const auto leadCtx   = (int16)CAEPedSpeechAudioEntity::s_Conversation[2 * stage + 0];
        const auto replyCtx  = (int16)CAEPedSpeechAudioEntity::s_Conversation[2 * stage + 1];
        if (stage == CAEPedSpeechAudioEntity::s_ConversationLength) {
            m_updateDirectionCount = 0;
        }
        if (m_leadSpeaker) {
            seq->AddTask(new CTaskComplexChat{ true, m_partner, m_updateDirectionCount, leadCtx });
            seq->AddTask(new CTaskComplexChat{ false, m_partner, m_updateDirectionCount, leadCtx });
        } else {
            seq->AddTask(new CTaskComplexChat{ false, m_partner, m_updateDirectionCount, replyCtx });
            seq->AddTask(new CTaskComplexChat{ true, m_partner, m_updateDirectionCount, replyCtx });
        }
    } else {
        if (m_leadSpeaker) {
            seq->AddTask(new CTaskSimpleChat{ (uint32)timeA });
            seq->AddTask(new CTaskSimpleStandStill{ timeB, false, false, 8.0f });
        } else {
            seq->AddTask(new CTaskSimpleStandStill{ timeA, false, false, 8.0f });
            seq->AddTask(new CTaskSimpleChat{ (uint32)timeB });
        }
    }

    return seq;
}
