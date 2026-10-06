/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "StdInc.h"

#include "CarCtrl.h"
#include "TrafficLights.h"
#include "TheScripts.h"
#include "GangWars.h"
#include "Game.h"
#include "General.h"
#include "GameLogic.h"
#include "CutsceneMgr.h"
#include "TheCarGenerators.h"
#include "eAreaCodes.h"
#include "TaskComplexLeaveAnyCar.h"
#include "VehicleRecording.h"
#include "TaskComplexGoToPointAndStandStill.h"
#include "EventPotentialGetRunOver.h"
#include "Cranes.h"
#include "Curves.h"
#include "ModelIndices.h"

#include <reversiblebugfixes/Bugs.hpp>

using namespace ModelIndices;

auto& apCarsToKeep = StaticRef<CVehicle*[2]>(0x969084);
auto& aCarsToKeepTime = StaticRef<std::array<uint32, 2>>(0x96907C);

void CCarCtrl::InjectHooks()
{
    RH_ScopedClass(CCarCtrl);
    RH_ScopedCategoryGlobal();

    using namespace ReversibleHooks;
    RH_ScopedInstall(Init, 0x4212E0);
    RH_ScopedInstall(ReInit, 0x4213B0);
    RH_ScopedInstall(InitSequence, 0x421740);
    RH_ScopedInstall(FindSequenceElement, 0x421770);
    RH_ScopedInstall(FindSpeedMultiplier, 0x4224E0);
    RH_ScopedInstall(FindSpeedMultiplierWithSpeedFromNodes, 0x424130);
    RH_ScopedInstall(FindIntersection2Lines, 0x4226F0);
    RH_ScopedInstall(FindPathDirection, 0x422090);
    RH_ScopedInstall(ChooseModel, 0x424CE0);
    RH_ScopedInstall(ClitargetOrientationToLink, 0x422760);
    RH_ScopedInstall(CreateConvoy, 0x42C740);
    RH_ScopedInstall(CreatePoliceChase, 0x42C2B0);
    RH_ScopedInstall(DealWithBend_Racing, 0x428040);
    RH_ScopedInstall(DragCarToPoint, 0x42EC90);
    RH_ScopedInstall(FindAngleToWeaveThroughTraffic, 0x4325C0);
    RH_ScopedInstall(FindLinksToGoWithTheseNodes, 0x42B470);
    RH_ScopedInstall(FindMaximumSpeedForThisCarInTraffic, 0x434400);
    RH_ScopedInstall(FindNodesThisCarIsNearestTo, 0x42BD20);
    RH_ScopedInstall(FindPercDependingOnDistToLink, 0x422620);
    RH_ScopedInstall(FindGhostRoadHeight, 0x422370);
    RH_ScopedInstall(FireHeliRocketsAtTarget, 0x42B270);
    RH_ScopedInstall(FlyAIHeliInCertainDirection, 0x429A70);
    RH_ScopedInstall(FlyAIHeliToTarget_FixedOrientation, 0x423940);
    RH_ScopedInstall(FlyAIPlaneInCertainDirection, 0x423000);
    RH_ScopedInstall(GenerateCarCreationCoors2, 0x424210);
    RH_ScopedInstall(GenerateEmergencyServicesCar, 0x42F9C0);
    RH_ScopedInstall(GenerateOneEmergencyServicesCar, 0x42B7D0);
    RH_ScopedInstall(GenerateOneRandomCar, 0x430050);
    RH_ScopedInstall(GetAIHeliToAttackPlayer, 0x42F3C0);
    RH_ScopedInstall(GetAIHeliToFlyInDirection, 0x42A730);
    RH_ScopedInstall(GetAIPlaneToAttackPlayer, 0x429780);
    RH_ScopedInstall(GetAIPlaneToDoDogFight, 0x429890);
    RH_ScopedInstall(GetAIPlaneToDoDogFightAgainstPlayer, 0x42F370);
    RH_ScopedInstall(IsThisAnAppropriateNode, 0x42DAB0);
    RH_ScopedInstall(JoinCarWithRoadSystem, 0x42F5A0);
    RH_ScopedInstall(JoinCarWithRoadSystemGotoCoors, 0x42F870);
    RH_ScopedInstall(PickNextNodeAccordingStrategy, 0x432B10);
    RH_ScopedInstall(MapCouldMoveInThisArea, 0x424100);
    RH_ScopedInstall(PickNextNodeRandomly, 0x42DE80);
    RH_ScopedInstall(PickNextNodeToChaseCar, 0x426EF0);
    RH_ScopedInstall(PickNextNodeToFollowPath, 0x427740);
    RH_ScopedInstall(PossiblyRemoveVehicle, 0x424F80);
    RH_ScopedInstall(ReconsiderRoute, 0x42FC40);
    RH_ScopedInstall(RegisterVehicleOfInterest, 0x423DE0);
    RH_ScopedInstall(ScanForPedDanger, 0x42CE40);
    RH_ScopedInstall(SetCoordsOfScriptCar, 0x4342A0);
    RH_ScopedInstall(SetUpDriverAndPassengersForVehicle, 0x4217C0);
    RH_ScopedInstall(SlowCarDownForCarsSectorList<CPtrListDoubleLink<CVehicle*>>, 0x432420);
    RH_ScopedInstall(SlowCarDownForObjectsSectorList<CPtrListDoubleLink<CObject*>>, 0x42D4F0);
    RH_ScopedInstall(SlowCarDownForOtherCar, 0x42D0E0);
    RH_ScopedInstall(SlowCarDownForPedsSectorList<CPtrListDoubleLink<CPed*>>, 0x425440);
    RH_ScopedInstall(SteerAIBoatWithPhysicsAttackingPlayer, 0x428DE0);
    RH_ScopedInstall(SteerAIBoatWithPhysicsCirclingPlayer, 0x429090);
    RH_ScopedInstall(SteerAIBoatWithPhysicsHeadingForTarget, 0x428BE0);
    RH_ScopedInstall(SteerAICarBlockingPlayerForwardAndBack, 0x422B20);
    RH_ScopedInstall(SteerAICarParkParallel, 0x433BA0);
    RH_ScopedInstall(SteerAICarParkPerpendicular, 0x433EA0);
    RH_ScopedInstall(SteerAICarTowardsPointInEscort, 0x4336D0);
    RH_ScopedInstall(SteerAICarWithPhysics, 0x437C20);
    RH_ScopedInstall(SteerAICarWithPhysicsFollowPath, 0x434900);
    RH_ScopedInstall(SteerAICarWithPhysicsFollowPath_Racing, 0x435830);
    RH_ScopedInstall(SteerAICarWithPhysicsFollowPreRecordedPath, 0x432DD0);
    RH_ScopedInstall(SteerAICarWithPhysicsHeadingForTarget, 0x433280);
    RH_ScopedInstall(SteerAICarWithPhysicsTryingToBlockTarget, 0x4335E0);
    RH_ScopedInstall(SteerAICarWithPhysicsTryingToBlockTarget_Stop, 0x428990);
    RH_ScopedInstall(SteerAICarWithPhysics_OnlyMission, 0x436A90);
    RH_ScopedInstall(SteerAIHeliAsPoliceHeli, 0x42AAD0);
    RH_ScopedInstall(SteerAIHeliFlyingAwayFromPlayer, 0x42ACB0);
    RH_ScopedInstall(SteerAIHeliToCrashAndBurn, 0x4238E0);
    RH_ScopedInstall(SteerAIHeliToFollowEntity, 0x42A750);
    RH_ScopedInstall(SteerAIHeliToKeepEntityInView, 0x42AEB0);
    RH_ScopedInstall(SteerAIHeliToLand, 0x42AD30);
    RH_ScopedInstall(SteerAIHeliTowardsTargetCoors, 0x42A630);
    RH_ScopedInstall(SteerAIPlaneToCrashAndBurn, 0x423880);
    RH_ScopedInstall(SteerAIPlaneToFollowEntity, 0x4237F0);
    RH_ScopedInstall(SteerAIPlaneTowardsTargetCoors, 0x423790);
    RH_ScopedInstall(StopCarIfNodesAreInvalid, 0x422590);
    RH_ScopedInstall(SwitchBetweenPhysicsAndGhost, 0x4222A0);
    RH_ScopedInstall(SwitchVehicleToRealPhysics, 0x423FC0);
    RH_ScopedInstall(TestCollisionBetween2MovingRects, 0x425B30);
    RH_ScopedInstall(TestCollisionBetween2MovingRects_OnlyFrontBumper, 0x425F70);
    RH_ScopedInstall(TestWhetherToFirePlaneGuns, 0x429520);
    RH_ScopedInstall(ThisVehicleShouldTryNotToTurn, 0x421FE0);
    RH_ScopedInstall(TriggerDogFightMoves, 0x429300);
    RH_ScopedInstall(UpdateCarCount, 0x424000);
    RH_ScopedInstall(UpdateCarOnRails, 0x436540);
    RH_ScopedInstall(WeaveForObject, 0x426BC0);
    RH_ScopedInstall(WeaveForOtherCar, 0x426350);
    RH_ScopedInstall(WeaveForPed, 0x426970);
    RH_ScopedInstall(WeaveThroughCarsSectorList<CPtrListDoubleLink<CVehicle*>>, 0x42D680);
    RH_ScopedInstall(WeaveThroughObjectsSectorList<CPtrListDoubleLink<CObject*>>, 0x42D950);
    RH_ScopedInstall(WeaveThroughPedsSectorList<CPtrListDoubleLink<CPed*>>, 0x42D7E0);
    RH_ScopedInstall(ChooseGangCarModel, 0x421A40);
    RH_ScopedInstall(ChoosePoliceCarModel, 0x421980);
    RH_ScopedInstall(CreateCarForScript, 0x431F80);
    RH_ScopedInstall(ChooseBoatModel, 0x421970);
    RH_ScopedInstall(ChooseCarModelToLoad, 0x421900);
    RH_ScopedInstall(GetNewVehicleDependingOnCarModel, 0x421440);
    RH_ScopedInstall(IsAnyoneParking, 0x42C250);
    RH_ScopedInstall(IsThisVehicleInteresting, 0x423EA0);
    RH_ScopedInstall(ClearInterestingVehicleList, 0x423F00);
    RH_ScopedInstall(JoinCarWithRoadAccordingToMission, 0x432CB0);
    RH_ScopedInstall(PossiblyFireHSMissile, 0x429600);
    RH_ScopedInstall(PruneVehiclesOfInterest, 0x423F10);
    RH_ScopedInstall(RemoveCarsIfThePoolGetsFull, 0x4322B0);
    RH_ScopedInstall(RemoveDistantCars, 0x42CD10);
    RH_ScopedInstall(RemoveFromInterestingVehicleList, 0x423ED0);
    RH_ScopedInstall(ScriptGenerateOneEmergencyServicesCar, 0x42FBC0);
    RH_ScopedInstall(SlowCarDownForObject, 0x426220);
    RH_ScopedInstall(SlowCarOnRailsDownForTrafficAndLights, 0x434790);
    RH_ScopedInstall(FindMaxSteerAngle, 0x427FE0);
    RH_ScopedInstall(GenerateRandomCars, 0x4341C0);
}

// 0x4212E0
void CCarCtrl::Init() {
    ZoneScoped;

    CarDensityMultiplier = 1.0f;
    NumRandomCars = 0;
    NumLawEnforcerCars = 0;
    NumMissionCars = 0;
    NumParkedCars = 0;
    NumPermanentVehicles = 0;
    NumAmbulancesOnDuty = 0;
    NumFireTrucksOnDuty = 0;

    LastTimeAmbulanceCreated = 0;
    LastTimeFireTruckCreated = 0;
    bAllowEmergencyServicesToBeCreated = true;
    bCarsGeneratedAroundCamera = false;
    CountDownToCarsAtStart = 2;

    TimeNextMadDriverChaseCreated = CGeneral::GetRandomNumberInRange(600.0f, 1200.0f);

    std::ranges::fill(apCarsToKeep, nullptr);
    for (auto& group : CPopulation::m_LoadedGangCars) {
        group.Clear();
    }
    CPopulation::m_AppropriateLoadedCars.Clear();
    CPopulation::m_InAppropriateLoadedCars.Clear();
    CPopulation::m_LoadedBoats.Clear();
}

// 0x4213B0
void CCarCtrl::ReInit() {
    CarDensityMultiplier = 1.0f;
    NumRandomCars = 0;
    NumLawEnforcerCars = 0;
    NumMissionCars = 0;
    NumParkedCars = 0;
    NumPermanentVehicles = 0;
    NumAmbulancesOnDuty = 0;
    NumFireTrucksOnDuty = 0;

    LastTimeLawEnforcerCreated = 0;

    bAllowEmergencyServicesToBeCreated = true;
    CountDownToCarsAtStart = 2;

    std::ranges::fill(apCarsToKeep, nullptr);
    for (auto& group : CPopulation::m_LoadedGangCars) {
        group.Clear();
    }
    CPopulation::m_AppropriateLoadedCars.Clear();
    CPopulation::m_InAppropriateLoadedCars.Clear();
    CPopulation::m_LoadedBoats.Clear();
}

// 0x421970
int32 CCarCtrl::ChooseBoatModel() {
    return CPopulation::m_LoadedBoats.PickLeastUsedModel(1);
}

// 0x421900
int32 CCarCtrl::ChooseCarModelToLoad(int32 groupID) {
    const auto numCarsInGroup = CPopulation::m_nNumCarsInGroup[groupID];
    if (numCarsInGroup > 0) {
        for (auto i = 0; i < 16; i++) { // 16 tries
            const auto model = CPopulation::m_CarGroups[groupID][CGeneral::GetRandomNumberInRange(numCarsInGroup)];
            if (!CStreaming::IsModelLoaded(model)) {
                return model;
            }
        }
    }
    return -1;
}

eModelID CCarCtrl::ChooseGangCarModel(eGangID loadedCarGroupId) {
    return CPopulation::PickGangCar(loadedCarGroupId);
}

// 0x424CE0
int32 CCarCtrl::ChooseModel(int32* arg1) {
    const auto totalCarTypes = static_cast<int32>(
        CPopCycle::m_NumDealers_Cars + CPopCycle::m_NumGangs_Cars + CPopCycle::m_NumCops_Cars + CPopCycle::m_NumOther_Cars
    );
    if (totalCarTypes < 1) {
        return -1;
    }

    if (CCheat::m_aCheatsActive[0x52] && CGeneral::GetRandomNumberInRange(0, 100) != 0) {
        return -1;
    }

    const auto randomValue = CGeneral::GetRandomNumber() / static_cast<float>(UINT16_MAX + 1u);
    if (randomValue < CPopCycle::m_NumDealers_Cars / totalCarTypes && !CCheat::m_aCheatsActive[0x33]) {
        *arg1 = 25;
        const auto model = CPopulation::m_CarGroups[POPCYCLE_CARGROUP_DEALERS][0];
        return CStreaming::IsModelLoaded(model) ? model : -1;
    }

    auto dealerAndGangCars = CPopCycle::m_NumDealers_Cars;
    if (!CCheat::m_aCheatsActive[0x33]) {
        dealerAndGangCars += CPopCycle::m_NumGangs_Cars;
    }
    if ((CCheat::m_aCheatsActive[0x33] || randomValue < dealerAndGangCars / totalCarTypes)
        && !CPopulation::m_bDontCreateRandomGangMembers) {
        const auto gangCarStrength = CPopCycle::m_pCurrZoneInfo->GetSumOfGangDensity();
        if (gangCarStrength < 1) {
            return -1;
        }

        auto gang = CGeneral::GetRandomNumberInRange(gangCarStrength);
        auto gangId = 0;
        while (gang > CPopCycle::m_pCurrZoneInfo->GangStrength[gangId]) {
            gang -= CPopCycle::m_pCurrZoneInfo->GangStrength[gangId++];
        }
        if (CCheat::m_aCheatsActive[0x33]) {
            gangId = CGeneral::GetRandomNumberInRange(0, 9);
        }

        *arg1 = gangId + 14;
        if (const auto model = CPopulation::PickGangCar(static_cast<eGangID>(gangId)); model != MODEL_INVALID) {
            for (auto tries = 0; tries < 10; tries++) {
                const auto candidate = CPopulation::m_CarGroups[gangId + POPCYCLE_CARGROUP_BALLAS][CGeneral::GetRandomNumberInRange(0, 23)];
                if (candidate != CPopulation::m_DefaultModelIDForUnusedSlot && CStreaming::IsModelLoaded(candidate)) {
                    return candidate;
                }
            }
        }
        return -1;
    }

    if (randomValue < (dealerAndGangCars + CPopCycle::m_NumCops_Cars) / totalCarTypes) {
        if (!CPopulation::m_bDontCreateRandomCops && !CGangWars::GangWarFightingGoingOn()) {
            *arg1 = 13;
            return ChoosePoliceCarModel(0);
        }
        return -1;
    }

    *arg1 = 0;
    if (CTheScripts::ForceRandomCarModel != MODEL_INVALID) {
        return CTheScripts::ForceRandomCarModel;
    }
    return CPopulation::m_AppropriateLoadedCars.PickRandomCar(true, false);
}

int32 CCarCtrl::ChoosePoliceCarModel(uint32 ignoreLvpd1Model) {
    CWanted* playerWanted = FindPlayerWanted();
    if (playerWanted->AreSwatRequired()
        && CStreaming::IsModelLoaded(MODEL_ENFORCER)
        && CStreaming::IsModelLoaded(MODEL_SWAT)
    ) {
        if (CGeneral::GetRandomNumberInRange(0, 3) == 2)
            return MODEL_ENFORCER;
    }
    else
    {
        if (playerWanted->AreFbiRequired()
            && CStreaming::IsModelLoaded(MODEL_FBIRANCH)
            && CStreaming::IsModelLoaded(MODEL_FBI))
            return MODEL_FBIRANCH;

        if (playerWanted->AreArmyRequired()
            && CStreaming::IsModelLoaded(MODEL_RHINO)
            && CStreaming::IsModelLoaded(MODEL_BARRACKS)
            && CStreaming::IsModelLoaded(MODEL_ARMY))
            return (CGeneral::GetRandomNumber() < 0x3FFF) + MODEL_RHINO;
    }
    return CStreaming::GetDefaultCopCarModel(ignoreLvpd1Model);
}

// 0x423F00
void CCarCtrl::ClearInterestingVehicleList() {
    // The game stores exactly two protected vehicles in this list.
    apCarsToKeep[0] = nullptr;
    apCarsToKeep[1] = nullptr;
}

// 0x422760
// NOTE: The real name (per Android symbols) is `ClipTargetOrientationToLink` - kept as-is to match the existing header declaration.
// Clips `*targetOrientation` so it doesn't point further away than the link's near/far lane-boundary directions relative to (x, y).
void CCarCtrl::ClitargetOrientationToLink(CVehicle* vehicle, CCarPathLinkAddress linkAddr, int8 dir, float* targetOrientation, float x, float y) {
    if (!ThePaths.m_pNaviNodes[linkAddr.m_wAreaId])
        return;

    const auto& link = ThePaths.GetCarPathLink(linkAddr);
    const auto vehiclePos = vehicle->GetPosition();

    const auto dirX = (float)link.m_dir.x * (float)dir;
    const auto dirY = (float)link.m_dir.y * (float)dir;

    // NOTE: This result is discarded in both binaries - matched here for fidelity.
    CGeneral::GetATanOfXY(x - vehiclePos.x, y - vehiclePos.y);

    // Work out the near/far lane-offset (in lane-widths) from the link centerline
    float laneOffsetNear, laneOffsetFar;
    if (link.m_numOppositeDirLanes != 0 && link.m_numSameDirLanes != 0) {
        uint8 a = link.m_numOppositeDirLanes;
        uint8 b = link.m_numSameDirLanes;
        if (dir)
            std::swap(a, b);
        laneOffsetNear = (float)b;
        laneOffsetFar  = link.GetNodePathWidth() / 5.4f + (float)a;
    } else {
        const auto count = link.m_numOppositeDirLanes != 0 ? link.m_numOppositeDirLanes : link.m_numSameDirLanes;
        laneOffsetNear = laneOffsetFar = (float)count * 0.5f;
    }

    const auto angleFar = CGeneral::GetATanOfXY(
        (x + 5.4f * dirY * (laneOffsetFar - 0.3f)) - vehiclePos.x,
        (y - 5.4f * dirX * (laneOffsetFar - 0.3f)) - vehiclePos.y
    );
    const auto angleNear = CGeneral::GetATanOfXY(
        (x - 5.4f * dirY * (laneOffsetNear - 0.3f)) - vehiclePos.x,
        (y + 5.4f * dirX * (laneOffsetNear - 0.3f)) - vehiclePos.y
    );

    const auto orientation = *targetOrientation;
    auto relFar  = CGeneral::LimitRadianAngle(angleFar - orientation);
    auto relNear = CGeneral::LimitRadianAngle(angleNear - orientation);

    // Only clip if at least one of the two candidate deltas points the "right" way (both wedge sides ahead)
    if (relFar >= 0.0f || relNear >= 0.0f) {
        if (relFar > 0.0f && relNear > 0.0f)
            relFar = std::min(relFar, relNear);

        *targetOrientation = orientation + relFar;
    }

    // Normalize the (possibly untouched) result into [0, 2*PI)
    while (*targetOrientation < 0.0f)
        *targetOrientation += TWO_PI;
    while (*targetOrientation >= TWO_PI)
        *targetOrientation -= TWO_PI;
}

// 0x431F80
CVehicle* CCarCtrl::CreateCarForScript(int32 modelid, CVector posn, bool doMissionCleanup) {
    if (CModelInfo::IsBoatModel(modelid))
    {
        auto* boat = new CBoat(modelid, eVehicleCreatedBy::MISSION_VEHICLE);
        if (posn.z <= MAP_Z_LOW_LIMIT)
            posn.z = CWorld::FindGroundZForCoord(posn.x, posn.y);

        posn.z += boat->GetDistanceFromCentreOfMassToBaseOfModel();
        boat->SetPosn(posn);

        CTheScripts::ClearSpaceForMissionEntity(posn, boat);
        boat->vehicleFlags.bEngineOn = false;
        boat->vehicleFlags.bIsLocked = true;
        boat->SetStatus(STATUS_ABANDONED);
        JoinCarWithRoadSystem(boat);

        boat->m_autoPilot.SetCarMission(eCarMission::MISSION_NONE);
        boat->m_autoPilot.m_nTempAction = TEMPACT_NONE;
        boat->m_autoPilot.m_speed = 20.0F;
        boat->m_autoPilot.SetCruiseSpeed(20);

        if (doMissionCleanup)
            boat->m_bIsStaticWaitingForCollision = true;

        boat->m_autoPilot.movementFlags.bIsStopped = true;
        CWorld::Add(boat);

        if (doMissionCleanup)
            CTheScripts::MissionCleanUp.AddEntityToList(GetVehiclePool()->GetRef(boat), MISSION_CLEANUP_ENTITY_TYPE_VEHICLE);

        return boat;
    }

    auto* vehicle = GetNewVehicleDependingOnCarModel(modelid, eVehicleCreatedBy::MISSION_VEHICLE);
    if (posn.z <= MAP_Z_LOW_LIMIT)
        posn.z = CWorld::FindGroundZForCoord(posn.x, posn.y);

    posn.z += vehicle->GetDistanceFromCentreOfMassToBaseOfModel();
    vehicle->SetPosn(posn);

    if (!doMissionCleanup)
    {
        if (vehicle->IsAutomobile())
            vehicle->AsAutomobile()->PlaceOnRoadProperly();
        else if (vehicle->IsBike())
            vehicle->AsBike()->PlaceOnRoadProperly();
    }

    if (vehicle->IsTrain())
        vehicle->AsTrain()->trainFlags.bNotOnARailRoad = true;

    CTheScripts::ClearSpaceForMissionEntity(posn, vehicle);
    vehicle->vehicleFlags.bIsLocked = true;
    vehicle->SetStatus(STATUS_ABANDONED);
    JoinCarWithRoadSystem(vehicle);
    vehicle->vehicleFlags.bEngineOn = false;
    vehicle->vehicleFlags.bHasBeenOwnedByPlayer = true;

    vehicle->m_autoPilot.SetCarMission(eCarMission::MISSION_NONE);
    vehicle->m_autoPilot.m_nTempAction = TEMPACT_NONE;
    vehicle->m_autoPilot.m_nCarDrivingStyle = DRIVING_STYLE_STOP_FOR_CARS;
    vehicle->m_autoPilot.m_speed = 13.0F;
    vehicle->m_autoPilot.SetCruiseSpeed(13);
    vehicle->m_autoPilot.m_nCurrentLane = 0;
    vehicle->m_autoPilot.m_nNextLane = 0;

    if (doMissionCleanup)
        vehicle->m_bIsStaticWaitingForCollision = true;

    CWorld::Add(vehicle);
    if (doMissionCleanup)
        CTheScripts::MissionCleanUp.AddEntityToList(GetVehiclePool()->GetRef(vehicle), MISSION_CLEANUP_ENTITY_TYPE_VEHICLE);

    if (vehicle->IsSubRoadVehicle())
        vehicle->m_autoPilot.movementFlags.bIsStopped = true;

    return vehicle;
}

// 0x42C740
// Spawns 2-3 vehicles of the same model as `vehicle`, chained one behind the other along its forward
// direction, each carrying a driver. Used to create escort/convoy vehicles (e.g. FBI convoy).
bool CCarCtrl::CreateConvoy(CVehicle* vehicle, int32 driverType) {
    const auto modelId  = vehicle->m_nModelIndex;
    const auto boundRadius = vehicle->GetColModel()->GetBoundRadius();

    auto numToSpawn = 2;
    if (CGeneral::GetRandomNumber() * RAND_MAX_FLOAT_RECIPROCAL * 100.0f > 50.0f)
        numToSpawn = 3;

    const auto spacing = boundRadius * 2.0f + 2.5f;

    auto  success   = false;
    auto* chainTail = vehicle;
    for (auto i = 0; i < numToSpawn; i++) {
        auto* newCar = GetNewVehicleDependingOnCarModel(modelId, RANDOM_VEHICLE);
        if (!newCar)
            continue;

        auto tailMatrix = chainTail->GetMatrix();
        auto spawnPos = chainTail->GetPosition() - tailMatrix.GetForward() * (spacing * (float)(i + 1));

        // NOTE: The original probes for ground height with two candidate rays picking the closer result;
        // simplified here to the equivalent single-probe helper already used for this purpose elsewhere.
        spawnPos.z = CWorld::FindGroundZForCoord(spawnPos.x, spawnPos.y);

        if (spawnPos.z <= 1000000000.0f) {
            spawnPos.z += newCar->GetDistanceFromCentreOfMassToBaseOfModel();

            int16 numColliding = 0;
            CWorld::FindObjectsKindaColliding(spawnPos, boundRadius, true, &numColliding, 2, nullptr, false, true, true, false, false);

            if (numColliding == 0) {
                if (!success) {
                    // First successfully-placed vehicle: give the convoy leader some forward move-speed.
                    chainTail->SetStatus(STATUS_PHYSICS);
                    chainTail->m_vecMoveSpeed = tailMatrix.GetForward() * 0.02f;
                }

                newCar->SetMatrix(tailMatrix);
                newCar->SetPosn(spawnPos);
                newCar->vehicleFlags.bIsLocked = true;
                CWorld::Add(newCar);

                if (newCar->IsAutomobile())
                    newCar->AsAutomobile()->PlaceOnRoadProperly();
                else if (newCar->IsBike())
                    newCar->AsBike()->PlaceOnRoadProperly();

                success = true;
                SetUpDriverAndPassengersForVehicle(newCar, driverType, 0, true, false, 99);

                // Chain: the new car follows `chainTail`
                chainTail->RegisterReference(reinterpret_cast<CEntity**>(&newCar->m_autoPilot.m_TargetEntity));
                chainTail = newCar;
                continue;
            }
        }

        delete newCar;
    }
    return success;
}

// 0x42C2B0
// Spawns a single police car at `nodeAddress` (probabilistically gated, plus a driver-type-based gang-car
// check) to go after `vehicle`, and marks its driver as hostile towards the player.
bool CCarCtrl::CreatePoliceChase(CVehicle* vehicle, int32 driverType, CNodeAddress nodeAddress) {
    const auto bSkipRandomGate = (uint32)(driverType - 0xE) < 10u || CPopCycle::m_NumGangs_Cars == 0.0f;
    if (!bSkipRandomGate && (int32)(CGeneral::GetRandomNumber() * RAND_MAX_FLOAT_RECIPROCAL * 4.0f) != 0)
        return false;

    const auto modelId = ChoosePoliceCarModel(0);
    if (modelId < 0 || !CStreaming::IsModelLoaded(modelId))
        return false;

    auto* newCar = GetNewVehicleDependingOnCarModel(modelId, RANDOM_VEHICLE);
    if (!newCar)
        return false;

    auto spawnPos = ThePaths.GetPathNode(nodeAddress)->GetPosition();
    // NOTE: simplified from the original's two-probe ground-height search (see CreateConvoy).
    spawnPos.z = CWorld::FindGroundZForCoord(spawnPos.x, spawnPos.y);

    if (spawnPos.z > 1000000000.0f) {
        delete newCar;
        return false;
    }

    spawnPos.z += newCar->GetDistanceFromCentreOfMassToBaseOfModel();
    const auto boundRadius = newCar->GetColModel()->GetBoundRadius();

    int16 numColliding = 0;
    if (!vehicle->GetIsOnScreen() && TheCamera.IsSphereVisible(spawnPos, boundRadius)) {
        numColliding = 1; // Don't spawn where the player could see it appear out of nowhere.
    } else {
        CWorld::FindObjectsKindaColliding(spawnPos, boundRadius, true, &numColliding, 2, nullptr, false, true, true, false, false);
    }

    if (numColliding != 0) {
        delete newCar;
        return false;
    }

    newCar->SetPosn(spawnPos);
    newCar->vehicleFlags.bIsLocked = true;
    SetUpDriverAndPassengersForVehicle(newCar, driverType, 2, true, true, 99);

    if (newCar->IsAutomobile())
        newCar->AsAutomobile()->PlaceOnRoadProperly();
    else if (newCar->IsBike())
        newCar->AsBike()->PlaceOnRoadProperly();

    CWorld::Add(newCar);
    newCar->SetStatus(STATUS_PHYSICS);

    // NOTE: The original additionally marks the new car's driver (and passengers) as hostile towards the
    // player via a CEventAcquaintancePedHate, and registers the vehicle for wanted-level pursuit tracking.
    // Neither the ped-event system nor that pursuit-tracking field are reversed in this codebase yet, so
    // this behavior is intentionally left out here rather than guessed at.

    return true;
}

