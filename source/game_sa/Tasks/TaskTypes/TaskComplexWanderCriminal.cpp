#include "StdInc.h"

#include "TaskComplexWanderCriminal.h"
#include "EventVehicleToSteal.h"
#include "CarEnterExit.h"

void CTaskComplexWanderCriminal::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexWanderCriminal, 0x85A23C, 15);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(LookForCarsToSteal, 0x66B4F0);
    RH_ScopedVMTInstall(ScanForStuff, 0x670350);
}

// 0x48E610
CTaskComplexWanderCriminal::CTaskComplexWanderCriminal(eMoveState MoveState, uint8 Dir, bool bWanderSensibly) : CTaskComplexWander(MoveState, Dir, bWanderSensibly) { }

// 0x670350
void CTaskComplexWanderCriminal::ScanForStuff(CPed* ped) {
    if (!m_TaskTimer.m_bStarted) {
        m_TaskTimer.Start(50);
        // NOTE: Original is `now - (int32)(RandomFloat(0, -30'000))`
        const auto rnd     = static_cast<float>(CGeneral::GetRandomNumber() & 0xFFFF) * (1.f / 32768.f) * -30'000.f;
        m_nMinNextScanTime = CTimer::GetTimeInMS() - static_cast<int32>(rnd);
    }

    if (!m_TaskTimer.IsOutOfTime()) {
        return;
    }
    m_TaskTimer.Start(50);

    if (CTimer::GetTimeInMS() >= m_nMinNextScanTime) {
        LookForCarsToSteal(ped);
    }
}

// 0x66B4F0
void CTaskComplexWanderCriminal::LookForCarsToSteal(CPed* ped) {
    CVehicle* closestVeh{};
    float     closestDistSq{ FLT_MAX };
    for (auto i = 0; i < 16; i++) {
        const auto veh = static_cast<CVehicle*>(ped->GetIntelligence()->GetVehicleEntities()[i]);
        if (!veh || !CCarEnterExit::IsVehicleStealable(veh, ped)) {
            continue;
        }
        const auto distSq = (ped->GetPosition() - veh->GetPosition()).SquaredMagnitude();
        if (distSq < closestDistSq) {
            closestDistSq = distSq;
            closestVeh    = veh;
        }
    }

    if (!closestVeh) {
        return;
    }

    m_nMinNextScanTime = CTimer::GetTimeInMS() + 30'000;

    if (static_cast<float>(CGeneral::GetRandomNumber()) * RAND_MAX_FLOAT_RECIPROCAL < 0.2f) {
        CEventVehicleToSteal event{ closestVeh };
        ped->GetEventGroup().Add(&event, false);
    }
}
