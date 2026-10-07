/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include "TaskTimer.h"
#include "AttractorScanner.h"

class CPed;
class CEntity;

class CPedAcquaintanceScanner {
public:
    static inline auto& ms_fThresholdDotProduct = StaticRef<float, 0xC0B034>();
    static inline auto& ms_iAcquaintanceScanPeriod = StaticRef<int32, 0x8D2358>();
    static inline auto& ms_iAcquaintanceLatencyPeriodDefinite = StaticRef<int32, 0x8D235C>(); // 3000
    static inline auto& ms_iAcquaintanceLatencyPeriodMaybe = StaticRef<int32, 0x8D2360>();    // 200

    CTaskTimer m_timer;
    bool m_bScanAllowedScriptPed;
    bool m_bScanAllowedInVehicle;
    bool m_bScanAllowedScriptedTask;

    void ScanForPedAcquaintanceEvents(CPed& ped, CEntity** entities, int32 count);
    bool IsScanPermitted(CPed& ped);
    void FindClosestAcquaintance(CPed& ped, int32 acquaintanceScanTypeExclusive, CEntity** nearbyPeds, int32 maxNumPeds, CPed*& outAcquaintancePed, int32& outAcquaintancePedScanType);
    int32 ScanAcquaintanceTypes(CPed& ped, int32 acquaintanceScanTypeExclusive, int32 addedType, CPed* otherPed, CPed*& outAcquaintancePed, int32& outAcquaintancePedScanType);
    static bool CanJoinLARiot(CPed& ped, CPed& otherPed);
    bool AddAcquaintanceEvent(CPed& ped, int32 acquaintanceType, CPed* acquaintancePed);

    void SetOnlyScriptPedAllowed() {
        m_bScanAllowedScriptPed    = true;
        m_bScanAllowedInVehicle    = false;
        m_bScanAllowedScriptedTask = false;
    }

    void TurnOffAllScanners() {
        m_bScanAllowedScriptPed    = false;
        m_bScanAllowedInVehicle    = false;
        m_bScanAllowedScriptedTask = false;
    }
};

class CVehiclePotentialCollisionScanner {
public:
    CTaskTimer m_timer;
    void ScanForVehiclePotentialCollisionEvents(const CPed& ped, CEntity** entities, int32 count);
};

// NOTE: Has no members, in `CEventScanner::ScanForEvents` a temporary is used as `this`
class CPedPotentialCollisionScanner {
public:
    void ScanForPedPotentialCollisionEvents(const CPed& ped, CPed* closestPed);
};

class CObjectPotentialCollisionScanner {
public:
    CTaskTimer m_timer;
    void ScanForObjectPotentialCollisionEvents(const CPed& ped);
};

class CSexyPedScanner {
public:
    CTaskTimer m_timer;
    void ScanForSexyPedEvents(const CPed& ped, CEntity** entities, int32 count);
};

class CNearbyFireScanner {
public:
    CTaskTimer m_timer;
    void ScanForNearbyFires(const CPed& ped);
};

VALIDATE_SIZE(CPedAcquaintanceScanner, 0x10);
VALIDATE_SIZE(CVehiclePotentialCollisionScanner, 0xC);
VALIDATE_SIZE(CObjectPotentialCollisionScanner, 0xC);
VALIDATE_SIZE(CSexyPedScanner, 0xC);
VALIDATE_SIZE(CNearbyFireScanner, 0xC);

class CEventScanner {
public:
    uint32                            m_nNextScanTime;
    CVehiclePotentialCollisionScanner m_vehiclePotentialCollisionScanner;
    CObjectPotentialCollisionScanner  m_objectPotentialCollisionScanner;
    CAttractorScanner                 m_attractorScanner;
    CPedAcquaintanceScanner           m_pedAcquaintanceScanner;
    CSexyPedScanner                   m_sexyPedScanner;
    CNearbyFireScanner                m_nearbyFireScanner;

    static inline auto& m_sDeadPedWalkingTimer = StaticRef<uint32, 0xC0B038>();

public:
    static void InjectHooks();

    CEventScanner();
    ~CEventScanner() = default;

    void Clear();
    void ScanForEvents(CPed& ped);
    void ScanForEventsNow(const CPed& ped, bool bDontScan);

    auto& GetAcquaintanceScanner() {
        return m_pedAcquaintanceScanner;
    }
};

VALIDATE_SIZE(CEventScanner, 0xD4);