// 0x428040
// Detects a sharp bend between the vehicle's current link (link1) and its upcoming link (link2, possibly
// advanced to link3/link4 if those turn even sharper close by), and if found, computes a cutting point
// (`pos`) and heading (`arg11`) that steers the racing line through the bend, plus a speed factor (`arg12`).
// NOTE: This is an extremely dense piece of racing-line geometry. Translated from the (much cleaner)
// Android disassembly, using already-reversed helpers where they matched; treat with extra scrutiny.
bool CCarCtrl::DealWithBend_Racing(CVehicle* vehicle, CCarPathLinkAddress LinkAddress1, CCarPathLinkAddress LinkAddress2, CCarPathLinkAddress LinkAddress3, CCarPathLinkAddress LinkAddress4, char dir1, char dir2, char dir3, char dir4, float timeStep, float* outHeading, float* outSpeedFactor, float* outAngleDiff, float* outDist, CVector* outPos) {
    if (!LinkAddress1.IsValid() || !LinkAddress2.IsValid())
        return false;

    *outSpeedFactor = 1.0f;

    if (!ThePaths.m_pNaviNodes[LinkAddress1.m_wAreaId] || !ThePaths.m_pNaviNodes[LinkAddress2.m_wAreaId])
        return false;

    const auto& link1 = ThePaths.GetCarPathLink(LinkAddress1);
    const auto  vehiclePos = vehicle->GetPosition();
    const auto  link1Coors = link1.GetNodeCoors();

    *outDist = (link1Coors - CVector2D{ vehiclePos.x, vehiclePos.y }).Magnitude();
    if (*outDist > 60.0f)
        return false;

    auto* targetLink = &ThePaths.GetCarPathLink(LinkAddress2);
    auto  targetDir  = dir2;

    if (LinkAddress3.IsValid()) {
        auto& link3 = ThePaths.GetCarPathLink(LinkAddress3);
        if ((link3.GetNodeCoors() - targetLink->GetNodeCoors()).SquaredMagnitude() < 100.0f) {
            targetLink = &link3;
            targetDir  = dir3;

            if (LinkAddress4.IsValid()) {
                auto& link4 = ThePaths.GetCarPathLink(LinkAddress4);
                if ((link4.GetNodeCoors() - targetLink->GetNodeCoors()).SquaredMagnitude() < 100.0f) {
                    targetLink = &link4;
                    targetDir  = dir4;
                }
            }
        }
    }

    const auto targetDirX = (float)targetLink->m_dir.x * (float)targetDir;
    const auto targetDirY = (float)targetLink->m_dir.y * (float)targetDir;

    const auto link1Heading   = CGeneral::GetATanOfXY(link1.m_dir.x, link1.m_dir.y);
    const auto targetHeading  = CGeneral::GetATanOfXY(targetDirX, targetDirY);

    auto angleDiff = link1Heading - targetHeading;
    while (angleDiff > PI)  angleDiff -= TWO_PI;
    while (angleDiff < -PI) angleDiff += TWO_PI;
    *outAngleDiff = angleDiff;

    const auto bIsBmx = vehicle->m_nVehicleSubType == VEHICLE_TYPE_BMX;
    const auto angleThreshold1 = bIsBmx ? 0.34906587f : 0.52359879f;
    const auto angleThreshold2 = bIsBmx ? 0.69813174f : 1.04719758f;
    const auto distThreshold   = bIsBmx ? 50.0f : 40.0f;

    const auto link1LaneSum   = link1.m_numOppositeDirLanes + link1.m_numSameDirLanes;
    const auto targetLaneSum  = targetLink->m_numOppositeDirLanes + targetLink->m_numSameDirLanes;

    const auto bBend = (std::abs(angleDiff) >= angleThreshold1 && (link1LaneSum < 4 || std::abs(angleDiff) >= angleThreshold2))
                     || targetLaneSum < 4;
    if (!bBend)
        return false;

    // Link1's own two lane-boundary corner points (biased by dir1), whichever is closer to targetLink's node
    // becomes the "near" corner and the other the "far" corner (then clipped to at most 11 units apart).
    const auto link1DirX = (float)link1.m_dir.x * (float)dir1;
    const auto link1DirY = (float)link1.m_dir.y * (float)dir1;
    const auto link1OppOffset  = link1.OneWayLaneOffset() + (float)link1.m_numOppositeDirLanes - 1.0f;
    const auto link1SameOffset = link1.OneWayLaneOffset() + (float)link1.m_numSameDirLanes - 1.0f;
    const CVector2D link1CornerA{ link1Coors.x + 5.4f * link1DirY * link1OppOffset,  link1Coors.y - 5.4f * link1DirX * link1OppOffset };
    const CVector2D link1CornerB{ link1Coors.x - 5.4f * link1DirY * link1SameOffset, link1Coors.y + 5.4f * link1DirX * link1SameOffset };

    auto nearPoint = link1CornerB, farPoint = link1CornerA;
    if ((link1CornerB - targetLink->GetNodeCoors()).SquaredMagnitude() <= (link1CornerA - targetLink->GetNodeCoors()).SquaredMagnitude()) {
        nearPoint = link1CornerA;
        farPoint  = link1CornerB;
    }

    if (const auto len = (farPoint - nearPoint).Magnitude(); len > 11.0f)
        farPoint = nearPoint + (farPoint - nearPoint) * (11.0f / len);

    // Same idea for targetLink's two corners (biased by targetDir), picking whichever is closer to link1's node.
    const auto targetOppOffset  = targetLink->OneWayLaneOffset() + (float)targetLink->m_numOppositeDirLanes - 1.0f;
    const auto targetSameOffset = targetLink->OneWayLaneOffset() + (float)targetLink->m_numSameDirLanes - 1.0f;
    const CVector2D targetCornerA{ targetLink->GetNodeCoors().x + 5.4f * targetDirY * targetOppOffset,  targetLink->GetNodeCoors().y - 5.4f * targetDirX * targetOppOffset };
    const CVector2D targetCornerB{ targetLink->GetNodeCoors().x - 5.4f * targetDirY * targetSameOffset, targetLink->GetNodeCoors().y + 5.4f * targetDirX * targetSameOffset };

    auto targetNearPoint = targetCornerB;
    if ((targetCornerA - link1Coors).SquaredMagnitude() <= (targetCornerB - link1Coors).SquaredMagnitude())
        targetNearPoint = targetCornerA;

    CVector2D cutPoint;
    if (std::abs(link1DirX - targetDirX) >= 0.1f || link1DirY - targetDirY == 0.0f) {
        float ix = 0.0f, iy = 0.0f;
        FindIntersection2Lines(farPoint.x, farPoint.y, link1DirX, link1DirY, targetNearPoint.x, targetNearPoint.y, targetDirX, targetDirY, &ix, &iy);
        cutPoint = { ix, iy };
    } else {
        cutPoint = farPoint;
    }

    *outPos = CVector{ cutPoint.x, cutPoint.y, outPos->z };

    const auto clippedLen = (farPoint - nearPoint).Magnitude();
    if (*outDist < clippedLen) {
        const auto turnSharpness = std::min(std::abs(2.0f * angleDiff / *outDist), 1.0f) * 0.6f;
        *outSpeedFactor = 1.0f - turnSharpness * (clippedLen - *outDist) / clippedLen;
    }

    // NOTE: the original blends the returned heading between the cut point and an un-clipped corner point
    // (based on `timeStep`, easing into the bend); simplified here to always use the cut point.
    *outHeading = CGeneral::GetATanOfXY(outPos->x - vehiclePos.x, outPos->y - vehiclePos.y);
    return true;
}

// 0x42EC90
// Directly repositions/re-orients the vehicle's matrix so it "drags" towards `pos`, by probing ground
// height at its front and rear and rebuilding the matrix from those two ground-contact points.
// NOTE: skips the original's stored-collision-poly cache (probing fresh each call instead - functionally
// equivalent, just slower) since that cache isn't reversed in this codebase yet.
void CCarCtrl::DragCarToPoint(CVehicle* vehicle, CVector* pos) {
    auto& matrix = vehicle->GetMatrix();
    const auto  vehPos = matrix.GetPosition();
    const auto  halfLength = vehicle->GetColModel()->GetBoundingBox().GetWidth() * 0.95f * 0.5f;

    // Clamp the target point onto the segment running along the vehicle's right axis, 1.5x its length
    // in front of its left edge (mirrors the original's clip-to-segment-else-project fallback).
    const CVector2D segStart{ vehPos.x - halfLength * matrix.GetRight().x, vehPos.y - halfLength * matrix.GetRight().y };
    const CVector2D segEnd{ segStart.x + matrix.GetRight().x * halfLength * 3.0f, segStart.y + matrix.GetRight().y * halfLength * 3.0f };
    const CVector2D target{ pos->x, pos->y };

    const auto segDelta = segEnd - segStart;
    const auto segLenSq = segDelta.SquaredMagnitude();
    const auto t = segLenSq > 0.0f ? std::clamp((target - segStart).Dot(segDelta) / segLenSq, 0.0f, 1.0f) : 0.0f;
    const CVector2D clippedTarget = segStart + segDelta * t;

    // NOTE: the original widens this search half-width to 100 after 16 consecutive calls with no valid
    // ground hit (a per-vehicle "stuck" counter); not modeled here since that counter field isn't reversed.
    constexpr auto kSearchHalfWidth = 3.0f;

    CEntity* hitEntity{};
    CColPoint colPoint{};

    // Front ground Z (at the clamped target point)
    if (CWorld::ProcessVerticalLine({ clippedTarget.x, clippedTarget.y, vehPos.z + kSearchHalfWidth }, -(kSearchHalfWidth * 2.0f), colPoint, hitEntity, true, true, false, true, true, false, nullptr))
        vehicle->m_fVehicleFrontGroundZ = colPoint.m_vecPoint.z;

    // Rear ground Z (at the vehicle's own rear-axle-ish position, mirrored across the vehicle center)
    const CVector2D rearPoint = target - (clippedTarget - CVector2D{ vehPos.x, vehPos.y });
    if (CWorld::ProcessVerticalLine({ rearPoint.x, rearPoint.y, vehPos.z + kSearchHalfWidth }, -(kSearchHalfWidth * 2.0f), colPoint, hitEntity, true, true, false, true, true, false, nullptr))
        vehicle->m_fVehicleRearGroundZ = colPoint.m_vecPoint.z;

    const auto frontZ = vehicle->m_fVehicleFrontGroundZ;
    const auto rearZ  = vehicle->m_fVehicleRearGroundZ;

    // Rebuild the matrix from the front/rear ground-contact points
    CVector forward{ (target.y - rearPoint.y) / halfLength, -(target.x - clippedTarget.x) / halfLength, 0.0f };
    forward.z = (frontZ - rearZ) / halfLength;
    forward.Normalise();

    matrix.GetForward() = forward;
    matrix.GetRight()   = CrossProduct(forward, CVector{ 0.0f, 0.0f, 1.0f });
    matrix.GetUp()      = CrossProduct(matrix.GetRight(), forward);
    matrix.GetPosition() = CVector{
        (clippedTarget.x + target.x) * 0.5f,
        (rearPoint.y + target.y) * 0.5f,
        vehicle->GetDistanceFromCentreOfMassToBaseOfModel() + (frontZ + rearZ) * 0.5f
    };

    vehicle->UpdateLightingFromStoredPolys();
}

// 0x4325C0
float CCarCtrl::FindAngleToWeaveThroughTraffic(CVehicle* vehicle, CPhysical* physical, float arg3, float arg4, float arg5) {
    const auto distanceToTest = std::min(2.0f, vehicle->GetMoveSpeed().Magnitude2D() * 2.5f + 1.0f) * arg5 * 12.0f;
    const auto vehiclePos = vehicle->GetPosition();
    const auto left = vehiclePos.x - distanceToTest;
    const auto right = vehiclePos.x + distanceToTest;
    const auto top = vehiclePos.y - distanceToTest;
    const auto bottom = vehiclePos.y + distanceToTest;
    const auto xStart = std::max(0, CWorld::GetSectorX(left));
    const auto xEnd = std::min(MAX_SECTORS_X - 1, CWorld::GetSectorX(right));
    const auto yStart = std::max(0, CWorld::GetSectorY(top));
    const auto yEnd = std::min(MAX_SECTORS_Y - 1, CWorld::GetSectorY(bottom));
    auto angleToWeaveLeft = arg3;
    auto angleToWeaveRight = arg3;

    CWorld::AdvanceCurrentScanCode();
    float previousLeft = -9999.9f;
    float previousRight = -9999.9f;
    while (angleToWeaveLeft != previousLeft || angleToWeaveRight != previousRight) {
        previousLeft = angleToWeaveLeft;
        previousRight = angleToWeaveRight;
        for (auto y = yStart; y <= yEnd; y++) {
            for (auto x = xStart; x <= xEnd; x++) {
                auto& sector = CWorld::GetRepeatSector(x, y);
                WeaveThroughCarsSectorList(sector.Vehicles, vehicle, physical, left, top, right, bottom, &angleToWeaveLeft, &angleToWeaveRight);
                WeaveThroughPedsSectorList(sector.Peds, vehicle, physical, left, top, right, bottom, &angleToWeaveLeft, &angleToWeaveRight);
                WeaveThroughObjectsSectorList(sector.Objects, vehicle, left, top, right, bottom, &angleToWeaveLeft, &angleToWeaveRight);
            }
        }
    }

    const auto angleDiffFromActualToTarget = CGeneral::LimitRadianAngle(arg4 - arg3);
    const auto angleToBisectActualToTarget = CGeneral::LimitRadianAngle(arg3 + angleDiffFromActualToTarget * 0.5f);
    const auto angleDiffLeft = std::abs(CGeneral::LimitRadianAngle(angleToWeaveLeft - angleToBisectActualToTarget));
    const auto angleDiffRight = std::abs(CGeneral::LimitRadianAngle(angleToWeaveRight - angleToBisectActualToTarget));
    if (angleDiffLeft > HALF_PI && angleDiffRight > HALF_PI)
        return angleToBisectActualToTarget;
    if (std::abs(angleDiffLeft - angleDiffRight) < 0.08f)
        return angleToWeaveRight;
    return angleDiffLeft < angleDiffRight ? angleToWeaveLeft : angleToWeaveRight;
}

// 0x4226F0
void CCarCtrl::FindIntersection2Lines(float arg1, float arg2, float arg3, float arg4, float arg5, float arg6, float arg7, float arg8, float* arg9, float* arg10) {
    const float denominator = arg3 * arg8 - arg4 * arg7;
    const float t = denominator == 0.0f ? 0.0f : ((arg5 - arg1) * arg8 - (arg6 - arg2) * arg7) / denominator;
    *arg9 = arg3 * t + arg1;
    *arg10 = arg4 * t + arg2;
}

// 0x42B470
void CCarCtrl::FindLinksToGoWithTheseNodes(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;
    if (vehicle->m_nForcedRandomRouteSeed) {
        srand(vehicle->m_nForcedRandomRouteSeed);
    }

    const auto currentAddress = autoPilot.m_currentAddress;
    const auto nextAddress = autoPilot.m_startingRouteNode;
    const auto* currentNode = ThePaths.GetPathNode(currentAddress);
    if (!currentNode || !nextAddress.IsValid() || !ThePaths.GetPathNode(nextAddress)) {
        return;
    }

    autoPilot.m_nNextPathNodeInfo = ThePaths.FindLinkBetweenNodes(currentAddress, nextAddress);
    const auto direction = [](CNodeAddress from, CNodeAddress to) {
        if (to.m_wAreaId != from.m_wAreaId) {
            return to.m_wAreaId > from.m_wAreaId ? int8{1} : int8{-1};
        }
        return to.m_wNodeId >= from.m_wNodeId ? int8{1} : int8{-1};
    };
    autoPilot.m_nNextLane = direction(currentAddress, nextAddress);

    CNodeAddress firstLinkedAddress{};
    CNodeAddress closestAddress{};
    auto closestDistance = std::numeric_limits<float>::max();
    size_t linkedNodeCount = 0;
    bool foundClosest = false;
    for (const auto& linkedNode : ThePaths.GetNodeLinkedNodes(*currentNode)) {
        const auto candidateAddress = linkedNode.GetAddress();
        if (linkedNodeCount++ == 0) {
            firstLinkedAddress = candidateAddress;
        }
        if (candidateAddress == nextAddress) {
            continue;
        }
        const auto distance = CCollision::DistToLine(
            currentNode->GetPosition(),
            linkedNode.GetPosition(),
            vehicle->GetPosition()
        );
        if (distance < closestDistance) {
            closestDistance = distance;
            closestAddress = candidateAddress;
            foundClosest = true;
        }
    }
    if (linkedNodeCount == 0) {
        return;
    }
    if (linkedNodeCount == 1 || !foundClosest) {
        closestAddress = firstLinkedAddress;
    }

    autoPilot.m_nCurrentPathNodeInfo = ThePaths.FindLinkBetweenNodes(currentAddress, closestAddress);
    autoPilot.m_nCurrentLane = direction(currentAddress, closestAddress);
}

// 0x434400
float CCarCtrl::FindMaximumSpeedForThisCarInTraffic(CVehicle* vehicle) {
    const auto& autoPilot = vehicle->m_autoPilot;
    if (autoPilot.m_nCarDrivingStyle == DRIVING_STYLE_AVOID_CARS
        || autoPilot.m_nCarDrivingStyle == DRIVING_STYLE_PLOUGH_THROUGH
        || autoPilot.m_nCarDrivingStyle == DRIVING_STYLE_DRIVINGMODE_AVOIDCARS_OBEYLIGHTS) {
        return autoPilot.m_fMaxTrafficSpeed;
    }

    const auto vehiclePos = vehicle->GetPosition();
    const auto left = vehiclePos.x - 14.0f;
    const auto right = vehiclePos.x + 14.0f;
    const auto top = vehiclePos.y - 14.0f;
    const auto bottom = vehiclePos.y + 14.0f;
    const auto xStart = std::max(0, CWorld::GetSectorX(left));
    const auto xEnd = std::min(MAX_SECTORS_X - 1, CWorld::GetSectorX(right));
    const auto yStart = std::max(0, CWorld::GetSectorY(top));
    const auto yEnd = std::min(MAX_SECTORS_Y - 1, CWorld::GetSectorY(bottom));
    const auto trafficSpeed = autoPilot.m_fMaxTrafficSpeed * autoPilot.m_SpeedMult;
    auto maxSpeed = trafficSpeed;

    CWorld::AdvanceCurrentScanCode();
    for (auto y = yStart; y <= yEnd; y++) {
        for (auto x = xStart; x <= xEnd; x++) {
            auto& sector = CWorld::GetRepeatSector(x, y);
            if (autoPilot.m_nCarDrivingStyle != DRIVING_STYLE_DRIVINGMODE_AVOIDCARS_STOPFORPEDS_OBEYLIGHTS) {
                SlowCarDownForCarsSectorList(sector.Vehicles, vehicle, left, top, right, bottom, &maxSpeed, trafficSpeed);
            }
            SlowCarDownForPedsSectorList(sector.Peds, vehicle, left, top, right, bottom, &maxSpeed, trafficSpeed);
            SlowCarDownForObjectsSectorList(sector.Objects, vehicle, left, top, right, bottom, &maxSpeed, trafficSpeed);
        }
    }

    vehicle->vehicleFlags.bWarnedPeds = true;
    if (autoPilot.m_nCarDrivingStyle == DRIVING_STYLE_STOP_FOR_CARS
        || autoPilot.m_nCarDrivingStyle == DRIVING_STYLE_STOP_FOR_CARS_IGNORE_LIGHTS) {
        return maxSpeed;
    }
    return (maxSpeed + trafficSpeed) * 0.5f;
}

// 0x42BD20
void CCarCtrl::FindNodesThisCarIsNearestTo(CVehicle* vehicle, CNodeAddress& nodeAddress1, CNodeAddress& nodeAddress2) {
    nodeAddress1 = {};
    nodeAddress2 = {};

    const auto position = vehicle->GetPosition();
    const auto regionX = ThePaths.FindXRegionForCoors(position.x);
    const auto regionY = ThePaths.FindYRegionForCoors(position.y);
    const auto regionOriginX = ThePaths.FindXCoorsForRegion(regionX);
    const auto regionOriginY = ThePaths.FindYCoorsForRegion(regionY);
    const auto xInRegion = position.x - regionOriginX;
    const auto yInRegion = position.y - regionOriginY;
    const auto minRegionX = std::max(0, static_cast<int32>(regionX) - (xInRegion < 200.0f));
    const auto maxRegionX = std::min(7, static_cast<int32>(regionX) + (xInRegion > 550.0f));
    const auto minRegionY = std::max(0, static_cast<int32>(regionY) - (yInRegion < 200.0f));
    const auto maxRegionY = std::min(7, static_cast<int32>(regionY) + (yInRegion > 550.0f));

    const auto vehicleForward = vehicle->GetMatrix().GetForward();
    auto bestScore = 99999.9f;
    for (auto y = minRegionY; y <= maxRegionY; ++y) {
        for (auto x = minRegionX; x <= maxRegionX; ++x) {
            const auto area = CPathFind::GetAreaIdFromXY(x, y);
            if (!ThePaths.IsAreaLoaded(area)) {
                continue;
            }
            for (auto nodeIndex = 0u; nodeIndex < ThePaths.m_anNumVehicleNodes[area]; ++nodeIndex) {
                const auto& node = ThePaths.m_pPathNodes[area][nodeIndex];
                const auto nodePos = node.GetPosition();
                if ((nodePos - position).Magnitude() >= 150.0f || !node.m_nNumLinks) {
                    continue;
                }

                for (const auto& linkedNode : ThePaths.GetNodeLinkedNodes(node)) {
                    const auto linkedAddress = linkedNode.GetAddress();
                    const auto linkedPos = linkedNode.GetPosition();
                    const auto distance = CCollision::DistToLine(nodePos, linkedPos, position);
                    const auto pathDirection = (linkedPos - nodePos).Normalized();
                    const auto facingPenalty = 1.0f - DotProduct(pathDirection, vehicleForward);
                    const auto score = distance + facingPenalty * 5.0f;
                    if (score < bestScore) {
                        bestScore = score;
                        nodeAddress1 = node.GetAddress();
                        nodeAddress2 = linkedAddress;
                    }
                }
            }
        }
    }
}

// 0x422090
int8 CCarCtrl::FindPathDirection(CNodeAddress nodeAddress1, CNodeAddress nodeAddress2, CNodeAddress nodeAddress3, bool* arg4) {
    *arg4 = false;
    if (!nodeAddress1.IsValid() || !nodeAddress2.IsValid() || !nodeAddress3.IsValid()) {
        return 0;
    }
    const auto* prev = ThePaths.GetPathNode(nodeAddress1);
    const auto* cur = ThePaths.GetPathNode(nodeAddress2);
    const auto* next = ThePaths.GetPathNode(nodeAddress3);
    if (!prev || !cur || !next) {
        return 0;
    }
    const auto a = cur->GetPosition() - prev->GetPosition();
    const auto b = next->GetPosition() - cur->GetPosition();
    const float aLen = std::sqrt(a.x * a.x + a.y * a.y);
    const float bLen = std::sqrt(b.x * b.x + b.y * b.y);
    if (aLen == 0.0f || bLen == 0.0f) {
        return 0;
    }
    const float turn = (a.x * b.y - a.y * b.x) / (aLen * bLen);
    if (turn > 0.77f) return 1;
    if (turn < -0.77f) return 2;
    return 4;
}

// 0x422620
float CCarCtrl::FindPercDependingOnDistToLink(CVehicle* vehicle, CCarPathLinkAddress linkAddress) {
    const auto& link = ThePaths.GetCarPathLink(linkAddress);
    const auto distance = (link.GetNodeCoors() - CVector2D{ vehicle->GetPosition() }).Magnitude();
    if (distance < 5.0f) {
        return 0.5f;
    }
    if (distance >= 15.0f) {
        return 1.0f;
    }
    return (distance - 5.0f) * 0.05f + 0.5f;
}

// 0x421770
int32 CCarCtrl::FindSequenceElement(int32 arg1) {
    if (bSequenceOtherWay) {
        return (arg1 + SequenceRandomOffset) % SequenceElements;
    }
    return (SequenceElements - arg1 + SequenceRandomOffset) % SequenceElements;
}

// 0x4224E0
float CCarCtrl::FindSpeedMultiplier(float arg1, float arg2, float arg3, float arg4) {
    while (arg1 < -PI) arg1 += 2.0f * PI;
    while (arg1 > PI) arg1 -= 2.0f * PI;
    arg1 = std::abs(arg1);
    const float delta = std::max(0.0f, arg1 - arg2);
    const float range = arg3 - arg2;
    return delta < range ? 1.0f - delta / range * (1.0f - arg4) : arg4;
}

// 0x424130
float CCarCtrl::FindSpeedMultiplierWithSpeedFromNodes(int8 arg1) {
    if (arg1 == -1) return 0.5f;
    if (arg1 == 0) return 0.65f;
    if (arg1 == 2) return 2.3f;
    return 1.0f;
}

float CCarCtrl::FindGhostRoadHeight(CVehicle* vehicle) {
    const auto& currentAddress = vehicle->m_autoPilot.m_currentAddress;
    const auto& startingAddress = vehicle->m_autoPilot.m_startingRouteNode;
    if (!currentAddress || !startingAddress) {
        return 0.0f;
    }

    const auto* currentNode = ThePaths.GetPathNode(currentAddress);
    const auto* startingNode = ThePaths.GetPathNode(startingAddress);
    if (!currentNode || !startingNode) {
        return 0.0f;
    }

    const auto vehiclePos = vehicle->GetPosition();
    const auto currentDist = (currentNode->GetPosition() - vehiclePos).Magnitude2D();
    const auto startingDist = (startingNode->GetPosition() - vehiclePos).Magnitude2D();
    return (currentDist * startingNode->GetPosition().z + startingDist * currentNode->GetPosition().z)
        / (currentDist + startingDist);
}

// 0x42B270
void CCarCtrl::FireHeliRocketsAtTarget(CAutomobile* entityLauncher, CEntity* entity) {
    auto* heli = entityLauncher->AsHeli();
    if (heli->m_nFiringMultiplier != 0 && heli->m_nFiringMultiplier != 4)
        return;

    if (CTimer::GetTimeInMS() / 250u == CTimer::GetPreviousTimeInMS() / 250u)
        return;

    const auto targetPos = entity->GetPosition();
    const auto& matrix = entityLauncher->GetMatrix();
    const auto launcherPos = matrix.GetPosition();
    if ((launcherPos - targetPos).Magnitude() >= 80.0f)
        return;

    const auto forwardPoint = launcherPos + matrix.GetForward();
    if (CCollision::DistToMathematicalLine(&launcherPos, &forwardPoint, &targetPos) >= 7.0f)
        return;

    const auto rocketOffset = (CTimer::GetTimeInMS() / 250u & 1) ? -1.5f : 1.5f;
    const auto rocketPos = launcherPos + matrix.GetForward() * 4.0f + matrix.GetRight() * rocketOffset;
    CProjectileInfo::AddProjectile(entityLauncher, WEAPON_ROCKET, rocketPos, 0.0f, &matrix.GetForward(), nullptr);
}

// 0x429A70
void CCarCtrl::FlyAIHeliInCertainDirection(CHeli* heli, float orientation, float distance, bool slowDownAtTarget) {
    const auto& ap = heli->m_autoPilot;
    const bool circling = ap.m_nCarMission == MISSION_HELI_NEWS_BEHAVIOUR
        && distance < ap.m_ucHeliTargetDist2 && heli->m_vecMoveSpeed.Magnitude2D() < 0.01f;
    if (circling) {
        orientation += HALF_PI;
    }
    const auto& position = heli->GetPosition();
    const auto& forward = heli->GetForward();
    const auto lineOfSight = [](const CVector& start, const CVector& end, CColPoint& point) {
        CEntity* hit{};
        return CWorld::ProcessLineOfSight(start, end, point, hit, true, false, false, false, false, false, false, true);
    };
    if ((CTimer::GetTimeInMS() + heli->m_nRandomSeed) % 500
        < (CTimer::GetPreviousTimeInMS() + heli->m_nRandomSeed) % 500) {
        heli->field_9AC = heli->m_fMaxAltitude;
        const auto origin = position + heli->m_vecMoveSpeed * 50.0f;
        const CVector heading{std::cos(orientation), std::sin(orientation), 0.0f};
        auto down = heading;
        down.z = -1.0f;
        down.Normalise();
        CColPoint point{};
        if (lineOfSight(origin, origin + down * 60.0f, point)) {
            heli->field_9AC = std::max(heli->field_9AC, point.m_vecPoint.z + heli->m_fMinAltitude * (circling ? 0.5f : 1.0f));
        }
        const auto lookAhead = heli->m_vecMoveSpeed.Magnitude2D() * 100.0f
            + (heli->vehicleFlags.bSirenOrAlarm ? 5.0f : 30.0f);
        heli->field_9B8 = true;
        if (!lineOfSight(origin, origin + heading * lookAhead, point)) {
            heli->field_9B8 = false;
            heli->m_fSteeringLeftRight = 0.0f;
        } else {
            auto right = heli->GetRight();
            right.z = 0.0f;
            right.Normalise();
            const auto clearance = [&](float side) {
                const auto start = position + right * side;
                if (lineOfSight(position, start, point)) {
                    return 0.0f;
                }
                if (lineOfSight(start, start + heading * lookAhead, point)) {
                    return (point.m_vecPoint - start).Magnitude();
                }
                return 1000.0f;
            };
            const auto rightClearance = clearance(10.0f);
            const auto leftClearance = clearance(-10.0f);
            heli->m_fSteeringLeftRight = leftClearance <= rightClearance ? -0.5f : 0.5f;
        }
    }
    if (ap.m_nCarMission == MISSION_HELI_LAND_TOUCHING_DOWN) {
        heli->field_9B8 = false;
        heli->m_fSteeringLeftRight = 0.0f;
    }
    const auto heading = CGeneral::GetATanOfXY(forward.x, forward.y);
    const auto altitudeError = heli->field_9AC - (position.z + heli->m_vecMoveSpeed.z * 100.0f);
    const auto throttle = altitudeError * (altitudeError > 0.0f ? 0.1f : 0.2f)
        + ((CGeneral::GetRandomNumber() & 15) - 7) * 0.002f;
    heli->m_fAccelerationBreakStatus = std::clamp(throttle, -0.3f, 1.0f);
    heli->m_fLeftRightSkid = std::clamp(-2.0f * CGeneral::LimitRadianAngle(orientation - heading), -1.0f, 1.0f);
    if (distance > 60.0f || !slowDownAtTarget) {
        heli->m_fSteeringUpDown = -0.8f;
    } else {
        const auto predictedDelta = position + heli->m_vecMoveSpeed * 50.0f - ap.m_vecDestinationCoors;
        const auto targetDistance = static_cast<float>(ap.m_ucHeliTargetDist2);
        const auto remaining = predictedDelta.Magnitude2D() - targetDistance;
        if (remaining >= 0.0f) {
            heli->m_fSteeringUpDown = remaining * -0.8f / (30.0f - targetDistance);
        }
    }
    if (heli->m_fSteeringUpDown < 0.0f) {
        const auto headingError = std::abs(CGeneral::LimitRadianAngle(heading - orientation));
        heli->m_fSteeringUpDown *= std::max(1.0f - 2.12206578f * headingError, 0.0f);
        if (headingError > HALF_PI && DotProduct(forward, heli->m_vecMoveSpeed) > 0.0f) {
            heli->m_fSteeringUpDown = 0.3f;
        }
    }
    if (throttle - 0.5f > 0.0f) {
        heli->m_fSteeringUpDown *= 1.0f - std::min(throttle - 0.5f, 1.0f);
    }
    if (heli->m_fSteeringUpDown < 0.0f) {
        const auto speedLimit = std::clamp((ap.m_nCruiseSpeed
            - DotProduct(forward, heli->m_vecMoveSpeed) * 60.0f) * 0.1f, 0.0f, 1.0f);
        heli->m_fSteeringUpDown = std::max(heli->m_fSteeringUpDown, -speedLimit);
        if (ap.m_ucHeliSpeedMult && heli->m_fSteeringUpDown < -0.2f && !heli->field_9B8) {
            const auto acceleration = ap.m_ucHeliSpeedMult * CTimer::GetTimeStep() * 0.001f;
            heli->m_vecMoveSpeed.x += forward.x * acceleration;
            heli->m_vecMoveSpeed.y += forward.y * acceleration;
        }
    }
    if (heli->field_9B8) {
        auto planarForward = forward;
        planarForward.z = 0.0f;
        planarForward.Normalise();
        heli->m_fSteeringUpDown = 2.0f * DotProduct(planarForward, heli->m_vecMoveSpeed);
    }
    heli->m_fSteeringUpDown = std::clamp(heli->m_fSteeringUpDown, -1.0f, 1.0f);
}

