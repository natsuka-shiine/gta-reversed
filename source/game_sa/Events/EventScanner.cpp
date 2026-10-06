#include "StdInc.h"

#include "EventScanner.h"

#include "TaskSimpleGoTo.h"
#include "TaskComplexKillPedOnFoot.h"

#include "EventAcquaintancePedRespect.h"
#include "EventAcquaintancePedLike.h"
#include "EventAcquaintancePedDislike.h"
#include "EventAcquaintancePedHate.h"
#include "EventAcquaintancePedHateBadlyLit.h"
#include "EventSeenCop.h"
#include "EventOnFire.h"
#include "EventEscalator.h"
#include "EventPedEnteredMyVehicle.h"
#include "EventAreaCodes.h"
#include "EventSexyPed.h"
#include "EventFireNearby.h"
#include "EventPotentialWalkIntoFire.h"
#include "EventPotentialWalkIntoPed.h"
#include "EventPotentialWalkIntoObject.h"
#include "ObjectScanner.h"
#include "InterestingEvents.h"
#include "FireManager.h"

void CEventScanner::InjectHooks() {
    RH_ScopedCategory("Events");

    {
        RH_ScopedClass(CPedPotentialCollisionScanner);
        RH_ScopedInstall(ScanForPedPotentialCollisionEvents, 0x606580);
    }
    {
        RH_ScopedClass(CObjectPotentialCollisionScanner);
        RH_ScopedInstall(ScanForObjectPotentialCollisionEvents, 0x606890);
    }
    {
        RH_ScopedClass(CPedAcquaintanceScanner);
        RH_ScopedInstall(ScanForPedAcquaintanceEvents, 0x607D80);
        RH_ScopedInstall(IsScanPermitted, 0x603A30);
        RH_ScopedInstall(FindClosestAcquaintance, 0x607A90);
        RH_ScopedInstall(ScanAcquaintanceTypes, 0x607560);
        RH_ScopedInstall(AddAcquaintanceEvent, 0x606BA0);
        RH_ScopedInstall(CanJoinLARiot, 0x603AF0);
    }
    {
        RH_ScopedClass(CSexyPedScanner);
        RH_ScopedInstall(ScanForSexyPedEvents, 0x603BF0);
    }
    {
        RH_ScopedClass(CNearbyFireScanner);
        RH_ScopedInstall(ScanForNearbyFires, 0x603E70);
    }
}

// 0x605300
CEventScanner::CEventScanner() {
    m_nNextScanTime = CTimer::GetTimeInMS() + CGeneral::GetRandomNumberInRange(3000u); // Originally should be -3000.0f (float value)
}

void CEventScanner::Clear() {
    m_attractorScanner.Clear();
}

