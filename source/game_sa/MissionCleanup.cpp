/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "MissionCleanup.h"
#include "TheScripts.h"
#include "Garages.h"
#include "CullZones.h"
#include "VehicleRecording.h"
#include "PostEffects.h"
#include "EntryExitManager.h"
#include "Hud.h"
#include "UserDisplay.h"
#include "PedType.h"
#include "PedGroups.h"
#include "TaskSequences.h"
#include "TaskComplexUseMobilePhone.h"
#include "Scripted2dEffects.h"
#include "DecisionMakers/DecisionMakerTypesFileLoader.h"
#include "EventGunShot.h"
#include "CarCtrl.h"
#include "Train.h"
#include "Plane.h"
#include "UpsideDownCarCheck.h"
#include "FxSystem.h"
#include "IplStore.h"
#include "ColStore.h"

void CMissionCleanup::InjectHooks() {
    RH_ScopedClass(CMissionCleanup);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Process, 0x468560);
    RH_ScopedInstall(FindFree, 0x4637C0);
    RH_ScopedInstall(CheckIfCollisionHasLoadedForMissionObjects, 0x4652D0);
}

CMissionCleanup::CMissionCleanup() {
    Init();
}

/* Initializes data
 * @addr 0x4637A0
 */
void CMissionCleanup::Init() {
    m_Count = 0;
    m_Objects.fill(tMissionCleanupEntity());
}

/* Performs a clean-up
 * @addr 0x468560
 */