// 0x423940
void CCarCtrl::FlyAIHeliToTarget_FixedOrientation(CHeli* heli, float orientation, CVector target) {
    const auto seed = static_cast<uint32>(heli->m_nRandomSeed);
    if ((CTimer::GetTimeInMS() + seed) % 500 < (CTimer::GetPreviousTimeInMS() + seed) % 500) {
        heli->field_9AC = heli->m_fMaxAltitude;
        const auto origin = heli->GetPosition() + heli->GetMoveSpeed() * 50.0f;
        CVector direction{std::cos(orientation), std::sin(orientation), -1.0f};
        direction.Normalise();
        CColPoint point;
        CEntity* entity{};
        if (CWorld::ProcessLineOfSight(origin, origin + direction * 60.0f, point, entity, true, false, false, false, false, false, false, true)) {
            heli->field_9AC = std::max(heli->field_9AC, point.m_vecPoint.z + heli->m_fMinAltitude);
        }
    }
    const auto& matrix = heli->GetMatrix();
    const auto heading = CGeneral::GetATanOfXY(matrix.GetForward().x, matrix.GetForward().y);
    const auto altitudeDifference = heli->field_9AC - (heli->GetPosition().z + heli->GetMoveSpeed().z * 100.0f);
    heli->m_fAccelerationBreakStatus = altitudeDifference * (altitudeDifference > 0.0f ? 0.1f : 0.2f);
    heli->m_fAccelerationBreakStatus += ((CGeneral::GetRandomNumber() & 0xF) - 7) * 0.002f;
    heli->m_fAccelerationBreakStatus = std::clamp(heli->m_fAccelerationBreakStatus, -0.3f, 1.0f);

    auto angle = orientation - heading;
    while (angle > PI) {
        angle -= TWO_PI;
    }
    while (angle < -PI) {
        angle += TWO_PI;
    }
    heli->m_fLeftRightSkid = std::clamp(angle * -0.5f, -1.0f, 1.0f);
    const auto& position = heli->GetPosition();
    const CVector delta{target.x - position.x, target.y - position.y, 0.0f};
    const auto rightSpeed = DotProduct(heli->GetMoveSpeed(), matrix.GetRight());
    const auto forwardSpeed = DotProduct(heli->GetMoveSpeed(), matrix.GetForward());
    const auto lateralDistance = DotProduct(delta, matrix.GetRight()) + rightSpeed * 80.0f;
    const auto forwardDistance = DotProduct(delta, matrix.GetForward()) + forwardSpeed * 80.0f;
    heli->m_fSteeringLeftRight = std::abs(lateralDistance) < 5.0f ? rightSpeed : lateralDistance * -0.02f;
    heli->m_fSteeringLeftRight = std::clamp(heli->m_fSteeringLeftRight, -0.75f, 0.75f);
    heli->m_fSteeringUpDown = std::abs(forwardDistance) < 5.0f ? forwardSpeed : forwardDistance * -0.015f;
    heli->m_fSteeringUpDown = std::clamp(heli->m_fSteeringUpDown, -0.5f, 0.5f);
}

// 0x423000
void CCarCtrl::FlyAIPlaneInCertainDirection(CPlane* plane) {
    auto& ap = plane->m_autoPilot;
    const auto& forward = plane->GetForward();
    const auto heading = CGeneral::GetATanOfXY(forward.x, forward.y);
    const auto now = CTimer::GetTimeInMS();
    if ((now + plane->m_nRandomSeed) % 1000 < (CTimer::GetPreviousTimeInMS() + plane->m_nRandomSeed) % 1000) {
        auto targetHeading = plane->m_planeHeading;
        switch (ap.m_nTempAction) {
        case TEMPACT_PLANE_FLY_STRAIGHT: targetHeading = heading; break;
        case TEMPACT_PLANE_SHARP_LEFT: targetHeading = heading - 2.0f; break;
        case TEMPACT_PLANE_SHARP_RIGHT: targetHeading = heading + 2.0f; break;
        }
        plane->m_altitude = 500.0f;
        plane->m_planeHeadingPrev = plane->m_planeHeading + PI;
        const auto error = CGeneral::LimitRadianAngle(heading - targetHeading);
        const auto turn = std::abs(error) < 0.52359879f ? 0.0f : error * 1.5f;
        const auto findHeight = [&](float direction) {
            constexpr std::array pitches{0.104719758f, 0.052359879f, 0.0f, -0.34906587f, -0.69813174f, -1.04719758f};
            const auto start = plane->GetPosition() + plane->m_vecMoveSpeed * 50.0f;
            auto height = 0.0f;
            for (auto i = 0u; i < pitches.size(); i++) {
                const auto pitch = pitches[i];
                const CVector directionVector{std::cos(direction) * std::cos(pitch), std::sin(direction) * std::cos(pitch), std::sin(pitch)};
                CColPoint point{};
                CEntity* hit{};
                if (CWorld::ProcessLineOfSight(start, start + directionVector * 200.0f, point, hit,
                        true, false, false, false, false, false, false, true)) {
                    if (i == 0) {
                        return 100000.0f;
                    }
                    height = std::max(height, point.m_vecPoint.z);
                }
            }
            return height;
        };
        for (auto pass = 0; pass < 2; pass++) {
            for (auto i = 1; i < 20; i++) {
                const auto offset = (i / 2) * (i & 1 ? 0.261799395f : -0.261799395f);
                const bool preferred = (offset < 0.0f && offset > turn) || (offset > 0.0f && offset < turn);
                if (preferred != (pass == 0)) {
                    continue;
                }
                const auto height = findHeight(targetHeading + offset);
                if (height < 150.0f) {
                    plane->m_planeHeadingPrev = plane->m_planeHeading + offset * 1.1f;
                    plane->m_altitude = std::max(height + plane->m_minAltitude, plane->m_maxAltitude);
                    break;
                }
            }
        }
    }
    const auto minimumSpeed = plane->vehicleFlags.bSirenOrAlarm ? 7.0f : 32.0f;
    const auto speed = (forward.x * plane->m_vecMoveSpeed.x + forward.y * plane->m_vecMoveSpeed.y) * 60.0f;
    if (std::ranges::any_of(plane->m_fWheelsSuspensionCompression, [](float compression) { return compression < 1.0f; })) {
        plane->m_nStartedFlyingTime = now;
    }
    const bool takingOff = now - plane->m_nStartedFlyingTime <= 4000;
    if (takingOff) {
        plane->m_fSteeringUpDown = speed >= minimumSpeed ? 0.4f : 0.0f;
    } else {
        if (plane->m_LandingGearAngle != 1.0f) {
            plane->SetGearUp();
        }
        const auto pitch = std::asin(forward.z);
        const auto predictedPitch = pitch + (pitch - plane->m_forwardZ) * 100.0f / CTimer::GetTimeStep();
        plane->m_forwardZ = pitch;
        auto desiredPitch = std::clamp((plane->m_altitude - (plane->GetPosition().z + plane->m_vecMoveSpeed.z * 100.0f)) / 30.0f, -0.4f, 0.4f);
        if (speed < minimumSpeed) {
            desiredPitch = std::min(desiredPitch, 0.25f);
        }
        plane->m_fSteeringUpDown = (desiredPitch - predictedPitch) * 0.5f;
    }
    const auto headingError = CGeneral::LimitRadianAngle(plane->m_planeHeadingPrev - heading);
    if (takingOff) {
        plane->m_fLeftRightSkid = std::clamp(-(plane->m_planeCreationHeading - heading) * 10.0f, -1.0f, 1.0f);
        plane->m_fSteeringLeftRight = 0.0f;
    } else {
        const auto limit = plane->vehicleFlags.bSirenOrAlarm ? 0.7f : 0.9f;
        const auto turn = std::clamp(-headingError * 1.5f, -limit, limit);
        auto desiredRoll = std::abs(turn) < 0.1f ? 0.0f : -turn;
        plane->m_fLeftRightSkid = std::abs(turn) < 0.1f ? turn * 4.0f : turn;
        if (speed < minimumSpeed * 1.2f) {
            desiredRoll *= std::max(1.0f - (minimumSpeed * 1.2f - speed) / (minimumSpeed * 0.5f), 0.0f);
        }
        const auto& right = plane->GetRight();
        auto horizontalRight = right.Magnitude2D();
        if (plane->GetUp().z < 0.0f) {
            horizontalRight = -horizontalRight;
        }
        const auto roll = std::atan2(right.z, horizontalRight);
        const auto predictedRoll = roll + 30.0f / std::max(CTimer::GetTimeStep(), 1.0f) * (roll - plane->m_fSteeringFactor);
        plane->m_fSteeringLeftRight = std::clamp(-CGeneral::LimitRadianAngle(desiredRoll - predictedRoll), -1.0f, 1.0f);
        plane->m_fSteeringFactor = roll;
        const auto pitchControl = plane->m_fSteeringUpDown;
        plane->m_fSteeringUpDown += std::abs(0.23f * roll);
        if (pitchControl < 0.0f) {
            plane->m_fSteeringUpDown = std::min(plane->m_fSteeringUpDown, pitchControl * 0.5f);
        }
    }
    plane->m_fSteeringUpDown = std::clamp(plane->m_fSteeringUpDown, -1.0f, 1.0f);
    plane->m_fAccelerationBreakStatus = plane->m_fAccelerationBreakStatusPrev;
    if (ap.m_nTempAction == TEMPACT_PLANE_FLY_UP) {
        plane->m_fSteeringUpDown = 1.0f;
        if (speed < 20.0f) {
            ap.m_nTempAction = TEMPACT_NONE;
        }
    }
}

// 0x424210
bool CCarCtrl::GenerateCarCreationCoors2(CVector center, float directionX, float directionY, float requiredDotProduct, bool requiredInside, float creationDistOnScreen, float creationDistOffScreen, CVector* result, CNodeAddress* from, CNodeAddress* to, float* fraction, bool ignoreSwitchedOff, bool noWater) {
    static CNodeAddress nearest, secondNearest, nearestIncludingSwitchedOff, secondIncludingSwitchedOff;
    static CVector updateCoors;
    static uint32 updateTime;
    if ((center - updateCoors).Magnitude2D() > 10.0f || CTimer::GetTimeInMS() > updateTime) {
        ThePaths.Find2NodesForCarCreation(center, &nearest, &secondNearest, true);
        ThePaths.Find2NodesForCarCreation(center, &nearestIncludingSwitchedOff, &secondIncludingSwitchedOff, false);
        updateTime = CTimer::GetTimeInMS() + 5000;
        updateCoors = center;
        if (nearest.IsAreaValid() && ThePaths.m_pPathNodes[nearest.m_wAreaId]) {
            const auto* node = ThePaths.GetPathNode(nearest);
            const auto linkAddress = ThePaths.m_pNaviLinks[node->m_wAreaId][node->m_wBaseLinkId];
            if (ThePaths.m_pPathNodes[linkAddress.m_wAreaId]) {
                const auto& link = ThePaths.GetCarPathLink(linkAddress);
                CPopulation::m_bMoreCarsAndFewerPeds = (link.m_numOppositeDirLanes >= 2 || link.m_numSameDirLanes >= 2)
                    && (link.GetNodeCoors() - CVector2D{center}).Magnitude() < 40.0f && node->m_bHighway;
            }
        }
    }
    CNodeAddress current;
    if ((CGeneral::GetRandomNumber() & 15) == 4 && !noWater) {
        current = ThePaths.FindNodeClosestToCoors(center, PATH_TYPE_VEH, 50.0f, 0, 0, 1, 1);
        creationDistOnScreen *= 1.5f;
    } else if (CGeneral::GetRandomNumber() & 3) {
        current = ignoreSwitchedOff ? nearest : nearestIncludingSwitchedOff;
    } else {
        current = ignoreSwitchedOff ? secondNearest : secondIncludingSwitchedOff;
    }
    if (!current.IsAreaValid() || !ThePaths.m_pPathNodes[current.m_wAreaId]) {
        return false;
    }
    std::array<CNodeAddress, 30> visited;
    visited[0] = current;
    size_t count = 1;
    float travelled = 0.0f;
    while (count < visited.size() && travelled < 230.0f) {
        if (!ThePaths.m_pPathNodes[current.m_wAreaId]) {
            break;
        }
        const auto* currentNode = ThePaths.GetPathNode(current);
        InitSequence(currentNode->m_nNumLinks);
        CNodeAddress next;
        for (auto i = 0u; i < currentNode->m_nNumLinks; i++) {
            const auto index = currentNode->m_wBaseLinkId + FindSequenceElement(i);
            const auto candidate = ThePaths.m_pNodeLinks[current.m_wAreaId][index];
            const auto linkAddress = ThePaths.m_pNaviLinks[current.m_wAreaId][index];
            if (!ThePaths.m_pPathNodes[candidate.m_wAreaId] || !ThePaths.m_pPathNodes[linkAddress.m_wAreaId]) {
                continue;
            }
            if (std::find(visited.begin(), visited.begin() + count, candidate) == visited.begin() + count) {
                next = candidate;
                break;
            }
        }
        if (!next.IsAreaValid()) {
            return false;
        }
        const auto* nextNode = ThePaths.GetPathNode(next);
        const auto a = currentNode->GetPosition();
        const auto b = nextNode->GetPosition();
        if (!ignoreSwitchedOff || (!currentNode->m_isSwitchedOff && !nextNode->m_isSwitchedOff)) {
            const auto distanceA = (a - center).Magnitude2D();
            const auto distanceB = (b - center).Magnitude2D();
            const auto crossesRadius = [&](float radius, bool visible) {
                if ((distanceA - radius) * (distanceB - radius) >= 0.0f) {
                    return false;
                }
                const auto offsetA = std::abs(distanceA - radius);
                const auto offsetB = std::abs(distanceB - radius);
                const auto f = offsetA / (offsetA + offsetB);
                *result = a * (1.0f - f) + b * f;
                if (TheCamera.IsSphereVisible(*result, 5.0f) != visible) {
                    return false;
                }
                *fraction = f;
                return true;
            };
            if (crossesRadius(creationDistOnScreen, true) || crossesRadius(creationDistOffScreen, false)) {
                const auto delta = a - b;
                if (delta.Magnitude2D() * 0.5f >= std::abs(delta.z)) {
                    if (CGeneral::GetRandomNumber() & 8) {
                        *from = current;
                        *to = next;
                    } else {
                        *from = next;
                        *to = current;
                        *fraction = 1.0f - *fraction;
                    }
                    auto* toNode = ThePaths.GetPathNode(*to);
                    if (toNode->m_onDeadEnd && ThePaths.ThisNodeWillLeadIntoADeadEnd(toNode, ThePaths.GetPathNode(*from)) && ignoreSwitchedOff) {
                        return false;
                    }
                    CVector2D direction{*result - center};
                    direction.Normalise();
                    const auto dot = direction.x * directionX + direction.y * directionY;
                    if (requiredInside ? dot <= requiredDotProduct : dot > requiredDotProduct) {
                        return false;
                    }
                    sprintf(gString, "tell Obbe it happened again %d/%d %d/%d", from->m_wAreaId, from->m_wNodeId, to->m_wAreaId, to->m_wNodeId);
                    return true;
                }
            }
        }
        visited[count++] = next;
        travelled += (b - a).Magnitude2D();
        current = next;
    }
    return false;
}

// 0x42F9C0
void CCarCtrl::GenerateEmergencyServicesCar() {
    if (!bAllowEmergencyServicesToBeCreated
        || CGangWars::GangWarFightingGoingOn()
        || FindPlayerPed(-1)->GetPlayerWanted()->GetWantedLevel() > eWantedLevel::WANTED_LEVEL_3
        || CGame::currArea
        || !CTheZones::m_CurrLevel) {
        return;
    }

    if (NumLawEnforcerCars + NumRandomCars + NumMissionCars + NumParkedCars
        + NumAmbulancesOnDuty + NumFireTrucksOnDuty > MaxNumberOfCarsInUse) {
        return;
    }

    auto* accidentManager = CAccidentManager::GetInstance();
    if (!NumAmbulancesOnDuty) {
        if (accidentManager->GetNumberOfFreeAccidents() < 2) {
            CStreaming::StreamAmbulanceAndMedic(false);
        } else {
            auto playerPos = FindPlayerCoors(-1);
            if (auto* accident = accidentManager->GetNearestFreeAccident(playerPos, false); accident
                && CStreaming::StreamAmbulanceAndMedic(true)
                && CTimer::GetTimeInMS() > static_cast<uint32>(LastTimeAmbulanceCreated) + 30000) {
                if (GenerateOneEmergencyServicesCar(
                        CStreaming::ms_aDefaultAmbulanceModel[CTheZones::m_CurrLevel], accident->m_pPed->GetPosition())) {
                    LastTimeAmbulanceCreated = CTimer::GetTimeInMS();
                }
            }
        }
    }

    if (!NumFireTrucksOnDuty) {
        if (gFireManager.GetNumOfNonScriptFires() < 3) {
            CStreaming::StreamFireEngineAndFireman(false);
        } else {
            const auto playerPos = FindPlayerCoors(-1);
            if (auto* fire = gFireManager.FindNearestFire(playerPos, true, true); fire
                && CStreaming::StreamFireEngineAndFireman(true)
                && CTimer::GetTimeInMS() > static_cast<uint32>(LastTimeFireTruckCreated) + 35000) {
                if (GenerateOneEmergencyServicesCar(
                        CStreaming::ms_aDefaultFireEngineModel[CTheZones::m_CurrLevel], fire->GetPosition())) {
                    LastTimeFireTruckCreated = CTimer::GetTimeInMS();
                }
            }
        }
    }
}

// 0x42B7D0
CAutomobile* CCarCtrl::GenerateOneEmergencyServicesCar(uint32 modelId, CVector posn) {
    CVector spawn;
    CNodeAddress from, to;
    float between{};
    bool found = false;
    auto attempts = 0;
    for (; attempts < 5; attempts++) {
        if (GenerateCarCreationCoors2(FindPlayerCentreOfWorld(CWorld::PlayerInFocus), 0.707f, 0.707f, -1.0f,
                true, 160.0f, 160.0f, &spawn, &from, &to, &between, false, false)
            && !ThePaths.GetPathNode(from)->m_bWaterNode) {
            int16 count{};
            CWorld::FindObjectsKindaColliding(spawn, 10.0f, true, &count, 2, nullptr, false, true, true, false, false);
            found = count == 0;
        }
        if (found) {
            attempts++;
            break;
        }
    }
    if (!found || attempts >= 5) {
        return nullptr;
    }
    auto* const car = new CAutomobile(modelId, RANDOM_VEHICLE, true);
    car->SetPosn(spawn);
    auto direction = posn - spawn;
    const auto length = direction.Magnitude2D();
    if (length != 0.0f) {
        direction.x /= length;
        direction.y /= length;
    } else {
        direction.x = 1.0f;
    }
    auto& matrix = car->GetMatrix();
    matrix.GetForward() = CVector{direction.x, direction.y, 0.0f};
    matrix.GetRight() = CVector{direction.y, -direction.x, 0.0f};
    matrix.GetUp() = CVector{0.0f, 0.0f, 1.0f};
    const auto roadHeight = (1.0f - between) * ThePaths.GetPathNode(from)->GetPosition().z
        + between * ThePaths.GetPathNode(to)->GetPosition().z;
    CColPoint point{};
    CEntity* hit{};
    auto height = 1.0e9f;
    if (CWorld::ProcessVerticalLine(spawn, 1000.0f, point, hit, true, false, false, false, true, false, nullptr)) {
        height = point.m_vecPoint.z;
    }
    if (CWorld::ProcessVerticalLine(spawn, -1000.0f, point, hit, true, false, false, false, true, false, nullptr)
        && std::abs(point.m_vecPoint.z - roadHeight) < std::abs(height - roadHeight)) {
        height = point.m_vecPoint.z;
    }
    if (height == 1.0e9f) {
        delete car;
        return nullptr;
    }
    spawn.z = height + car->GetDistanceFromCentreOfMassToBaseOfModel();
    car->SetPosn(spawn);
    car->m_vecMoveSpeed = CVector{};
    car->PlaceOnRoadProperly();
    CWorld::Add(car);
    switch (modelId) {
    case MODEL_FIRETRUK:
        car->vehicleFlags.bIsFireTruckOnDuty = true;
        NumFireTrucksOnDuty++;
        CCarAI::AddFiretruckOccupants(car);
        car->vehicleFlags.bDisableParticles = true;
        break;
    case MODEL_AMBULAN:
        car->vehicleFlags.bIsAmbulanceOnDuty = true;
        NumAmbulancesOnDuty++;
        CCarAI::AddAmbulanceOccupants(car);
        car->vehicleFlags.bDisableParticles = true;
        break;
    case MODEL_ENFORCER:
    case MODEL_COPBIKE:
    case MODEL_COPCARLA:
    case MODEL_COPCARSF:
    case MODEL_COPCARVG:
    case MODEL_COPCARRU:
        CCarAI::AddPoliceCarOccupants(car, false);
        car->vehicleFlags.bDisableParticles = true;
        break;
    }
    return car;
}