// 0x607E30
void CEventScanner::ScanForEvents(CPed& ped) {
    if (CTimer::GetTimeInMS() <= m_nNextScanTime) {
        return;
    }

    const auto intel = ped.GetIntelligence();
    auto&      eg    = intel->GetEventGroup();

    m_vehiclePotentialCollisionScanner.ScanForVehiclePotentialCollisionEvents(ped, intel->GetVehicleEntities(), (int32)MAX_NUM_ENTITIES);
    CPedPotentialCollisionScanner{}.ScanForPedPotentialCollisionEvents(ped, intel->GetPedScanner().GetClosestPedInRange());
    m_objectPotentialCollisionScanner.ScanForObjectPotentialCollisionEvents(ped);
    m_pedAcquaintanceScanner.ScanForPedAcquaintanceEvents(ped, intel->GetPedEntities(), (int32)MAX_NUM_ENTITIES);
    m_attractorScanner.ScanForAttractorsInRange(ped);
    m_nearbyFireScanner.ScanForNearbyFires(ped);
    intel->m_mentalState.Process(ped);

    // 0x607ECE
    if (!ped.bIsStanding && (ped.bIsInTheAir || CPedGeometryAnalyser::IsInAir(ped))) {
        CEventInAir event{};
        eg.Add(&event);
    } else if (ped.bIsInTheAir) {
        if (intel->GetEventHandler().GetCurrentEventType() != EVENT_IN_AIR) {
            ped.bIsInTheAir = false;
        }
    }

    // 0x607F5A
    if (ped.m_pFire) {
        CEventOnFire event{};

        const auto tSimplestActive = ped.IsPlayer()
            ? nullptr
            : intel->GetTaskManager().GetSimplestActiveTask();
        if (!tSimplestActive || tSimplestActive->MakeAbortable(&ped, ABORT_PRIORITY_URGENT, &event)) {
            eg.Add(&event);
        } else {
            CWeapon::GenerateDamageEvent(&ped, ped.m_pFire->GetEntityStartedFire(), WEAPON_FLAMETHROWER, 5, PED_PIECE_TORSO, 0);
        }
    }

    // 0x607FF0
    m_sexyPedScanner.ScanForSexyPedEvents(ped, intel->GetPedEntities(), (int32)MAX_NUM_ENTITIES);

    // 0x607FFF
    if (const auto tKillPedOnFoot = intel->GetTaskManager().Find<CTaskComplexKillPedOnFoot>(false)) {
        if (const auto killTarget = tKillPedOnFoot->m_target) {
            if (killTarget->m_pContactEntity && ped.m_pContactEntity) {
                if (killTarget->m_pContactEntity->GetAreaCode() != ped.m_pContactEntity->GetAreaCode()) {
                    CEventAreaCodes event{ killTarget };
                    eg.Add(&event);
                }
            }
        }
    }

    // 0x6080B2
    if (const auto contactEntity = ped.m_pContactEntity) {
        if (contactEntity->GetModelIndex() == ModelIndices::MI_ESCALATORSTEP || contactEntity->GetModelIndex() == ModelIndices::MI_ESCALATORSTEP8) {
            CEventEscalator event{};
            eg.Add(&event);
        }
    }

    // 0x608114
    if (ped.IsInVehicle() && ped.m_pVehicle->IsBoat() && !ped.IsPlayer()) {
        const auto vehicle = ped.m_pVehicle;
        if (FindPlayerPed()->m_pContactEntity == vehicle) {
            CEventPedEnteredMyVehicle event{ FindPlayerPed(), vehicle, TARGET_DOOR_DRIVER };
            event.m_TaskId = TASK_COMPLEX_LEAVE_CAR_AND_FLEE; // 0x60818B
            eg.Add(&event);
        }
    }

    // 0x6081A4
    if (ped.m_fHealth <= 0.0f && ped.IsAlive()) {
        if (CTimer::GetTimeInMS() > m_sDeadPedWalkingTimer) {
            CEventDamage event{ nullptr, 0, WEAPON_UNIDENTIFIED, PED_PIECE_TORSO, 0, false, false };
            if (!eg.HasEventOfType(&event)) {
                CWeapon::GenerateDamageEvent(&ped, nullptr, WEAPON_FALL, 10, PED_PIECE_TORSO, 0);
                m_sDeadPedWalkingTimer = CTimer::GetTimeInMS() + 2000;
            }
        }
    }
}

// 0x6053D0
void CEventScanner::ScanForEventsNow(const CPed& ped, bool bDontScan) {
    if (bDontScan)
        return;

    auto scanner = &m_vehiclePotentialCollisionScanner;
    if (scanner->m_timer.m_bStarted) { // todo: inlined?
        scanner->m_timer.m_nStartTime = CTimer::GetTimeInMS();
        scanner->m_timer.m_nInterval = -1;
        scanner->m_timer.m_bStarted = true;
    }
    scanner->ScanForVehiclePotentialCollisionEvents(ped, ped.GetIntelligence()->GetVehicleEntities(), 16);
}

