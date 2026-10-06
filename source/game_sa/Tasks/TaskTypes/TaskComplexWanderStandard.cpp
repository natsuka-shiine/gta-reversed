#include "StdInc.h"

#include "TaskComplexWanderStandard.h"
#include "EventSexyVehicle.h"
#include "EventChatPartner.h"
#include "EventAcquaintancePedDislike.h"
#include "PedGroups.h"
#include "GameLogic.h"

void CTaskComplexWanderStandard::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexWanderStandard, 0x85A200, 15);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(WillChat, 0x66AED0);
    RH_ScopedInstall(SetNextMinScanTime, 0x66AF60);
    RH_ScopedInstall(LookForSexyCars, 0x66AFD0);
    RH_ScopedInstall(LookForChatPartners, 0x66FDA0);
    RH_ScopedInstall(LookForGangMembers, 0x670100);
}

// 0x48E4F0
CTaskComplexWanderStandard::CTaskComplexWanderStandard(eMoveState MoveState, uint8 Dir, bool bWanderSensibly) :
    CTaskComplexWander(MoveState, Dir, bWanderSensibly),
    m_nMinNextScanTime{ 0 }
{
}

// 0x672600
void CTaskComplexWanderStandard::ScanForStuff(CPed* ped) {
    if (!m_TaskTimer.m_bStarted) {
        m_TaskTimer.m_nStartTime = CTimer::GetTimeInMS();
        m_TaskTimer.m_nInterval = 50;
        m_TaskTimer.m_bStarted = true;
    }

    if (CTimer::GetTimeInMS() < m_nMinNextScanTime)
        return;

    if (m_TaskTimer.m_bStarted) { // V547 Expression 'm_TaskTimer.m_bStarted' is always true.
        if (m_TaskTimer.m_bStopped) {
            m_TaskTimer.m_nStartTime = CTimer::GetTimeInMS();
            m_TaskTimer.m_bStopped = false;
        }

        if (CTimer::GetTimeInMS() >= m_TaskTimer.m_nStartTime + m_TaskTimer.m_nInterval) {
            m_TaskTimer.m_nInterval = 50;
            m_TaskTimer.m_nStartTime = CTimer::GetTimeInMS();
            m_TaskTimer.m_bStarted = true;
            if (!LookForGangMembers(ped) && !LookForChatPartners(ped)) {
                CTaskComplexWanderStandard::LookForSexyCars(ped);
            }
        }
    }
}

// 0x66AED0
bool CTaskComplexWanderStandard::WillChat(const CPed& first, const CPed& second) {
    if (first.m_nPedType == PED_TYPE_PROSTITUTE || second.m_nPedType == PED_TYPE_PROSTITUTE) {
        return false;
    }
    if (first.m_nPedType == PED_TYPE_COP || second.m_nPedType == PED_TYPE_COP) {
        return false;
    }
    if (first.IsPlayer() || second.IsPlayer()) {
        return false;
    }
    if (IsPedTypeGang(first.m_nPedType) || IsPedTypeGang(second.m_nPedType)) { // 0x5FE9C0
        return false;
    }
    return CPedIntelligence::AreFriends(first, second);
}

// 0x66AF60
void CTaskComplexWanderStandard::SetNextMinScanTime(CPed* ped) {
    const auto activeTask = ped->GetTaskManager().GetActiveTask();
    if (!activeTask) {
        return;
    }
    if (activeTask->GetTaskType() != GetTaskType()) {
        return;
    }
    if (static_cast<CTaskComplexWander*>(activeTask)->GetWanderType() != GetWanderType()) {
        return;
    }
    static_cast<CTaskComplexWanderStandard*>(ped->GetTaskManager().GetActiveTask())->m_nMinNextScanTime = CTimer::GetTimeInMS() + 100'000;
}

// 0x66AFD0
bool CTaskComplexWanderStandard::LookForSexyCars(CPed* ped) {
    const auto& pedPos = ped->GetPosition();
    const auto  vehicles = ped->GetIntelligence()->GetVehicleEntities();
    for (auto i = 0; i < 16; i++) {
        const auto veh = static_cast<CVehicle*>(vehicles[i]);
        if (!veh || veh == ped->m_pVehicle) {
            continue;
        }
        if (veh->m_pHandlingData->m_nMonetaryValue <= 40'000) {
            continue;
        }
        if (veh->m_fHealth <= 500.0f) {
            continue;
        }
        const auto& vehPos = veh->GetPosition();
        const auto  pedToVeh = vehPos - pedPos;
        if (pedToVeh.SquaredMagnitude() >= sq(5.0f)) {
            continue;
        }
        if (pedToVeh.Dot(ped->GetForward()) <= 0.0f) { // Vehicle is behind the ped
            continue;
        }
        if (!CWorld::GetIsLineOfSightClear(pedPos, vehPos, true, false, false, true, false, false, false)) {
            continue;
        }
        CEventSexyVehicle event{ veh };
        ped->GetIntelligence()->GetEventGroup().Add(&event, false);
        SetNextMinScanTime(ped);
        return true;
    }
    return false;
}