// 0x430050
void CCarCtrl::GenerateOneRandomCar() {
    const auto center = FindPlayerCentreOfWorld(CWorld::PlayerInFocus);
    const auto playerSpeed = FindPlayerSpeed();
    const auto total = NumRandomCars + NumLawEnforcerCars + NumMissionCars + NumAmbulancesOnDuty + NumFireTrucksOnDuty;
    auto density = CarDensityMultiplier;
    if (CCullZones::FewerCars()) {
        density *= 0.6f;
    }
    if (total >= CPopulation::FindCarMultiplierMotorway() * MaxNumberOfCarsInUse * density
        || total >= CPopulation::FindCarMultiplierMotorway() * density * (CPopCycle::m_NumOther_Cars + CPopCycle::m_NumCops_Cars + CPopCycle::m_NumGangs_Cars + CPopCycle::m_NumDealers_Cars)) {
        return;
    }
    const auto* wanted = FindPlayerWanted();
    const auto now = CTimer::GetTimeInMS();
    const auto wantedLevel = static_cast<int32>(wanted->GetWantedLevel());
    const bool createCop = wantedLevel > 1 && NumLawEnforcerCars < wanted->m_MaxCopCarsInPursuit
        && wanted->m_NumCopsInPursuit < wanted->m_MaxCopsInPursuit && !CGame::currArea
        && !CGangWars::GangWarFightingGoingOn()
        && (wantedLevel > 3 || (wantedLevel > 2 && now > static_cast<uint32>(LastTimeLawEnforcerCreated) + 5000) || now > static_cast<uint32>(LastTimeLawEnforcerCreated) + 8000);
    int32 rating;
    int32 model;
    if (createCop) {
        model = ChoosePoliceCarModel(0);
        rating = 13;
    } else {
        model = ChooseModel(&rating);
        if (model == -1 || ((rating == 13 || rating == 24) && wantedLevel >= 1)) {
            return;
        }
    }
    if (CGameLogic::LaRiotsActiveHere() && !gbLARiots_NoPoliceCars && (CGeneral::GetRandomNumber() & 127) < 55) {
        model = ChoosePoliceCarModel(0);
        rating = 13;
    }
    const bool lookingDown = TheCamera.m_mCameraMatrix.GetForward().z < -0.9f;
    float directionX, directionY, requiredDot;
    bool requiredInside;
    if (lookingDown) {
        directionX = directionY = 0.707f;
        requiredDot = -1.0f;
        requiredInside = true;
    } else {
        const auto* playerVehicle = FindPlayerVehicle();
        const auto speed = playerVehicle ? playerVehicle->m_vecMoveSpeed.Magnitude2D() : 0.0f;
        if (speed <= 0.1f) {
            directionX = TheCamera.m_fCamFrontXNorm;
            directionY = TheCamera.m_fCamFrontYNorm;
            requiredDot = 0.707f;
            requiredInside = !(CTimer::GetFrameCounter() & 1);
        } else {
            directionX = playerVehicle->m_vecMoveSpeed.x / speed;
            directionY = playerVehicle->m_vecMoveSpeed.y / speed;
            const auto phase = CTimer::GetFrameCounter() & 3;
            requiredDot = (speed > 0.4f ? phase < 2 : phase == 0) ? 0.85f : 0.707f;
            requiredInside = speed > 0.4f ? phase != 3 : phase < 2;
        }
    }
    CVector position;
    CNodeAddress from, to;
    float fraction;
    if (!GenerateCarCreationCoors2(center, directionX, directionY, requiredDot, requiredInside,
        TheCamera.m_fGenerationDistMultiplier * 160.0f, 38.0f, &position, &from, &to, &fraction, rating != 13 || wantedLevel < 1, false)) {
        return;
    }
    const auto* fromNode = ThePaths.GetPathNode(from);
    const auto* toNode = ThePaths.GetPathNode(to);
    if (static_cast<uint32>(CGeneral::GetRandomNumber() & 15) > std::min<uint32>(fromNode->m_nSpawnProbability, toNode->m_nSpawnProbability)) {
        return;
    }
    const bool water = fromNode->m_bWaterNode;
    if (water) {
        if (rating == 13) {
            model = MODEL_PREDATOR;
            rating = 24;
            if (!CStreaming::IsModelLoaded(model)) {
                CStreaming::RequestModel(model, STREAMING_KEEP_IN_MEMORY);
                return;
            }
        } else {
            model = CPopulation::m_LoadedBoats.PickLeastUsedModel(1);
            if (model == -1 || !CStreaming::IsModelLoaded(model)) {
                return;
            }
        }
    }
    int16 collisions;
    CWorld::FindObjectsKindaColliding(position, water ? 40.0f : 8.0f, true, &collisions, 2, nullptr, false, true, true, false, false);
    if (collisions) {
        return;
    }
    auto linkIndex = 0u;
    while (linkIndex < fromNode->m_nNumLinks && ThePaths.m_pNodeLinks[from.m_wAreaId][fromNode->m_wBaseLinkId + linkIndex] != to) {
        linkIndex++;
    }
    const auto nextLinkAddress = ThePaths.m_pNaviLinks[from.m_wAreaId][fromNode->m_wBaseLinkId + linkIndex];
    const auto& nextLink = ThePaths.GetCarPathLink(nextLinkAddress);
    const auto lanes = nextLink.m_attachedTo == to ? nextLink.m_numOppositeDirLanes : nextLink.m_numSameDirLanes;
    if (!lanes || (lanes < 2 && (model == MODEL_BUS || model == MODEL_COACH))
        || (lanes >= 2 && CModelInfo::GetVehicleModelInfo(model)->IsBMX())) {
        return;
    }
    bool specialZone = false;
    if (CPopCycle::m_pCurrZone) {
        const auto zoneType = CTheZones::GetZoneInfo(position, nullptr)->PopType;
        if (zoneType >= 17 && zoneType <= 19) {
            if (zoneType != CPopCycle::m_nCurrentZoneType) {
                return;
            }
            specialZone = true;
        }
    }
    auto* vehicle = GetNewVehicleDependingOnCarModel(model, RANDOM_VEHICLE);
    auto& ap = vehicle->m_autoPilot;
    auto& flags = vehicle->vehicleFlags;
    ap.m_currentAddress = from;
    ap.m_startingRouteNode = to;
    ap.m_endingRouteNode.ResetAreaId();
    ap.m_nTempAction = TEMPACT_NONE;
    if (rating == 13) {
        if (wantedLevel) {
            ap.m_nCruiseSpeed = CCarAI::FindPoliceCarSpeedForWantedLevel(vehicle);
            ap.m_nCarMission = vehicle->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE
                ? CCarAI::FindPoliceBikeMissionForWantedLevel() : CCarAI::FindPoliceCarMissionForWantedLevel();
            ap.m_nCarDrivingStyle = DRIVING_STYLE_AVOID_CARS;
        } else {
            ap.m_nCruiseSpeed = static_cast<uint8>(CGeneral::GetRandomNumberInRange(18.0f, 24.0f));
            ap.m_nCarDrivingStyle = DRIVING_STYLE_STOP_FOR_CARS;
            ap.m_nCarMission = MISSION_CRUISE;
        }
        if (model == MODEL_HOTRING) {
            vehicle->m_nPrimaryColor = vehicle->m_nSecondaryColor = 0;
        }
        flags.bCreatedAsPoliceVehicle = true;
    } else if (rating == 24) {
        ap.m_nCruiseSpeed = static_cast<uint8>(CGeneral::GetRandomNumberInRange(14.0f, 18.0f));
        ap.m_nCarDrivingStyle = DRIVING_STYLE_AVOID_CARS;
        ap.m_nCarMission = CCarAI::FindPoliceBoatMissionForWantedLevel();
        flags.bCreatedAsPoliceVehicle = true;
    } else {
        ap.m_nCruiseSpeed = static_cast<uint8>(CGeneral::GetRandomNumberInRange(13.0f, 21.0f));
        if (rating == 3) {
            ap.m_nCruiseSpeed = static_cast<uint8>(CGeneral::GetRandomNumberInRange(18.0f, 27.0f));
        } else if (rating == 1) {
            ap.m_nCruiseSpeed = static_cast<uint8>(CGeneral::GetRandomNumberInRange(10.0f, 15.0f));
        }
        const auto& box = vehicle->GetColModel()->GetBoundingBox();
        if (box.m_vecMax.y - box.m_vecMin.y > 10.0f || rating == 5) {
            ap.m_nCruiseSpeed = ap.m_nCruiseSpeed * 3 / 4;
        }
        if (water) {
            const auto fastBoat = model == MODEL_SQUALO || model == MODEL_SPEEDER || model == MODEL_JETMAX;
            ap.m_nCruiseSpeed = static_cast<uint8>(fastBoat ? CGeneral::GetRandomNumberInRange(25.0f, 35.0f) : CGeneral::GetRandomNumberInRange(15.0f, 24.0f));
        }
        ap.m_nCarMission = MISSION_CRUISE;
        ap.m_nCarDrivingStyle = DRIVING_STYLE_STOP_FOR_CARS;
    }
    if (model == MODEL_MRWHOOP) {
        flags.bSirenOrAlarm = true;
    }
    ap.m_nNextPathNodeInfo = nextLinkAddress;
    ap.m_nCurrentLane = ap.m_nNextLane = CGeneral::GetRandomNumber() % lanes;
    const auto randomInt = [](int32 max) { return static_cast<int32>(CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * max); };
    const auto appearance = vehicle->GetVehicleAppearance();
    const auto madDriverChance = CGameLogic::LaRiotsActiveHere() ? 80 : appearance == VEHICLE_APPEARANCE_BIKE ? 50 : appearance == VEHICLE_APPEARANCE_BOAT ? 10 : 200;
    const bool aggressive = CCheat::IsActive(CHEAT_AGGRESSIVE_DRIVERS);
    bool madDriver = false;
    if (!water && rating != 13 && !specialZone && (!randomInt(madDriverChance) || aggressive)) {
        madDriver = true;
        fraction = 1.0f;
    }
    const auto fromPos = fromNode->GetPosition();
    const auto toPos = toNode->GetPosition();
    const auto nodeDistance = (toPos - fromPos).Magnitude2D();
    const auto& box = vehicle->GetColModel()->GetBoundingBox();
    const auto halfLength = (box.m_vecMax.y - box.m_vecMin.y) * 0.5f + 1.0f;
    fraction = nodeDistance * 0.5f < halfLength ? 0.5f : std::clamp(fraction, halfLength / nodeDistance, 1.0f - halfLength / nodeDistance);
    const auto addressLess = [](CNodeAddress a, CNodeAddress b) {
        return a.m_wAreaId < b.m_wAreaId || (a.m_wAreaId == b.m_wAreaId && a.m_wNodeId < b.m_wNodeId);
    };
    ap._smthNext = addressLess(from, to) ? -1 : 1;
    if (fromNode->m_nNumLinks == 1) {
        delete vehicle;
        return;
    }
    int32 oldIndex;
    do {
        oldIndex = CGeneral::GetRandomNumber() % fromNode->m_nNumLinks;
        ap.m_nCurrentPathNodeInfo = ThePaths.m_pNaviLinks[from.m_wAreaId][fromNode->m_wBaseLinkId + oldIndex];
    } while (ap.m_nCurrentPathNodeInfo.m_wAreaId == ap.m_nNextPathNodeInfo.m_wAreaId && ap.m_nCurrentPathNodeInfo.m_wCarPathLinkId == ap.m_nNextPathNodeInfo.m_wCarPathLinkId);
    if (!ThePaths.m_pPathNodes[ap.m_nCurrentPathNodeInfo.m_wAreaId]) {
        delete vehicle;
        return;
    }
    ap._smthCurr = addressLess(ThePaths.m_pNodeLinks[from.m_wAreaId][fromNode->m_wBaseLinkId + oldIndex], from) ? -1 : 1;
    auto forward = toPos - fromPos;
    CVector2D forwardXY{forward};
    forwardXY.Normalise();
    forward.Normalise();
    auto& matrix = vehicle->GetMatrix();
    matrix.GetForward() = forward;
    matrix.GetRight() = CVector{forwardXY.y, -forwardXY.x, 0.0f};
    matrix.GetUp() = CVector{0.0f, 0.0f, 1.0f};
    const auto distanceToNext = (CVector{nextLink.GetNodeCoors(), 0.0f} - fromPos).Magnitude2D();
    const auto distanceFromNext = (CVector{nextLink.GetNodeCoors(), 0.0f} - toPos).Magnitude2D();
    float curveFraction;
    if (fraction >= distanceToNext / (distanceToNext + distanceFromNext)) {
        PickNextNodeRandomly(vehicle);
        const auto nodePos = ThePaths.GetPathNode(ap.m_currentAddress)->GetPosition();
        const auto distanceToCurrent = (CVector{ThePaths.GetCarPathLink(ap.m_nCurrentPathNodeInfo).GetNodeCoors(), 0.0f} - nodePos).Magnitude2D();
        const auto distanceToNew = (CVector{ThePaths.GetCarPathLink(ap.m_nNextPathNodeInfo).GetNodeCoors(), 0.0f} - nodePos).Magnitude2D();
        curveFraction = (distanceToCurrent - (position - nodePos).Magnitude2D()) / (distanceToCurrent + distanceToNew);
    } else {
        const auto distanceToCurrent = (CVector{ThePaths.GetCarPathLink(ap.m_nCurrentPathNodeInfo).GetNodeCoors(), 0.0f} - fromPos).Magnitude2D();
        curveFraction = (distanceToCurrent + (position - fromPos).Magnitude2D()) / (distanceToCurrent + distanceToNext);
    }
    curveFraction = std::clamp(curveFraction, 0.0f, 1.0f);
    const auto& currentLink = ThePaths.GetCarPathLink(ap.m_nCurrentPathNodeInfo);
    const auto& followingLink = ThePaths.GetCarPathLink(ap.m_nNextPathNodeInfo);
    CVector currentDir{CVector2D{currentLink.m_dir} * static_cast<float>(ap._smthCurr), 0.0f};
    CVector followingDir{CVector2D{followingLink.m_dir} * static_cast<float>(ap._smthNext), 0.0f};
    auto currentOffset = (currentLink.OneWayLaneOffset() + ap.m_nCurrentLane) * 5.4f;
    auto followingOffset = (followingLink.OneWayLaneOffset() + ap.m_nNextLane) * 5.4f;
    if (vehicle->IsBMX()) {
        currentOffset += 1.458f;
        followingOffset += 1.458f;
    }
    const auto* routeNode = ThePaths.GetPathNode(ap.m_startingRouteNode);
    ap.field_41 = routeNode->m_bNotHighway | (routeNode->m_bHighway << 1);
    ap.m_SpeedMult = FindSpeedMultiplierWithSpeedFromNodes(ap.field_41);
    ap.m_speed = ap.m_nCruiseSpeed * ap.m_SpeedMult;
    const auto currentCoors = CVector{currentLink.GetNodeCoors(), 0.0f} + CVector{currentDir.y, -currentDir.x, 0.0f} * currentOffset;
    const auto followingCoors = CVector{followingLink.GetNodeCoors(), 0.0f} + CVector{followingDir.y, -followingDir.x, 0.0f} * followingOffset;
    ap.m_timeToGetToNextLink = static_cast<int32>(CCurves::CalcSpeedScaleFactor(currentCoors, followingCoors, currentDir.x, currentDir.y, followingDir.x, followingDir.y) * 1000.0f / ap.m_speed);
    ap.m_timeToLeaveLink = static_cast<int32>(static_cast<float>(now) - curveFraction * ap.m_timeToGetToNextLink);
    CVector curveSpeed;
    CCurves::CalcCurvePoint(currentCoors, followingCoors, currentDir, followingDir, static_cast<float>(now - ap.m_timeToLeaveLink) / ap.m_timeToGetToNextLink, ap.m_timeToGetToNextLink, position, curveSpeed);
    auto spawnPos = position + (fromPos - toPos) * (2.0f / (fromPos - toPos).Magnitude());
    spawnPos.z = fromPos.z * (1.0f - fraction) + toPos.z * fraction;
    float height = 1.0e9f;
    if (water) {
        if (!CWaterLevel::GetWaterLevel(spawnPos, height, true)) {
            delete vehicle;
            return;
        }
    } else {
        CColPoint point;
        CEntity* entity;
        if (CWorld::ProcessVerticalLine(spawnPos, 1000.0f, point, entity, true, false, false, false, true)) {
            height = point.m_vecPoint.z;
        }
        if (CWorld::ProcessVerticalLine(spawnPos, -1000.0f, point, entity, true, false, false, false, true)
            && std::abs(point.m_vecPoint.z - spawnPos.z) < std::abs(height - spawnPos.z)) {
            height = point.m_vecPoint.z;
        }
    }
    if (height == 1.0e9f || std::abs(height - spawnPos.z) > 7.0f) {
        delete vehicle;
        return;
    }
    if (CModelInfo::IsBoatModel(model)) {
        spawnPos.z = height;
        vehicle->m_nExtendedRemovalRange = 255;
    } else {
        spawnPos.z = height + vehicle->GetHeightAboveRoad();
    }
    vehicle->SetPosn(spawnPos);
    vehicle->m_vecMoveSpeed = CVector{};
    const auto relativeSpeed = curveSpeed / 60.0f - playerSpeed;
    const auto relativePos = position - center;
    if (rating == 13) {
        vehicle->SetStatus(ap.m_nCarMission == MISSION_CRUISE ? STATUS_SIMPLE : STATUS_PHYSICS);
    } else if (rating == 24 || water) {
        vehicle->SetStatus(STATUS_PHYSICS);
    } else if (vehicle->GetStatus() != STATUS_PHYSICS) {
        vehicle->SetStatus(STATUS_SIMPLE);
    }
    CVisibilityPlugins::SetClumpAlpha(vehicle->GetRpClump(), 0);
    if (CCheat::IsActive(CHEAT_FUNHOUSE_THEME) && vehicle->IsAutomobile()) {
        vehicle->AddVehicleUpgrade(ModelIndices::MI_HYDRAULICS);
    }
    const auto distance = (center - vehicle->GetPosition()).Magnitude2D();
    const auto removalRange = std::max(static_cast<float>(vehicle->m_nExtendedRemovalRange), 170.0f);
    const auto generationMult = TheCamera.m_fGenerationDistMultiplier;
    if (vehicle->GetIsOnScreen()) {
        if (distance < generationMult * 150.0f || distance > generationMult * removalRange
            || (TheCamera.GetPosition() - vehicle->GetPosition()).Magnitude2D() < generationMult * 120.0f || lookingDown || model == MODEL_MARQUIS) {
            delete vehicle;
            return;
        }
    } else if (distance > removalRange / 170.0f * 45.0f && !lookingDown) {
        delete vehicle;
        return;
    }
    CWorld::FindObjectsKindaColliding(vehicle->GetPosition(), vehicle->GetColModel()->GetBoundRadius(), true, &collisions, 2, nullptr, false, true, true, false, false);
    if (collisions || relativeSpeed.x * relativePos.x + relativeSpeed.y * relativePos.y >= 0.0f) {
        delete vehicle;
        return;
    }
    auto* modelInfo = vehicle->GetVehicleModelInfo();
    modelInfo->ChooseVehicleColour(vehicle->m_nPrimaryColor, vehicle->m_nSecondaryColor, vehicle->m_nTertiaryColor, vehicle->m_nQuaternaryColor, 1);
    CWorld::Add(vehicle);
    if (model == MODEL_TRACTOR || model == MODEL_COMBINE || vehicle->IsBMX()) {
        ap.m_nCruiseSpeed /= 3;
    }
    if (CGameLogic::LaRiotsActiveHere()) {
        vehicle->m_fHealth = static_cast<float>(CGeneral::GetRandomNumber() % 1000);
    }
    if (rating == 13) {
        LastTimeLawEnforcerCreated = now;
    }
    if (model == MODEL_TOPFUN) {
        vehicle->SetStatus(STATUS_PHYSICS);
        ap.m_nCarDrivingStyle = DRIVING_STYLE_AVOID_CARS;
    }
    if (vehicle->IsAutomobile()) {
        if ((rating == 0 || (rating >= 4 && rating <= 6) || rating == 13) && !randomInt(20)) {
            vehicle->AsAutomobile()->SetRandomDamage(false);
        } else if ((rating == 1 || (rating >= 14 && rating <= 23)) && !randomInt(8)) {
            vehicle->AsAutomobile()->SetRandomDamage(true);
        }
    }
    if (vehicle->IsBike() && ap.m_nCarDrivingStyle == DRIVING_STYLE_STOP_FOR_CARS) {
        vehicle->SetStatus(STATUS_PHYSICS);
        ap.m_nCarDrivingStyle = DRIVING_STYLE_DRIVINGMODE_AVOIDCARS_STOPFORPEDS_OBEYLIGHTS;
    }
    const bool policeChase = !water && rating != 13 && FindPlayerPed()->GetWantedLevel() == eWantedLevel::WANTED_CLEAN
        && (aggressive || TimeNextMadDriverChaseCreated <= 0.0f) && !specialZone && CreatePoliceChase(vehicle, rating, from);
    if (policeChase) {
        TimeNextMadDriverChaseCreated = CGameLogic::LaRiotsActiveHere() ? CGeneral::GetRandomNumberInRange(240.0f, 480.0f) : CGeneral::GetRandomNumberInRange(600.0f, 1200.0f);
    } else if (madDriver) {
        const auto canConvoy = model == MODEL_FREEWAY || model == MODEL_PCJ600 || model == MODEL_FCR900 || model == MODEL_NRG500 || model == MODEL_BF400 || model == MODEL_WAYFARER;
        const bool convoy = canConvoy && !gbLARiots && !randomInt(7) && CreateConvoy(vehicle, rating);
        SetUpDriverAndPassengersForVehicle(vehicle, rating, 1, true, false, 99);
        if (!convoy) {
            vehicle->SetStatus(STATUS_PHYSICS);
            ap.m_nCarDrivingStyle = DRIVING_STYLE_AVOID_CARS;
            ap.m_nCruiseSpeed += 10;
            vehicle->m_vecMoveSpeed = vehicle->GetForward() * ap.m_nCruiseSpeed * 0.02f;
            if ((CGameLogic::LaRiotsActiveHere() || aggressive) && vehicle->m_pDriver) {
                vehicle->m_pDriver->bWantedByPolice = true;
            }
            flags.bMadDriver = true;
        }
    } else if (rating == 13 || rating == 24) {
        CCarAI::AddPoliceCarOccupants(vehicle, false);
    } else {
        bCarIsBeingCreated = true;
        SetUpDriverAndPassengersForVehicle(vehicle, rating, 0, false, false, 99);
        bCarIsBeingCreated = false;
    }
    if (rating == 13 || rating == 24) {
        vehicle->ChangeLawEnforcerState(true);
    }
    CStreaming::PossiblyStreamCarOutAfterCreation(model);
    modelInfo->m_nTimesUsed = std::min(static_cast<int32>(modelInfo->m_nTimesUsed) + 1, 120);
}

// 0x4341C0
void CCarCtrl::GenerateRandomCars() {
    if (CCutsceneMgr::ms_running) {
        CountDownToCarsAtStart = 2;
        return;
    }
    if (CGangWars::DontCreateCivilians() || !CGame::CanSeeOutSideFromCurrArea()) {
        return;
    }

    if (CGameLogic::LaRiotsActiveHere() && TimeNextMadDriverChaseCreated > 480.0f) {
        TimeNextMadDriverChaseCreated = CGeneral::GetRandomNumberInRange(240.0f, 480.0f);
    }
    TimeNextMadDriverChaseCreated -= (CTimer::GetTimeStep() * 0.02f);

    if (NumRandomCars < 45) {
        if (CountDownToCarsAtStart) {
            CountDownToCarsAtStart--;
            for (auto i = 100; i --> 0;) {
                GenerateOneRandomCar();
            }
            CTheCarGenerators::GenerateEvenIfPlayerIsCloseCounter = 20;
        } else {
            GenerateOneRandomCar();
            GenerateOneRandomCar();
        }
    }

}

// 0x42F3C0
void CCarCtrl::GetAIHeliToAttackPlayer(CAutomobile* automobile) {
    const auto playerPos = FindPlayerCoors();
    const auto delta = playerPos - automobile->GetPosition();
    auto orientation = CGeneral::GetATanOfXY(delta.x, delta.y);
    auto distance = delta.Magnitude2D();
    automobile->AsHeli()->m_fMaxAltitude = playerPos.z;
    auto& autoPilot = automobile->m_autoPilot;
    autoPilot.m_vecDestinationCoors = playerPos;

    if (autoPilot.m_nCarMission == MISSION_HELI_ATTACK_PLAYER) {
        if (distance < 15.0f) {
            autoPilot.m_nCarMission = MISSION_HELI_ATTACK_PLAYER_FLY_AWAY;
        }
        distance += 50.0f;
    } else if (autoPilot.m_nCarMission == MISSION_HELI_ATTACK_PLAYER_FLY_AWAY) {
        if (distance > 18.0f) {
            autoPilot.m_nCarMission = MISSION_HELI_ATTACK_PLAYER;
        }
        orientation += PI;
    }

    FlyAIHeliInCertainDirection(automobile->AsHeli(), orientation, distance, false);
    TestWhetherToFirePlaneGuns(automobile, FindPlayerEntity());
    FireHeliRocketsAtTarget(automobile, FindPlayerEntity());
}

// 0x42A730
void CCarCtrl::GetAIHeliToFlyInDirection(CAutomobile* automobile) {
    FlyAIHeliInCertainDirection(automobile->AsHeli(), automobile->m_fAircraftGoToHeading, 1000.0f, false);
}

// 0x429780
void CCarCtrl::GetAIPlaneToAttackPlayer(CAutomobile* automobile) {
    const auto playerCoors = FindPlayerCoors(-1);
    const auto playerSpeed = FindPlayerSpeed(-1);
    const auto planeCoors = automobile->GetPosition();
    auto* plane = automobile->AsPlane();
    plane->m_planeHeading = CGeneral::GetATanOfXY(
        playerCoors.x + playerSpeed.x * 50.0f - planeCoors.x,
        playerCoors.y + playerSpeed.y * 50.0f - planeCoors.y
    );
    plane->m_maxAltitude = playerCoors.z + playerSpeed.z * 50.0f;
    FlyAIPlaneInCertainDirection(plane);

    if (auto* playerVehicle = FindPlayerVehicle(-1, false)) {
        if (playerVehicle->GetVehicleAppearance() == VEHICLE_APPEARANCE_PLANE) {
            TriggerDogFightMoves(automobile, playerVehicle);
        }
        TestWhetherToFirePlaneGuns(automobile, playerVehicle);
        PossiblyFireHSMissile(automobile, playerVehicle);
    } else {
        TestWhetherToFirePlaneGuns(automobile, nullptr);
        PossiblyFireHSMissile(automobile, nullptr);
    }
}

// 0x429890
void CCarCtrl::GetAIPlaneToDoDogFight(CAutomobile* automobile) {
    auto* const target = automobile->m_autoPilot.m_TargetEntity;
    const auto targetPos = target->GetPosition() + target->m_vecMoveSpeed * 50.0f;
    const auto vehiclePos = automobile->GetPosition();
    auto* const plane = automobile->AsPlane();

    if (automobile->m_autoPilot.m_bPlaneDogfightSomething) {
        const auto& destination = automobile->m_autoPilot.m_vecDestinationCoors;
        plane->m_planeHeading = CGeneral::GetATanOfXY(destination.x - vehiclePos.x, destination.y - vehiclePos.y);
        plane->m_maxAltitude = destination.z;

        if ((destination - vehiclePos).Magnitude2D() < 50.0f) {
            automobile->m_autoPilot.m_bPlaneDogfightSomething = false;
        }
    } else {
        plane->m_planeHeading = CGeneral::GetATanOfXY(targetPos.x - vehiclePos.x, targetPos.y - vehiclePos.y);
        plane->m_maxAltitude = targetPos.z;

        if ((CGeneral::GetRandomNumber() & 0x3FF) == 0x1F4) {
            automobile->m_autoPilot.m_bPlaneDogfightSomething = true;
            automobile->m_autoPilot.m_vecDestinationCoors = CVector{
                targetPos.x + CGeneral::GetRandomNumber() * (600.0f / 32766.0f) - 300.0f,
                targetPos.y + CGeneral::GetRandomNumber() * (600.0f / 32766.0f) - 300.0f,
                targetPos.z + 50.0f
            };
        }
    }

    FlyAIPlaneInCertainDirection(plane);
    TestWhetherToFirePlaneGuns(automobile, target);
    PossiblyFireHSMissile(automobile, target);
}

// 0x42F370
void CCarCtrl::GetAIPlaneToDoDogFightAgainstPlayer(CAutomobile* automobile) {
    automobile->m_autoPilot.m_TargetEntity = FindPlayerVehicle(-1, false);
    if (!automobile->m_autoPilot.m_TargetEntity)
        automobile->m_autoPilot.m_TargetEntity = reinterpret_cast<CVehicle*>(FindPlayerPed(-1)); // Original stores the ped in the entity slot
    GetAIPlaneToDoDogFight(automobile);
}

// 0x421440
CVehicle* CCarCtrl::GetNewVehicleDependingOnCarModel(int32 modelId, uint8 createdByRaw) {
    const auto createdBy = static_cast<eVehicleCreatedBy>(createdByRaw);
    switch (CModelInfo::GetModelInfo(modelId)->AsVehicleModelInfoPtr()->m_nVehicleType) {
    case VEHICLE_TYPE_MTRUCK:
        return new CMonsterTruck(modelId, createdBy);
    case VEHICLE_TYPE_QUAD:
        return new CQuadBike(modelId, createdBy);
    case VEHICLE_TYPE_HELI:
        return new CHeli(modelId, createdBy);
    case VEHICLE_TYPE_PLANE:
        return new CPlane(modelId, createdBy);
    case VEHICLE_TYPE_BOAT:
        return new CBoat(modelId, createdBy);
    case VEHICLE_TYPE_TRAIN:
        return new CTrain(modelId, createdBy);
    case VEHICLE_TYPE_BIKE:
        return new CBike(modelId, createdBy);
    case VEHICLE_TYPE_BMX:
        return new CBmx(modelId, createdBy);
    case VEHICLE_TYPE_TRAILER:
        return new CTrailer(modelId, createdBy);
    case VEHICLE_TYPE_AUTOMOBILE:
        return new CAutomobile(modelId, createdBy, 1);
    }
    return nullptr;
}

// 0x42C250
bool CCarCtrl::IsAnyoneParking() {
    for (auto& veh : GetVehiclePool()->GetAllValid()) {
        switch (veh.m_autoPilot.m_nCarMission) {
        case eCarMission::MISSION_PARK_PARALLEL:
        case eCarMission::MISSION_PARK_PARALLEL_2:
        case eCarMission::MISSION_PARK_PERPENDICULAR:
        case eCarMission::MISSION_PARK_PERPENDICULAR_2:
            return true;
        }
    }
    return false;
}

// 0x42DAB0
bool CCarCtrl::IsThisAnAppropriateNode(CVehicle* vehicle, CNodeAddress veryOld, CNodeAddress old, CNodeAddress candidate, bool againstTraffic) {
    if (!ThePaths.m_pPathNodes[candidate.m_wAreaId] || veryOld == candidate) {
        return false;
    }
    const auto* const oldNode = ThePaths.GetPathNode(old);
    const auto* const node = ThePaths.GetPathNode(candidate);
    if (oldNode->m_bWaterNode != node->m_bWaterNode && vehicle->GetModelIndex() != MODEL_VORTEX) {
        return false;
    }
    const auto& bounds = vehicle->GetColModel()->GetBoundingBox();
    switch (node->m_nBehaviourType) {
    case 1:
    case 2: {
        const auto* const driver = vehicle->m_pDriver;
        if (vehicle->GetCreatedBy() == MISSION_VEHICLE || (driver && driver->GetCreatedBy() == 2)) {
            return false;
        }
        const auto* const model = CModelInfo::GetVehicleModelInfo(vehicle->GetModelIndex());
        if (node->m_nBehaviourType == 1 && model->m_nVehicleClass == VEHICLE_CLASS_BIG) {
            return false;
        }
        if (!againstTraffic) {
            const auto delta = node->GetPosition() - vehicle->GetPosition();
            if (node->m_nBehaviourType == 1) {
                if (CrossProduct(node->GetPosition() - oldNode->GetPosition(), delta).z < 0.0f) {
                    return false;
                }
            } else if (DotProduct(vehicle->GetRight(), delta) < 0.0f) {
                return false;
            }
        }
        if (vehicle->IsLawEnforcementVehicle() || model->m_nVehicleClass == VEHICLE_CLASS_BIG
            || (driver && driver->m_nPedType == PED_TYPE_PROSTITUTE) || IsAnyoneParking()) {
            return false;
        }
        int16 count{};
        CWorld::FindObjectsKindaColliding(node->GetPosition(), 5.0f, true, &count, 2, nullptr,
            false, true, false, false, false);
        return count == 0;
    }
    case 5:
    case 10:
        if (againstTraffic || bounds.m_vecMax.z < 2.0f) {
            return false;
        }
        break;
    case 8:
    case 9:
        if (againstTraffic || bounds.m_vecMax.z > 1.5f || bounds.m_vecMax.x > 2.0f || bounds.m_vecMax.y > 4.0f) {
            return false;
        }
        break;
    }
    if (vehicle->GetCreatedBy() == MISSION_VEHICLE && vehicle->m_autoPilot.m_nCarMission == MISSION_CRUISE
        && node->m_bDontWander && !oldNode->m_bDontWander) {
        return false;
    }
    return (!node->m_onDeadEnd || oldNode->m_onDeadEnd)
        && (!node->m_isSwitchedOff || oldNode->m_isSwitchedOff) && !againstTraffic;
}

// 0x423EA0
bool CCarCtrl::IsThisVehicleInteresting(CVehicle* vehicle) {
    for (auto& car : apCarsToKeep) {
        if (car == vehicle) {
            return true;
        }
    }
    return false;
}

// 0x432CB0
void CCarCtrl::JoinCarWithRoadAccordingToMission(CVehicle* vehicle) {
    switch (vehicle->m_autoPilot.m_nCarMission) {
    case MISSION_NONE:
    case MISSION_CRUISE:
    case MISSION_WAITFORDELETION:
    case MISSION_EMERGENCYVEHICLE_STOP:
    case MISSION_STOP_FOREVER:
    case MISSION_FOLLOW_RECORDED_PATH:
    case MISSION_PARK_PERPENDICULAR:
    case MISSION_PARK_PARALLEL:
    case MISSION_PARK_PERPENDICULAR_2:
    case MISSION_PARK_PARALLEL_2:
        return JoinCarWithRoadSystem(vehicle);
    case MISSION_RAMPLAYER_FARAWAY:
    case MISSION_RAMPLAYER_CLOSE:
    case MISSION_BLOCKPLAYER_FARAWAY:
    case MISSION_BLOCKPLAYER_CLOSE:
    case MISSION_BLOCKPLAYER_HANDBRAKESTOP:
    case MISSION_BOAT_ATTACKPLAYER:
    case MISSION_SLOWLY_DRIVE_TOWARDS_PLAYER_1:
    case MISSION_SLOWLY_DRIVE_TOWARDS_PLAYER_2:
    case MISSION_BLOCKPLAYER_FORWARDANDBACK:
    case MISSION_APPROACHPLAYER_FARAWAY:
    case MISSION_APPROACHPLAYER_CLOSE:
    case MISSION_BOAT_CIRCLEPLAYER: {
        JoinCarWithRoadSystemGotoCoors(vehicle, FindPlayerCoors(-1), true, vehicle->IsSubBoat());
        break;
    }
    case MISSION_GOTOCOORDINATES:
    case MISSION_GOTOCOORDINATES_STRAIGHTLINE:
    case MISSION_GOTOCOORDINATES_ACCURATE:
    case MISSION_GOTOCOORDINATES_STRAIGHTLINE_ACCURATE:
    case MISSION_GOTOCOORDINATES_ASTHECROWSWIMS:
    case MISSION_GOTOCOORDINATES_RACING: {
        JoinCarWithRoadSystemGotoCoors(vehicle, vehicle->m_autoPilot.m_vecDestinationCoors, true, vehicle->IsSubBoat());
        break;
    }
    case MISSION_RAMCAR_FARAWAY:
    case MISSION_RAMCAR_CLOSE:
    case MISSION_BLOCKCAR_FARAWAY:
    case MISSION_BLOCKCAR_CLOSE:
    case MISSION_BLOCKCAR_HANDBRAKESTOP:
    case MISSION_PROTECTION_REAR:
    case MISSION_PROTECTION_FRONT:
    case MISSION_ESCORT_LEFT:
    case MISSION_ESCORT_RIGHT:
    case MISSION_ESCORT_REAR:
    case MISSION_ESCORT_FRONT:
    case MISSION_FOLLOWCAR_FARAWAY:
    case MISSION_FOLLOWCAR_CLOSE:
    case MISSION_KILLPED_FARAWAY:
    case MISSION_KILLPED_CLOSE:
    case MISSION_DO_DRIVEBY_CLOSE:
    case MISSION_DO_DRIVEBY_FARAWAY:
    case MISSION_ESCORT_LEFT_FARAWAY:
    case MISSION_ESCORT_RIGHT_FARAWAY:
    case MISSION_ESCORT_REAR_FARAWAY:
    case MISSION_ESCORT_FRONT_FARAWAY: {
        JoinCarWithRoadSystemGotoCoors(vehicle, vehicle->m_autoPilot.m_TargetEntity->GetPosition(), true, vehicle->IsSubBoat());
        break;
    }
    }
}

// 0x42F5A0
void CCarCtrl::JoinCarWithRoadSystem(CVehicle* vehicle) {
    auto& ap = vehicle->m_autoPilot;
    ap.m_endingRouteNode = {};
    ap.m_currentAddress = {};
    ap.m_nCurrentPathNodeInfo = {};
    ap.m_nNextPathNodeInfo = {};
    ap.m_nPreviousPathNodeInfo = {};

    const auto pos = vehicle->GetPosition();
    const auto forward = vehicle->GetMatrix().GetForward();
    auto node = ThePaths.FindNodeClosestToCoorsFavourDirection(pos, PATH_TYPE_VEH, { forward.x, forward.y });
    const auto* pathNode = ThePaths.GetPathNode(node);
    if (!pathNode || !pathNode->m_nNumLinks)
        return;

    auto previous = CNodeAddress{};
    auto closestDistance = std::numeric_limits<float>::max();
    for (const auto& candidate : ThePaths.GetNodeLinkedNodes(*pathNode)) {
        const auto distance = (candidate.GetPosition() - pathNode->GetPosition()).Magnitude2D();
        if (distance < closestDistance) {
            closestDistance = distance;
            previous = candidate.GetAddress();
        }
    }
    if (!previous.IsValid())
        return;

    const auto direction = pathNode->GetPosition() - ThePaths.GetPathNode(previous)->GetPosition();
    if (DotProduct2D(direction, { forward.x, forward.y }) < 0.0f)
        std::swap(previous, node);

    ap.m_currentAddress = previous;
    ap.m_nNextPathNodeInfo = ThePaths.FindLinkBetweenNodes(previous, node);
    FindLinksToGoWithTheseNodes(vehicle);
    ap.m_nCurrentLane = 0;
    ap.m_nNextLane = 0;
}

// 0x42F870
bool CCarCtrl::JoinCarWithRoadSystemGotoCoors(CVehicle* vehicle, const CVector& posn, bool unused, bool bIsBoat) {
    auto& autoPilot = vehicle->m_autoPilot;
    autoPilot.m_vecDestinationCoors = posn;
    auto& count = reinterpret_cast<int16&>(autoPilot.m_nPathFindNodesCount);
    ThePaths.DoPathSearch(
        PATH_TYPE_VEH, vehicle->GetPosition(), {}, posn,
        autoPilot.m_aPathFindNodesInfo.data(), count, 8, nullptr,
        999999.0f, nullptr, 999999.0f,
        autoPilot.carCtrlFlags.bCantGoAgainstTraffic, {},
        vehicle->GetModelIndex() == MODEL_VORTEX, bIsBoat
    );
    ThePaths.RemoveBadStartNode(vehicle->GetPosition(), autoPilot.m_aPathFindNodesInfo.data(), &count);
    if (count < 2) {
        JoinCarWithRoadSystem(vehicle);
        autoPilot.m_nPathFindNodesCount = 0;
        return true;
    }

    autoPilot.m_currentAddress = autoPilot.m_aPathFindNodesInfo[0];
    autoPilot.m_endingRouteNode.ResetAreaId();
    autoPilot.RemoveOnePathNode();
    autoPilot.m_startingRouteNode = autoPilot.m_aPathFindNodesInfo[0];
    autoPilot.RemoveOnePathNode();
    FindLinksToGoWithTheseNodes(vehicle);
    autoPilot.m_nCurrentLane = 0;
    autoPilot.m_nNextLane = 0;
    return false;
}

// 0x424100
bool CCarCtrl::MapCouldMoveInThisArea(float x, float y) {
    return false; // Stub in the PC version (The binary ignores the arguments)
}

// 0x432B10
bool CCarCtrl::PickNextNodeAccordingStrategy(CVehicle* vehicle) {
    switch (vehicle->m_autoPilot.m_nCarMission) {
    case MISSION_RAMPLAYER_FARAWAY:
    case MISSION_BLOCKPLAYER_FARAWAY: {
        const auto playerPos = FindPlayerCoors(-1);
        PickNextNodeToChaseCar(vehicle, playerPos.x, playerPos.y, playerPos.z);
        return false;
    }
    case MISSION_APPROACHPLAYER_FARAWAY: {
        const auto playerPos = FindPlayerCoors(-1);
        PickNextNodeToChaseCar(vehicle, playerPos.x, playerPos.y, playerPos.z);
        return false;
    }
    case MISSION_GOTOCOORDINATES:
    case MISSION_GOTOCOORDINATES_ACCURATE:
        return PickNextNodeToFollowPath(vehicle);
    case MISSION_RAMCAR_FARAWAY:
    case MISSION_BLOCKCAR_FARAWAY:
    case MISSION_FOLLOWCAR_FARAWAY:
    case MISSION_KILLPED_FARAWAY:
    case MISSION_DO_DRIVEBY_FARAWAY:
    case MISSION_PLANE_DOG_FIGHT_PLAYER:
    case MISSION_BOAT_CIRCLEPLAYER:
    case MISSION_ESCORT_LEFT_FARAWAY:
    case MISSION_ESCORT_RIGHT_FARAWAY:
        if (vehicle->m_autoPilot.m_TargetEntity) {
            const auto targetPos = vehicle->m_autoPilot.m_TargetEntity->GetPosition();
            PickNextNodeToChaseCar(vehicle, targetPos.x, targetPos.y, targetPos.z);
        }
        return false;
    default:
        PickNextNodeRandomly(vehicle);
        return false;
    }
}