// 0x603720
void CVehiclePotentialCollisionScanner::ScanForVehiclePotentialCollisionEvents(const CPed& ped, CEntity** entities, int32 count) {
    UNUSED(entities);
    UNUSED(count);

    constexpr auto CHECK_INTERVAL            = 500;  // How often to check for collision [in ms]
    constexpr auto VEH_BB_UP_DOWN_THRESHHOLD = 0.5f; // How much above/below the vehicle's BB the ped can be

    if (!m_timer.IsStarted()) {
        m_timer.Start(CHECK_INTERVAL);
    }
    if (!m_timer.IsOutOfTime()) {
        return;
    }
    m_timer.Start(CHECK_INTERVAL);

    const auto intel = ped.GetIntelligence();

    // Find ped's goto task
    const auto tSimplestActive = intel->GetTaskManager().GetSimplestActiveTask();
    if (!tSimplestActive || !CTask::IsGoToTask(tSimplestActive)) {
        return;
    }
    const auto tGoTo = static_cast<CTaskSimpleGoTo*>(tSimplestActive); // Can't use `CTask::Cast`, different goto tasks have different id's

    // Find a vehicle close-by
    const auto closestVeh = intel->GetVehicleScanner().GetClosestVehicleInRange();
    if (!closestVeh) {
        return;
    }

    const auto& pedPos   = ped.GetPosition();
    const auto  vehToPed = pedPos - closestVeh->GetPosition();

    // 0x60381A - Check if the ped is within the vehicle's BB along the vehicle's up axis (in world space)
    const auto& vehBB    = closestVeh->GetColModel()->GetBoundingBox();
    const auto& vehMat   = closestVeh->GetMatrix();
    const auto  bbMinWS  = vehMat.TransformPoint(vehBB.m_vecMin);
    const auto  bbMaxWS  = vehMat.TransformPoint(vehBB.m_vecMax);
    const auto  vehUp    = CVector{ vehMat.GetUp() };
    const auto  vehDown  = -vehUp;
    const bool  isInBBUpDown =
           vehUp.Dot(pedPos) - vehUp.Dot(bbMaxWS) < VEH_BB_UP_DOWN_THRESHHOLD
        && vehDown.Dot(pedPos) - vehDown.Dot(bbMinWS) < VEH_BB_UP_DOWN_THRESHHOLD;

    // 0x603935
    const auto range = closestVeh->m_nVehicleSubType == VEHICLE_TYPE_TRAIN
        ? 10.0f
        : 5.0f;

    if (!isInBBUpDown) {
        return;
    }

    // 0x603964 - Check if ped is close enough to the vehicle
    if (!ped.bHasJustLeftCar) {
        if (vehToPed.SquaredMagnitude() >= sq(range)) {
            return;
        }
    }

    // 0x603991 - Now, a more accurate check
    float intersectionLength{};
    if (CPedGeometryAnalyser::GetIsLineOfSightClear(ped, tGoTo->m_vecTargetPoint, *closestVeh, intersectionLength)) {
        return;
    }
    if (intersectionLength <= 0.5f) {
        return;
    }

    // 0x6039CE
    CEventPotentialWalkIntoVehicle event{ closestVeh, (int32)intel->GetMoveStateFromGoToTask() };
    intel->GetEventGroup().Add(&event);
}

// 0x606580
void CPedPotentialCollisionScanner::ScanForPedPotentialCollisionEvents(const CPed& ped, CPed* closestPed) {
    constexpr auto PED_SPHERE_RADIUS = 0.7f;

    if (!closestPed || !ped.GetUsesCollision()) {
        return;
    }

    const auto intel = ped.GetIntelligence();

    // Find ped's goto task
    const auto tSimplestActive = intel->GetTaskManager().GetSimplestActiveTask();
    if (!tSimplestActive || !CTask::IsGoToTask(tSimplestActive)) {
        return;
    }
    const auto tGoTo = static_cast<CTaskSimpleGoTo*>(tSimplestActive);

    if (intel->GetMoveStateFromGoToTask() == PEDMOVE_STILL) {
        return;
    }

    const auto& pedPos = ped.GetPosition();

    // 0x606620 - Find closest ped that's in the way [NOTE: `closestPed` isn't used at all, other than for the null check]
    CPed* pedInWay{};
    float pedInWayDistSq{ sq(2.5f) };
    for (const auto entity : intel->GetPedScanner().m_apEntities) {
        const auto otherPed = static_cast<CPed*>(entity);
        if (!otherPed || !otherPed->IsAlive() || !otherPed->GetUsesCollision()) {
            continue;
        }

        const auto pedToOther = otherPed->GetPosition() - pedPos;
        if (DotProduct(pedToOther, ped.GetForward()) <= 0.0f) { // Behind us
            continue;
        }

        CColSphere otherSphere;
        otherSphere.Set(PED_SPHERE_RADIUS, otherPed->GetPosition(), SURFACE_DEFAULT, 0, tColLighting{ 0xFF });

        CVector intersectPt1, intersectPt2;
        if (!otherSphere.IntersectEdge(pedPos, tGoTo->m_vecTargetPoint, intersectPt1, intersectPt2)) {
            continue;
        }

        if (const auto distSq = pedToOther.SquaredMagnitude(); distSq < pedInWayDistSq) {
            pedInWayDistSq = distSq;
            pedInWay       = otherPed;
        }
    }
    if (!pedInWay) {
        return;
    }

    // 0x606741 - If both peds are going in the same direction, and the other one is quicker, there's no need to do anything
    const auto IsOtherPedGettingOutOfWay = [&] {
        const auto tOtherSimplestActive = pedInWay->GetIntelligence()->GetTaskManager().GetSimplestActiveTask();
        if (!tOtherSimplestActive || !CTask::IsGoToTask(tOtherSimplestActive)) {
            return false;
        }
        if (DotProduct(ped.GetForward(), pedInWay->GetForward()) < 0.923f) {
            return false;
        }

        auto pedSpeed   = ped.m_vecMoveSpeed * 50.0f;
        auto otherSpeed = pedInWay->m_vecMoveSpeed * 50.0f;
        pedSpeed.z = otherSpeed.z = 0.0f;

        const auto pedSpeedSq   = pedSpeed.SquaredMagnitude();
        const auto otherSpeedSq = otherSpeed.SquaredMagnitude() + 0.25f;

        if (pedInWayDistSq <= 1.0f) {
            return false;
        }
        if (approxEqual(otherSpeedSq, 0.0f, 0.01f)) {
            return false;
        }
        return otherSpeedSq > pedSpeedSq;
    };
    if (IsOtherPedGettingOutOfWay()) {
        return;
    }

    // 0x606842
    CEventPotentialWalkIntoPed event{ pedInWay, tGoTo->m_vecTargetPoint, intel->GetMoveStateFromGoToTask() };
    intel->GetEventGroup().Add(&event);
}