// 0x66FDA0
bool CTaskComplexWanderStandard::LookForChatPartners(CPed* ped) {
    if (!g_surfaceInfos.IsPavement(ped->m_nContactSurface)) {
        return false;
    }
    if (m_nMoveState > PEDMOVE_WALK) {
        return false;
    }

    // Don't chat if the player is driving around
    const auto player = FindPlayerPed();
    if (player->bInVehicle && player->m_pVehicle) {
        if (player->m_pVehicle->GetMoveSpeed().SquaredMagnitude() > sq(0.2f)) {
            return false;
        }
    }

    if (CStreaming::IsVeryBusy()) {
        return false;
    }
    if (CGameLogic::LaRiotsActiveHere()) {
        return false;
    }

    const auto& pedPos = ped->GetPosition();
    const auto  pedIntel = ped->GetIntelligence();
    const auto  peds = pedIntel->GetPedEntities();
    for (auto i = 0; i < 16; i++) {
        const auto other = static_cast<CPed*>(peds[i]);
        if (!other) {
            continue;
        }
        if (!g_surfaceInfos.IsPavement(other->m_nContactSurface)) {
            continue;
        }
        const auto otherIntel = other->GetIntelligence();
        const auto otherActiveTask = otherIntel->GetTaskManager().GetActiveTask();
        if (!otherActiveTask || otherActiveTask->GetTaskType() != GetTaskType()) {
            continue;
        }
        if (pedIntel->FindTaskByType(TASK_COMPLEX_PARTNER_CHAT) || otherIntel->FindTaskByType(TASK_COMPLEX_PARTNER_CHAT)) {
            continue;
        }
        if (pedIntel->GetEventGroup().GetEventOfType(EVENT_CHAT_PARTNER) || otherIntel->GetEventGroup().GetEventOfType(EVENT_CHAT_PARTNER)) {
            continue;
        }
        if (pedIntel->FindTaskByType(TASK_COMPLEX_BE_IN_COUPLE) || otherIntel->FindTaskByType(TASK_COMPLEX_BE_IN_COUPLE)) {
            continue;
        }
        if (!WillChat(*ped, *other)) {
            continue;
        }
        const auto& otherPos = other->GetPosition();
        const auto  pedToOther = otherPos - pedPos;
        if (pedToOther.SquaredMagnitude() >= sq(10.0f)) {
            continue;
        }
        if (pedToOther.Dot(ped->GetForward()) <= 0.0f) { // Other is behind us
            continue;
        }
        if (pedToOther.Dot(other->GetForward()) >= 0.0f) { // Other isn't facing us
            continue;
        }
        if (!CWorld::GetIsLineOfSightClear(pedPos, otherPos, true, false, false, true, false, false, false)) {
            continue;
        }

        CEventChatPartner pedEvent{ true, other };
        pedIntel->GetEventGroup().Add(&pedEvent, false);

        CEventChatPartner otherEvent{ false, ped };
        otherIntel->GetEventGroup().Add(&otherEvent, false);

        SetNextMinScanTime(ped);
        SetNextMinScanTime(other);

        return true;
    }
    return false;
}

// 0x670100
bool CTaskComplexWanderStandard::LookForGangMembers(CPed* ped) {
    if (CPedGroups::GetPedsGroup(ped)) {
        return false;
    }

    CVector targetPos;
    ComputeTargetPos(ped, targetPos, m_NextNode);

    const auto& pedPos = ped->GetPosition();
    const auto  peds = ped->GetIntelligence()->GetPedEntities();
    for (auto i = 0; i < 16; i++) {
        const auto other = static_cast<CPed*>(peds[i]);
        if (!other) {
            continue;
        }
        const auto targetToPed   = pedPos - targetPos;
        const auto targetToOther = other->GetPosition() - targetPos;
        if (targetToOther.Dot(targetToPed) <= 0.0f) {
            continue;
        }
        if (targetToOther.SquaredMagnitude() >= sq(5.0f)) {
            continue;
        }
        const auto otherGroup = CPedGroups::GetPedsGroup(other);
        if (!otherGroup || otherGroup->GetMembership().CountMembersExcludingLeader() <= 0) {
            continue;
        }
        CEventAcquaintancePedDislike event{ other, TASK_SMART_FLEE_ENTITY_WALKING }; // Task ID = 940
        ped->GetIntelligence()->GetEventGroup().Add(&event, false);
        SetNextMinScanTime(ped);
        return true;
    }
    return false;
}