// 0x421740
void CCarCtrl::InitSequence(int32 numSequenceElements) {
    SequenceElements = numSequenceElements;
    SequenceRandomOffset = CGeneral::GetRandomNumber() % numSequenceElements;
    // The executable takes bit 4 of the second random value (0x421740).
    bSequenceOtherWay = (CGeneral::GetRandomNumber() >> 4) & 1;
}

// 0x42DE80
void CCarCtrl::PickNextNodeRandomly(CVehicle* vehicle) {
    if (vehicle->m_nForcedRandomRouteSeed) {
        srand(vehicle->m_nForcedRandomRouteSeed);
    }
    auto& ap = vehicle->m_autoPilot;
    const auto veryOld = ap.m_currentAddress;
    const auto old = ap.m_startingRouteNode;
    if (!ThePaths.m_pPathNodes[veryOld.m_wAreaId] || !ThePaths.m_pPathNodes[old.m_wAreaId]
        || !ThePaths.m_pPathNodes[ap.m_nNextPathNodeInfo.m_wAreaId]) {
        return;
    }
    const auto* const oldNode = ThePaths.GetPathNode(old);
    const auto& oldLink = ThePaths.GetCarPathLink(ap.m_nNextPathNodeInfo);
    const auto attached = oldLink.m_attachedTo == old;
    const auto lanes = attached ? oldLink.m_numOppositeDirLanes : oldLink.m_numSameDirLanes;
    const bool wasOneWay = !(attached ? oldLink.m_numSameDirLanes : oldLink.m_numOppositeDirLanes);
    auto directions = 0;
    if (ap.m_nNextLane == 0) {
        directions = 4;
    }
    if (ap.m_nNextLane == static_cast<int>(lanes) - 1) {
        directions |= 2;
    }
    if (lanes < 3 || !directions) {
        directions |= 1;
    }
    ap.m_endingRouteNode = veryOld;
    ap.m_currentAddress = old;
    if (ThisVehicleShouldTryNotToTurn(vehicle)) {
        directions = 1;
    }
    InitSequence(oldNode->m_nNumLinks);
    CNodeAddress next = veryOld;
    auto nextLinkAddress = ap.m_nNextPathNodeInfo;
    bool found = false;
    for (auto pass = 0; pass < 3 && !found; pass++) {
        for (auto i = 0u; i < oldNode->m_nNumLinks; i++) {
            const auto index = oldNode->m_wBaseLinkId + FindSequenceElement(i);
            const auto candidate = ThePaths.m_pNodeLinks[old.m_wAreaId][index];
            ap.m_startingRouteNode = candidate;
            if (!ThePaths.m_pPathNodes[candidate.m_wAreaId]) {
                continue;
            }
            bool sharpTurn{};
            const auto direction = pass == 0 ? FindPathDirection(veryOld, old, candidate, &sharpTurn) : 0;
            const auto linkAddress = ThePaths.m_pNaviLinks[old.m_wAreaId][index];
            if (!ThePaths.m_pPathNodes[linkAddress.m_wAreaId]
                || (pass == 0 && vehicle->GetStatus() == STATUS_SIMPLE && sharpTurn)) {
                continue;
            }
            const auto& link = ThePaths.GetCarPathLink(linkAddress);
            const auto linkAttached = link.m_attachedTo == old;
            const bool againstTraffic = !(linkAttached ? link.m_numSameDirLanes : link.m_numOppositeDirLanes);
            const bool oneWay = !(linkAttached ? link.m_numOppositeDirLanes : link.m_numSameDirLanes);
            if (pass == 0) {
                if (!IsThisAnAppropriateNode(vehicle, veryOld, old, candidate, againstTraffic)
                    || !(direction & directions) || (wasOneWay && oneWay)) {
                    continue;
                }
            } else if (candidate == veryOld || againstTraffic
                || (pass == 1 && ThePaths.GetPathNode(candidate)->m_isSwitchedOff
                    && !ThePaths.GetPathNode(veryOld)->m_isSwitchedOff)) {
                continue;
            }
            next = candidate;
            nextLinkAddress = linkAddress;
            found = true;
            break;
        }
    }
    ap.m_startingRouteNode = next;
    if (next == veryOld && vehicle->GetStatus() != STATUS_PHYSICS) {
        SwitchVehicleToRealPhysics(vehicle);
        vehicle->m_nFakePhysics = 0;
    }
    const auto behaviour = ThePaths.GetPathNode(next)->m_nBehaviourType;
    if (behaviour == 1 || behaviour == 2) {
        SwitchVehicleToRealPhysics(vehicle);
        vehicle->m_nFakePhysics = 0;
        ap.m_nCarMission = behaviour == 1 ? MISSION_PARK_PARALLEL : MISSION_PARK_PERPENDICULAR;
        ap.m_nCarDrivingStyle = DRIVING_STYLE_STOP_FOR_CARS;
    } else if (behaviour == 10) {
        ap.SetTempAction(TEMPACT_WAIT, 10000);
        if (vehicle->GetStatus() == STATUS_SIMPLE) {
            ap.ModifySpeed(0.0f);
        }
    }
    if (oldNode->m_nBehaviourType == 9 && vehicle->GetCreatedBy() != MISSION_VEHICLE) {
        ap.SetTempAction(TEMPACT_WAIT, 4500);
        if (vehicle->GetStatus() == STATUS_SIMPLE) {
            ap.ModifySpeed(0.0f);
        }
    }
    ap.m_timeToLeaveLink += ap.m_timeToGetToNextLink;
    ap.m_nPreviousPathNodeInfo = ap.m_nCurrentPathNodeInfo;
    ap.m_nCurrentPathNodeInfo = ap.m_nNextPathNodeInfo;
    ap._smthPrev = ap._smthCurr;
    ap._smthCurr = ap._smthNext;
    ap.m_nCurrentLane = ap.m_nNextLane;
    ap.m_nNextPathNodeInfo = nextLinkAddress;
    ap._smthNext = old.m_wAreaId < next.m_wAreaId
        || (old.m_wAreaId == next.m_wAreaId && old.m_wNodeId < next.m_wNodeId) ? -1 : 1;
    const auto& currentLink = ThePaths.GetCarPathLink(ap.m_nCurrentPathNodeInfo);
    const auto& nextLink = ThePaths.GetCarPathLink(nextLinkAddress);
    const auto nextLanes = !ThePaths.m_pPathNodes[nextLinkAddress.m_wAreaId] ? 1
        : ap._smthNext < 0 ? nextLink.m_numSameDirLanes : nextLink.m_numOppositeDirLanes;
    if ((ThePaths.GetPathNode(next)->GetPosition() - oldNode->GetPosition()).SquaredMagnitude2D() > 256.0f
        && ap.field_50-- == 1) {
        ap.field_50 = (CGeneral::GetRandomNumber() & 3) + 4;
        ap.m_nNextLane += CGeneral::GetRandomNumber() < 0x3FFF ? 1 : -1;
    }
    ap.m_nNextLane = std::max(0, std::min(static_cast<int>(ap.m_nNextLane), static_cast<int>(nextLanes) - 1));
    if (ap.carCtrlFlags.bStayInFastLane) {
        ap.m_nNextLane = 0;
    } else if (ap.carCtrlFlags.bStayInSlowLane) {
        ap.m_nNextLane = std::max(static_cast<int>(nextLanes) - 1, 0);
    }
    if (vehicle->GetStatus() == STATUS_SIMPLE) {
        const CVector2D currentDir = CVector2D{currentLink.m_dir} * ap._smthCurr;
        const CVector2D nextDir = CVector2D{nextLink.m_dir} * ap._smthNext;
        const auto currentOffset = (currentLink.OneWayLaneOffset() + ap.m_nCurrentLane) * 5.4f;
        const auto nextOffset = (nextLink.OneWayLaneOffset() + ap.m_nNextLane) * 5.4f;
        const auto currentPos = currentLink.GetNodeCoors() + CVector2D{currentDir.y, -currentDir.x} * currentOffset;
        const auto nextPos = nextLink.GetNodeCoors() + CVector2D{nextDir.y, -nextDir.x} * nextOffset;
        const auto scale = CCurves::CalcSpeedScaleFactor(
            {currentPos.x, currentPos.y, 0.0f}, {nextPos.x, nextPos.y, 0.0f},
            currentDir.x, currentDir.y, nextDir.x, nextDir.y);
        ap.m_timeToGetToNextLink = std::max(static_cast<int32>(scale * 1000.0f / ap.m_speed), 10);
    }
}

// 0x426EF0
bool CCarCtrl::PickNextNodeToChaseCar(CVehicle* vehicle, float destX, float destY, float destZ) {
    if (vehicle->m_nForcedRandomRouteSeed) {
        srand(vehicle->m_nForcedRandomRouteSeed);
    }
    auto& ap = vehicle->m_autoPilot;
    const auto old = ap.m_startingRouteNode;
    const auto* const oldNode = ThePaths.GetPathNode(old);
    const CVector destination{destX, destY, destZ};
    const auto restrictedSearch = static_cast<int>(CWeather::WeatherRegion) == 0 || static_cast<int>(CWeather::WeatherRegion) == 4;
    std::array<CNodeAddress, 2> route{};
    int16 count{};
    float distance{};
    ThePaths.DoPathSearch(PATH_TYPE_VEH, oldNode->GetPosition(), old, destination,
        route.data(), count, 2, &distance, restrictedSearch ? 50.0f : 999999.0f, nullptr,
        999999.0f, false, {}, vehicle->GetModelIndex() == MODEL_VORTEX, false);
    if (restrictedSearch && (!count || distance > (destination - vehicle->GetPosition()).Magnitude2D() * 3.0f)) {
        return true;
    }
    CNodeAddress next;
    auto linkIndex = 0;
    if (count == 1 || count == 2) {
        if (route[0] != old) {
            next = route[0];
        } else if (count == 2 && route[1] != old) {
            next = route[1];
        }
    }
    if (next.IsAreaValid()) {
        while (ThePaths.m_pNodeLinks[old.m_wAreaId][oldNode->m_wBaseLinkId + linkIndex] != next) {
            linkIndex++;
        }
    } else {
        const auto delta = destination - vehicle->GetPosition();
        const auto targetAngle = CGeneral::GetATanOfXY(delta.x, delta.y);
        auto bestAngle = 10.0f;
        for (auto i = 0u; i < oldNode->m_nNumLinks; i++) {
            const auto candidate = ThePaths.m_pNodeLinks[old.m_wAreaId][oldNode->m_wBaseLinkId + i];
            if ((candidate == ap.m_currentAddress && oldNode->m_nNumLinks > 1)
                || !ThePaths.m_pPathNodes[candidate.m_wAreaId]) {
                continue;
            }
            const auto direction = ThePaths.GetPathNode(candidate)->GetPosition() - oldNode->GetPosition();
            const auto angle = std::abs(CGeneral::LimitRadianAngle(CGeneral::GetATanOfXY(direction.x, direction.y) - targetAngle));
            if (angle <= bestAngle) {
                bestAngle = angle;
                next = candidate;
                linkIndex = i;
            }
        }
    }
    ap.m_endingRouteNode = ap.m_currentAddress;
    ap.m_currentAddress = old;
    ap.m_startingRouteNode = next;
    ap.m_timeToLeaveLink += ap.m_timeToGetToNextLink;
    ap.m_nPreviousPathNodeInfo = ap.m_nCurrentPathNodeInfo;
    ap.m_nCurrentPathNodeInfo = ap.m_nNextPathNodeInfo;
    ap._smthPrev = ap._smthCurr;
    ap._smthCurr = ap._smthNext;
    ap.m_nCurrentLane = ap.m_nNextLane;
    ap.m_nNextPathNodeInfo = ThePaths.m_pNaviLinks[old.m_wAreaId][oldNode->m_wBaseLinkId + linkIndex];
    if (StopCarIfNodesAreInvalid(vehicle)) {
        return true;
    }
    ap._smthNext = old.m_wAreaId < next.m_wAreaId
        || (old.m_wAreaId == next.m_wAreaId && old.m_wNodeId < next.m_wNodeId) ? -1 : 1;
    const auto& currentLink = ThePaths.GetCarPathLink(ap.m_nCurrentPathNodeInfo);
    const auto& nextLink = ThePaths.GetCarPathLink(ap.m_nNextPathNodeInfo);
    const auto lanes = ap._smthNext < 0 ? nextLink.m_numOppositeDirLanes : nextLink.m_numSameDirLanes;
    if ((nextLink.GetNodeCoors() - currentLink.GetNodeCoors()).SquaredMagnitude() > 256.0f) {
        switch (ap.m_nCarMission) {
        case MISSION_RAMPLAYER_FARAWAY:
        case MISSION_BLOCKPLAYER_FARAWAY:
        case MISSION_RAMCAR_FARAWAY:
        case MISSION_BLOCKCAR_FARAWAY:
        case MISSION_FOLLOWCAR_FARAWAY:
        case MISSION_KILLPED_FARAWAY:
        case MISSION_APPROACHPLAYER_FARAWAY:
        case MISSION_DO_DRIVEBY_FARAWAY:
            break;
        default:
            if (ap.field_50-- == 1) {
                ap.field_50 = (CGeneral::GetRandomNumber() & 3) + 4;
                ap.m_nNextLane += CGeneral::GetRandomNumber() < 0x3FFF ? 1 : -1;
            }
        }
    }
    ap.m_nNextLane = std::max(0, std::min(static_cast<int>(ap.m_nNextLane), static_cast<int>(lanes) - 1));
    if (ap.carCtrlFlags.bStayInFastLane) {
        ap.m_nNextLane = 0;
    } else if (ap.carCtrlFlags.bStayInSlowLane || vehicle->IsSubBMX()) {
        ap.m_nNextLane = std::max(static_cast<int>(lanes) - 1, 0);
    }
    currentLink.OneWayLaneOffset();
    nextLink.OneWayLaneOffset();
    return false;
}

// 0x427740
bool CCarCtrl::PickNextNodeToFollowPath(CVehicle* vehicle) {
    if (vehicle->m_nForcedRandomRouteSeed) {
        srand(vehicle->m_nForcedRandomRouteSeed);
    }
    auto& ap = vehicle->m_autoPilot;
    const auto previous = ap.m_startingRouteNode;
    if (!ap.m_nPathFindNodesCount) {
        ThePaths.DoPathSearch(PATH_TYPE_VEH, vehicle->GetPosition(), previous, ap.m_vecDestinationCoors,
            ap.m_aPathFindNodesInfo.data(), reinterpret_cast<int16&>(ap.m_nPathFindNodesCount), 8,
            nullptr, 999999.0f, nullptr, 999999.0f, ap.carCtrlFlags.bCantGoAgainstTraffic, {},
            vehicle->GetModelIndex() == MODEL_VORTEX, false);
        if (ap.m_nPathFindNodesCount >= 2) {
            if (ap.m_startingRouteNode == ap.m_aPathFindNodesInfo[0]) {
                ap.RemoveOnePathNode();
            }
            if (ap.m_startingRouteNode == ap.m_aPathFindNodesInfo[0]) {
                ap.RemoveOnePathNode();
            }
        }
        if (ap.m_nPathFindNodesCount < 2) {
            return true;
        }
    }
    ap.m_endingRouteNode = ap.m_currentAddress;
    ap.m_currentAddress = ap.m_startingRouteNode;
    ap.m_startingRouteNode = ap.m_aPathFindNodesInfo[0];
    ap.RemoveOnePathNode();
    ap.m_timeToLeaveLink += ap.m_timeToGetToNextLink;
    ap.m_nPreviousPathNodeInfo = ap.m_nCurrentPathNodeInfo;
    ap.m_nCurrentPathNodeInfo = ap.m_nNextPathNodeInfo;
    ap._smthPrev = ap._smthCurr;
    ap._smthCurr = ap._smthNext;
    ap.m_nCurrentLane = ap.m_nNextLane;
    ap.m_nNextPathNodeInfo = ThePaths.FindLinkBetweenNodes(ap.m_currentAddress, ap.m_startingRouteNode);
    const auto next = ap.m_startingRouteNode;
    ap._smthNext = previous.m_wAreaId < next.m_wAreaId
        || (previous.m_wAreaId == next.m_wAreaId && previous.m_wNodeId < next.m_wNodeId) ? -1 : 1;
    const auto& currentLink = ThePaths.GetCarPathLink(ap.m_nCurrentPathNodeInfo);
    const auto& nextLink = ThePaths.GetCarPathLink(ap.m_nNextPathNodeInfo);
    const auto lanes = ap._smthNext < 0 ? nextLink.m_numSameDirLanes : nextLink.m_numOppositeDirLanes;
    if ((nextLink.GetNodeCoors() - currentLink.GetNodeCoors()).SquaredMagnitude() > 256.0f
        && !(CGeneral::GetRandomNumber() & 0x600)) {
        ap.m_nNextLane += CGeneral::GetRandomNumber() < 0x3FFF ? 1 : -1;
    }
    ap.m_nNextLane = std::max(0, std::min(static_cast<int>(ap.m_nNextLane), static_cast<int>(lanes) - 1));
    if (ap.carCtrlFlags.bStayInFastLane) {
        ap.m_nNextLane = 0;
    } else if (ap.carCtrlFlags.bStayInSlowLane) {
        ap.m_nNextLane = std::max(static_cast<int>(lanes) - 1, 0);
    }
    const CVector2D currentDir = CVector2D{currentLink.m_dir} * ap._smthCurr;
    const CVector2D nextDir = CVector2D{nextLink.m_dir} * ap._smthNext;
    const auto currentOffset = (currentLink.OneWayLaneOffset() + ap.m_nCurrentLane) * 5.4f;
    const auto nextOffset = (nextLink.OneWayLaneOffset() + ap.m_nNextLane) * 5.4f;
    const auto currentPos = currentLink.GetNodeCoors() + CVector2D{currentDir.y, -currentDir.x} * currentOffset;
    const auto nextPos = nextLink.GetNodeCoors() + CVector2D{nextDir.y, -nextDir.x} * nextOffset;
    const auto scale = CCurves::CalcSpeedScaleFactor(
        CVector{currentPos.x, currentPos.y, 0.0f}, CVector{nextPos.x, nextPos.y, 0.0f},
        currentDir.x, currentDir.y, nextDir.x, nextDir.y);
    ap.m_timeToGetToNextLink = std::max(static_cast<int32>(scale * 1000.0f / ap.m_speed), 10);
    if (ThePaths.m_pPathNodes[next.m_wAreaId] && ThePaths.GetPathNode(next)->m_nBehaviourType == 2) {
        SwitchVehicleToRealPhysics(vehicle);
        ap.m_nCarMission = MISSION_PARK_PERPENDICULAR;
    }
    return false;
}

// 0x429600
void CCarCtrl::PossiblyFireHSMissile(CVehicle* entityLauncher, CEntity* targetEntity) {
    if (!targetEntity)
        return;

    if (CTimer::GetTimeInMS() / 2000u == CTimer::GetPreviousTimeInMS() / 2000u)
        return;

    const CVector launcherPos = entityLauncher->GetPosition();
    const CVector targetPos = targetEntity->GetPosition();
    CVector dir = targetPos - launcherPos;
    const float dist = dir.Magnitude();
    if (dist < 160.0f && dist > 30.0f) {
        CMatrix launcherMat = entityLauncher->GetMatrix();
        CVector dirNormalized = dir;
        dir.Normalise();
        if (DotProduct(launcherMat.GetForward(), dirNormalized) > 0.8f) {
            CProjectileInfo::AddProjectile(
                entityLauncher,
                eWeaponType::WEAPON_ROCKET_HS,
                launcherPos + launcherMat.GetForward() * 4.0f - launcherMat.GetUp() * 3.0f,
                1.0f,
                &entityLauncher->GetMatrix().GetForward(),
                targetEntity
            );
        }
    }
}

// 0x424F80
void CCarCtrl::PossiblyRemoveVehicle(CVehicle* vehicle) {
    if (static_cast<int8>(vehicle->m_nNumGettingIn) > 0
        || (vehicle->vehicleFlags.bIsRCVehicle && vehicle->m_autoPilot.m_TargetEntity)) {
        return;
    }
    const auto remove = [&] {
        CWorld::Remove(vehicle);
        delete vehicle;
    };
    const auto playerCentre = FindPlayerCentreOfWorld(CWorld::PlayerInFocus);
    const auto delta = vehicle->GetPosition() - playerCentre;
    const auto now = CTimer::GetTimeInMS();
    auto& flags = vehicle->vehicleFlags;
    if (!flags.bIsLocked && vehicle->CanBeDeleted() && !CCranes::IsThisCarBeingTargettedByAnyCrane(vehicle)) {
        if (flags.bFadeOut && CVisibilityPlugins::GetClumpAlpha(vehicle->GetRpClump()) == 0) {
            remove();
            return;
        }
        const auto& cam = TheCamera.GetActiveCam();
        auto range = 170.0f * TheCamera.m_fGenerationDistMultiplier;
        if (!vehicle->GetIsOnScreen() && !cam.m_bLookingBehind && !cam.m_bLookingLeft && !cam.m_bLookingRight
            && TheCamera.GetLookDirection() && vehicle->GetCreatedBy() != PARKED_VEHICLE
            && vehicle->GetModelIndex() != MODEL_AMBULAN && vehicle->GetModelIndex() != MODEL_FIRETRUK
            && !flags.bIsLawEnforcer && !flags.bDriverLastFrame && now >= vehicle->m_nTimeTillWeNeedThisCar
            && !flags.bVehicleCanBeTargettedByHS) {
            range = 45.0f;
        }
        if (TheCamera.m_mCameraMatrix.GetForward().z < -0.9f) {
            range = 70.0f;
        }
        range *= std::max(static_cast<float>(vehicle->m_nExtendedRemovalRange), 170.0f) * 0.00588235306f;
        if (delta.Magnitude2D() > range && vehicle->m_autoPilot.m_nCarMission != MISSION_PLANE_ATTACK_PLAYER_POLICE
            && !CGarages::IsPointWithinHideOutGarage(vehicle->GetPosition())) {
            if (IsThisVehicleInteresting(vehicle)) {
                vehicle->m_nFakePhysics = 10;
            } else if (vehicle->GetIsOnScreen()) {
                flags.bFadeOut = true;
            } else {
                remove();
            }
            return;
        }
        if (vehicle->GetStatus() == STATUS_SIMPLE && std::abs(vehicle->GetUp().z) < 0.74f) {
            remove();
            return;
        }
        if ((vehicle->GetStatus() == STATUS_PHYSICS || vehicle->GetStatus() == STATUS_WRECKED)
            && (vehicle->IsSubPlane() || vehicle->IsSubHeli()) && vehicle->m_bIsStuck) {
            remove();
            return;
        }
    }
    const auto style = vehicle->m_autoPilot.m_nCarDrivingStyle;
    if (!vehicle->IsSubHeli() && !vehicle->IsSubPlane() && vehicle->m_autoPilot.m_nTempAction != TEMPACT_STUCKINTRAFFIC
        && (vehicle->GetStatus() == STATUS_SIMPLE || (vehicle->GetStatus() == STATUS_PHYSICS
            && (style == DRIVING_STYLE_STOP_FOR_CARS || style == DRIVING_STYLE_STOP_FOR_CARS_IGNORE_LIGHTS)))
        && now - vehicle->m_autoPilot.m_nTimeSwitchedToRealPhysics > 5000 && !vehicle->m_nTimeTillWeNeedThisCar
        && now > 0 && !vehicle->GetIsOnScreen() && delta.Magnitude2D() > 22.0f
        && !IsThisVehicleInteresting(vehicle) && !flags.bIsLocked && vehicle->CanBeDeleted()
        && !CTrafficLights::ShouldCarStopForLight(vehicle, true) && !CTrafficLights::ShouldCarStopForBridge(vehicle)
        && !CGarages::IsPointWithinHideOutGarage(vehicle->GetPosition())) {
        remove();
        return;
    }
    if (vehicle->GetStatus() == STATUS_WRECKED && vehicle->m_nTimeWhenBlowedUp
        && now > vehicle->m_nTimeWhenBlowedUp + 60000 && now > vehicle->m_nTimeTillWeNeedThisCar
        && !vehicle->GetIsOnScreen() && delta.SquaredMagnitude() > 42.25f
        && !CGarages::IsPointWithinHideOutGarage(vehicle->GetPosition())) {
        remove();
    }
}

// 0x423F10
void CCarCtrl::PruneVehiclesOfInterest() {
    ZoneScoped;

    if ((CTimer::GetFrameCounter() % 64) == 19 && FindPlayerCoors(-1).z < 950.0f) {
        for (size_t i = 0; i < std::size(apCarsToKeep); i++) {
            if (apCarsToKeep[i]) {
                if (CTimer::GetTimeInMS() > aCarsToKeepTime[i] + 180000) {
                    apCarsToKeep[i] = nullptr;
                }
            }
        }
    }
}

// 0x42FC40
void CCarCtrl::ReconsiderRoute(CVehicle* vehicle) {
    if ((CTimer::GetTimeInMS() + vehicle->m_nRandomSeed) / 2000
        == (CTimer::GetPreviousTimeInMS() + vehicle->m_nRandomSeed) / 2000) {
        return;
    }
    auto& ap = vehicle->m_autoPilot;
    CVector destination;
    switch (ap.m_nCarMission) {
    case MISSION_RAMPLAYER_FARAWAY:
    case MISSION_BLOCKPLAYER_FARAWAY:
    case MISSION_APPROACHPLAYER_FARAWAY:
        destination = FindPlayerCoors(-1);
        break;
    case MISSION_GOTOCOORDINATES:
        destination = ap.m_vecDestinationCoors;
        break;
    case MISSION_RAMCAR_FARAWAY:
    case MISSION_BLOCKCAR_FARAWAY:
    case MISSION_KILLPED_FARAWAY:
    case MISSION_HELI_ATTACK_PLAYER_FLY_AWAY:
    case MISSION_DO_DRIVEBY_FARAWAY:
        destination = ap.m_TargetEntity->GetPosition();
        break;
    default:
        ap.m_ucCarMissionModeCounter = 0;
        return;
    }
    CNodeAddress oldNode, newNode;
    FindNodesThisCarIsNearestTo(vehicle, oldNode, newNode);
    if (!oldNode.IsAreaValid()) {
        return;
    }
    if ((ap.m_currentAddress == oldNode && ap.m_startingRouteNode == newNode)
        || (ap.m_currentAddress == newNode && ap.m_startingRouteNode == oldNode)
        || (ap.m_endingRouteNode == oldNode && ap.m_currentAddress == newNode)
        || (ap.m_endingRouteNode == newNode && ap.m_currentAddress == oldNode)
        || ap.m_currentAddress == newNode || ap.m_endingRouteNode == newNode) {
        ap.m_ucCarMissionModeCounter = 0;
        return;
    }
    if (++ap.m_ucCarMissionModeCounter <= 4) {
        return;
    }
    int16 count{};
    float distance{};
    ThePaths.DoPathSearch(PATH_TYPE_VEH, vehicle->GetPosition(), newNode, destination,
        nullptr, count, 0, &distance, 999999.0f, nullptr, 999999.0f,
        ap.carCtrlFlags.bCantGoAgainstTraffic, ap.m_currentAddress,
        vehicle->GetModelIndex() == MODEL_VORTEX, false);
    if (distance < 90000.0f && count >= 2) {
        ap.m_currentAddress = oldNode;
        ap.m_startingRouteNode = newNode;
        FindLinksToGoWithTheseNodes(vehicle);
        ThePaths.DoPathSearch(PATH_TYPE_VEH, vehicle->GetPosition(), newNode, destination,
            ap.m_aPathFindNodesInfo.data(), reinterpret_cast<int16&>(ap.m_nPathFindNodesCount), 8,
            nullptr, 999999.0f, nullptr, 999999.0f,
            ap.carCtrlFlags.bCantGoAgainstTraffic, ap.m_currentAddress,
            vehicle->GetModelIndex() == MODEL_VORTEX, false);
        ap.RemoveOnePathNode();
    }
    ap.m_ucCarMissionModeCounter = 0;
}

// 0x423DE0
void CCarCtrl::RegisterVehicleOfInterest(CVehicle* vehicle) {
    for (size_t i = 0; i < std::size(apCarsToKeep); i++) {
        if (apCarsToKeep[i] == vehicle) {
            aCarsToKeepTime[i] = CTimer::GetTimeInMS();
            return;
        }
    }

    for (size_t i = 0; i < std::size(apCarsToKeep); i++) {
        if (!apCarsToKeep[i]) {
            apCarsToKeep[i] = vehicle;
            aCarsToKeepTime[i] = CTimer::GetTimeInMS();
            return;
        }
    }

    size_t oldest = 0;
    for (size_t i = 1; i < std::size(apCarsToKeep); i++) {
        if (aCarsToKeepTime[i] < aCarsToKeepTime[oldest]) {
            oldest = i;
        }
    }
    apCarsToKeep[oldest] = vehicle;
    aCarsToKeepTime[oldest] = CTimer::GetTimeInMS();
}

// 0x4322B0
void CCarCtrl::RemoveCarsIfThePoolGetsFull() {
    ZoneScoped;

    if (CTimer::GetFrameCounter() % 8 != 3)
        return;

    if (GetVehiclePool()->GetNoOfFreeSpaces() >= 8)
        return;

    // Find closest deletable vehicle
    const CVector camPos = TheCamera.GetPosition();
    float fClosestDist = std::numeric_limits<float>::max();
    CVehicle* closestVeh = nullptr;
    for (auto& veh : GetVehiclePool()->GetAllValid()) {
        if (IsThisVehicleInteresting(&veh)) {
            continue;
        }
        if (veh.vehicleFlags.bIsLocked) {
            continue;
        }
        if (!veh.CanBeDeleted()) {
            continue;
        }
        if (CCranes::IsThisCarBeingTargettedByAnyCrane(&veh)) {
            continue;
        }

        const float fCamVehDist = (camPos - veh.GetPosition()).Magnitude();
        if (fClosestDist > fCamVehDist) {
            fClosestDist = fCamVehDist;
            closestVeh   = &veh;
        }
    }
    if (closestVeh) {
        CWorld::Remove(closestVeh);
        delete closestVeh;
    }
}

// 0x42CD10
void CCarCtrl::RemoveDistantCars() {
    ZoneScoped;

    // FIXBUGS: First remove vehicles that can be removed
    if (notsa::bugfixes::CCarCtrl_RemoveDistantCars_UseAfterFree) {
        for (auto& veh : GetVehiclePool()->GetAllValid()) {
            PossiblyRemoveVehicle(&veh);
        }
    }

    //... only then process them, this way we don't do use-after-free
    // only other solution would be `PossiblyRemoveVehicle` returning a `bool`
    // to indicate whenever the vehicle was deleted or not.
    for (auto& veh : GetVehiclePool()->GetAllValid()) {
        if (!notsa::bugfixes::CCarCtrl_RemoveDistantCars_UseAfterFree) {
            PossiblyRemoveVehicle(&veh); // This may or may not invalidate `veh`
        }
        if (!veh.vehicleFlags.bCreateRoadBlockPeds) {
            continue;
        }
        if (DistanceBetweenPoints(FindPlayerCentreOfWorld(), veh.GetPosition()) >= 54.5f) {
            continue;
        }
        CRoadBlocks::GenerateRoadBlockPedsForCar(
            &veh,
            veh.m_nPedsPositionForRoadBlock,
            veh.IsLawEnforcementVehicle() ? PED_TYPE_COP : PED_TYPE_GANG1
        );
        veh.vehicleFlags.bCreateRoadBlockPeds = false;
    }
}

// 0x423ED0
void CCarCtrl::RemoveFromInterestingVehicleList(CVehicle* vehicle) {
    for (auto& car : apCarsToKeep) {
        if (car == vehicle) {
            car = nullptr;
            break;
        }
    }
}