// 0x606890
void CObjectPotentialCollisionScanner::ScanForObjectPotentialCollisionEvents(const CPed& ped) {
    constexpr auto CHECK_INTERVAL = 500; // [in ms]

    if (!m_timer.IsStarted()) {
        m_timer.Start(CHECK_INTERVAL);
    }
    if (!m_timer.IsOutOfTime()) {
        return;
    }
    m_timer.Start(CHECK_INTERVAL);

    const auto intel = ped.GetIntelligence();

    // 0x606913
    const auto moveState = [&] {
        const auto tSimplestActive = intel->GetTaskManager().GetSimplestActiveTask();
        return tSimplestActive && CTask::IsGoToTask(tSimplestActive)
            ? static_cast<CTaskSimpleGoTo*>(tSimplestActive)->m_moveState
            : PEDMOVE_STILL;
    }();

    if (ped.IsPlayer() || moveState == PEDMOVE_STILL) {
        return;
    }

    const auto tSimplestActive = intel->GetTaskManager().GetSimplestActiveTask();
    if (!tSimplestActive || !CTask::IsGoToTask(tSimplestActive)) {
        return;
    }
    const auto tGoTo = static_cast<CTaskSimpleGoTo*>(tSimplestActive);

    // 0x606981
    CObjectScanner scanner{};
    scanner.ScanForObjectsInRange(ped);

    const auto obj = scanner.GetClosestObjectInRange();
    if (!obj || obj->objectFlags.bIsBroken || obj->objectFlags.bIsPickup || !obj->GetUsesCollision()) {
        return;
    }

    const auto& pedPos = ped.GetPosition();

    // 0x6069F8
    if ((pedPos - obj->GetPosition()).SquaredMagnitude() >= sq(7.5f)) {
        return;
    }

    // 0x606A2B - Check if the ped is roughly at the same height as the object
    const auto objCentreZ = obj->GetBoundCentre().z;
    const auto objRadius  = CModelInfo::GetModelInfo(obj->GetModelIndex())->GetColModel()->GetBoundRadius();
    if (pedPos.z - 1.0f > objCentreZ + objRadius) {
        return;
    }
    if (pedPos.z + 1.0f < objCentreZ - objRadius) {
        return;
    }

    // 0x606AB5
    float intersectionLength{};
    if (CPedGeometryAnalyser::GetIsLineOfSightClear(ped, tGoTo->m_vecTargetPoint, *obj, intersectionLength)) {
        return;
    }
    if (intersectionLength <= 0.5f) {
        return;
    }

    // 0x606AE4
    if (CPedGeometryAnalyser::GetIsLineOfSightClear(pedPos + CVector{ 0.0f, 0.0f, 0.75f }, tGoTo->m_vecTargetPoint, *obj)) {
        return;
    }

    // 0x606B3A
    CEventPotentialWalkIntoObject event{ obj, (int32)moveState };
    intel->GetEventGroup().Add(&event);
}

