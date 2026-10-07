#pragma once

#include "ConversationForPed.h"
#include "PedToPlayerConversations.h"
#include "ConversationNode.h"
#include "TempConversationNode.h"

class CPed;

enum {
    MAX_NUM_CONVERSATIONS = 14,
    MAX_NUM_CONVERSATION_NODES = 50,
    MAX_NUM_TEMP_CONVERSATION_NODES = 12,
};


class CConversations {
public:
    enum class eAwkwardSayStatus : int32 {
        INACTIVE = 0,
        LOADING  = 1,
        PLAYING  = 2,
    };

    static inline auto& m_AwkwardSayStatus              = StaticRef<eAwkwardSayStatus, 0x9691C4>();
    static inline auto& m_SettingUpConversation         = StaticRef<bool, 0x9691D0>();
    static inline auto& m_Conversations                 = StaticRef<std::array<CConversationForPed, MAX_NUM_CONVERSATIONS>, 0x9691D8>();
    static inline auto& m_Nodes                         = StaticRef<std::array<CConversationNode, MAX_NUM_CONVERSATION_NODES>, 0x969570>();
    static inline auto& m_aTempNodes                    = StaticRef<std::array<CTempConversationNode, MAX_NUM_TEMP_CONVERSATION_NODES>, 0x969360>();
    static inline auto& m_SettingUpConversationNumNodes = StaticRef<int32, 0x9691C8>();
    static inline auto& m_SettingUpConversationPed      = StaticRef<CPed*, 0x9691CC>();

public:
    static void Clear();

    static void Update();

    static void AwkwardSay(int32 whatToSay, CPed* speakingPed);

    static void StartSettingUpConversation(CPed* ped);

    static void DoneSettingUpConversation(bool suppressSubtitles);

    static void SetUpConversationNode(const char* name, const char* linkYes, const char* linkNo, int32 speech, int32 speechY, int32 speechN);

    static bool IsConversationAtNode(const char* name, CPed* ped);

    static void RemoveConversationForPed(CPed* ped);

    static CConversationForPed* FindFreeConversationSlot();

    static CConversationForPed* FindConversationForPed(CPed* ped);

    static int32 FindFreeNodeSlot();

    static bool IsPlayerInPositionForConversation(CPed* ped, bool randomConversation);

    static void EnableConversation(CPed* ped, bool enable);

    static CConversationForPed::eStatus GetConversationStatus(CPed* ped);

    static bool IsConversationGoingOn();

private: // NOTSA:
    friend void InjectHooksMain();
    static void InjectHooks();
};