// 0x42CE40
void CCarCtrl::ScanForPedDanger(CVehicle* vehicle) {
    const auto scanRadius = vehicle == FindPlayerVehicle(-1, false) ? 44.0f : 11.0f;
    const auto vehiclePos = vehicle->GetPosition();
    const auto left = vehiclePos.x - scanRadius;
    const auto right = vehiclePos.x + scanRadius;
    const auto top = vehiclePos.y - scanRadius;
    const auto bottom = vehiclePos.y + scanRadius;
    const auto xStart = std::max(0, CWorld::GetSectorX(left));
    const auto xEnd = std::min(MAX_SECTORS_X - 1, CWorld::GetSectorX(right));
    const auto yStart = std::max(0, CWorld::GetSectorY(top));
    const auto yEnd = std::min(MAX_SECTORS_Y - 1, CWorld::GetSectorY(bottom));
    auto maxSpeed = vehicle->m_autoPilot.m_fMaxTrafficSpeed;

    CWorld::AdvanceCurrentScanCode();
    for (auto y = yStart; y <= yEnd; y++) {
        for (auto x = xStart; x <= xEnd; x++) {
            auto& sector = CWorld::GetRepeatSector(x, y);
            SlowCarDownForPedsSectorList(sector.Peds, vehicle, left, top, right, bottom, &maxSpeed, vehicle->m_autoPilot.m_fMaxTrafficSpeed);
        }
    }
    vehicle->vehicleFlags.bWarnedPeds = true;
}

// 0x42FBC0
bool CCarCtrl::ScriptGenerateOneEmergencyServicesCar(uint32 modelId, CVector posn) {
    if (CStreaming::IsModelLoaded(modelId)) {
        if (auto pAuto = GenerateOneEmergencyServicesCar(modelId, posn)) {
            pAuto->m_autoPilot.m_vecDestinationCoors = posn;
            pAuto->m_autoPilot.SetCarMission(JoinCarWithRoadSystemGotoCoors(pAuto, posn, false, false) ? MISSION_GOTOCOORDINATES_STRAIGHTLINE : MISSION_GOTOCOORDINATES);
            return true;
        }
    }
    return false;
}

// 0x4342A0
void CCarCtrl::SetCoordsOfScriptCar(CVehicle* vehicle, float x, float y, float z, uint8 clearOrientation, uint8 addOffset) {
    const auto handle = GetVehiclePool()->GetRef(vehicle);
    if (z <= -100.0f) {
        z = CWorld::FindGroundZForCoord(x, y);
    }
    if (addOffset) {
        z += vehicle->GetDistanceFromCentreOfMassToBaseOfModel();
    }
    vehicle->SetIsStatic(false);
    CTheScripts::StuckCars.ClearStuckFlagForCar(handle);
    const auto isBoat = vehicle->IsBoat();
    const CVector position{x, y, z};
    vehicle->Teleport(position, clearOrientation != 0);
    if (!isBoat) {
        if (vehicle->IsAutomobile() || vehicle->IsTrailer()) {
            vehicle->AsAutomobile()->PlaceOnRoadProperly();
        } else if (vehicle->IsBike()) {
            vehicle->AsBike()->PlaceOnRoadProperly();
        }
    }
    CTheScripts::ClearSpaceForMissionEntity(position, vehicle);
    if (!isBoat) {
        JoinCarWithRoadAccordingToMission(vehicle);
    }
    vehicle->m_autoPilot.m_nTempAction = TEMPACT_NONE;
}

// 0x4217C0
void CCarCtrl::SetUpDriverAndPassengersForVehicle(CVehicle* vehicle, int32 carRating, int32 minPassengers, bool mustBeMale, bool criminal, int32 maxPassengers) {
    vehicle->SetUpDriver(carRating, mustBeMale, criminal);
    const auto isGang = carRating >= 14 && carRating <= 23;
    if (isGang && CGeneral::GetRandomNumber() < 0x3FFF) {
        vehicle->m_pDriver->GiveObjectToPedToHold(MI_GANG_SMOKE, 1);
    }

    const auto maximum = std::min(static_cast<uint32>(maxPassengers), static_cast<uint32>(vehicle->m_nMaxPassengers));
    auto passengers = static_cast<uint32>(minPassengers);
    for (auto i = passengers; i < maximum; i++) {
        if (CGeneral::GetRandomNumber() * 3.05185094e-05f < 0.125f) {
            passengers++;
        }
    }
    passengers = std::min(passengers, maximum);
    const auto model = vehicle->GetModelIndex();
    if (CModelInfo::IsCarModel(model)
        && CModelInfo::GetModelInfo(model)->GetAnimFileIndex() == CAnimManager::GetAnimationBlockIndex("van")) {
        passengers = std::min(passengers, 1u);
    }
    for (uint32 seat = 0; seat < passengers; seat++) {
        auto* passenger = vehicle->SetupPassenger(seat, carRating, mustBeMale, criminal);
        if (passenger && isGang && CGeneral::GetRandomNumber() < 0x3FFF) {
            passenger->GiveObjectToPedToHold(MI_GANG_SMOKE, 1);
        }
    }
}

// 0x432420
template<typename PtrListType>
void CCarCtrl::SlowCarDownForCarsSectorList(PtrListType& ptrList, CVehicle* vehicle, float minX, float minY, float maxX, float maxY, float* maxSpeed, float originalMaxSpeed) {
    for (auto* item : ptrList) {
        auto* other = static_cast<CEntity*>(item);
        if (other == vehicle || other->IsScanCodeCurrent() || !other->m_bUsesCollision) {
            continue;
        }
        other->SetCurrentScanCode();
        const auto centre = other->GetBoundCentre();
        if (!(centre.x > minX && centre.x < maxX && centre.y > minY && centre.y < maxY)
            || !(std::abs(centre.z - vehicle->GetPosition().z) < 10.0f)) {
            continue;
        }
        const auto& position = vehicle->GetPosition();
        const auto& forward = vehicle->GetMatrix().GetForward();
        const auto distance = CCollision::DistAlongLine2D(position.x, position.y, forward.x, forward.y, centre.x, centre.y);
        if (std::abs(centre.z - (position.z + distance * vehicle->GetForward().z)) < 3.0f) {
            SlowCarDownForOtherCar(other, vehicle, maxSpeed, originalMaxSpeed);
        }
    }
}

// 0x426220
void CCarCtrl::SlowCarDownForObject(CEntity* entity, CVehicle* vehicle, float* arg3, float arg4) {
    const CVector entityDir = entity->GetPosition() - vehicle->GetPosition();
    const float entityHeading = DotProduct(entityDir, vehicle->GetMatrix().GetForward());
    if (entityHeading > 0.0f && entityHeading < 20.0f) {
        if (entity->GetColModel()->GetBoundRadius() + vehicle->GetColModel()->GetBoundingBox().m_vecMax.x > fabs(DotProduct(entityDir, vehicle->GetMatrix().GetRight()))) {
            if (entityHeading >= 7.0f) {
                *arg3 = std::min(*arg3, (1.0f - (entityHeading - 7.0f) / 13.0f)) * arg4; // Original code multiplies by 0.07692308, which is the recp. of 13
            } else {
                *arg3 = 0.0f;
            }
        }
    }
}

// 0x42D4F0
template<typename PtrListType>
void CCarCtrl::SlowCarDownForObjectsSectorList(PtrListType& ptrList, CVehicle* vehicle, float minX, float minY, float maxX, float maxY, float* maxSpeed, float originalMaxSpeed) {
    for (auto* item : ptrList) {
        auto* object = static_cast<CEntity*>(item);
        if (object->IsScanCodeCurrent()) {
            continue;
        }
        object->SetCurrentScanCode();
        const auto model = object->GetModelIndex();
        if (model != MI_ROADWORKBARRIER1 && model != MI_ROADBLOCKFUCKEDCAR1 && model != MI_ROADBLOCKFUCKEDCAR2) {
            continue;
        }
        const auto centre = object->GetBoundCentre();
        if (!(centre.x > minX && centre.x < maxX && centre.y > minY && centre.y < maxY)
            || !(std::abs(centre.z - vehicle->GetPosition().z) < 10.0f)) {
            continue;
        }
        const auto& position = vehicle->GetPosition();
        const auto& forward = vehicle->GetMatrix().GetForward();
        const auto distance = CCollision::DistAlongLine2D(position.x, position.y, forward.x, forward.y, centre.x, centre.y);
        if (std::abs(centre.z - (position.z + distance * vehicle->GetForward().z)) < 3.0f) {
            SlowCarDownForObject(object, vehicle, maxSpeed, originalMaxSpeed);
        }
    }
}

// 0x42D0E0
void CCarCtrl::SlowCarDownForOtherCar(CEntity* entity, CVehicle* vehicle, float* maxSpeed, float originalMaxSpeed) {
    const auto makeForward = [](CEntity* e) {
        const auto& forward = e->GetMatrix().GetForward();
        CVector result{forward.x, forward.y, 0.0f};
        const auto length = result.Magnitude2D();
        if (length == 0.0f) {
            result.x = 1.0f;
        } else {
            result.x /= length;
            result.y /= length;
        }
        return result;
    };
    auto ourForward = makeForward(vehicle);
    const auto delta = entity->GetPosition() - vehicle->GetPosition();
    if (delta.x * ourForward.x + delta.y * ourForward.y < 0.0f) {
        return;
    }
    auto* other = static_cast<CVehicle*>(entity);
    const auto speedX = other->GetMoveSpeed().x * 60.0f - ourForward.x * originalMaxSpeed;
    const auto speedY = other->GetMoveSpeed().y * 60.0f - ourForward.y * originalMaxSpeed;
    auto hisForward = makeForward(entity);
    auto time = std::min(
        TestCollisionBetween2MovingRects_OnlyFrontBumper(other, vehicle, speedX, speedY, &ourForward, &hisForward),
        TestCollisionBetween2MovingRects(vehicle, other, -speedX, -speedY, &hisForward, &ourForward)
    );
    auto& autoPilot = vehicle->m_autoPilot;
    if (time >= 0.0f && time < 1.5f) {
        autoPilot.carCtrlFlags.bHonkAtCar = true;
        autoPilot.m_ObstructingEntity = entity;
        entity->RegisterReference(&autoPilot.m_ObstructingEntity);
        const auto inverseSpeed = 1.0f / originalMaxSpeed;
        if (time < inverseSpeed) {
            *maxSpeed = 0.0f;
        } else if (time < 3.0f * inverseSpeed) {
            *maxSpeed = std::min(*maxSpeed, 1.0f);
        } else {
            // Windows reuses this variable in the head-on check below.
            time = std::max((time - 0.2f) * 0.769230783f, 0.0f);
            *maxSpeed = std::min(*maxSpeed, time * originalMaxSpeed);
        }
    }
    const auto now = CTimer::GetTimeInMS();
    if (time >= 0.0f && time < 0.5f && entity->GetIsTypeVehicle()
        && now - autoPilot.m_nTimeSwitchedToRealPhysics > 15000
        && now - other->m_autoPilot.m_nTimeSwitchedToRealPhysics > 15000) {
        const auto& ourHeading = vehicle->GetMatrix().GetForward();
        const auto& hisHeading = entity->GetMatrix().GetForward();
        if (entity != FindPlayerVehicle() && ourHeading.x * hisHeading.x + ourHeading.y * hisHeading.y < -0.5f
            && reinterpret_cast<uintptr_t>(vehicle) < reinterpret_cast<uintptr_t>(entity)) {
            *maxSpeed = std::max(*maxSpeed, originalMaxSpeed * 0.2f);
            if (vehicle->GetStatus() == STATUS_SIMPLE) {
                SwitchVehicleToRealPhysics(vehicle);
            }
            autoPilot.m_nCarDrivingStyle = DRIVING_STYLE_AVOID_CARS;
            autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + 1000;
        }
    }
}

// 0x425440
template<typename PtrListType>
void CCarCtrl::SlowCarDownForPedsSectorList(PtrListType& ptrList, CVehicle* vehicle, float minX, float minY, float maxX, float maxY, float* maxSpeed, float originalMaxSpeed) {
    const auto& bounds = vehicle->GetColModel()->GetBoundingBox();
    auto width = bounds.m_vecMax.x;
    const auto length = bounds.m_vecMax.y;
    const auto& forward = vehicle->GetMatrix().GetForward();
    const auto speed = DotProduct(forward, vehicle->m_vecMoveSpeed);
    const auto style = vehicle->m_autoPilot.m_nCarDrivingStyle;
    const bool react = vehicle == FindPlayerVehicle(-1, false)
        || vehicle->IsTrain() || vehicle->IsPlane() || vehicle->IsHeli()
        || (style != DRIVING_STYLE_STOP_FOR_CARS && style != DRIVING_STYLE_DRIVINGMODE_AVOIDCARS_STOPFORPEDS_OBEYLIGHTS)
        || vehicle->GetStatus() == STATUS_PHYSICS;
    for (auto* item : ptrList) {
        auto* const entity = static_cast<CEntity*>(item);
        if (entity->IsScanCodeCurrent() || !entity->m_bUsesCollision) {
            continue;
        }
        entity->SetCurrentScanCode();
        const auto& pos = entity->GetPosition();
        const auto& vehiclePos = vehicle->GetPosition();
        if (!(pos.x > minX && pos.x < maxX && pos.y > minY && pos.y < maxY)
            || !(std::abs(pos.z - vehiclePos.z) < 6.0f)) {
            continue;
        }
        const auto distance = CCollision::DistAlongLine2D(vehiclePos.x, vehiclePos.y, forward.x, forward.y, pos.x, pos.y);
        if (!(std::abs(pos.z - (vehiclePos.z + distance * forward.z)) < 3.0f)) {
            continue;
        }
        const auto delta = pos - vehiclePos;
        const auto along = DotProduct(delta, forward);
        const auto lateral = std::abs(DotProduct(delta, vehicle->GetRight()));
        if ((style == DRIVING_STYLE_STOP_FOR_CARS || style == DRIVING_STYLE_SLOW_DOWN_FOR_CARS
                || style == DRIVING_STYLE_STOP_FOR_CARS_IGNORE_LIGHTS
                || style == DRIVING_STYLE_DRIVINGMODE_AVOIDCARS_STOPFORPEDS_OBEYLIGHTS)
            && (entity != FindPlayerPed(-1) || FindPlayerVehicle(-1, false) != vehicle)
            && along > length) {
            const auto gap = along - length;
            if (gap < speed * 200.0f) {
                if (vehicle->IsBike()) {
                    width *= 1.6f;
                }
                if (lateral <= width + 0.5f && gap < 13.0f) {
                    *maxSpeed = std::min(*maxSpeed, std::min(1.0f, std::max(gap - 1.0f, 0.0f) * 0.0769230798f * originalMaxSpeed));
                    vehicle->m_autoPilot.carCtrlFlags.bHonkAtPed = true;
                    if (gap < 4.0f) {
                        vehicle->m_autoPilot.SetTempAction(TEMPACT_WAIT, 4000);
                    }
                    if (gap < 2.5f) {
                        vehicle->m_autoPilot.SetTempAction(TEMPACT_BRAKE, 4000);
                    }
                }
            }
        }
        if (!entity->GetIsTypePed()) {
            continue;
        }
        auto* const ped = entity->AsPed();
        const auto addEvent = [&] {
            CEventPotentialGetRunOver event{vehicle};
            ped->GetEventGroup().Add(&event, false);
        };
        if (vehicle == FindPlayerVehicle(-1, false) && vehicle->m_HornCounter && delta.SquaredMagnitude() < 49.0f) {
            addEvent();
        }
        if (react && speed != 0.0f && (along < 0.0f) == (speed < 0.0f)
            && std::abs(along) > length && std::abs(speed) > 0.05f
            && std::abs(along) - length < std::abs(speed) * 50.0f && lateral <= width + 0.35f) {
            addEvent();
            if (vehicle->m_pDriver && vehicle->m_pDriver->IsPlayer()) {
                ped->GetIntelligence()->IncrementAngerAtPlayer(2);
            }
        }
    }
}

// 0x434790
void CCarCtrl::SlowCarOnRailsDownForTrafficAndLights(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;

    if ((((int8)CTimer::GetFrameCounter() + (int8)(vehicle->m_nRandomSeed)) & 3) == 0) {
        if (CTrafficLights::ShouldCarStopForLight(vehicle, false) || CTrafficLights::ShouldCarStopForBridge(vehicle)) {
            CCarAI::CarHasReasonToStop(vehicle);
            autoPilot.m_fMaxTrafficSpeed = 0.0f;
        } else {
            autoPilot.m_fMaxTrafficSpeed = FindMaximumSpeedForThisCarInTraffic(vehicle);
        }
    }

    if (autoPilot.m_fMaxTrafficSpeed >= autoPilot.m_speed) {
        autoPilot.ModifySpeed(std::min(autoPilot.m_fMaxTrafficSpeed, CTimer::GetTimeStep() * 0.05f + autoPilot.m_speed));
    } else if (autoPilot.m_speed >= 0.1f) {
        autoPilot.ModifySpeed(std::max(autoPilot.m_fMaxTrafficSpeed, autoPilot.m_speed - CTimer::GetTimeStep() * 0.7f));
    } else if (autoPilot.m_speed != 0.0f) {
        autoPilot.ModifySpeed(0.0f);
    }
}

// 0x428DE0
void CCarCtrl::SteerAIBoatWithPhysicsAttackingPlayer(CVehicle* vehicle, float* steerAngle, float* gas, float* brake, bool* handBrake) {
    const auto playerPos = FindPlayerCoors();
    const auto& position = vehicle->GetPosition();
    const auto distance = (playerPos - position).Magnitude();
    const auto prediction = std::min(distance * 0.05f, 2.0f);
    const auto target = playerPos + FindPlayerSpeed() * prediction * 60.0f;
    const auto& forward = vehicle->GetMatrix().GetForward();
    auto forwardX = forward.x;
    auto forwardY = forward.y;
    const auto length = std::sqrt(forwardX * forwardX + forwardY * forwardY);
    if (length == 0.0f) {
        forwardX = 1.0f;
    } else {
        forwardX /= length;
        forwardY /= length;
    }
    auto angle = CGeneral::GetATanOfXY(target.x - position.x, target.y - position.y)
        - CGeneral::GetATanOfXY(forwardX, forwardY);
    while (angle < -PI) {
        angle += TWO_PI;
    }
    while (angle > PI) {
        angle -= TWO_PI;
    }
    const auto desiredSpeed = vehicle->m_autoPilot.m_speed;
    const auto difference = desiredSpeed - vehicle->GetMoveSpeed().Magnitude2D() * 60.0f;
    if (difference <= 0.0f) {
        *gas = difference < -5.0f ? -0.2f : -0.1f;
    } else {
        const auto fraction = difference / desiredSpeed;
        *gas = fraction > 0.25f ? 1.0f : 1.0f - (0.25f - fraction) * 4.0f;
    }
    *brake = 0.0f;
    *steerAngle = angle;
    *handBrake = false;
    if (vehicle->GetModelIndex() == MODEL_PREDATOR && distance < 40.0f && angle < 0.15f) {
        vehicle->FireFixedMachineGuns();
    }
}

// 0x429090
void CCarCtrl::SteerAIBoatWithPhysicsCirclingPlayer(CVehicle* vehicle, float* steerAngle, float* gas, float* brake, bool* handBrake) {
    const auto playerPos = FindPlayerCoors();
    const auto& position = vehicle->GetPosition();
    CVector direction{playerPos.x - position.x, playerPos.y - position.y, 0.0f};
    direction.Normalise();
    const auto radius = (vehicle->m_nRandomSeed & 1) ? -12.0f : 26.0f;
    const CVector target{playerPos.x + direction.y * radius, playerPos.y - direction.x * radius, playerPos.z};
    const auto& forward = vehicle->GetMatrix().GetForward();
    auto forwardX = forward.x;
    auto forwardY = forward.y;
    const auto length = std::sqrt(forwardX * forwardX + forwardY * forwardY);
    if (length == 0.0f) {
        forwardX = 1.0f;
    } else {
        forwardX /= length;
        forwardY /= length;
    }
    auto angle = CGeneral::GetATanOfXY(target.x - position.x, target.y - position.y)
        - CGeneral::GetATanOfXY(forwardX, forwardY);
    while (angle < -PI) {
        angle += TWO_PI;
    }
    while (angle > PI) {
        angle -= TWO_PI;
    }
    const auto desiredSpeed = vehicle->m_autoPilot.m_speed;
    const auto difference = desiredSpeed - vehicle->GetMoveSpeed().Magnitude2D() * 60.0f;
    if (difference <= 0.0f) {
        *gas = difference < -5.0f ? -0.2f : -0.1f;
    } else {
        const auto fraction = difference / desiredSpeed;
        *gas = fraction > 0.25f ? 1.0f : 1.0f - (0.25f - fraction) * 4.0f;
    }
    *brake = 0.0f;
    *steerAngle = angle;
    *handBrake = false;
}

// 0x428BE0
void CCarCtrl::SteerAIBoatWithPhysicsHeadingForTarget(CVehicle* vehicle, float targetX, float targetY, float* steerAngle, float* gas, float* brake) {
    const auto& forward = vehicle->GetMatrix().GetForward();
    auto forwardX = forward.x;
    auto forwardY = forward.y;
    const auto length = std::sqrt(forwardX * forwardX + forwardY * forwardY);
    if (length == 0.0f) {
        forwardX = 1.0f;
    } else {
        forwardX /= length;
        forwardY /= length;
    }
    const auto& position = vehicle->GetPosition();
    auto angle = CGeneral::GetATanOfXY(targetX - position.x, targetY - position.y)
        - CGeneral::GetATanOfXY(forwardX, forwardY);
    while (angle < -PI) {
        angle += TWO_PI;
    }
    while (angle > PI) {
        angle -= TWO_PI;
    }
    angle = std::clamp(angle, -0.5f, 0.5f);

    const auto desiredSpeed = vehicle->m_autoPilot.m_speed;
    const auto speedDifference = desiredSpeed - vehicle->GetMoveSpeed().Magnitude2D() * 60.0f;
    if (speedDifference <= 0.0f) {
        *gas = speedDifference < -5.0f ? -0.2f : -0.1f;
        angle = -angle;
    } else {
        const auto fraction = speedDifference / desiredSpeed;
        *gas = fraction > 0.25f ? 1.0f : 1.0f - (0.25f - fraction) * 4.0f;
    }
    *brake = 0.0f;
    *steerAngle = angle;
}

// 0x422B20
void CCarCtrl::SteerAICarBlockingPlayerForwardAndBack(CVehicle* vehicle, float* steerAngle, float* gas, float* brake, bool* handBrake) {
    *steerAngle = 0.0f;
    *handBrake = false;
    const auto playerSpeed = FindPlayerSpeed();
    const auto& playerForward = FindPlayerEntity()->GetMatrix().GetForward();
    const CVector predictionSpeed{playerSpeed.x + playerForward.x * 0.1f, playerSpeed.y + playerForward.y * 0.1f, 0.0f};
    const auto& matrix = vehicle->GetMatrix();
    CVector right{matrix.GetRight().x, matrix.GetRight().y, 0.0f};
    right.Normalise();
    CVector forward{matrix.GetForward().x, matrix.GetForward().y, 0.0f};
    forward.Normalise();
    const auto delta = FindPlayerCoors() - vehicle->GetPosition();
    auto lateralSpeed = DotProduct(right, predictionSpeed);
    if (lateralSpeed == 0.0f) {
        lateralSpeed = 0.01f;
    }
    const auto time = -(DotProduct(right, delta) / lateralSpeed);
    if (time < 0.0f) {
        *gas = 0.0f;
        *brake = 0.0f;
        return;
    }
    const auto speed = DotProduct(forward, vehicle->GetMoveSpeed());
    const auto distance = DotProduct(forward, delta)
        + DotProduct(forward, predictionSpeed) * time - speed * time;
    if (distance > 0.0f) {
        *gas = std::min(distance * 0.1f, 1.0f);
        *brake = 0.0f;
    } else if (speed <= 0.0f) {
        *gas = std::max(distance * 0.1f, -1.0f);
        *brake = 0.0f;
    } else {
        *gas = 0.0f;
        *brake = std::min(distance * -0.1f, 1.0f);
        if (*brake > 0.95f) {
            *handBrake = true;
        }
    }
}

// 0x433BA0
void CCarCtrl::SteerAICarParkParallel(CVehicle* vehicle, float* steerAngle, float* gasPedal, float* brakePedal, bool* handBrake) {
    auto& autoPilot = vehicle->m_autoPilot;
    const auto current = autoPilot.m_currentAddress;
    const auto next = autoPilot.m_startingRouteNode;
    if (!ThePaths.m_pPathNodes[next.m_wAreaId] || !ThePaths.m_pPathNodes[current.m_wAreaId]) {
        autoPilot.m_nCarMission = MISSION_STOP_FOREVER;
        *steerAngle = 0.0f;
        *gasPedal = 0.0f;
        *brakePedal = 0.0f;
        return;
    }
    const auto nextPosition = ThePaths.GetPathNode(next)->GetPosition();
    auto target = nextPosition;
    if (autoPilot.m_nCarMission == MISSION_PARK_PARALLEL) {
        auto direction = nextPosition - ThePaths.GetPathNode(current)->GetPosition();
        direction.Normalise();
        target += direction;
    }
    SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, target.x, target.y, steerAngle, gasPedal, brakePedal, handBrake);
    autoPilot.m_nCruiseSpeed = std::min<uint8>(autoPilot.m_nCruiseSpeed, 8);
    const auto distance = (target - vehicle->GetPosition()).Magnitude2D();
    if (autoPilot.m_nCarMission == MISSION_PARK_PARALLEL) {
        if (distance < 4.0f) {
            autoPilot.m_nCarMission = MISSION_PARK_PARALLEL_2;
        }
    } else if (distance < 2.0f) {
        autoPilot.m_nCarMission = MISSION_STOP_FOREVER;
        vehicle->vehicleFlags.bLightsOn = false;
        vehicle->vehicleFlags.bEngineOn = false;
        const auto leaveCar = [](CPed* ped) {
            if (ped) {
                ped->GetIntelligence()->GetTaskManager().SetTask(new CTaskComplexLeaveAnyCar{0, true, false}, TASK_PRIMARY_PRIMARY);
            }
        };
        leaveCar(vehicle->m_pDriver);
        for (auto* passenger : vehicle->m_apPassengers) {
            leaveCar(passenger);
        }
    }
}

// 0x433EA0
void CCarCtrl::SteerAICarParkPerpendicular(CVehicle* vehicle, float* arg2, float* arg3, float* arg4, bool* arg5) {
    auto& autoPilot = vehicle->m_autoPilot;
    if (!autoPilot.m_nCurrentPathNodeInfo.IsValid()
        || !ThePaths.IsAreaNodesAvailable(autoPilot.m_nCurrentPathNodeInfo)
        || !autoPilot.m_nNextPathNodeInfo.IsValid()
        || !ThePaths.IsAreaNodesAvailable(autoPilot.m_nNextPathNodeInfo)) {
        autoPilot.m_nCarMission = MISSION_NONE;
        *arg2 = 0.0f;
        *arg3 = 0.0f;
        *arg4 = 0.0f;
        *arg5 = false;
        return;
    }

    const auto target = ThePaths.GetCarPathLink(autoPilot.m_nNextPathNodeInfo).GetNodeCoors();
    SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, target.x, target.y, arg2, arg3, arg4, arg5);
}

// 0x4336D0
void CCarCtrl::SteerAICarTowardsPointInEscort(CVehicle* vehicle, CVehicle* targetCar, float offsetX, float offsetY, float* steerAngle, float* gasPedal, float* brakePedal, bool* handBrake) {
    const auto target = targetCar->GetMatrix() * CVector{offsetX, offsetY, 0.0f} + targetCar->GetMoveSpeed();
    *handBrake = false;
    const auto& forward = vehicle->GetMatrix().GetForward();
    auto forwardX = forward.x;
    auto forwardY = forward.y;
    const auto length = std::sqrt(forwardX * forwardX + forwardY * forwardY);
    if (length == 0.0f) {
        forwardX = 1.0f;
    } else {
        forwardX /= length;
        forwardY /= length;
    }
    const auto ahead = target + targetCar->GetMatrix().GetForward() * 3.0f;
    const auto& position = vehicle->GetPosition();
    const auto direction = CGeneral::GetATanOfXY(ahead.x - position.x, ahead.y - position.y);
    const auto orientation = CGeneral::GetATanOfXY(forwardX, forwardY);
    auto angle = FindAngleToWeaveThroughTraffic(vehicle, nullptr, direction, orientation, 1.0f) - orientation;
    while (angle < -PI) {
        angle += TWO_PI;
    }
    while (angle > PI) {
        angle -= TWO_PI;
    }
    const auto maxSteer = FindMaxSteerAngle(vehicle);
    angle = std::clamp(angle, -maxSteer, maxSteer);
    CGeneral::GetATanOfXY(target.x - position.x, target.y - position.y); // Discarded in Windows.
    const auto dx = target.x - position.x;
    const auto dy = target.y - position.y;
    const auto forwardDistance = dx * forwardX + dy * forwardY;
    const auto distance = std::sqrt(dx * dx + dy * dy);
    auto desiredSpeed = 8.0f;
    if (forwardDistance <= 0.5f) {
        if (distance < 15.0f) {
            *steerAngle = 0.0f;
            *gasPedal = 0.0f;
            *brakePedal = forwardDistance < -3.0f ? 1.0f : 0.1f;
            return;
        }
    } else {
        const auto targetSpeed = targetCar->GetMoveSpeed().Magnitude() * 60.0f;
        desiredSpeed = distance >= 15.0f ? 300.0f
            : std::max(targetSpeed + std::min(forwardDistance - 0.5f - 0.1f, 4.0f), distance * 3.5f);
        desiredSpeed = std::min(desiredSpeed, targetSpeed + 10.0f);
    }
    const auto speed = vehicle->GetMoveSpeed().Magnitude() * 60.0f;
    const auto difference = desiredSpeed - speed;
    *brakePedal = 0.0f;
    if (difference <= 0.0f) {
        *gasPedal = 0.0f;
        *brakePedal = std::min(difference * -0.05f, 0.5f);
    } else {
        *gasPedal = speed >= 25.0f ? 1.0f : std::min(difference * 0.1f, 1.0f);
    }
    *steerAngle = angle;
}

// 0x437C20
void CCarCtrl::SteerAICarWithPhysics(CVehicle* vehicle) {
    SwitchBetweenPhysicsAndGhost(vehicle);
    if (vehicle->m_autoPilot.m_nCarCtrlFlags & 0x80) {
        JoinCarWithRoadAccordingToMission(vehicle);
        return;
    }

    float steerAngle{};
    float gasPedal{};
    float brakePedal{};
    bool handBrake{};
    SteerAICarWithPhysics_OnlyMission(vehicle, &steerAngle, &gasPedal, &brakePedal, &handBrake);
    vehicle->m_autoPilot.m_speed = gasPedal;
}

// 0x434900
void CCarCtrl::SteerAICarWithPhysicsFollowPath(CVehicle* vehicle, float* arg2, float* arg3, float* arg4, bool* arg5) {
    auto& autoPilot = vehicle->m_autoPilot;
    if (StopCarIfNodesAreInvalid(vehicle)) {
        *arg2 = 0.0f;
        *arg3 = 0.0f;
        *arg4 = 1.0f;
        *arg5 = false;
        return;
    }

    const auto target = ThePaths.GetCarPathLink(autoPilot.m_nNextPathNodeInfo).GetNodeCoors();
    SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, target.x, target.y, arg2, arg3, arg4, arg5);
}

// 0x435830
void CCarCtrl::SteerAICarWithPhysicsFollowPath_Racing(CVehicle* vehicle, float* arg2, float* arg3, float* arg4, bool* arg5) {
    auto& autoPilot = vehicle->m_autoPilot;
    if (StopCarIfNodesAreInvalid(vehicle)) {
        *arg2 = 0.0f;
        *arg3 = 0.0f;
        *arg4 = 1.0f;
        *arg5 = false;
        return;
    }

    const auto target = ThePaths.GetCarPathLink(autoPilot.m_nNextPathNodeInfo).GetNodeCoors();
    SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, target.x, target.y, arg2, arg3, arg4, arg5);
}