// 0x607D80
void CPedAcquaintanceScanner::ScanForPedAcquaintanceEvents(CPed& ped, CEntity** entities, int32 count) {
    if (!m_timer.IsStarted()) {
        m_timer.Start(ms_iAcquaintanceScanPeriod);
    }
    if (!m_timer.IsOutOfTime()) {
        return;
    }
    m_timer.Start(ms_iAcquaintanceScanPeriod);

    if (!IsScanPermitted(ped)) {
        return;
    }

    CPed* acquaintancePed{};
    int32 acquaintancePedScanType{ -1 };
    FindClosestAcquaintance(
        ped,
        -1, // Acquaintance scan type exclusive (-1 => all)
        entities,
        count,
        acquaintancePed,
        acquaintancePedScanType
    );
}

// 0x603A30
bool CPedAcquaintanceScanner::IsScanPermitted(CPed& ped) {
    if (!ped.IsAlive()) {
        return false;
    }

    if (ped.IsCreatedBy(PED_MISSION) && !m_bScanAllowedScriptPed) {
        bool bPermitted = ped.bInVehicle && m_bScanAllowedInVehicle;
        if (CPedScriptedTaskRecord::GetStatus(&ped) != eScriptedTaskStatus::EVENT_ASSOCIATED && m_bScanAllowedScriptedTask) { // (Has a scripted task)
            bPermitted = true;
        }
        if (!bPermitted) {
            return false;
        }
    }

    // 0x603A94
    const auto intel = ped.GetIntelligence();
    if (const auto currEvent = intel->GetEventHandler().GetHistory().GetCurrentEvent()) {
        if (currEvent->GetEventType() == EVENT_ACQUAINTANCE_PED_HATE) {
            if (!intel->m_nDmNumPedsToScan) {
                return false;
            }
            if (!static_cast<CEventEditableResponse*>(currEvent)->ComputeResponseTaskOfType(&ped, TASK_SIMPLE_INFORM_RESPECTED_FRIENDS)) {
                return false;
            }
            if (!intel->FindRespectedFriendInInformRange()) {
                return false;
            }
        }
    }

    return true;
}

// 0x603AF0
bool CPedAcquaintanceScanner::CanJoinLARiot(CPed& ped, CPed& otherPed) {
    const auto pedType = ped.m_nPedType;
    if (pedType == PED_TYPE_COP || pedType == PED_TYPE_MEDIC || pedType == PED_TYPE_FIREMAN) {
        return false;
    }
    if (ped.IsPlayer() || ped.IsCreatedBy(PED_MISSION)) {
        return false;
    }

    // Never riot against the player (or their group) if we respect them
    if (otherPed.IsPlayer()) {
        return !ped.GetIntelligence()->Respects(&otherPed);
    }

    // 0x603B5A
    if (const auto otherGroup = CPedGroups::GetPedsGroup(&otherPed)) {
        const auto& membership = otherGroup->GetMembership();
        if (membership.GetLeader() && membership.GetLeader()->IsPlayer()) {
            return !ped.GetIntelligence()->Respects(membership.GetLeader());
        }
    }

    // 0x603BA5 - Members of the same gang don't riot against each other
    if (IsPedTypeGang(pedType)) { // 0x5FE9C0
        const auto otherPedType = otherPed.m_nPedType;
        if (IsPedTypeGang(otherPedType) && pedType == otherPedType) {
            return false;
        }
    }

    return true;
}

