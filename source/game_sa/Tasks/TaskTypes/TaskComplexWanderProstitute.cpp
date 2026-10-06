#include "StdInc.h"

#include "TaskComplexWanderProstitute.h"
#include "TaskComplexProstituteSolicit.h"
#include "EventAcquaintancePedRespect.h"

void CTaskComplexWanderProstitute::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexWanderProstitute, 0x870148, 15);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedVMTInstall(ScanForStuff, 0x672700);
}

// 0x672690
CTaskComplexWanderProstitute::CTaskComplexWanderProstitute(eMoveState MoveState, uint8 Dir, bool bWanderSensibly) :
    CTaskComplexWanderStandard(MoveState, Dir, bWanderSensibly),
    m_nStartTimeInMs{ 0 }
{
}

// 0x672700
void CTaskComplexWanderProstitute::ScanForStuff(CPed* ped) {
    CTaskComplexWanderStandard::ScanForStuff(ped);

    if (CTimer::GetTimeInMS() <= m_nStartTimeInMs) {
        return;
    }
    m_nStartTimeInMs = CTimer::GetTimeInMS() + 2000;

    const auto intel = ped->GetIntelligence();
    const auto peds  = intel->GetPedEntities();

    // Don't solicit if there are cops nearby
    for (auto i = 0; i < 16; i++) {
        const auto other = static_cast<CPed*>(peds[i]);
        if (other && other->m_nPedType == PED_TYPE_COP) {
            return;
        }
    }

    // Look for (rich) punters
    for (auto i = 0; i < 16; i++) {
        const auto other = static_cast<CPed*>(peds[i]);
        if (!other) {
            continue;
        }
        const auto playerData = other->GetPlayerData();
        if (!playerData || !other->m_pVehicle || playerData->m_pLastProstituteShagged == ped) {
            continue;
        }
        const auto rnd = CGeneral::GetRandomNumberInRange(0, 50'000); // (int32)(rand() * (1 / 32768) * 50000)
        const auto veh = other->m_pVehicle;
        if (static_cast<int32>(veh->m_pHandlingData->m_nMonetaryValue) <= rnd) {
            continue;
        }
        if ((veh->GetMoveSpeed() * 50.0f).SquaredMagnitude() >= sq(2.0f)) { // Vehicle has to be (almost) stopped
            continue;
        }
        if (!CTaskComplexProstituteSolicit::IsTaskValid(ped, other)) {
            continue;
        }
        CEventAcquaintancePedRespect event{ other, TASK_COMPLEX_PROSTITUTE_SOLICIT };
        intel->GetEventGroup().Add(&event, false);
    }
}