// 0x432DD0
void CCarCtrl::SteerAICarWithPhysicsFollowPreRecordedPath(CVehicle* vehicle, float* steerAngle, float* gasPedal, float* brakePedal, bool* handBrake) {
    const auto stop = [&] {
        *steerAngle = 0.0f;
        *gasPedal = 0.0f;
        *brakePedal = 0.0f;
        *handBrake = false;
        vehicle->m_autoPilot.m_nCarMission = MISSION_STOP_FOREVER;
    };
    const auto recording = vehicle->m_autoPilot.m_vehicleRecordingId;
    if (recording < 0 || !CVehicleRecording::pPlaybackBuffer[recording]) {
        stop();
        return;
    }
    if (CVehicleRecording::bPlaybackPaused[recording]) {
        *steerAngle = 0.0f;
        *gasPedal = 0.0f;
        *brakePedal = 0.5f;
        *handBrake = false;
        return;
    }
    auto& index = CVehicleRecording::PlaybackIndex[recording];
    const auto frames = CVehicleRecording::GetFramesFromPlaybackBuffer(recording);
    // Windows retains the original target frame while advancing the playback index.
    const auto& targetFrame = frames[index / sizeof(CVehicleStateEachFrame)];
    auto frameIndex = index / sizeof(CVehicleStateEachFrame);
    for (;;) {
        const auto distance = (vehicle->GetPosition() - frames[frameIndex].m_vecPosn).Magnitude();
        const auto nextDistance = (vehicle->GetPosition() - frames[frameIndex + 1].m_vecPosn).Magnitude();
        if (distance >= 10.0f && nextDistance >= distance) {
            break;
        }
        index += sizeof(CVehicleStateEachFrame);
        if (static_cast<size_t>(index) >= CVehicleRecording::PlaybackBufferSize[recording] - sizeof(CVehicleStateEachFrame)) {
            CVehicleRecording::StopPlaybackWithIndex(recording);
            vehicle->m_autoPilot.m_vehicleRecordingId = -1;
            stop();
            return;
        }
        frameIndex = index / sizeof(CVehicleStateEachFrame);
    }
    const auto& forward = vehicle->GetMatrix().GetForward();
    const auto orientation = CGeneral::GetATanOfXY(forward.x, forward.y);
    const auto delta = targetFrame.m_vecPosn - vehicle->GetPosition();
    auto angle = FindAngleToWeaveThroughTraffic(vehicle, nullptr, CGeneral::GetATanOfXY(delta.x, delta.y), orientation, 2.0f) - orientation;
    while (angle < -PI) {
        angle += TWO_PI;
    }
    while (angle > PI) {
        angle -= TWO_PI;
    }
    const auto maxSteer = FindMaxSteerAngle(vehicle);
    *steerAngle = std::clamp(angle, -maxSteer, maxSteer);
    const CVector recordedVelocity = targetFrame.m_sVelocity;
    auto desiredSpeed = recordedVelocity.Magnitude2D() * CVehicleRecording::PlaybackSpeed[recording] * 60.0f;
    if (index <= 800 && desiredSpeed <= 5.0f) {
        desiredSpeed = 5.0f;
    }
    const auto speed = vehicle->GetMoveSpeed().Magnitude2D() * 60.0f;
    const auto difference = desiredSpeed - speed;
    *brakePedal = 0.0f;
    if (difference <= 0.0f) {
        *gasPedal = 0.0f;
        *brakePedal = std::min(difference * -0.05f, 0.5f);
    } else {
        *gasPedal = std::min(difference * (speed < 2.0f ? 0.25f : 0.125f), 1.0f);
    }
    *handBrake = false;
}

// 0x433280
void CCarCtrl::SteerAICarWithPhysicsHeadingForTarget(CVehicle* vehicle, CPhysical* exception, float targetX, float targetY, float* steerAngle, float* gasPedal, float* brakePedal, bool* handBrake) {
    *handBrake = false;
    const auto& forward = vehicle->GetMatrix().GetForward();
    auto forwardX = forward.x;
    auto forwardY = forward.y;
    const auto length = std::sqrt(forwardX * forwardX + forwardY * forwardY);
    if (length == 0.0f) {
        forwardX = 1.0f;
    } else {
        forwardX /= length;
        forwardY /= length;
    }
    const auto& position = vehicle->GetPosition();
    const auto targetDirection = CGeneral::GetATanOfXY(targetX - position.x, targetY - position.y);
    const auto orientation = CGeneral::GetATanOfXY(forwardX, forwardY);
    auto direction = targetDirection;
    switch (vehicle->m_autoPilot.m_nCarDrivingStyle) {
    case DRIVING_STYLE_AVOID_CARS:
    case DRIVING_STYLE_DRIVINGMODE_AVOIDCARS_OBEYLIGHTS:
    case DRIVING_STYLE_DRIVINGMODE_AVOIDCARS_STOPFORPEDS_OBEYLIGHTS:
        direction = FindAngleToWeaveThroughTraffic(vehicle, exception, direction, orientation, 1.0f);
        break;
    }
    auto angle = direction - orientation;
    while (angle < -PI) {
        angle += TWO_PI;
    }
    while (angle > PI) {
        angle -= TWO_PI;
    }
    const auto speed = vehicle->GetMoveSpeed().Magnitude();
    if (speed > 0.3f && std::abs(angle) > 0.7f) {
        *handBrake = true;
    }
    const auto maxSteer = FindMaxSteerAngle(vehicle);
    angle = std::clamp(angle, -maxSteer, maxSteer);
    const auto speedMultiplier = FindSpeedMultiplier(targetDirection - orientation, 0.4f, 1.2f, 0.4f);
    const auto currentSpeed = speed * 60.0f;
    const auto difference = speedMultiplier * vehicle->m_autoPilot.m_speed - currentSpeed;
    *brakePedal = 0.0f;
    if (difference <= 0.0f) {
        *gasPedal = 0.0f;
        *brakePedal = std::min(difference * -0.05f, 0.5f);
    } else {
        *gasPedal = currentSpeed >= 25.0f ? 1.0f : std::min(difference * 0.1f, 1.0f);
        if (vehicle->IsSubBMX() && difference > 3.0f && vehicle->AsBmx()->m_fControlJump <= 0.0f) {
            vehicle->AsBmx()->m_fControlJump = 10.0f;
        }
    }
    *steerAngle = angle;
}

// 0x4335E0
void CCarCtrl::SteerAICarWithPhysicsTryingToBlockTarget(CVehicle* vehicle, CEntity* unused, float targetX, float targetY, float targetVelX, float targetVelY, float* steerAngle, float* gasPedal, float* brakePedal, bool* handBrake) {
    const auto speed = std::sqrt(targetVelX * targetVelX + targetVelY * targetVelY);
    if (speed > 0.13f) {
        const auto scale = 0.13f / speed;
        targetVelX *= scale;
        targetVelY *= scale;
    }

    targetX += targetVelX * 60.0f;
    targetY += targetVelY * 60.0f;
    auto& autoPilot = vehicle->m_autoPilot;
    autoPilot.m_nCarDrivingStyle = DRIVING_STYLE_AVOID_CARS;
    SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, targetX, targetY, steerAngle, gasPedal, brakePedal, handBrake);

    const auto& position = vehicle->GetPosition();
    const auto dx = targetX - position.x;
    const auto dy = targetY - position.y;
    if (dx * dx + dy * dy < 25.0f) {
        autoPilot.m_nCarMission = autoPilot.m_nCarMission == MISSION_BLOCKCAR_CLOSE
            ? MISSION_BLOCKCAR_HANDBRAKESTOP
            : MISSION_BLOCKPLAYER_HANDBRAKESTOP;
    }
}

// 0x428990
void CCarCtrl::SteerAICarWithPhysicsTryingToBlockTarget_Stop(CVehicle* vehicle, float targetX, float targetY, float targetVelX, float targetVelY, float* steerAngle, float* gasPedal, float* brakePedal, bool* handBrake) {
    *steerAngle = 0.0f;
    *gasPedal = 0.0f;
    *brakePedal = 1.0f;
    *handBrake = true;
    const auto& position = vehicle->GetPosition();
    const auto dx = position.x - targetX;
    const auto dy = position.y - targetY;
    const auto distanceSquared = dx * dx + dy * dy;
    auto& autoPilot = vehicle->m_autoPilot;
    if (distanceSquared > 100.0f) {
        autoPilot.m_nCarMission = autoPilot.m_nCarMission == MISSION_BLOCKCAR_HANDBRAKESTOP
            ? MISSION_BLOCKCAR_CLOSE : MISSION_BLOCKPLAYER_CLOSE;
        return;
    }
    auto shouldLeave = false;
    if (autoPilot.m_nCarMission == MISSION_BLOCKCAR_HANDBRAKESTOP) {
        if (vehicle->m_pDriver) {
            const auto* task = vehicle->m_pDriver->GetTaskManager().GetActiveTask();
            if (task && task->GetTaskType() == TASK_COMPLEX_CAR_DRIVE_MISSION) {
                return;
            }
        }
        const auto& speed = vehicle->GetMoveSpeed();
        shouldLeave = speed.x * speed.x + speed.y * speed.y < 0.0001f
            && targetVelX * targetVelX + targetVelY * targetVelY < 0.0004f;
    } else {
        auto* playerVehicle = FindPlayerVehicle();
        // The original accesses both adjacent 16-bit timers as one signed word.
        auto& stoppedTime = reinterpret_cast<int32&>(vehicle->m_nCopsInCarTimer);
        if (playerVehicle && playerVehicle->GetMoveSpeed().Magnitude() < 0.05f) {
            stoppedTime = static_cast<int32>(stoppedTime + CTimer::GetTimeStep() * 16.666666f);
        } else {
            stoppedTime = 0;
        }
        shouldLeave = (!playerVehicle || playerVehicle->IsUpsideDown()
            || (playerVehicle->GetMoveSpeed().Magnitude() < 0.05f && stoppedTime > 2500))
            && distanceSquared < 100.0f;
    }
    if (shouldLeave && vehicle->vehicleFlags.bIsLawEnforcer) {
        CCarAI::TellOccupantsToLeaveCar(vehicle);
        autoPilot.m_speed = 0.0f;
        autoPilot.m_nCarMission = MISSION_NONE;
    }
}

// 0x436A90
void CCarCtrl::SteerAICarWithPhysics_OnlyMission(CVehicle* vehicle, float* steer, float* gas, float* brake, bool* handbrake) {
    auto& ap = vehicle->m_autoPilot;
    const auto headingFor = [&](CPhysical* target, const CVector& position) {
        SteerAICarWithPhysicsHeadingForTarget(vehicle, target, position.x, position.y, steer, gas, brake, handbrake);
    };
    switch (ap.m_nCarMission) {
    case MISSION_NONE:
    {
        *steer = 0.0f; *gas = 0.0f; *brake = 0.5f; *handbrake = true; return;
    }
    case MISSION_CRUISE:
    case MISSION_RAMPLAYER_FARAWAY:
    case MISSION_BLOCKPLAYER_FARAWAY:
    case MISSION_GOTOCOORDINATES:
    case MISSION_GOTOCOORDINATES_ACCURATE:
    case MISSION_RAMCAR_FARAWAY:
    case MISSION_BLOCKCAR_FARAWAY:
    case MISSION_APPROACHPLAYER_FARAWAY:
    case MISSION_FOLLOWCAR_FARAWAY:
    case MISSION_KILLPED_FARAWAY:
    case MISSION_DO_DRIVEBY_FARAWAY:
    case MISSION_ESCORT_LEFT_FARAWAY:
    case MISSION_ESCORT_RIGHT_FARAWAY:
    case MISSION_ESCORT_REAR_FARAWAY:
    case MISSION_ESCORT_FRONT_FARAWAY:
    {
        return SteerAICarWithPhysicsFollowPath(vehicle, steer, gas, brake, handbrake);
    }
    case MISSION_RAMPLAYER_CLOSE:
    {
        auto targetPos = FindPlayerCoors(-1);
        auto* const playerCar = FindPlayerVehicle(-1, false);
        if (playerCar) {
            const auto dot = DotProduct(playerCar->GetForward(), vehicle->GetForward());
            if (!(vehicle->m_nRandomSeed & 1) || dot <= 0.5f) {
                const auto offset = (static_cast<int>(vehicle->m_nRandomSeed) - 128) * 0.00625f;
                targetPos.x += playerCar->GetRight().x * offset;
                targetPos.y += playerCar->GetRight().y * offset;
            } else {
                auto width = vehicle->GetColModel()->GetBoundingBox().m_vecMax.x
                    + playerCar->GetColModel()->GetBoundingBox().m_vecMax.x - 0.2f;
                if (!(vehicle->m_nRandomSeed & 2)) { width = -width; }
                targetPos.x += playerCar->GetRight().x * width;
                targetPos.y += playerCar->GetRight().y * width;
                if ((vehicle->GetPosition() - targetPos).Magnitude2D()
                        < (playerCar->m_vecMoveSpeed - vehicle->m_vecMoveSpeed).Magnitude() * 12.0f + 2.0f
                    && ap.m_nTempAction == TEMPACT_NONE) {
                    ap.SetTempAction(vehicle->m_nRandomSeed & 2 ? TEMPACT_TURNLEFT : TEMPACT_TURNRIGHT, 250);
                }
            }
            if (dot < 0.0f) {
                const auto& speed = FindPlayerSpeed(-1);
                targetPos.x += speed.x * dot * -0.02f;
                targetPos.y += speed.y * dot * -0.02f;
            }
            if (vehicle->vehicleFlags.bIsLawEnforcer && playerCar->m_vecMoveSpeed.Magnitude2D() > 0.4f) {
                if (auto* passenger = playerCar->PickRandomPassenger()) {
                    passenger->Say(CTX_GLOBAL_CAR_POLICE_PURSUIT);
                }
            }
        }
        return headingFor(playerCar, targetPos);
    }
    case MISSION_BLOCKPLAYER_CLOSE:
    {
        const auto pos = FindPlayerCoors(-1);
        const auto& speed = FindPlayerSpeed(-1);
        return SteerAICarWithPhysicsTryingToBlockTarget(vehicle, FindPlayerEntity(-1), pos.x, pos.y,
            speed.x, speed.y, steer, gas, brake, handbrake);
    }
    case MISSION_BLOCKPLAYER_HANDBRAKESTOP:
    {
        const auto pos = FindPlayerCoors(-1);
        const auto& speed = FindPlayerSpeed(-1);
        return SteerAICarWithPhysicsTryingToBlockTarget_Stop(vehicle, pos.x, pos.y, speed.x, speed.y,
            steer, gas, brake, handbrake);
    }
    case MISSION_GOTOCOORDINATES_STRAIGHTLINE:
    case MISSION_GOTOCOORDINATES_STRAIGHTLINE_ACCURATE:
    {
        return headingFor(nullptr, ap.m_vecDestinationCoors);
    }
    case MISSION_EMERGENCYVEHICLE_STOP:
    case MISSION_STOP_FOREVER:
    {
        *steer = 0.0f; *gas = 0.0f; *brake = 0.5f; *handbrake = true; return;
    }
    case MISSION_GOTOCOORDINATES_ASTHECROWSWIMS:
    {
        SteerAIBoatWithPhysicsHeadingForTarget(vehicle, ap.m_vecDestinationCoors.x, ap.m_vecDestinationCoors.y,
            steer, gas, brake);
        *handbrake = false;
        return;
    }
    case MISSION_RAMCAR_CLOSE:
    case MISSION_KILLPED_CLOSE:
    {
        return headingFor(ap.m_TargetEntity, ap.m_TargetEntity->GetPosition());
    }
    case MISSION_BLOCKCAR_CLOSE:
    {
        const auto pos = ap.m_TargetEntity->GetPosition();
        const auto& speed = ap.m_TargetEntity->m_vecMoveSpeed;
        return SteerAICarWithPhysicsTryingToBlockTarget(vehicle, ap.m_TargetEntity, pos.x, pos.y,
            speed.x, speed.y, steer, gas, brake, handbrake);
    }
    case MISSION_BLOCKCAR_HANDBRAKESTOP:
    {
        const auto pos = ap.m_TargetEntity->GetPosition();
        const auto& speed = ap.m_TargetEntity->m_vecMoveSpeed;
        return SteerAICarWithPhysicsTryingToBlockTarget_Stop(vehicle, pos.x, pos.y, speed.x, speed.y,
            steer, gas, brake, handbrake);
    }
    case MISSION_HELI_FLYTOCOORS:
    {
        return SteerAIHeliTowardsTargetCoors(vehicle->AsAutomobile());
    }
    case MISSION_BOAT_ATTACKPLAYER:
    {
        return SteerAIBoatWithPhysicsAttackingPlayer(vehicle, steer, gas, brake, handbrake);
    }
    case MISSION_PLANE_FLYTOCOORS:
    {
        return SteerAIPlaneTowardsTargetCoors(vehicle->AsAutomobile());
    }
    case MISSION_HELI_ATTACK_PLAYER:
    case MISSION_HELI_ATTACK_PLAYER_FLY_AWAY:
    {
        return GetAIHeliToAttackPlayer(vehicle->AsAutomobile());
    }
    case MISSION_SLOWLY_DRIVE_TOWARDS_PLAYER_1:
    {
        return headingFor(nullptr, ap.m_vecDestinationCoors);
    }
    case MISSION_SLOWLY_DRIVE_TOWARDS_PLAYER_2:
    {
        return headingFor(nullptr, FindPlayerCoors(-1));
    }
    case MISSION_BLOCKPLAYER_FORWARDANDBACK:
    {
        return SteerAICarBlockingPlayerForwardAndBack(vehicle, steer, gas, brake, handbrake);
    }
    case MISSION_ESCORT_LEFT:
    {
        auto* const target = ap.m_TargetEntity;
        const auto offset = -(vehicle->GetColModel()->GetBoundingBox().m_vecMax.x
            + target->GetColModel()->GetBoundingBox().m_vecMax.x + 2.0f);
        return SteerAICarTowardsPointInEscort(vehicle, target, offset,
            0.0f, steer, gas, brake, handbrake);
    }
    case MISSION_ESCORT_RIGHT:
    {
        auto* const target = ap.m_TargetEntity;
        const auto offset = (vehicle->GetColModel()->GetBoundingBox().m_vecMax.x
            + target->GetColModel()->GetBoundingBox().m_vecMax.x + 2.0f);
        return SteerAICarTowardsPointInEscort(vehicle, target, offset,
            0.0f, steer, gas, brake, handbrake);
    }
    case MISSION_ESCORT_REAR:
    {
        auto* const target = ap.m_TargetEntity;
        const auto offset = -(vehicle->GetColModel()->GetBoundingBox().m_vecMax.y
            + target->GetColModel()->GetBoundingBox().m_vecMax.y + 7.0f);
        return SteerAICarTowardsPointInEscort(vehicle, target, 0.0f,
            offset, steer, gas, brake, handbrake);
    }
    case MISSION_ESCORT_FRONT:
    {
        auto* const target = ap.m_TargetEntity;
        const auto offset = (vehicle->GetColModel()->GetBoundingBox().m_vecMax.y
            + target->GetColModel()->GetBoundingBox().m_vecMax.y + 7.0f);
        return SteerAICarTowardsPointInEscort(vehicle, target, 0.0f,
            offset, steer, gas, brake, handbrake);
    }
    case MISSION_GOTOCOORDINATES_RACING:
    {
        return SteerAICarWithPhysicsFollowPath_Racing(vehicle, steer, gas, brake, handbrake);
    }
    case MISSION_FOLLOW_RECORDED_PATH:
    {
        return SteerAICarWithPhysicsFollowPreRecordedPath(vehicle, steer, gas, brake, handbrake);
    }
    case MISSION_PLANE_ATTACK_PLAYER:
    case MISSION_PLANE_ATTACK_PLAYER_POLICE:
    {
        return GetAIPlaneToAttackPlayer(vehicle->AsAutomobile());
    }
    case MISSION_PLANE_FLYINDIRECTION:
    {
        return FlyAIPlaneInCertainDirection(vehicle->AsPlane());
    }
    case MISSION_PLANE_FOLLOW_ENTITY:
    {
        return SteerAIPlaneToFollowEntity(vehicle->AsAutomobile());
    }
    case MISSION_HELI_FLYINDIRECTION:
    {
        return GetAIHeliToFlyInDirection(vehicle->AsAutomobile());
    }
    case MISSION_HELI_FOLLOW_ENTITY:
    case MISSION_HELI_NEWS_BEHAVIOUR:
    {
        return SteerAIHeliToFollowEntity(vehicle->AsAutomobile());
    }
    case MISSION_HELI_POLICE_BEHAVIOUR:
    {
        return SteerAIHeliAsPoliceHeli(vehicle->AsAutomobile());
    }
    case MISSION_HELI_FLY_AWAY_FROM_PLAYER:
    {
        return SteerAIHeliFlyingAwayFromPlayer(vehicle->AsAutomobile());
    }
    case MISSION_APPROACHPLAYER_CLOSE:
    {
        auto targetPos = FindPlayerCoors(-1);
        if ((targetPos - vehicle->GetPosition()).Magnitude() < 10.0f) {
            *steer = 0.0f; *gas = 0.0f; *brake = 1.0f; *handbrake = false;
            return;
        }
        auto* const playerCar = FindPlayerVehicle(-1, false);
        if (playerCar) {
            const auto delta = vehicle->GetPosition() - targetPos;
            // Windows uses the X component in both products here.
            if ((playerCar->GetForward().x + playerCar->GetForward().y) * delta.x > 0.0f) {
                const auto right = playerCar->GetMatrix().TransformPoint({4.0f, 0.0f, 0.0f});
                const auto left = playerCar->GetMatrix().TransformPoint({-4.0f, 0.0f, 0.0f});
                targetPos = (vehicle->GetPosition() - left).Magnitude() <= (vehicle->GetPosition() - right).Magnitude() ? left : right;
            }
        }
        return headingFor(playerCar, targetPos);
    }
    case MISSION_PARK_PERPENDICULAR:
    case MISSION_PARK_PERPENDICULAR_2:
    {
        return SteerAICarParkPerpendicular(vehicle, steer, gas, brake, handbrake);
    }
    case MISSION_PARK_PARALLEL:
    case MISSION_PARK_PARALLEL_2:
    {
        return SteerAICarParkParallel(vehicle, steer, gas, brake, handbrake);
    }
    case MISSION_HELI_LAND:
    case MISSION_HELI_LAND_TOUCHING_DOWN:
    {
        return SteerAIHeliToLand(vehicle->AsAutomobile());
    }
    case MISSION_HELI_KEEP_ENTITY_IN_VIEW:
    {
        return SteerAIHeliToKeepEntityInView(vehicle->AsAutomobile());
    }
    case MISSION_FOLLOWCAR_CLOSE:
    {
        auto* const target = ap.m_TargetEntity;
        headingFor(target, target->GetPosition());
        const auto distance = (vehicle->GetPosition() - target->GetPosition()).Magnitude();
        const auto followDistance = static_cast<float>(ap.m_ucCarFollowDist);
        if (distance < followDistance + 10.0f) {
            const auto gap = distance - followDistance;
            const auto difference = target->m_vecMoveSpeed.Magnitude2D() * 60.0f
                + gap * (gap < 0.0f ? 5.0f : 2.0f) - vehicle->m_vecMoveSpeed.Magnitude2D() * 60.0f;
            if (difference >= 0.0f) { *gas = std::min(difference * 0.05f, 1.0f); *brake = 0.0f; }
            else { *gas = 0.0f; *brake = std::min(difference * -0.1f, 1.0f); }
        }
        return;
    }
    case MISSION_PLANE_CRASH_AND_BURN:
    {
        return SteerAIPlaneToCrashAndBurn(vehicle->AsAutomobile());
    }
    case MISSION_HELI_CRASH_AND_BURN:
    {
        return SteerAIHeliToCrashAndBurn(vehicle->AsAutomobile());
    }
    case MISSION_DO_DRIVEBY_CLOSE:
    {
        if (auto* const target = ap.m_TargetEntity) {
            const auto delta = target->GetPosition() - vehicle->GetPosition();
            CVector offset{delta.y, -delta.x, 0.0f};
            offset.Normalise();
            offset *= vehicle->m_nRandomSeed & 1 ? -10.0f : 10.0f;
            headingFor(nullptr, target->GetPosition() + offset);
        }
        return;
    }
    case MISSION_PLANE_DOG_FIGHT_ENTITY:
    {
        return GetAIPlaneToDoDogFight(vehicle->AsAutomobile());
    }
    case MISSION_PLANE_DOG_FIGHT_PLAYER:
    {
        return GetAIPlaneToDoDogFightAgainstPlayer(vehicle->AsAutomobile());
    }
    case MISSION_BOAT_CIRCLEPLAYER:
    {
        return SteerAIBoatWithPhysicsCirclingPlayer(vehicle, steer, gas, brake, handbrake);
    }
    default: return;
    }
}

// 0x42AAD0
void CCarCtrl::SteerAIHeliAsPoliceHeli(CAutomobile* automobile) {
    auto* heli = automobile->AsHeli();
    const auto targetPos = automobile->m_autoPilot.m_TargetEntity->GetPosition();
    const auto heliPos = automobile->GetPosition();
    const auto delta = targetPos - heliPos;
    auto targetAltitude = std::max(targetPos.z, 6.0f);
    if (delta.Magnitude2D() > 50.0f)
        targetAltitude = std::max(targetAltitude, 25.0f);

    automobile->m_autoPilot.m_vecDestinationCoors.x = targetPos.x;
    automobile->m_autoPilot.m_vecDestinationCoors.y = targetPos.y;
    heli->m_fMaxAltitude = targetAltitude;
    FlyAIHeliInCertainDirection(heli, CGeneral::GetATanOfXY(delta.x, delta.y), delta.Magnitude2D(), true);
}

// 0x42ACB0
void CCarCtrl::SteerAIHeliFlyingAwayFromPlayer(CAutomobile* automobile) {
    auto* heli = automobile->AsHeli();
    const auto playerPos = FindPlayerCoors(-1);
    const auto heliPos = automobile->GetPosition();
    const auto heading = CGeneral::GetATanOfXY(playerPos.x - heliPos.x, playerPos.y - heliPos.y);
    FlyAIHeliInCertainDirection(heli, heading + PI, 1000.0f, false);
}

// 0x4238E0
void CCarCtrl::SteerAIHeliToCrashAndBurn(CAutomobile* automobile) {
    auto* heli = automobile->AsHeli();
    heli->m_fLeftRightSkid = -0.3f;
    heli->m_fSteeringUpDown = (automobile->m_nFlags & 1) ? heli->field_A14 : -heli->field_A14;
    heli->m_fSteeringLeftRight = -0.5f;
    heli->m_fAccelerationBreakStatus = (automobile->m_nFlags & 1) ? 1.0f : -1.0f;
}

// 0x42A750
void CCarCtrl::SteerAIHeliToFollowEntity(CAutomobile* automobile) {
    auto& ap = automobile->m_autoPilot;
    auto* const target = static_cast<CEntity*>(ap.m_TargetEntity);
    auto* const heli = automobile->AsHeli();
    const auto targetPos = target->GetPosition();
    ap.m_vecDestinationCoors = targetPos;
    if (ap.field_4A) {
        const auto& forward = target->GetMatrix().GetForward();
        ap.m_vecDestinationCoors.x += forward.x * static_cast<uint8>(ap.field_4A);
        ap.m_vecDestinationCoors.y += forward.y * static_cast<uint8>(ap.field_4A);
    }
    const auto delta = targetPos - automobile->GetPosition();
    const auto distance = delta.Magnitude2D();
    heli->m_fMaxAltitude = std::max(targetPos.z, distance > 50.0f ? 25.0f : 6.0f);
    if (automobile->m_fAircraftGoToHeading >= 0.0f) {
        FlyAIHeliToTarget_FixedOrientation(heli, automobile->m_fAircraftGoToHeading, targetPos);
    } else {
        const auto direction = ap.m_vecDestinationCoors - automobile->GetPosition();
        FlyAIHeliInCertainDirection(heli, CGeneral::GetATanOfXY(direction.x, direction.y), distance, true);
    }
    if (ap.carCtrlFlags.bDoTargetCatchupCheck && distance < 25.0f) {
        if (target->GetIsTypeVehicle()) {
            auto* const vehicle = target->AsVehicle();
            if (vehicle->GetStatus() == STATUS_SIMPLE || vehicle->GetStatus() == STATUS_PHYSICS) {
                vehicle->m_autoPilot.m_nCarMission = MISSION_CRUISE;
                vehicle->m_autoPilot.m_nCruiseSpeed = 100;
                vehicle->m_autoPilot.m_nCarDrivingStyle = DRIVING_STYLE_AVOID_CARS;
                vehicle->SetStatus(STATUS_PHYSICS);
            }
        } else if (target->GetIsTypePed()) {
            auto* const task = target->AsPed()->GetTaskManager().GetActiveTask();
            if (task && task->GetTaskType() == TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL) {
                static_cast<CTaskComplexGoToPointAndStandStill*>(task)->m_moveState = PEDMOVE_SPRINT;
            }
        }
        ap.carCtrlFlags.bDoTargetCatchupCheck = false;
    }
    if (ap.carCtrlFlags.bHeliFollowTarget && CTimer::GetTimeInMS() > automobile->m_nCreationTime + 50000) {
        ap.carCtrlFlags.bHeliFollowTarget = false;
        ap.m_nCarMission = MISSION_HELI_FLY_AWAY_FROM_PLAYER;
    }
    if (ap.m_nCarMission == MISSION_HELI_NEWS_BEHAVIOUR && automobile->m_fHealth < 300.0f) {
        ap.m_nCarMission = MISSION_HELI_FLY_AWAY_FROM_PLAYER;
    }
}

// 0x42AEB0
void CCarCtrl::SteerAIHeliToKeepEntityInView(CAutomobile* automobile) {
    auto* const target = automobile->m_autoPilot.m_TargetEntity;
    auto* const heli = automobile->AsHeli();
    const auto targetPos = target->GetPosition();
    const auto vehiclePos = automobile->GetPosition();
    const auto targetDistance = (targetPos - vehiclePos).Magnitude2D();
    const auto heliTargetDistance = static_cast<float>(automobile->m_autoPilot.m_ucHeliTargetDist);

    if (targetDistance > heliTargetDistance * 2.0f) {
        SteerAIHeliToFollowEntity(automobile);
        return;
    }

    const auto headingToTarget = CGeneral::GetATanOfXY(targetPos.x - vehiclePos.x, targetPos.y - vehiclePos.y);
    const auto relativeHeading = CGeneral::LimitRadianAngle(headingToTarget + HALF_PI - CGeneral::GetATanOfXY(automobile->GetMatrix().GetForward().x, automobile->GetMatrix().GetForward().y));
    heli->m_fLeftRightSkid = std::clamp(-relativeHeading, -1.0f, 1.0f);

    const auto targetAltitude = targetPos.z + 15.0f;
    heli->m_fMaxAltitude = targetAltitude;
    heli->field_9AC = targetAltitude;
    automobile->m_autoPilot.m_vecDestinationCoors = targetPos;

    const auto altitudeDifference = targetAltitude - (automobile->GetPosition().z + automobile->GetMoveSpeed().z * 100.0f);
    heli->m_fAccelerationBreakStatus = (altitudeDifference > 0.0f ? altitudeDifference * 0.1f : altitudeDifference * 0.2f) + 0.3f;
    heli->m_fAccelerationBreakStatus = std::clamp(heli->m_fAccelerationBreakStatus + ((CGeneral::GetRandomNumber() & 0xF) - 7) * 0.002f, 0.0f, 1.0f);

    if (targetDistance < heliTargetDistance * 0.5f) {
        heli->m_fSteeringLeftRight = 0.5f;
    } else if (targetDistance <= heliTargetDistance) {
        heli->m_fSteeringLeftRight = DotProduct(automobile->GetMoveSpeed(), automobile->GetMatrix().GetRight());
    } else {
        heli->m_fSteeringLeftRight = -0.5f;
    }

    heli->m_fSteeringUpDown = targetDistance < heliTargetDistance * 1.5f
        ? DotProduct(automobile->GetMoveSpeed(), automobile->GetMatrix().GetForward())
        : 0.0f;
}