// 0x606BA0
bool CPedAcquaintanceScanner::AddAcquaintanceEvent(CPed& ped, int32 acquaintanceType, CPed* acquaintancePed) {
    auto& eventGroup = ped.GetIntelligence()->GetEventGroup();

    switch (acquaintanceType) {
    case ACQUAINTANCE_RESPECT: {
        CEventAcquaintancePedRespect event{ acquaintancePed };
        return eventGroup.Add(&event, false) != nullptr;
    }
    case ACQUAINTANCE_LIKE: {
        CEventAcquaintancePedLike event{ acquaintancePed };
        return eventGroup.Add(&event, false) != nullptr;
    }
    case ACQUAINTANCE_IGNORE: { // Only cops are of interest
        if (acquaintancePed->m_nPedType != PED_TYPE_COP) {
            return false;
        }
        CEventSeenCop event{ acquaintancePed };
        return eventGroup.Add(&event, false) != nullptr;
    }
    case ACQUAINTANCE_DISLIKE: {
        CEventAcquaintancePedDislike event{ acquaintancePed };
        return eventGroup.Add(&event, false) != nullptr;
    }
    case ACQUAINTANCE_HATE: {
        const float lightLevel = ped.GetIntelligence()->CanSeeEntityWithLights(acquaintancePed, 0);
        if (lightLevel < 0.0f) { // 0x606D95 - Can see them, but it's too dark to be sure
            CEventAcquaintancePedHateBadlyLit event{ acquaintancePed, (int32)CTimer::GetTimeInMS(), acquaintancePed->GetPosition() };
            return eventGroup.Add(&event, false) != nullptr;
        }
        if (lightLevel == 0.0f) {
            return false;
        }

        // 0x606CB5
        const bool bRiot = CCheat::IsActive(CHEAT_HAVE_ABOUNTY_ON_YOUR_HEAD)
            || (CGameLogic::LaRiotsActiveHere() && CanJoinLARiot(ped, *acquaintancePed));
        if (!bRiot) {
            CEventAcquaintancePedHate event{ acquaintancePed };
            return eventGroup.Add(&event, false) != nullptr;
        }

        // 0x606CF9 - Rioting, the whole group should attack
        CEventAcquaintancePedHate event{ acquaintancePed };
        if (auto* const group = CPedGroups::GetPedsGroup(&ped)) {
            event.m_TaskId = TASK_GROUP_KILL_THREATS_BASIC;
            return group->GetIntelligence().AddEvent(&event);
        }
        event.m_TaskId = TASK_COMPLEX_KILL_PED_ON_FOOT;
        return eventGroup.Add(&event, false) != nullptr;
    }
    default:
        return false;
    }
}

// 0x607560
int32 CPedAcquaintanceScanner::ScanAcquaintanceTypes(CPed& ped, int32 acquaintanceScanTypeExclusive, int32 addedType, CPed* otherPed, CPed*& outAcquaintancePed, int32& outAcquaintancePedScanType) {
    const float lightLevel = ped.GetIntelligence()->CanSeeEntityWithLights(otherPed, 0);

    for (int32 acqType = ACQUAINTANCE_HATE; acqType >= 0; acqType--) {
        if (acqType == addedType) { // Only types of higher priority than what was already added
            break;
        }

        if (acquaintanceScanTypeExclusive != -1) {
            if (acquaintanceScanTypeExclusive != acqType) {
                continue;
            }
        }

        // 0x6075A2 - When scanning for all types cops always count as `IGNORE` acquaintances
        const bool bIsIgnoredCop = acquaintanceScanTypeExclusive == -1 && acqType == ACQUAINTANCE_IGNORE && otherPed->m_nPedType == PED_TYPE_COP;
        if (!bIsIgnoredCop) {
            if (!(ped.m_acquaintance.GetAcquaintances(acqType) & CPedType::GetPedFlag(otherPed->m_nPedType))) {
                if (!CGameLogic::LaRiotsActiveHere() || !CanJoinLARiot(ped, *otherPed)) {
                    continue;
                }
            }
        }

        // 0x607601 - Must be (possibly) visible, except for `HATE`, where anything but exactly `0` is enough
        if (!(lightLevel > 0.0f) && !(acqType == ACQUAINTANCE_HATE && lightLevel != 0.0f)) {
            continue;
        }

        // 0x607628
        outAcquaintancePed         = otherPed;
        outAcquaintancePedScanType = acqType;

        if (acquaintanceScanTypeExclusive != -1) {
            return acquaintanceScanTypeExclusive;
        }

        if (!outAcquaintancePed) {
            continue;
        }

        // 0x60763F
        const bool bAdded = AddAcquaintanceEvent(ped, acqType, outAcquaintancePed);

        // 0x60764F - (Not using `m_timer.Start()`, as the original has no check for the interval's sign here)
        m_timer.m_nStartTime = CTimer::GetTimeInMS();
        m_timer.m_nInterval  = outAcquaintancePedScanType == ACQUAINTANCE_HATE && lightLevel < 0.0f
            ? ms_iAcquaintanceLatencyPeriodMaybe
            : ms_iAcquaintanceLatencyPeriodDefinite;
        m_timer.m_bStarted   = true;

        if (bAdded) {
            return outAcquaintancePedScanType;
        }
    }

    return -1;
}