void CMissionCleanup::Process() {
    auto&       player0   = CWorld::Players[0];
    auto* const pad0      = CPad::GetPad(0);

    if (CTheScripts::bScriptHasFadedOut) {
        CTheScripts::bScriptHasFadedOut = false;
        if (player0.m_nPlayerState != PLAYERSTATE_HAS_BEEN_ARRESTED && player0.m_nPlayerState != PLAYERSTATE_HAS_DIED) {
            TheCamera.Fade(0.5f, eFadeFlag::FADE_OUT); // (Value `1`)
            pad0->bPlayerSafe = false;
        }
    }

    CTrain::DisableRandomTrains(false);
    CTrain::ReleaseMissionTrains();
    CPlane::SwitchAmbientPlanes(true);
    CPopulation::PedDensityMultiplier = 1.0f;
    CCarCtrl::CarDensityMultiplier    = 1.0f;
    if (CGangWars::bTrainingMission) {
        CGangWars::bTrainingMission = false;
        CTheZones::FillZonesWithGangColours(!CGangWars::bGangWarsActive);
    }
    CPopulation::m_AllRandomPedsThisType            = -1;
    CGangWars::bCanTriggerGangWarWhenOnAMission     = false;
    CGangWars::ClearSpecificZonesToTriggerGangWar();
    CPopulation::m_bDontCreateRandomGangMembers     = false;
    CPopulation::m_bOnlyCreateRandomGangMembers     = false;
    CPopulation::m_bDontCreateRandomCops            = false;
    CGarages::NoResprays                            = false;
    CGarages::AllRespraysCloseOrOpen(true);
    CCullZones::bMilitaryZonesDisabled              = false;
    FindPlayerWanted()->m_Multiplier                = 1.0f;
    CPickups::RemoveMissionPickUps();
    CRoadBlocks::ClearScriptRoadBlocks();
    CStreaming::DisableCopBikes(false);
    g_LoadMonitor.EnableAmbientCrime();
    CObject::bArea51SamSiteDisabled          = false;
    CObject::bAircraftCarrierSamSiteDisabled = true;
    ThePaths.ReleaseRequestedNodes();
    ThePaths.UnMarkAllRoadNodesAsDontWander();
    ThePaths.TidyUpNodeSwitchesAfterMission();
    CVehicleRecording::RemoveAllRecordingsThatArentUsed();

    TheCamera.SetWideScreenOff();
    TheCamera.m_nModeForTwoPlayersSeparateCars              = MODE_TWOPLAYER_SEPARATE_CARS;
    TheCamera.m_nModeForTwoPlayersSameCarShootingAllowed    = MODE_TWOPLAYER_IN_CAR_AND_SHOOTING;
    TheCamera.m_nModeForTwoPlayersSameCarShootingNotAllowed = MODE_BEHINDCAR;
    TheCamera.m_nModeForTwoPlayersNotBothInCar              = MODE_TWOPLAYER;
    TheCamera.m_bDisableFirstPersonInCar                    = false;
    TheCamera.m_bCinemaCamera                               = false;
    TheCamera.InitialiseScriptableComponents();
    TheCamera.ResetDuckingSystem(nullptr);
    StaticRef<float, 0x8CCB84>()             = 1.0f; // gCurDistForCam
    gAllowScriptedFixedCameraCollision     = false;
    CGameLogic::bScriptCoopGameGoingOn     = false;
    CTheScripts::bDrawCrossHair            = eCrossHairType::NONE;
    CSpecialFX::bVideoCam                  = false;
    CSpecialFX::bLiftCam                   = false;
    CPostEffects::ScriptResetForEffects();
    CEntryExitManager::ms_bDisabled = false;
    if (CGame::currArea == AREA_CODE_NORMAL_WORLD) {
        CTimeCycle::StopExtraColour(false);
    }
    for (uint8 slot = 0; slot < 4; slot++) {
        AudioEngine.ClearMissionAudio(slot);
    }
    CWeather::ReleaseWeather();
    gFireManager.m_nMaxFireGenerationsAllowed = 99'999;
    for (int32 slot = 0; slot < 10; slot++) {
        CStreaming::SetMissionDoesntRequireSpecialChar(slot);
    }
    CTheScripts::ClearAllSuppressedCarModels();
    CTheScripts::ForceRandomCarModel             = -1;
    CStreaming::ms_disableStreaming              = false;
    CHud::m_ItemToFlash                          = ITEM_NONE;
    CHud::bScriptDontDisplayRadar                = false;
    CHud::bScriptDontDisplayVehicleName          = false;
    CHud::bScriptDontDisplayAreaName             = false;
    CHud::bScriptForceDisplayWithCounters        = false;
    FrontEndMenuManager.m_bMenuAccessWidescreen  = false;
    CTheScripts::RadarZoomValue                  = 0;
    CTheScripts::RadarShowBlipOnAllLevels        = false;
    CTheScripts::HideAllFrontEndMapBlips         = false;
    C3dMarkers::ForceRender(false);
    CTheScripts::bDisplayHud                            = true;
    CTheScripts::fCameraHeadingWhenPlayerIsAttached     = 0.0f;
    CTheScripts::fCameraHeadingStepWhenPlayerIsAttached = 0.0f;
    CTheScripts::bEnableCraneRaise                      = true;
    CTheScripts::bEnableCraneLower                      = true;
    CTheScripts::bEnableCraneRelease                    = true;
    CUserDisplay::OnscnTimer.m_bPaused                  = false;
    CTheScripts::bUseMessageFormatting                  = false;
    CTheScripts::MessageCentre                          = 0;
    CTheScripts::MessageWidth                           = 0;
    CTheScripts::bDrawOddJobTitleBeforeFade             = true;
    CTheScripts::bDrawSubtitlesBeforeFade               = true;

    // Reset player group stuff
    {
        auto* const playerPed  = player0.m_pPed;
        auto* const playerData = playerPed->GetPlayerData();
        auto&       group      = CPedGroups::ms_groups[playerData->m_nPlayerGroup];

        group.GetIntelligence().SetDefaultTaskAllocatorType(ePedGroupDefaultTaskAllocatorType::FOLLOW_LIMITED);
        group.GetIntelligence().SetGroupDecisionMakerType(eDecisionMakerType::UNKNOWN);
        group.GetMembership().SetSeparationRange(120.0f);
        playerData->m_bGroupStuffDisabled       = false;
        playerPed->ForceGroupToAlwaysFollow(false);
        playerPed->ForceGroupToNeverFollow(false);
        playerPed->MakePlayerGroupReappear();
        playerData->m_nScriptLimitToGangSize = 99;
        playerPed->m_fireDmgMult             = 1.0f;

        // NOTE: These 2 are accessed through `CWorld::Players[0].m_PlayerData` in the original code
        player0.m_PlayerData.m_nFadeDrunkenness = 1;
        player0.m_PlayerData.m_nDrugLevel       = 0;

        playerPed->EnablePedSpeech();
        playerPed->EnablePedSpeechForScriptSpeech();
    }

    pad0->SetDrunkInputDelay(0);
    pad0->bApplyBrakes                    = false;
    pad0->bDisablePlayerEnterCar          = false;
    pad0->bDisablePlayerDuck              = false;
    pad0->bDisablePlayerFireWeapon        = false;
    pad0->bDisablePlayerFireWeaponWithL1  = false;
    pad0->bDisablePlayerCycleWeapon       = false;
    pad0->bDisablePlayerJump              = false;
    pad0->bDisablePlayerDisplayVitalStats = false;
    player0.m_bCanDoDriveBy               = true;

    // Stop the player from using the phone (Inlined `CTaskComplexUseMobilePhone::Stop` - 0x634A40)
    if (auto* const task = player0.m_pPed->GetTaskManager().FindTaskByType(TASK_PRIMARY_PRIMARY, TASK_COMPLEX_USE_MOBILE_PHONE)) {
        if (task->GetTaskType() == TASK_COMPLEX_USE_MOBILE_PHONE) {
            auto* const phone = static_cast<CTaskComplexUseMobilePhone*>(task);
            if (!phone->m_bQuit) {
                phone->m_bQuit = true;
                phone->MakeAbortable(player0.m_pPed, ABORT_PRIORITY_LEISURE, nullptr);
            }
        }
    }

    CVehicle::bDisableRemoteDetonation          = false;
    CVehicle::bDisableRemoteDetonationOnContact = false;
    CGameLogic::ClearSkip(true);
    if (CGameLogic::GameState != GAMELOGIC_STATE_BUSTED && CGameLogic::GameState != GAMELOGIC_STATE_WASTED) {
        if (FindPlayerPed()->m_nPedState != PEDSTATE_DEAD && FindPlayerPed()->m_nPedState != PEDSTATE_DIE) {
            CRestart::ClearRespawnPointForDurationOfMission();
        }
    }
    CTheScripts::RiotIntensity = 0;
    gFireManager.ClearAllScriptFireFlags();
    CTheScripts::StoreVehicleIndex     = -1;
    CTheScripts::StoreVehicleWasRandom = true;
    for (auto& car : CTheScripts::UpsideDownCars.m_aUpsideDownCars) { // Inlined `CUpsideDownCarCheck::Init`
        car.Clear();
    }
    CTheScripts::StuckCars.Init();
    CStats::bShowUpdateStats                     = true;
    CEventGunShot::ms_fGunShotSenseRangeForRiot2 = -1.0f;
    CHud::m_fHelpMessageBoxWidth                 = 200.0f;
    CVehicle::ms_forceVehicleLightsOff           = false;

    if (!CTheScripts::bMiniGameInProgress) {
        if (!CWorld::Players[CWorld::PlayerInFocus].m_pRemoteVehicle) {
            TheCamera.Restore();
        }
        player0.m_pPed->GetPlayerData()->m_pWanted->m_bPoliceBackOff    = false;
        player0.m_pPed->GetPlayerData()->m_pWanted->m_bEverybodyBackOff = false;
        player0.MakePlayerSafe(false, 10'000.0f);
        CHud::SetHelpMessage(nullptr, true, false, false);
    }

    for (auto& entity : m_Objects) {
        if (entity.type == MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_EMPTY) {
            continue;
        }

        switch (entity.type) {
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_VEHICLE: {
            if (auto* const veh = GetVehiclePool()->GetAtRef(entity.handle)) {
                CTheScripts::CleanUpThisVehicle(veh);
            }
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_PED: {
            if (auto* const ped = GetPedPool()->GetAtRef(entity.handle)) {
                CTheScripts::CleanUpThisPed(ped);
            }
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_OBJECT: {
            if (auto* const obj = GetObjectPool()->GetAtRef(entity.handle)) {
                CTheScripts::CleanUpThisObject(obj);
            }
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_PARTICLE: {
            const auto idx = CTheScripts::GetActualScriptThingIndex(entity.handle, SCRIPT_THING_EFFECT_SYSTEM);
            if (idx >= 0) {
                if (auto* const fx = CTheScripts::ScriptEffectSystemArray[idx].m_pFxSystem) {
                    fx->Kill();
                    CTheScripts::RemoveScriptEffectSystem(entity.handle);
                }
            }
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_GROUP: {
            const auto idx = CTheScripts::GetActualScriptThingIndex(entity.handle, SCRIPT_THING_PED_GROUP);
            if (idx >= 0) {
                CPedGroups::RemoveGroup(idx);
            }
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_PED_QUEUE: {
            const auto idx = CTheScripts::GetActualScriptThingIndex(entity.handle, SCRIPT_THING_2D_EFFECT);
            if (idx >= 0) {
                CScripted2dEffects::ms_activated[idx] = false;
            }
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_TASK_SEQUENCE: {
            const auto idx = CTheScripts::GetActualScriptThingIndex(entity.handle, SCRIPT_THING_SEQUENCE_TASK);
            if (idx >= 0) {
                auto& sequence = CTaskSequences::ms_taskSequence[idx];
                if (sequence.m_RefCnt != 0) { // Still in use, flush it later
                    sequence.m_bFlushTasks = true;
                } else {
                    sequence.m_bFlushTasks = false;
                    sequence.Flush();
                }
                CTaskSequences::ms_bIsOpened[idx]                 = false;
                CTheScripts::ScriptSequenceTaskArray[idx].m_bUsed = false;
            }
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_DECISION_MAKER: {
            const auto idx = CTheScripts::GetActualScriptThingIndex(entity.handle, SCRIPT_THING_DECISION_MAKER);
            if (idx >= 0) {
                CDecisionMakerTypesFileLoader::UnloadDecisionMaker(static_cast<eDecisionMakerType>(idx));
            }
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_SEARCHLIGHT: {
            if (CTheScripts::GetActualScriptThingIndex(entity.handle, SCRIPT_THING_SEARCH_LIGHT) >= 0) {
                CTheScripts::RemoveScriptSearchLight(entity.handle);
            }
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_CHECKPOINT: {
            if (CTheScripts::GetActualScriptThingIndex(entity.handle, SCRIPT_THING_CHECKPOINT) >= 0) {
                CTheScripts::RemoveScriptCheckpoint(entity.handle);
            }
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_TXD: {
            CTheScripts::RemoveScriptTextureDictionary();
            break;
        }
        default:
            break;
        }

        RemoveEntityFromList(entity.handle, static_cast<MissionCleanUpEntityType>(entity.type));
    }

    // Reset the acquaintances of/towards the mission ped types
    for (int32 pedType = PED_TYPE_PLAYER1; pedType < PED_TYPE_MISSION1; pedType++) {
        for (int32 missionPedType = PED_TYPE_MISSION1; missionPedType <= PED_TYPE_MISSION8; missionPedType++) {
            for (int32 acquaintance = 0; acquaintance < ACQUAINTANCE_NUM; acquaintance++) {
                CPedType::ClearPedTypeAsAcquaintance(acquaintance, static_cast<ePedType>(pedType), CPedType::GetPedFlag(static_cast<ePedType>(missionPedType)));
            }
        }
    }
    for (int32 missionPedType = PED_TYPE_MISSION1; missionPedType <= PED_TYPE_MISSION8; missionPedType++) {
        for (int32 acquaintance = 0; acquaintance < ACQUAINTANCE_NUM; acquaintance++) {
            CPedType::ClearPedTypeAcquaintances(acquaintance, static_cast<ePedType>(missionPedType));
        }
    }
}

/* Finds a free entity, returns NULL if no free entity can be found.
 * @addr 0x4637C0
 */
tMissionCleanupEntity* CMissionCleanup::FindFree() {
    for (auto& entity : m_Objects) {
        if (entity.type == MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_EMPTY) {
            return &entity;
        }
    }
    return nullptr;
}

/* Adds entity to list
 * @addr 0x4637E0
 */
void CMissionCleanup::AddEntityToList(int32 handle, MissionCleanUpEntityType type) {
    for (auto& entity : m_Objects) {
        if (entity.type == MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_EMPTY) {
            entity.handle = handle;
            entity.type   = type;
            ++m_Count;
            return;
        }
    }
}

/* Remotes entity from list
 * @addr 0x4654B0
 */
void CMissionCleanup::RemoveEntityFromList(int32 handle, MissionCleanUpEntityType type) {
    for (auto& entity : m_Objects) {
        if (entity.type != type || entity.handle != handle) {
            continue;
        }

        switch (entity.type) {
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_VEHICLE: {
            auto* veh = GetVehiclePool()->GetAtRef(entity.handle);

            if (veh && veh->m_bIsStaticWaitingForCollision) {
                veh->m_bIsStaticWaitingForCollision = false;
                if (!veh->GetIsStatic()) {
                    veh->AddToMovingList();
                }
            }
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_PED: {
            auto* ped = GetPedPool()->GetAtRef(entity.handle);

            if (ped && ped->m_bIsStaticWaitingForCollision) {
                ped->m_bIsStaticWaitingForCollision = false;
                if (!ped->GetIsStatic()) {
                    ped->AddToMovingList();
                }
            }
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_OBJECT: {
            auto* obj = GetObjectPool()->GetAtRef(entity.handle);

            if (obj && obj->m_bIsStaticWaitingForCollision) {
                obj->m_bIsStaticWaitingForCollision = false;
                if (!obj->GetIsStatic()) {
                    obj->AddToMovingList();
                }
            }
            break;
        }
        default:
            break;
        }

        entity = tMissionCleanupEntity();
        --m_Count;
    }
}

/* Checks if collision has loaded for mission objects
 * @addr 0x4652D0
 */
void CMissionCleanup::CheckIfCollisionHasLoadedForMissionObjects() {
    ZoneScoped;

    // Returns true if the entity was waiting for the collision, and it has loaded now
    const auto ProcessEntity = [](CPhysical* entity) {
        if (!entity || !entity->m_bIsStaticWaitingForCollision) {
            return false;
        }
        if (!CColStore::HasCollisionLoaded(entity->GetPosition(), entity->GetAreaCode())) {
            return false;
        }
        // NOTE: The area code is passed in as the `playerNumber` argument (That's what the original code does)
        if (!CIplStore::HaveIplsLoaded(entity->GetPosition(), entity->GetAreaCode())) {
            return false;
        }
        entity->m_bIsStaticWaitingForCollision = false;
        if (!entity->GetIsStatic()) {
            entity->AddToMovingList();
        }
        return true;
    };

    for (auto& entity : m_Objects) {
        switch (entity.type) {
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_VEHICLE: {
            auto* const veh = GetVehiclePool()->GetAtRef(entity.handle);
            if (!ProcessEntity(veh)) {
                break;
            }
            switch (veh->m_nVehicleType) {
            case VEHICLE_TYPE_AUTOMOBILE:
            case VEHICLE_TYPE_TRAILER:
                veh->AsAutomobile()->PlaceOnRoadProperly();
                break;
            case VEHICLE_TYPE_BIKE:
                veh->AsBike()->PlaceOnRoadProperly();
                break;
            default:
                break;
            }
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_PED: {
            ProcessEntity(GetPedPool()->GetAtRef(entity.handle));
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_OBJECT: {
            ProcessEntity(GetObjectPool()->GetAtRef(entity.handle));
            break;
        }
        default:
            break;
        }
    }
}

// NOTSA
void CMissionCleanup::AddEntityToList(CObject& obj) {
    return AddEntityToList(GetObjectPool()->GetRef(&obj), MISSION_CLEANUP_ENTITY_TYPE_OBJECT);
}

// NOTSA
void CMissionCleanup::AddEntityToList(CPed& ped) {
    return AddEntityToList(GetPedPool()->GetRef(&ped), MISSION_CLEANUP_ENTITY_TYPE_PED);
}

// NOTSA
void CMissionCleanup::AddEntityToList(CVehicle& veh) {
    return AddEntityToList(GetVehiclePool()->GetRef(&veh), MISSION_CLEANUP_ENTITY_TYPE_VEHICLE);
}

// NOTSA
void CMissionCleanup::RemoveEntityFromList(CObject& obj) {
    return RemoveEntityFromList(GetObjectPool()->GetRef(&obj), MISSION_CLEANUP_ENTITY_TYPE_OBJECT);
}

void CMissionCleanup::RemoveEntityFromList(CPed& ped) {
    return RemoveEntityFromList(GetPedPool()->GetRef(&ped), MISSION_CLEANUP_ENTITY_TYPE_PED);
}

void CMissionCleanup::RemoveEntityFromList(CVehicle& veh) {
    return RemoveEntityFromList(GetVehiclePool()->GetRef(&veh), MISSION_CLEANUP_ENTITY_TYPE_VEHICLE);
}