// 0x42AD30
void CCarCtrl::SteerAIHeliToLand(CAutomobile* automobile) {
    auto* heli = automobile->AsHeli();
    const auto targetPos = automobile->m_autoPilot.m_vecDestinationCoors;
    const auto delta = targetPos - automobile->GetPosition();
    const auto distance = delta.Magnitude2D();
    const auto orientation = CGeneral::GetATanOfXY(delta.x, delta.y);
    FlyAIHeliInCertainDirection(heli, orientation, distance, true);

    if (distance < 10.0f && automobile->GetMoveSpeed().Magnitude2D() < 0.05f) {
        heli->m_fMinAltitude = 0.0f;
        heli->m_fMaxAltitude = 0.0f;
        heli->field_99C = 0;
        heli->m_fSteeringUpDown = 0.0f;
        heli->m_fSteeringLeftRight = 0.0f;
        heli->m_fAccelerationBreakStatus = 0.0f;
    }
}

// 0x42A630
void CCarCtrl::SteerAIHeliTowardsTargetCoors(CAutomobile* automobile) {
    auto* heli = automobile->AsHeli();
    const auto targetPos = automobile->m_autoPilot.m_vecDestinationCoors;
    const auto vehiclePos = automobile->GetPosition();
    if (automobile->m_fAircraftGoToHeading >= 0.0f) {
        FlyAIHeliToTarget_FixedOrientation(heli, automobile->m_fAircraftGoToHeading, targetPos);
        return;
    }

    const auto delta = targetPos - vehiclePos;
    FlyAIHeliInCertainDirection(heli, CGeneral::GetATanOfXY(delta.x, delta.y), delta.Magnitude2D(), true);
}

// 0x423880
void CCarCtrl::SteerAIPlaneToCrashAndBurn(CAutomobile* automobile) {
    auto* plane = automobile->AsPlane();
    plane->m_fLeftRightSkid = -0.3f;
    plane->m_fSteeringUpDown = (automobile->m_nFlags & 1) ? 1.0f : -1.0f;
    plane->m_fSteeringLeftRight = plane->m_fSteeringUpDown;
    plane->m_fAccelerationBreakStatus = 0.0f;
}

// 0x4237F0
void CCarCtrl::SteerAIPlaneToFollowEntity(CAutomobile* automobile) {
    auto* plane = automobile->AsPlane();
    const auto targetPos = automobile->m_autoPilot.m_TargetEntity->GetPosition();
    const auto vehiclePos = automobile->GetPosition();
    plane->m_planeHeading = CGeneral::GetATanOfXY(targetPos.x - vehiclePos.x, targetPos.y - vehiclePos.y);
    plane->m_maxAltitude = targetPos.z;
    FlyAIPlaneInCertainDirection(plane);
}

// 0x423790
void CCarCtrl::SteerAIPlaneTowardsTargetCoors(CAutomobile* automobile) {
    auto* plane = automobile->AsPlane();
    const auto targetPos = automobile->m_autoPilot.m_vecDestinationCoors;
    const auto vehiclePos = automobile->GetPosition();
    plane->m_planeHeading = CGeneral::GetATanOfXY(targetPos.x - vehiclePos.x, targetPos.y - vehiclePos.y);
    FlyAIPlaneInCertainDirection(plane);
}

// 0x422590
bool CCarCtrl::StopCarIfNodesAreInvalid(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;
    if (autoPilot.m_nCurrentPathNodeInfo.IsValid()
        && ThePaths.IsAreaNodesAvailable(autoPilot.m_nCurrentPathNodeInfo)
        && autoPilot.m_nNextPathNodeInfo.IsValid()
        && ThePaths.IsAreaNodesAvailable(autoPilot.m_nNextPathNodeInfo)
        && autoPilot.m_currentAddress.IsAreaValid()
        && ThePaths.IsAreaNodesAvailable(autoPilot.m_currentAddress)
        && autoPilot.m_startingRouteNode.IsAreaValid()
        && ThePaths.IsAreaNodesAvailable(autoPilot.m_startingRouteNode)) {
        return false;
    }

    autoPilot.movementFlags.bIsStopped = true;
    return true;
}

// 0x4222A0
void CCarCtrl::SwitchBetweenPhysicsAndGhost(CVehicle* vehicle) {
    if (!vehicle->physicalFlags.bInfiniteMass
        || vehicle->GetCreatedBy() != MISSION_VEHICLE
        || vehicle->m_nVehicleType == VEHICLE_TYPE_HELI
        || vehicle->m_nVehicleType == VEHICLE_TYPE_PLANE
        || vehicle->m_nVehicleType == VEHICLE_TYPE_BOAT) {
        return;
    }

    if (vehicle->GetStatus() == STATUS_SIMPLE) {
        if (!CColStore::HasCollisionLoaded(vehicle->GetPosition(), AREA_CODE_NORMAL_WORLD)) {
            vehicle->SetStatus(STATUS_GHOST);
            if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_AUTOMOBILE) {
                auto* automobile = vehicle->AsAutomobile();
                for (auto wheel = 0; wheel < 4; wheel++) {
                    automobile->m_damageManager.SetWheelStatus((eCarWheel)wheel, WHEEL_STATUS_OK);
                }
            }
        }
    } else if (vehicle->GetStatus() == STATUS_GHOST
               && CColStore::HasCollisionLoaded(vehicle->GetPosition(), AREA_CODE_NORMAL_WORLD)) {
        vehicle->SetStatus(STATUS_PHYSICS);
        if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_AUTOMOBILE) {
            vehicle->AsAutomobile()->PlaceOnRoadProperly();
        } else if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_BIKE) {
            vehicle->AsBike()->PlaceOnRoadProperly();
        }
    }
}

// 0x423FC0
void CCarCtrl::SwitchVehicleToRealPhysics(CVehicle* vehicle) {
    vehicle->SetStatus(STATUS_PHYSICS);
    vehicle->m_autoPilot.m_nTempAction = TEMPACT_NONE;
    vehicle->m_autoPilot.m_nTimeToStartMission = CTimer::GetTimeInMS() + 2000;
    vehicle->m_autoPilot.m_nTimeSwitchedToRealPhysics = CTimer::GetTimeInMS();
}

// 0x425B30
float CCarCtrl::TestCollisionBetween2MovingRects(CVehicle* his, CVehicle* ours, float speedX, float speedY, CVector* ourForward, CVector* hisForward) {
    const auto& ourBox = ours->GetColModel()->GetBoundingBox();
    const auto& hisBox = his->GetColModel()->GetBoundingBox();
    const auto delta = his->GetPosition() - ours->GetPosition();
    const auto lateralSpeed = speedX * ourForward->y - speedY * ourForward->x;
    const auto longitudinalSpeed = speedX * ourForward->x + speedY * ourForward->y;

    // Find the time interval during which a moving point lies within one axis.
    const auto interval = [](float position, float speed, float minimum, float maximum) {
        auto entry = 0.0f;
        auto exit = 1.0f;
        if (position > maximum) {
            entry = 1.0f;
            if (speed < 0.0f) {
                const auto t = -(position - maximum) / speed;
                if (t < 1.0f) {
                    entry = t;
                    exit = std::min(1.0f, t - (maximum - minimum) / speed);
                }
            }
        } else if (position < minimum) {
            entry = 1.0f;
            if (speed > 0.0f) {
                const auto t = -(position - minimum) / speed;
                if (t < 1.0f) {
                    entry = t;
                    exit = std::min(1.0f, t + (maximum - minimum) / speed);
                }
            }
        } else if (speed > 0.0f) {
            exit = (maximum - position) / speed;
        } else if (speed < 0.0f) {
            exit = -(position - minimum) / speed;
        }
        return std::pair{entry, exit};
    };

    auto collisionTime = 1.0f;
    for (auto corner = 0; corner < 2; corner++) {
        const auto lateral = corner ? -hisBox.m_vecMax.x : hisBox.m_vecMax.x;
        const auto x = delta.x + hisBox.m_vecMax.y * hisForward->x + lateral * hisForward->y;
        const auto y = delta.y + hisBox.m_vecMax.y * hisForward->y - lateral * hisForward->x;
        const auto [lateralEntry, lateralExit] = interval(x * ourForward->y - y * ourForward->x, lateralSpeed, -ourBox.m_vecMax.x, ourBox.m_vecMax.x);
        const auto [longitudinalEntry, longitudinalExit] = interval(x * ourForward->x + y * ourForward->y, longitudinalSpeed, ourBox.m_vecMin.y, ourBox.m_vecMax.y);
        const auto entry = std::max(lateralEntry, longitudinalEntry);
        if (entry < lateralExit && entry < longitudinalExit) {
            collisionTime = std::min(collisionTime, entry);
        }
    }
    return collisionTime;
}

// 0x425F70
float CCarCtrl::TestCollisionBetween2MovingRects_OnlyFrontBumper(CVehicle* his, CVehicle* ours, float speedX, float speedY, CVector* ourForward, CVector* hisForward) {
    const auto& ourBox = ours->GetColModel()->GetBoundingBox();
    const auto& hisBox = his->GetColModel()->GetBoundingBox();
    const auto& ourPosition = ours->GetPosition();
    const auto& hisPosition = his->GetPosition();
    const CVector2D left{
        ourPosition.x + ourBox.m_vecMax.y * ourForward->x + ourBox.m_vecMax.x * ourForward->y,
        ourPosition.y + ourBox.m_vecMax.y * ourForward->y - ourBox.m_vecMax.x * ourForward->x
    };
    const CVector2D right{
        ourPosition.x + ourBox.m_vecMax.y * ourForward->x - ourBox.m_vecMax.x * ourForward->y,
        ourPosition.y + ourBox.m_vecMax.y * ourForward->y + ourBox.m_vecMax.x * ourForward->x
    };
    auto collisionTime = 1.0f;
    for (auto corner = 0; corner < 4; corner++) {
        const auto longitudinal = corner < 2 ? hisBox.m_vecMax.y : hisBox.m_vecMin.y;
        const auto lateral = (corner & 1) ? -hisBox.m_vecMax.x : hisBox.m_vecMax.x;
        const CVector2D point{
            hisPosition.x + longitudinal * hisForward->x + lateral * hisForward->y,
            hisPosition.y + longitudinal * hisForward->y - lateral * hisForward->x
        };
        const auto before = (point.x - left.x) * ourForward->x + (point.y - left.y) * ourForward->y;
        const auto after = (point.x + speedX - left.x) * ourForward->x + (point.y + speedY - left.y) * ourForward->y;
        if (before > 0.0f && after < 0.0f) {
            const auto crossLeft = (left.x - point.x) * speedY - (left.y - point.y) * speedX;
            const auto crossRight = (right.x - point.x) * speedY - (right.y - point.y) * speedX;
            if (crossLeft * crossRight < 0.0f) {
                collisionTime = std::min(collisionTime, before / (before - after));
            }
        }
    }
    return collisionTime;
}

// 0x429520
void CCarCtrl::TestWhetherToFirePlaneGuns(CVehicle* vehicle, CEntity* target) {
    vehicle->vehicleFlags.bFireGun = false;
    if ((vehicle->m_nVehicleWeaponInUse != CAR_WEAPON_NOT_USED && vehicle->m_nVehicleWeaponInUse != CAR_WEAPON_HEAVY_GUN) || !target)
        return;

    auto targetOffset = target->GetPosition() - vehicle->GetPosition();
    if (targetOffset.Magnitude() >= 150.0f)
        return;

    targetOffset.Normalise();
    if (DotProduct(vehicle->GetMatrix().GetForward(), targetOffset) >= 0.8f)
        vehicle->vehicleFlags.bFireGun = true;
}

// 0x421FE0
bool CCarCtrl::ThisVehicleShouldTryNotToTurn(CVehicle* vehicle) {
    switch (vehicle->m_nModelIndex) {
    case 0x193:
    case 0x196:
    case 0x1AF:
    case 0x1B5:
    case 0x1BB:
    case 0x1C7:
    case 0x202:
    case 0x203:
    case 0x20C:
        return true;
    default:
        return false;
    }
}

// 0x429300
void CCarCtrl::TriggerDogFightMoves(CVehicle* vehicle1, CVehicle* vehicle2) {
    auto& autoPilot = vehicle1->m_autoPilot;
    if (autoPilot.m_nCarMission != MISSION_NONE)
        return;

    auto positionDelta = vehicle1->GetPosition() - vehicle2->GetPosition();
    if (positionDelta.Magnitude() >= 70.0f)
        return;

    positionDelta.Normalise();
    const auto targetDirection = DotProduct(vehicle2->GetMatrix().GetForward(), positionDelta);
    if (std::abs(vehicle1->GetPosition().z - vehicle2->GetPosition().z) >= 15.0f)
        return;

    switch ((CGeneral::GetRandomNumber() & 0xFF) - 12) {
    case 0:
        if (targetDirection >= 0.0f) {
            autoPilot.m_nCarMission = MISSION_RAMCAR_CLOSE;
            autoPilot.m_nTimeToStartMission = CTimer::GetTimeInMS() + (CGeneral::GetRandomNumber() & 0x3FF) + 1500;
        }
        break;
    case 1:
        if (targetDirection >= 0.0f) {
            autoPilot.m_nCarMission = MISSION_BLOCKCAR_FARAWAY;
            autoPilot.m_nTimeToStartMission = CTimer::GetTimeInMS() + (CGeneral::GetRandomNumber() & 0x1FF) + 700;
        }
        break;
    case 2:
        if (targetDirection >= 0.0f) {
            autoPilot.m_nCarMission = MISSION_BLOCKCAR_CLOSE;
            autoPilot.m_nTimeToStartMission = CTimer::GetTimeInMS() + (CGeneral::GetRandomNumber() & 0x1FF) + 700;
        }
        break;
    case 3:
        if (targetDirection >= 0.7f) {
            autoPilot.m_nCarMission = MISSION_RAMCAR_FARAWAY;
            autoPilot.m_nTimeToStartMission = CTimer::GetTimeInMS() + (CGeneral::GetRandomNumber() & 0x7FF) + 3000;
        }
        break;
    }
}

// 0x424000
void CCarCtrl::UpdateCarCount(CVehicle* vehicle, uint8 bDecrease) {
    const auto change = [bDecrease](auto& count) {
        if (bDecrease) {
            if (count > 0) {
                count--;
            }
        } else {
            count++;
        }
    };

    switch (vehicle->GetCreatedBy()) {
    case RANDOM_VEHICLE:
        if (vehicle->vehicleFlags.bIsLawEnforcer) {
            change(NumLawEnforcerCars);
        }
        change(NumRandomCars);
        break;
    case MISSION_VEHICLE:
        if (bDecrease && vehicle->vehicleFlags.bIsLawEnforcer) {
            vehicle->vehicleFlags.bIsLawEnforcer = false;
            change(NumLawEnforcerCars);
        }
        change(NumMissionCars);
        break;
    case PARKED_VEHICLE:
        change(NumParkedCars);
        break;
    case PERMANENT_VEHICLE:
        change(NumPermanentVehicles);
        break;
    }
}

// 0x436540
void CCarCtrl::UpdateCarOnRails(CVehicle* vehicle) {
    StopCarIfNodesAreInvalid(vehicle);
    auto& autoPilot = vehicle->m_autoPilot;
    if (autoPilot.movementFlags.bIsStopped) {
        return;
    }
    const auto action = autoPilot.m_nTempAction;
    if (action == TEMPACT_WAIT || action == TEMPACT_BRAKE || action == TEMPACT_STUCKINTRAFFIC) {
        vehicle->GetMoveSpeed() = CVector{};
        autoPilot.ModifySpeed(0.0f);
        if (action != TEMPACT_STUCKINTRAFFIC && CTimer::GetTimeInMS() > autoPilot.m_nTempActionTime) {
            autoPilot.m_nTempAction = TEMPACT_NONE;
            autoPilot.m_nTimeToStartMission = CTimer::GetTimeInMS();
            autoPilot.m_nTimeSwitchedToRealPhysics = CTimer::GetTimeInMS();
        }
        return;
    }
    SlowCarOnRailsDownForTrafficAndLights(vehicle);
    if (static_cast<uint64>(CTimer::GetTimeInMS()) >= static_cast<uint64>(autoPilot.m_timeToLeaveLink) + autoPilot.m_timeToGetToNextLink) {
        PickNextNodeAccordingStrategy(vehicle);
    }
    if (vehicle->GetStatus() == STATUS_PHYSICS) {
        return;
    }
    const auto currentAddress = autoPilot.m_nCurrentPathNodeInfo;
    const auto nextAddress = autoPilot.m_nNextPathNodeInfo;
    const auto& current = ThePaths.GetCarPathLink(currentAddress);
    const auto& next = ThePaths.GetCarPathLink(nextAddress);
    const auto currentX = static_cast<float>(current.m_dir.x) * autoPilot._smthCurr;
    const auto currentY = static_cast<float>(current.m_dir.y) * autoPilot._smthCurr;
    const auto nextX = static_cast<float>(next.m_dir.x) * autoPilot._smthNext;
    const auto nextY = static_cast<float>(next.m_dir.y) * autoPilot._smthNext;
    auto currentOffset = (current.OneWayLaneOffset() + autoPilot.m_nCurrentLane) * 5.4f;
    auto nextOffset = (next.OneWayLaneOffset() + autoPilot.m_nNextLane) * 5.4f;
    if (vehicle->IsSubBMX()) {
        currentOffset += 1.458f;
        nextOffset += 1.458f;
    }
    const auto makeDirection = [&](uint16 link, float x, float y) {
        const auto seed = static_cast<uint32>(vehicle->m_nRandomSeed) + link;
        CVector direction{x + (static_cast<int32>(seed & 7) - 3) * 0.009f, y + (static_cast<int32>((seed >> 3) & 7) - 3) * 0.009f, 0.0f};
        direction.Normalise();
        return direction;
    };
    const auto currentDirection = makeDirection(currentAddress.m_wAreaId << 10 | currentAddress.m_wCarPathLinkId, currentX, currentY);
    const auto nextDirection = makeDirection(nextAddress.m_wAreaId << 10 | nextAddress.m_wCarPathLinkId, nextX, nextY);
    const auto currentCoors = current.GetNodeCoors();
    const auto nextCoors = next.GetNodeCoors();
    const CVector start{currentCoors.x + currentOffset * currentY, currentCoors.y - currentOffset * currentX, 0.0f};
    const CVector end{nextCoors.x + nextOffset * nextY, nextCoors.y - nextOffset * nextX, 0.0f};
    const auto elapsed = CTimer::GetTimeInMS() - static_cast<uint32>(autoPilot.m_timeToLeaveLink);
    const auto fraction = static_cast<float>(elapsed) / autoPilot.m_timeToGetToNextLink;
    CVector coors, speed;
    CCurves::CalcCurvePoint(start, end, currentDirection, nextDirection, fraction, autoPilot.m_timeToGetToNextLink, coors, speed);
    coors.z = 15.0f;
    DragCarToPoint(vehicle, &coors);
    if (currentAddress.m_wAreaId != nextAddress.m_wAreaId || currentAddress.m_wCarPathLinkId != nextAddress.m_wCarPathLinkId) {
        vehicle->GetMoveSpeed() = speed * 0.0166666675f;
    }
}

// 0x426BC0
void CCarCtrl::WeaveForObject(CEntity* entity, CVehicle* vehicle, float* arg3, float* arg4) {
    float rightCoef;
    float forwardCoef;
    const auto modelIndex = entity->GetModelIndex();
    if (modelIndex == MI_TRAFFICLIGHTS) {
        rightCoef = 2.957f;
        forwardCoef = 0.147f;
    } else if (modelIndex == MI_SINGLESTREETLIGHTS1) {
        rightCoef = 0.744f;
        forwardCoef = 0.0f;
    } else if (modelIndex == MI_SINGLESTREETLIGHTS2) {
        rightCoef = 0.043f;
        forwardCoef = 0.0f;
    } else if (modelIndex == MI_SINGLESTREETLIGHTS3) {
        rightCoef = 1.143f;
        forwardCoef = 0.145f;
    } else if (modelIndex == MI_DOUBLESTREETLIGHTS) {
        rightCoef = 0.0f;
        forwardCoef = -0.048f;
    } else if (CModelInfo::GetModelInfo(modelIndex)->SwaysInWind()) { // 0x426C50
        rightCoef = 0.0f;
        forwardCoef = 0.0f;
    } else {
        return;
    }

    const CVector2D offset{
        entity->GetPosition().x + rightCoef * entity->GetRight().x + forwardCoef * entity->GetForward().x - vehicle->GetPosition().x,
        entity->GetPosition().y + rightCoef * entity->GetRight().y + forwardCoef * entity->GetForward().y - vehicle->GetPosition().y
    };
    const auto angleBetweenVehicleAndObject = CGeneral::GetATanOfXY(offset.x, offset.y);
    const auto distance = offset.Magnitude();
    const auto angleToWeave = (1.2f * 2.0f * vehicle->GetColModel()->GetBoundingBox().m_vecMax.x + 0.3f) / distance * 0.5f;

    auto diffToLeftAngle = CGeneral::LimitRadianAngle(angleBetweenVehicleAndObject - *arg3);
    diffToLeftAngle = std::abs(diffToLeftAngle);
    if (diffToLeftAngle < angleToWeave) {
        *arg3 = angleBetweenVehicleAndObject - angleToWeave;
        while (*arg3 < -PI) {
            *arg3 += TWO_PI;
        }
    }

    auto diffToRightAngle = CGeneral::LimitRadianAngle(angleBetweenVehicleAndObject - *arg4);
    diffToRightAngle = std::abs(diffToRightAngle);
    if (diffToRightAngle < angleToWeave) {
        *arg4 = angleBetweenVehicleAndObject + angleToWeave;
        while (*arg4 > PI) {
            *arg4 -= TWO_PI;
        }
    }
}

// 0x426350
void CCarCtrl::WeaveForOtherCar(CEntity* entity, CVehicle* vehicle, float* leftAngle, float* rightAngle) {
    const auto& ap = vehicle->m_autoPilot;
    if ((ap.m_nCarMission == MISSION_RAMPLAYER_CLOSE && entity == FindPlayerVehicle(-1, false))
        || (ap.m_nCarMission == MISSION_RAMCAR_CLOSE && entity == ap.m_TargetEntity)
        || (ap.m_nCarMission == MISSION_FOLLOWCAR_CLOSE && (entity == ap.m_TargetEntity
            || (entity->GetIsTypeVehicle() && entity->AsVehicle()->vehicleFlags.bIsRCVehicle)))
        || (ap.m_nCarMission == MISSION_KILLPED_CLOSE && entity->GetIsTypePed()
            && entity->AsPed()->bInVehicle && entity->AsPed()->m_pVehicle == ap.m_TargetEntity)) {
        return;
    }
    auto* const other = entity->AsVehicle();
    switch (other->m_autoPilot.m_nCarMission) {
    case MISSION_PROTECTION_REAR:
    case MISSION_PROTECTION_FRONT:
    case MISSION_ESCORT_LEFT:
    case MISSION_ESCORT_RIGHT:
    case MISSION_ESCORT_REAR:
    case MISSION_ESCORT_FRONT:
        if (other->m_autoPilot.m_TargetEntity == vehicle) {
            return;
        }
        break;
    }
    const auto delta = entity->GetPosition() - vehicle->GetPosition();
    const auto along = delta.x * vehicle->GetForward().x + delta.y * vehicle->GetForward().y;
    if (along < 0.0f) {
        return;
    }
    const auto across = delta.x * vehicle->GetRight().x + delta.y * vehicle->GetRight().y;
    const auto otherAlong = delta.x * entity->GetForward().x + delta.y * entity->GetForward().y;
    const auto otherAcross = delta.x * entity->GetRight().x + delta.y * entity->GetRight().y;
    const auto makeEdges = [](CEntity* car, float y, float x) {
        const auto& box = car->GetColModel()->GetBoundingBox();
        const auto& matrix = car->GetMatrix();
        return std::array<CVector, 4>{
            matrix.TransformPoint({box.m_vecMin.x - 0.2f, y, 0.0f}),
            matrix.TransformVector({box.m_vecMax.x + 0.2f - box.m_vecMin.x - 0.2f, 0.0f, 0.0f}),
            matrix.TransformPoint({x, box.m_vecMin.y - 0.2f, 0.0f}),
            matrix.TransformVector({0.0f, box.m_vecMax.y + 0.2f - box.m_vecMin.y - 0.2f, 0.0f})
        };
    };
    const auto& box = vehicle->GetColModel()->GetBoundingBox();
    const auto& otherBox = entity->GetColModel()->GetBoundingBox();
    const auto ourEdges = makeEdges(vehicle, along <= 0.0f ? box.m_vecMin.y - 0.2f : box.m_vecMax.y + 0.2f,
        across <= 0.0f ? box.m_vecMin.x - 0.2f : box.m_vecMax.x + 0.2f);
    const auto theirEdges = makeEdges(entity, otherAlong >= 0.0f ? otherBox.m_vecMin.y - 0.2f : otherBox.m_vecMax.y + 0.2f,
        otherAcross >= 0.0f ? otherBox.m_vecMin.x - 0.2f : otherBox.m_vecMax.x + 0.2f);
    const auto speed = vehicle->m_vecMoveSpeed.Magnitude2D();
    const auto testAngle = [&](float angle) {
        auto stationary = ourEdges;
        auto moving = theirEdges;
        CVector movement{(other->m_vecMoveSpeed.x - std::cos(angle) * speed) * 100.0f,
            (other->m_vecMoveSpeed.y - std::sin(angle) * speed) * 100.0f, 0.0f};
        if (box.m_vecMax.x < otherBox.m_vecMax.x) {
            std::swap(stationary, moving);
            movement = -movement;
        }
        const auto intersects = [](const CVector& start, const CVector& direction, const CVector& edgeStart, const CVector& edgeDirection) {
            return CCollision::Test2DLineAgainst2DLine(start.x, start.y, direction.x, direction.y,
                edgeStart.x, edgeStart.y, edgeDirection.x, edgeDirection.y);
        };
        for (auto edge = 0; edge < 4; edge += 2) {
            const auto a = moving[edge];
            const auto b = a + moving[edge + 1];
            const auto c = a + movement;
            const auto d = b + movement;
            for (auto ours = 0; ours < 4; ours += 2) {
                if (intersects(stationary[ours], stationary[ours + 1], a, b - a)
                    || intersects(stationary[ours], stationary[ours + 1], b, c - b)
                    || intersects(stationary[ours], stationary[ours + 1], c, d - c)
                    || intersects(stationary[ours], stationary[ours + 1], d, a - d)) {
                    return true;
                }
            }
        }
        return false;
    };
    for (auto i = 0; i < 8 && testAngle(*leftAngle); i++) {
        *leftAngle -= 0.104719758f;
        if (*leftAngle < 0.0f) {
            *leftAngle += TWO_PI;
        }
    }
    for (auto i = 0; i < 8 && testAngle(*rightAngle); i++) {
        *rightAngle += 0.104719758f;
        if (*rightAngle > TWO_PI) {
            *rightAngle -= TWO_PI;
        }
    }
}

// 0x426970
void CCarCtrl::WeaveForPed(CEntity* entity, CVehicle* vehicle, float* leftAngle, float* rightAngle) {
    const auto& autoPilot = vehicle->m_autoPilot;
    if ((autoPilot.m_nCarMission == MISSION_RAMPLAYER_CLOSE && entity == FindPlayerPed())
        || (autoPilot.m_nCarMission == MISSION_KILLPED_CLOSE && entity == autoPilot.m_TargetEntity)) {
        return;
    }
    const auto delta = entity->GetPosition() - vehicle->GetPosition();
    const auto angle = CGeneral::GetATanOfXY(delta.x, delta.y);
    const auto distance = delta.Magnitude2D();
    if (distance < 1.0f) {
        return;
    }
    const auto margin = (vehicle->GetColModel()->GetBoundingBox().m_vecMax.x * 2.4f + 0.8f) / distance * 0.5f;
    if (std::abs(CGeneral::LimitRadianAngle(angle - *leftAngle)) < margin) {
        *leftAngle = angle - margin;
        while (*leftAngle < -PI) {
            *leftAngle += TWO_PI;
        }
    }
    if (std::abs(CGeneral::LimitRadianAngle(angle - *rightAngle)) < margin) {
        *rightAngle = angle + margin;
        while (*rightAngle > PI) {
            *rightAngle -= TWO_PI;
        }
    }
}

// 0x42D680
template<typename PtrListType>
void CCarCtrl::WeaveThroughCarsSectorList(PtrListType& ptrList, CVehicle* vehicle, CPhysical* exception, float minX, float minY, float maxX, float maxY, float* leftAngle, float* rightAngle) {
    for (auto* item : ptrList) {
        auto* other = static_cast<CVehicle*>(item);
        if (other->IsScanCodeCurrent() || !other->m_bUsesCollision || other == exception) {
            continue;
        }
        other->SetCurrentScanCode();
        const auto centre = other->GetBoundCentre();
        if (centre.x > minX && centre.x < maxX && centre.y > minY && centre.y < maxY
            && std::abs(other->GetPosition().z - vehicle->GetPosition().z) < 8.0f
            && other != vehicle
            && (!vehicle->vehicleFlags.bIsRCVehicle || !other->vehicleFlags.bIsRCVehicle)) {
            WeaveForOtherCar(other, vehicle, leftAngle, rightAngle);
        }
    }
}

// 0x42D950
template<typename PtrListType>
void CCarCtrl::WeaveThroughObjectsSectorList(PtrListType& ptrList, CVehicle* vehicle, float minX, float minY, float maxX, float maxY, float* leftAngle, float* rightAngle) {
    for (auto* item : ptrList) {
        auto* object = static_cast<CEntity*>(item);
        if (object->IsScanCodeCurrent() || !object->m_bUsesCollision) {
            continue;
        }
        object->SetCurrentScanCode();
        const auto position = object->GetPosition();
        if (position.x > minX && position.x < maxX && position.y > minY && position.y < maxY
            && std::abs(position.z - vehicle->GetPosition().z) < 8.0f
            && object->GetMatrix().GetUp().z > 0.9f) {
            WeaveForObject(object, vehicle, leftAngle, rightAngle);
        }
    }
}

// 0x42D7E0
template<typename PtrListType>
void CCarCtrl::WeaveThroughPedsSectorList(PtrListType& ptrList, CVehicle* vehicle, CPhysical* exception, float minX, float minY, float maxX, float maxY, float* leftAngle, float* rightAngle) {
    for (auto* item : ptrList) {
        auto* ped = static_cast<CPed*>(item);
        if (ped->IsScanCodeCurrent() || !ped->m_bUsesCollision || ped == exception) {
            continue;
        }
        ped->SetCurrentScanCode();
        const auto position = ped->GetPosition();
        if (position.x > minX && position.x < maxX && position.y > minY && position.y < maxY
            && std::abs(position.z - vehicle->GetPosition().z) < 4.0f
            && ped->m_pVehicle != vehicle && ped->m_pContactEntity != vehicle) {
            WeaveForPed(ped, vehicle, leftAngle, rightAngle);
        }
    }
}

// 0x427FE0
float CCarCtrl::FindMaxSteerAngle(CVehicle* veh) {
    return std::clamp(0.9f - veh->GetMoveSpeed().Magnitude(), 0.2f, 0.7f);
}