// 0x607A90
void CPedAcquaintanceScanner::FindClosestAcquaintance(CPed& ped, int32 acquaintanceScanTypeExclusive, CEntity** nearbyPeds, int32 maxNumPeds, CPed*& outAcquaintancePed, int32& outAcquaintancePedScanType) {
    outAcquaintancePed = nullptr;

    CPed* candidates[16];
    int32 numCandidates{};

    for (int32 i = 0; i < maxNumPeds; i++) {
        const auto otherPed = static_cast<CPed*>(nearbyPeds[i]);
        if (!otherPed || !otherPed->IsAlive()) {
            continue;
        }

        // 0x607AE6 - Check if the other ped is in front of us (ignored if either ped is in a vehicle)
        CVector dir = otherPed->GetPosition() - ped.GetPosition();
        dir.Normalise();
        if (DotProduct(dir, ped.GetMatrix().GetForward()) <= ms_fThresholdDotProduct && !ped.bInVehicle && !otherPed->bInVehicle) {
            continue;
        }

        // 0x607B76
        bool bIsAcquaintance = acquaintanceScanTypeExclusive == -1 && otherPed->m_nPedType == PED_TYPE_COP;
        if (!bIsAcquaintance) {
            for (int32 acqId = 4; acqId >= 0 && !bIsAcquaintance; acqId--) {
                if (acquaintanceScanTypeExclusive != -1 && acquaintanceScanTypeExclusive != acqId) {
                    continue;
                }
                if (ped.m_acquaintance.GetAcquaintances(acqId) & CPedType::GetPedFlag(otherPed->m_nPedType)) {
                    bIsAcquaintance = true;
                }
            }
            if (!bIsAcquaintance) {
                if (!CGameLogic::LaRiotsActiveHere() || !CanJoinLARiot(ped, *otherPed)) {
                    continue;
                }
            }
        }

        // 0x607BFA
        const auto bIsCandidate = [&] {
            if (otherPed->m_nPedType == PED_TYPE_COP) {
                return true;
            }
            if (CGameLogic::LaRiotsActiveHere() && CanJoinLARiot(ped, *otherPed)) {
                return true;
            }
            // NOTE: The original code passes `5` as the count, but only ever initializes the first 4 elements
            //       (so the 5th one is whatever was on the stack). We only check the 4 valid ones.
            eEventType eventTypes[]{
                EVENT_ACQUAINTANCE_PED_HATE,
                EVENT_ACQUAINTANCE_PED_DISLIKE,
                EVENT_ACQUAINTANCE_PED_RESPECT,
                static_cast<eEventType>(40),
            };
            return CDecisionMakerTypes::GetInstance()->HasResponse(&ped, eventTypes, (int32)std::size(eventTypes));
        }();
        if (bIsCandidate) {
            candidates[numCandidates++] = otherPed;
        }
    }

    // 0x607C70
    int32 addedType = -1;
    for (int32 i = 0; i < numCandidates; i++) {
        if (addedType == 4) {
            continue;
        }
        const auto otherPed         = candidates[i];
        const auto useDirectionTest = !ped.bInVehicle && !otherPed->bInVehicle;
        if (CPedGeometryAnalyser::CanPedTargetPed(ped, *otherPed, useDirectionTest)) {
            addedType = ScanAcquaintanceTypes(ped, acquaintanceScanTypeExclusive, addedType, otherPed, outAcquaintancePed, outAcquaintancePedScanType);
        }
    }
}

// 0x603BF0
void CSexyPedScanner::ScanForSexyPedEvents(const CPed& ped, CEntity** entities, int32 count) {
    constexpr auto CHECK_INTERVAL          = 500;  // How often to check [in ms]
    constexpr auto INTERVAL_AFTER_SEXY_PED = 3000; // Next check after having found a sexy ped [in ms]
    constexpr auto MAX_DIST                = 100.0f;

    if (!m_timer.IsStarted()) {
        m_timer.Start(CHECK_INTERVAL);
    }
    if (!m_timer.IsOutOfTime()) {
        return;
    }
    m_timer.Start(CHECK_INTERVAL);

    if (!ped.IsCreatedBy(PED_MISSION) && ped.bInVehicle) {
        return;
    }

    const auto pedPos = ped.GetPosition();

    CPed* sexiestPed{};
    float sexiestPedDistSq{ 1.0e10f };
    for (auto i = 0; i < count; i++) {
        const auto otherPed = static_cast<CPed*>(entities[i]);
        if (!otherPed) {
            continue;
        }
        if (otherPed->m_nPedType != PED_TYPE_CIVFEMALE) {
            continue;
        }
        if (otherPed->m_pStats->m_nSexiness <= ped.m_pStats->m_nSexiness) {
            continue;
        }
        if (otherPed->bInVehicle) {
            continue;
        }

        const auto otherPos   = otherPed->GetPosition();
        const auto pedToOther = otherPos - pedPos;
        const auto distSq     = pedToOther.SquaredMagnitude();
        if (distSq >= sq(MAX_DIST)) {
            continue;
        }

        if (ped.m_pPlayerData) {
            g_InterestingEvents.Add(CInterestingEvents::INTERESTING_EVENT_9, otherPed);
        }

        if (distSq >= sexiestPedDistSq) {
            continue;
        }
        if (DotProduct(pedToOther, ped.GetForward()) <= 0.0f) { // Ped is behind us
            continue;
        }
        if (!CWorld::GetIsLineOfSightClear(pedPos, otherPos, true, false, false, true, false, false, false)) {
            continue;
        }

        sexiestPed       = otherPed;
        sexiestPedDistSq = distSq;
    }

    if (!sexiestPed) {
        return;
    }

    // 0x603DD4
    CEventSexyPed event{ sexiestPed };
    ped.GetIntelligence()->GetEventGroup().Add(&event);
    m_timer.Start(INTERVAL_AFTER_SEXY_PED);
}

// 0x603E70
void CNearbyFireScanner::ScanForNearbyFires(const CPed& ped) {
    constexpr auto CHECK_INTERVAL     = 100; // How often to check [in ms]
    constexpr auto MAX_HEIGHT_DIFF    = 2.0f;
    constexpr auto NEARBY_RANGE       = 20.0f;
    constexpr auto WALK_INTO_RANGE    = 4.0f;

    if (!m_timer.IsStarted()) {
        m_timer.Start(0);
    }
    if (!m_timer.IsOutOfTime()) {
        return;
    }
    m_timer.Start(CHECK_INTERVAL);

    const auto  intel   = ped.GetIntelligence();
    auto&       taskMgr = intel->GetTaskManager();
    const auto& pedPos  = ped.GetPosition();

    // 0x603EFD
    const auto moveState = [&] {
        const auto tSimplestActive = taskMgr.GetSimplestActiveTask();
        return tSimplestActive && CTask::IsGoToTask(tSimplestActive)
            ? static_cast<CTaskSimpleGoTo*>(tSimplestActive)->m_moveState
            : PEDMOVE_STILL;
    }();

    const auto tActive = taskMgr.GetActiveTask();

    const auto fire = gFireManager.FindNearestFire(pedPos, false, false);

    // 0x603F5A
    CVector pedToFire{};
    float   distSq{};
    if (fire) {
        pedToFire = fire->GetPosition() - pedPos;
        distSq    = pedToFire.SquaredMagnitude();
        if (distSq < sq(NEARBY_RANGE) && std::abs(pedToFire.z) < MAX_HEIGHT_DIFF) {
            CEventFireNearby event{ fire->GetPosition() };
            intel->GetEventGroup().Add(&event);
        }
    }

    // 0x60400B
    if (!tActive || tActive->GetTaskType() == TASK_COMPLEX_WALK_ROUND_FIRE) {
        return;
    }
    const auto tSimplestActive = taskMgr.GetSimplestActiveTask();
    if (!tSimplestActive || !CTask::IsGoToTask(tSimplestActive)) {
        return;
    }
    if (!fire) {
        return;
    }
    if (distSq >= sq(WALK_INTO_RANGE) || std::abs(pedToFire.z) >= MAX_HEIGHT_DIFF) {
        return;
    }

    // 0x60406B
    CEventPotentialWalkIntoFire event{ fire->GetPosition(), fire->m_Strength, moveState };
    intel->GetEventGroup().Add(&event);
}
