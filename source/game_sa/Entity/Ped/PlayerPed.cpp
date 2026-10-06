/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "PlayerPed.h"
#include "TagManager.h"
#include "PedClothesDesc.h"
#include "PedStats.h"
#include "TaskSimpleUseGun.h"
#include "EntryExitManager.h"
#include "MBlur.h"
#include "Events/EventPlayerCommandToGroupAttack.h"
#include "Tasks/TaskTypes/TaskSimplePlayerOnFoot.h"
#include "Tasks/TaskTypes/TaskComplexFacial.h"
#include "Events/EventPlayerCommandToGroupGather.h"
#include "Tasks/TaskTypes/TaskComplexSmartFleeEntity.h"
#include "Tasks/TaskTypes/TaskComplexBeInGroup.h"
#include "Tasks/TaskTypes/TaskSimpleFight.h"
#include "Events/EventNewGangMember.h"
#include "Events/EventDontJoinPlayerGroup.h"
#include "Radar.h"

bool CPlayerPed::bDebugPlayerInvincible;
bool CPlayerPed::bDebugTargeting;
bool CPlayerPed::bDebugTapToTarget;

void CPlayerPed::InjectHooks() {
    RH_ScopedVirtualClass(CPlayerPed, 0x86D168, 26);
    RH_ScopedCategory("Entity/Ped");

    RH_ScopedInstall(ResetSprintEnergy, 0x60A530);
    RH_ScopedInstall(ResetPlayerBreath, 0x60A8A0);
    RH_ScopedInstall(RemovePlayerPed, 0x6094A0);
    RH_ScopedInstall(Busted, 0x609EF0);
    RH_ScopedInstall(GetWantedLevel, 0x41BE60);
    RH_ScopedInstall(SetWantedLevel, 0x609F10);
    RH_ScopedInstall(SetWantedLevelNoDrop, 0x609F30);
    RH_ScopedInstall(CheatWantedLevel, 0x609F50);
    RH_ScopedInstall(DoStuffToGoOnFire, 0x60A020);
    RH_ScopedVMTInstall(Load, 0x5D46E0);
    RH_ScopedVMTInstall(Save, 0x5D57E0);
    RH_ScopedInstall(DeactivatePlayerPed, 0x609520);
    RH_ScopedInstall(ReactivatePlayerPed, 0x609540);
    RH_ScopedInstall(GetPadFromPlayer, 0x609560);
    RH_ScopedInstall(CanPlayerStartMission, 0x609590);
    RH_ScopedInstall(IsHidden, 0x609620);
    RH_ScopedInstall(PlayerWantsToAttack, 0x60CC50);
    RH_ScopedInstall(SetInitialState, 0x60CD20);
    RH_ScopedInstall(EvaluateNeighbouringTarget, 0x60D1C0);
    RH_ScopedInstall(ReApplyMoveAnims, 0x609650);
    RH_ScopedInstall(DoesPlayerWantNewWeapon, 0x609710);
    RH_ScopedInstall(ProcessPlayerWeapon, 0x6097F0);
    RH_ScopedInstall(PickWeaponAllowedFor2Player, 0x609800);
    RH_ScopedInstall(UpdateCameraWeaponModes, 0x609830);
    RH_ScopedInstall(ClearWeaponTarget, 0x609c80);
    RH_ScopedInstall(GetWeaponRadiusOnScreen, 0x609CD0);
    RH_ScopedInstall(PedCanBeTargettedVehicleWise, 0x609D90);
    RH_ScopedInstall(FindTargetPriority, 0x609DE0);
    RH_ScopedInstall(Clear3rdPersonMouseTarget, 0x609ED0);
    RH_ScopedInstall(CanIKReachThisTarget, 0x609F80);
    RH_ScopedInstall(GetPlayerInfoForThisPlayerPed, 0x609FF0);
    RH_ScopedInstall(AnnoyPlayerPed, 0x60A040);
    RH_ScopedInstall(ClearAdrenaline, 0x60A070);
    RH_ScopedInstall(DisbandPlayerGroup, 0x60A0A0);
    RH_ScopedInstall(MakeGroupRespondToPlayerTakingDamage, 0x60A110);
    RH_ScopedInstall(TellGroupToStartFollowingPlayer, 0x60A1D0);
    RH_ScopedInstall(MakePlayerGroupDisappear, 0x60A440);
    RH_ScopedInstall(MakePlayerGroupReappear, 0x60A4B0);
    RH_ScopedInstall(HandleSprintEnergy, 0x60A550);
    RH_ScopedInstall(GetButtonSprintResults, 0x60A820);
    RH_ScopedInstall(HandlePlayerBreath, 0x60A8D0);
    RH_ScopedOverloadedInstall(MakeChangesForNewWeapon, "", 0x60B460, void(CPlayerPed::*)(eWeaponType));
    RH_ScopedGlobalInstall(LOSBlockedBetweenPeds, 0x60B550);
    RH_ScopedInstall(DoesTargetHaveToBeBroken, 0x60C0C0);
    RH_ScopedInstall(SetPlayerMoveBlendRatio, 0x60C520);
    RH_ScopedInstall(FindPedToAttack, 0x60C5F0);
    RH_ScopedInstall(ForceGroupToAlwaysFollow, 0x60C7C0);
    RH_ScopedInstall(ForceGroupToNeverFollow, 0x60C800);
    RH_ScopedOverloadedInstall(MakeChangesForNewWeapon, "BySlot", 0x60D000, void(CPlayerPed::*)(uint32));
    RH_ScopedInstall(EvaluateTarget, 0x60D020);
    RH_ScopedInstall(PlayerHasJustAttackedSomeone, 0x60D5A0);
    RH_ScopedInstall(SetupPlayerPed, 0x60D790);
    RH_ScopedInstall(ProcessAnimGroups, 0x6098F0);
    RH_ScopedInstall(ControlButtonSprint, 0x60A610);
    RH_ScopedInstall(SetRealMoveAnim, 0x60A9C0);
    RH_ScopedInstall(Compute3rdPersonMouseTarget, 0x60B650);
    RH_ScopedInstall(DrawTriangleForMouseRecruitPed, 0x60BA80);
    RH_ScopedInstall(KeepAreaAroundPlayerClear, 0x60C1E0);
    RH_ScopedInstall(MakeThisPedJoinOurGroup, 0x60C840);
    RH_ScopedInstall(ProcessGroupBehaviour, 0x60D350);
    RH_ScopedInstall(ProcessWeaponSwitch, 0x60D850);
    RH_ScopedInstall(FindWeaponLockOnTarget, 0x60DC50);
    RH_ScopedInstall(FindNextWeaponLockOnTarget, 0x60E530);

    RH_ScopedVMTInstall(ProcessControl, 0x60EA90);
    RH_ScopedVMTInstall(SetMoveAnim, 0x609490);
}

// TODO: To class and create 2 function
struct CPlayerPedDataSaveStructure {
    uint32          ChaosLevel{};
    eWantedLevel    WantedLevel{};
    CPedClothesDesc ClothesDesc{};
    uint32          ChosenWeapon{};
    // float          Multiplier{}; // Mobile
};

VALIDATE_SIZE(CPlayerPedDataSaveStructure, 0x84 /* + 0x4 */);

// 0x5D46E0
bool CPlayerPed::Load() {
    CPed::Load();

    CGenericGameStorage::LoadDataFromWorkBuffer<uint32>(); // Discard structure size
    auto sd = CGenericGameStorage::LoadDataFromWorkBuffer<CPlayerPedDataSaveStructure>();

    CWanted* wanted = GetPlayerWanted();
    wanted->m_ChaosLevel = sd.ChaosLevel;
    wanted->m_WantedLevel= sd.WantedLevel;

    *GetPlayerData()->m_pPedClothesDesc = sd.ClothesDesc;
    GetPlayerData()->m_nChosenWeapon   = sd.ChosenWeapon;

    return true;
}

// 0x5D57E0
bool CPlayerPed::Save() {
    CPlayerPedDataSaveStructure saveData{};

    CWanted* wanted = GetPlayerWanted();
    saveData.ChaosLevel = wanted->m_ChaosLevel;
    saveData.WantedLevel = wanted->m_WantedLevel;
    saveData.ChosenWeapon = GetPlayerData()->m_nChosenWeapon;
    saveData.ClothesDesc  = *GetPlayerData()->m_pPedClothesDesc;

    CPed::Save();
    CGenericGameStorage::SaveDataToWorkBuffer(sizeof(CPlayerPedDataSaveStructure));
    CGenericGameStorage::SaveDataToWorkBuffer(saveData);

    return true;
}

// 0x60D5B0
CPlayerPed::CPlayerPed(int32 playerId, bool bGroupCreated) : CPed(PED_TYPE_PLAYER1) {
    m_pPlayerData = &CWorld::Players[playerId].m_PlayerData;
    GetPlayerData()->AllocateData();

    CPed::SetModelIndex(MODEL_PLAYER);

    CPlayerPed::SetInitialState(bGroupCreated);

    CEntity::ClearReference(m_pTargetedObject);

    SetPedState(PEDSTATE_IDLE);

    gPlayIdlesAnimBlockIndex = CAnimManager::GetAnimationBlockIndex("playidles");

    if (!bGroupCreated) {
        GetPlayerData()->m_nPlayerGroup = CPedGroups::AddGroup();

        auto& group = CPedGroups::GetGroup(GetPlayerData()->m_nPlayerGroup);
        group.GetIntelligence().SetDefaultTaskAllocatorType(ePedGroupDefaultTaskAllocatorType::RANDOM);
        group.m_bIsMissionGroup = true;
        group.m_groupMembership.SetLeader(this);
        group.Process();

        GetPlayerData()->m_bGroupStuffDisabled = false;
        GetPlayerData()->m_bGroupAlwaysFollow  = false;
        GetPlayerData()->m_bGroupNeverFollow   = false;
    }

    m_fMaxHealth = CStats::GetFatAndMuscleModifier(STAT_MOD_MAX_HEALTH);
    m_fHealth    = m_fMaxHealth;

    m_nFightingStyle      = STYLE_GRAB_KICK;
    m_nAllowedAttackMoves = 15;

    m_p3rdPersonMouseTarget = nullptr;
    field_7A0 = 0;
    m_pedSpeech.Initialise(this);
    GetIntelligence()->m_fDmRadius = 30.0f;
    GetIntelligence()->m_nDmNumPedsToScan = 2;

    bUsedForReplay = bGroupCreated;
}

// 0x6094A0
void CPlayerPed::RemovePlayerPed(int32 playerId) {
    CPed* player = FindPlayerPed(playerId);
    CPlayerInfo* playerInfo = &FindPlayerInfo(playerId);
    if (player)
    {
        CVehicle* playerVehicle = player->m_pVehicle;
        if (playerVehicle && playerVehicle->m_pDriver == player)
        {
            playerVehicle->SetStatus(STATUS_PHYSICS);
            playerVehicle->m_GasPedal = 0.0f;
            playerVehicle->m_BrakePedal = 0.1f;
        }
        CWorld::Remove(static_cast<CEntity*>(player));
        delete player;
        playerInfo->m_pPed = nullptr;
    }
}

// 0x609520
void CPlayerPed::DeactivatePlayerPed(int32 playerId) {
    assert(playerId >= 0);
    CWorld::Remove(FindPlayerPed(playerId));
}

// 0x609540
void CPlayerPed::ReactivatePlayerPed(int32 playerId) {
    assert(playerId >= 0);
    CWorld::Add(FindPlayerPed(playerId));
}

// 0x609560
CPad* CPlayerPed::GetPadFromPlayer() const {
    switch (m_nPedType) {
    case PED_TYPE_PLAYER1:
        return CPad::GetPad(0);

    case PED_TYPE_PLAYER2:
        return CPad::GetPad(1);
    }
    assert(0); // shouldn't happen
    return nullptr;
}

// 0x609590
bool CPlayerPed::CanPlayerStartMission() {
    if (CGameLogic::GameState != GAMELOGIC_STATE_PLAYING || CGameLogic::IsCoopGameGoingOn())
        return false;

    if (!IsPedInControl() && !IsStateDriving())
        return false;

    auto& taskMgr = GetTaskManager();
    if (taskMgr.GetTaskPrimary(TASK_PRIMARY_PHYSICAL_RESPONSE))
        return false;

    if (taskMgr.GetTaskPrimary(TASK_PRIMARY_EVENT_RESPONSE_NONTEMP))
        return false;

    auto primaryTask = taskMgr.GetTaskPrimary(TASK_PRIMARY_PRIMARY);
    if (primaryTask != nullptr && primaryTask->GetTaskType() != TASK_SIMPLE_CAR_DRIVE)
        return false;

    if (taskMgr.GetTaskSecondary(TASK_SECONDARY_ATTACK))
        return false;

    if (!IsAlive())
        return false;

    return !GetEventGroup().GetEventOfType(EVENT_SCRIPT_COMMAND);
}

// 0x609620
bool CPlayerPed::IsHidden() {
    return bInVehicle && GetLightingTotal() <= 0.05f;
}

// 0x609650
void CPlayerPed::ReApplyMoveAnims() {
    constexpr AnimationId anims[]{
        ANIM_ID_WALK,
        ANIM_ID_RUN,
        ANIM_ID_SPRINT,
        ANIM_ID_IDLE,
        ANIM_ID_WALK_START
    };
    for (const AnimationId& id : anims) {
        if (CAnimBlendAssociation* anim = RpAnimBlendClumpGetAssociation(GetRpClump(), id)) {
            if (anim->GetHashKey() != CAnimManager::GetAnimAssociation(m_nAnimGroup, id)->GetHashKey()) {
                CAnimBlendAssociation* addedAnim = CAnimManager::AddAnimation(GetRpClump(), m_nAnimGroup, id);
                addedAnim->m_BlendDelta = anim->m_BlendDelta;
                addedAnim->m_BlendAmount = anim->m_BlendAmount;

                anim->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
                anim->m_BlendDelta = -1000.0f;
            }
        }
    }
}

// 0x609710
bool CPlayerPed::DoesPlayerWantNewWeapon(eWeaponType weaponType, bool arg1) {
    // GetPadFromPlayer(); // Called, but not used

    auto weaponSlot = GetWeaponSlot(weaponType);
    auto weaponInSlotType = GetWeaponInSlot(weaponSlot).m_Type;
    if (weaponInSlotType == weaponType)
        return true;

    if (weaponInSlotType == eWeaponType::WEAPON_UNARMED)
        return true;

    if (arg1)
        return false;

    if (GetIntelligence()->GetTaskJetPack())
        return false;

    /* !See comment!
    if (m_nActiveWeaponSlot == weaponSlot
        && CWeaponInfo::GetWeaponInfo(weaponInSlotType, GetWeaponSkill(weaponInSlotType))->flags.bAimWithArm   \ One of these two is always false, so
        && !CWeaponInfo::GetWeaponInfo(weaponInSlotType, GetWeaponSkill(weaponInSlotType))->flags.bAimWithArm) / the whole expression is always false
        return false;
    */

    if (m_nActiveWeaponSlot == weaponSlot) {
        switch (m_nPedState) {
        case PEDSTATE_ATTACK:
        case PEDSTATE_AIMGUN:
            return false;
        }
    }

    return true;
}

// 0x6097F0
void CPlayerPed::ProcessPlayerWeapon(CPad* pad) {
    /* empty */
}

// 0x609800
void CPlayerPed::PickWeaponAllowedFor2Player() {
    if (!GetWeaponInSlot(GetPlayerData()->m_nChosenWeapon).CanBeUsedFor2Player()) {
        GetPlayerData()->m_nChosenWeapon = eWeaponType::WEAPON_UNARMED;
    }
}

// 0x609830
void CPlayerPed::UpdateCameraWeaponModes(CPad* pad) {
    switch (GetActiveWeapon().m_Type) {
    case eWeaponType::WEAPON_M4:
        TheCamera.SetNewPlayerWeaponMode(eCamMode::MODE_M16_1STPERSON, 0, 0);
        break;

    case eWeaponType::WEAPON_SNIPERRIFLE:
        TheCamera.SetNewPlayerWeaponMode(eCamMode::MODE_SNIPER, 0, 0);
        break;

    case eWeaponType::WEAPON_RLAUNCHER:
        TheCamera.SetNewPlayerWeaponMode(eCamMode::MODE_ROCKETLAUNCHER, 0, 0);
        break;

    case eWeaponType::WEAPON_RLAUNCHER_HS:
        TheCamera.SetNewPlayerWeaponMode(eCamMode::MODE_ROCKETLAUNCHER_HS, 0, 0);
        break;

    case eWeaponType::WEAPON_CAMERA:
        TheCamera.SetNewPlayerWeaponMode(eCamMode::MODE_CAMERA, 0, 0);
        break;

    default:
        TheCamera.ClearPlayerWeaponMode();
        break;
    }
}

// 0x6098F0
void CPlayerPed::ProcessAnimGroups() {
    auto* const playerData = GetPlayerData();

    // Figure out the motion group to use
    const auto GetMotionGroup = [&]() -> AssocGroupId {
        // Offsets the given `ANIM_GROUP_PLAYER*` group by the current body type (normal, fat, muscular) of the player
        const auto OffsetByDefaultGroup = [](int32 playerGroup) {
            return (AssocGroupId)(playerGroup + (int32)CClothes::GetDefaultPlayerMotionGroup() - (int32)ANIM_GROUP_PLAYER);
        };
        // For the plain `ANIM_GROUP_PLAYER` the 2nd player uses the group of its model (unless that is the plain one too)
        const auto GetPlainGroup = [&] {
            if (m_nPedType == PED_TYPE_PLAYER2) {
                const auto modelGroup = CModelInfo::GetModelInfo(m_nModelIndex)->AsPedModelInfoPtr()->m_nAnimType;
                if (modelGroup != ANIM_GROUP_PLAYER) {
                    return modelGroup;
                }
            }
            return OffsetByDefaultGroup(ANIM_GROUP_PLAYER);
        };

        const auto fpsMoveHeading = playerData->m_fFPSMoveHeading;
        if ((fpsMoveHeading <= DegreesToRadians(-50.0f) || fpsMoveHeading >= DegreesToRadians(50.0f))
            && TheCamera.GetActiveCam().Using3rdPersonMouseCam()
            && CanStrafeOrMouseControl()
        ) {
            return m_nAnimGroup == ANIM_GROUP_PLAYER
                ? GetPlainGroup()
                : OffsetByDefaultGroup(m_nAnimGroup);
        }

        auto weaponType = WEAPON_UNARMED;
        if (m_pWeaponObject) {
            if (auto* const mi = CVisibilityPlugins::GetClumpModelInfo(m_pWeaponObject); mi && mi->GetModelType() == MODEL_INFO_WEAPON) {
                weaponType = mi->AsWeaponModelInfoPtr()->m_weaponInfo;
                switch (weaponType) {
                case WEAPON_RLAUNCHER:
                case WEAPON_RLAUNCHER_HS:
                    return OffsetByDefaultGroup(ANIM_GROUP_PLAYERROCKET);
                }
            }
        }

        if (GetIntelligence()->GetTaskJetPack()) {
            return ANIM_GROUP_PLAYERJETPACK;
        }

        switch (weaponType) {
        case WEAPON_BASEBALLBAT:
        case WEAPON_SHOVEL:
        case WEAPON_POOL_CUE:
            return OffsetByDefaultGroup(ANIM_GROUP_PLAYERBBBAT);
        case WEAPON_CHAINSAW:
        case WEAPON_FLAMETHROWER:
        case WEAPON_MINIGUN:
            return OffsetByDefaultGroup(ANIM_GROUP_PLAYERCSAW);
        case WEAPON_M4:
        case WEAPON_AK47:
        case WEAPON_SPAS12_SHOTGUN:
        case WEAPON_SHOTGUN:
        case WEAPON_SNIPERRIFLE:
        case WEAPON_COUNTRYRIFLE:
            return OffsetByDefaultGroup(ANIM_GROUP_PLAYER2ARMED);
        }

        if (playerData->m_pPedClothesDesc->GetIsWearingBalaclava()) {
            return ANIM_GROUP_PLAYERSNEAK;
        }

        return GetPlainGroup();
    };

    if (const auto motionGroup = GetMotionGroup(); m_nAnimGroup != motionGroup) {
        m_nAnimGroup = motionGroup;
        ReApplyMoveAnims();
    }

    // Now deal with the anim blocks of the melee weapon and of the fighting style
    bool releaseWeaponAnims = false, releaseStyleAnims = false;

    // Makes sure the anim block of the melee combo is loaded and referenced
    const auto ProcessMeleeAnimReference = [](int32 combo, uint32& referencedGroup, bool& release) {
        if (combo == MELEE_COMBO_UNARMED_1) { // No anims required
            if (referencedGroup != 0) {
                release = true;
            }
            return;
        }
        const auto requiredGroup = CTaskSimpleFight::m_aComboData[combo - MELEE_COMBO_UNARMED_1].m_nAnimGroup;
        if ((uint32)requiredGroup == referencedGroup) {
            return;
        }
        if (referencedGroup != 0) {
            release = true;
        }
        auto* animBlock = CAnimManager::GetAnimationBlock(requiredGroup);
        if (!animBlock) {
            animBlock = CAnimManager::GetAnimationBlock(CAnimManager::GetAnimBlockName(requiredGroup));
        }
        const auto animBlockIndex = CAnimManager::GetAnimationBlockIndex(animBlock);
        if (!animBlock->IsLoaded) {
            CStreaming::RequestModel(IFPToModelId(animBlockIndex), STREAMING_KEEP_IN_MEMORY);
        } else if (referencedGroup == 0) {
            CAnimManager::AddAnimBlockRef(animBlockIndex);
            referencedGroup = (uint32)requiredGroup;
        }
    };

    const auto* const weaponInfo = CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, GetWeaponSkill());
    if (weaponInfo->m_nWeaponFire != WEAPON_FIRE_MELEE || bInVehicle) {
        releaseWeaponAnims = true;
        releaseStyleAnims  = true;
    } else {
        ProcessMeleeAnimReference((int8)weaponInfo->m_nBaseCombo, playerData->m_nMeleeWeaponAnimReferenced, releaseWeaponAnims);
        ProcessMeleeAnimReference((uint8)m_nFightingStyle, playerData->m_nMeleeWeaponAnimReferencedExtra, releaseStyleAnims);
    }

    const auto ReleaseMeleeAnimReference = [](uint32& referencedGroup) {
        if (referencedGroup != 0) {
            CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex((AssocGroupId)referencedGroup));
            referencedGroup = 0;
        }
    };
    if (releaseWeaponAnims) {
        ReleaseMeleeAnimReference(playerData->m_nMeleeWeaponAnimReferenced);
    }
    if (releaseStyleAnims) {
        ReleaseMeleeAnimReference(playerData->m_nMeleeWeaponAnimReferencedExtra);
    }
}

// 0x609C80
void CPlayerPed::ClearWeaponTarget() {
    if (IsPlayer()) {
        CEntity::ClearReference(m_pTargetedObject);
        TheCamera.ClearPlayerWeaponMode();
        CWeaponEffects::ClearCrossHair(m_nPedType);
    }
}

// 0x609CD0
float CPlayerPed::GetWeaponRadiusOnScreen() {
    CWeapon& wep = GetActiveWeapon();
    CWeaponInfo& wepInfo = wep.GetWeaponInfo(this);

    if (wep.IsTypeMelee())
        return 0.0f;

    const float accuracyProg = 0.5f / wepInfo.m_fAccuracy;
    switch (wep.m_Type) {
    case eWeaponType::WEAPON_SHOTGUN:
    case eWeaponType::WEAPON_SPAS12_SHOTGUN:
    case eWeaponType::WEAPON_SAWNOFF_SHOTGUN:
        return std::max(0.2f, accuracyProg); // here they multiply *accuracyProg * 1.0f* :thinking

    default: {
        const float rangeProg = std::min(1.0f, 15.0f / wepInfo.m_fWeaponRange);
        const float radius = (GetPlayerData()->m_fAttackButtonCounter * 0.5f + 1.0f) * rangeProg * accuracyProg;
        if (bIsDucking)
            return std::max(0.2f, radius / 2.0f);
        return std::max(0.2f, radius);
    }
    }
}

// 0x609D90
bool CPlayerPed::PedCanBeTargettedVehicleWise(CPed* ped) {
    if (ped->bInVehicle) {
        CVehicle* veh = ped->m_pVehicle;
        return veh && (veh->IsBike() || veh->vehicleFlags.bVehicleCanBeTargetted);
    }
    return true;
}

// 0x609DE0
float CPlayerPed::FindTargetPriority(CEntity* entity) {
    switch (entity->GetType()) {
    case ENTITY_TYPE_VEHICLE:
        return 0.1f;

    case ENTITY_TYPE_PED: {
        auto ped = entity->AsPed();

        if (ped->bThisPedIsATargetPriority)
            return 1.0f;

        if (ped->GetTaskManager().HasAnyOf<TASK_COMPLEX_KILL_PED_ON_FOOT, TASK_COMPLEX_ARREST_PED>()) {
            return 0.8f;
        }

        if (CPedGroups::AreInSameGroup(this, ped))
            return 0.05f;

        if (ped->m_nPedType == PED_TYPE_GANG2)
            return 0.06f;

        if (ped->IsCreatedByMission())
            return 0.25f;

        return 0.1f;
    }

    case ENTITY_TYPE_OBJECT: {
        switch (entity->AsObject()->m_nObjectType) {
        case eObjectType::OBJECT_MISSION:
        case eObjectType::OBJECT_MISSION2:
            return 0.1f;

        default:
            return 0.0f;
        }
    }
    default: {
        return 0.1f;
    }
    }
}

// 0x609ED0
void CPlayerPed::Clear3rdPersonMouseTarget() {
    CEntity::ClearReference(m_p3rdPersonMouseTarget);
}

// 0x609EF0
void CPlayerPed::Busted() {
    CWanted* wanted = GetWanted();
    if (wanted) {
        wanted->m_ChaosLevel = 0;
    }
}

// 0x41BE60
eWantedLevel CPlayerPed::GetWantedLevel() const {
    if (const auto* wanted = GetWanted()) {
        return wanted->GetWantedLevel();
    }

    return eWantedLevel::WANTED_CLEAN;
}

// 0x609F10
void CPlayerPed::SetWantedLevel(eWantedLevel level) {
    CWanted* wanted = GetWanted();
    wanted->SetWantedLevel(level);
}

// 0x609F30
void CPlayerPed::SetWantedLevelNoDrop(eWantedLevel level) {
    CWanted* wanted = GetWanted();
    wanted->SetWantedLevelNoDrop(level);
}

// 0x609F50
void CPlayerPed::CheatWantedLevel(eWantedLevel level) {
    CWanted* wanted = GetWanted();
    wanted->CheatWantedLevel(level);
}

// 0x609F80
bool CPlayerPed::CanIKReachThisTarget(CVector posn, CWeapon* weapon, bool arg2) {
    if (!weapon->GetWeaponInfo(this).flags.bAimWithArm) {
        const CVector thisPos = GetPosition();
        return (posn - thisPos).Magnitude2D() >= thisPos.z - posn.z;
    }
    return true;
}

// 0x609FF0
CPlayerInfo* CPlayerPed::GetPlayerInfoForThisPlayerPed() {
    // TODO: Use range for here
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (FindPlayerPed(i) == this)
            return &FindPlayerInfo(i);
    }
    return nullptr;
}

// 0x60A020
void CPlayerPed::DoStuffToGoOnFire() {
    if (m_nPedState == PEDSTATE_SNIPER_MODE)
        TheCamera.ClearPlayerWeaponMode();
}

// 0x60A040
void CPlayerPed::AnnoyPlayerPed(bool arg0) {
    auto& temper = m_pStats->m_nTemper;

    if (temper < 52) {
        temper++;
    } else if (arg0) {
        if (temper < 55)
            temper++;
        else
            temper = 46;
    }
}

// 0x60A070
void CPlayerPed::ClearAdrenaline() {
    if (GetPlayerData()->m_bAdrenaline && GetPlayerData()->m_nAdrenalineEndTime != 0) {
        GetPlayerData()->m_nAdrenalineEndTime = 0;
        CTimer::ResetTimeScale();
    }
}

// 0x60A0A0
void CPlayerPed::DisbandPlayerGroup() {
    auto& ms = GetPlayerGroup().GetMembership();
    if (const auto numMembers = ms.CountMembersExcludingLeader()) {
        Say(numMembers > 1 ? CTX_GLOBAL_ORDER_DISBAND_MANY : CTX_GLOBAL_ORDER_ATTACK_SINGLE);
    } else {
        ms.RemoveAllFollowers(true);
    }
}

// 0x60A110
void CPlayerPed::MakeGroupRespondToPlayerTakingDamage(CEventDamage& damageEvent) {
    auto& group = GetPlayerGroup();
    if (!damageEvent.m_pSourceEntity)
        return;
    if (group.GetMembership().CountMembersExcludingLeader() < 1)
        return;
    if (!group.m_bMembersEnterLeadersVehicle)
        return;

    CEventGroupEvent groupEvent(this, damageEvent.Clone());
    group.GetIntelligence().AddEvent(&groupEvent);
}

// 0x60A1D0
void CPlayerPed::TellGroupToStartFollowingPlayer(bool arg0, bool arg1, bool arg2) {
    if (GetPlayerData()->m_bGroupAlwaysFollow && !arg0)
        return;
    if (GetPlayerData()->m_bGroupNeverFollow && arg0)
        return;

    CPedGroup& group = GetPlayerGroup();
    CPedGroupIntelligence& groupIntel = group.GetIntelligence();
    CPedGroupMembership& membership = group.GetMembership();
    if (!arg2 && !membership.CountMembersExcludingLeader())
        return;

    group.m_bMembersEnterLeadersVehicle = arg0;
    groupIntel.SetDefaultTaskAllocatorType(ePedGroupDefaultTaskAllocatorType::RANDOM);
    if (arg0) {
        CEventPlayerCommandToGroup playerCmdEvent;
        playerCmdEvent.ComputeResponseTaskType(&group);
        if (playerCmdEvent.WillRespond()) {
            auto gatherCmdEvent = new CEventPlayerCommandToGroup(ePlayerGroupCommand::PLAYER_GROUP_COMMAND_GATHER);
            gatherCmdEvent->m_TaskId = playerCmdEvent.m_TaskId;

            CEventGroupEvent groupEvent(this, gatherCmdEvent);
            groupIntel.AddEvent(&groupEvent);
        }
    }

    if (arg1) {
        const uint32 nMembers = membership.CountMembersExcludingLeader();
        if (!nMembers)
            return;

        if (arg0) {
            const float distToFurthest = group.FindDistanceToFurthestMember();
            if (nMembers > 1) {
                if (distToFurthest >= 3.0f)
                    Say(distToFurthest >= 10.0f ? CTX_GLOBAL_ORDER_FOLLOW_FAR_MANY : CTX_GLOBAL_ORDER_FOLLOW_NEAR_MANY);
                else
                    Say(CTX_GLOBAL_ORDER_FOLLOW_VNEAR_MANY);
            } else {
                if (distToFurthest >= 3.0f)
                    Say(distToFurthest >= 10.0f ? CTX_GLOBAL_ORDER_FOLLOW_FAR_ONE : CTX_GLOBAL_ORDER_FOLLOW_NEAR_ONE);
                else
                    Say(CTX_GLOBAL_ORDER_FOLLOW_VNEAR_ONE);
            }
        } else if (nMembers > 1) {
            Say(CTX_GLOBAL_ORDER_WAIT_MANY);
        } else {
            Say(CTX_GLOBAL_ORDER_WAIT_ONE);
        }
    }
}

// 0x60A440
void CPlayerPed::MakePlayerGroupDisappear() {
    CPedGroupMembership& membership = GetPlayerGroup().GetMembership();
    for (int i = 0; i < TOTAL_PED_GROUP_FOLLOWERS; i++) {
        if (CPed* member = membership.GetMember(i)) {
            if (!member->IsCreatedByMission()) {
                member->SetCollisionProcessed(false);
                member->SetIsVisible(false);
                abTempNeverLeavesGroup[i] = member->bNeverLeavesGroup;
                member->bNeverLeavesGroup = true;
            }
        }
    }
}

// 0x60A4B0
void CPlayerPed::MakePlayerGroupReappear() {
    CPedGroupMembership& membership = GetPlayerGroup().GetMembership();
    for (int i = 0; i < TOTAL_PED_GROUP_FOLLOWERS; i++) {
        if (CPed* member = membership.GetMember(i)) {
            if (!member->IsCreatedByMission()) {
                member->SetIsVisible(true);
                if (!member->bInVehicle)
                    member->SetUsesCollision(true);
                member->bNeverLeavesGroup = abTempNeverLeavesGroup[i];
            }
        }
    }
}

// 0x60A530
void CPlayerPed::ResetSprintEnergy()
{
    GetPlayerData()->m_fTimeCanRun = CStats::GetFatAndMuscleModifier(STAT_MOD_TIME_CAN_RUN);
}

// 0x60A550
bool CPlayerPed::HandleSprintEnergy(bool sprint, float adrenalineConsumedPerTimeStep) {
    float& timeCanRun = GetPlayerData()->m_fTimeCanRun;
    if (sprint) {
        if (FindPlayerInfo().m_bDoesNotGetTired)
            return true;
        if (GetPlayerData()->m_bAdrenaline || adrenalineConsumedPerTimeStep == 0.0f)
            return true;

        if (timeCanRun > -150.0f) { // TODO: Find out what this magic number is
            timeCanRun = std::max(-150.0f, timeCanRun - CTimer::GetTimeStep() * adrenalineConsumedPerTimeStep);
            return true;
        }
    } else {
        if (CStats::GetFatAndMuscleModifier(STAT_MOD_TIME_CAN_RUN) > timeCanRun) {
            timeCanRun += CTimer::GetTimeStep() * adrenalineConsumedPerTimeStep / 2.0f;
        }
    }
    return false;
}

constexpr auto PLAYER_SPRINT_THRESHOLD{ 5.0f }; // 0x8D2458
constexpr struct tPlayerSprintSet { // From 0x8D2460
    float field_0;
    float field_4;
    float field_8;
    float field_C;
    float field_10;
    float field_14;
    float field_18;
    float field_1C;
} PLAYER_SPRINT_SET[] = {
    // 0x0, 0x4,  0x8,  0xC,  0x10,  0x14, 0x18, 0x1C
    { 4.0f, 0.7f, 0.2f, 5.0f, 10.0f, 1.0f, 0.5f, 0.3f }, // GROUND
    { 4.0f, 0.7f, 0.2f, 5.0f, 10.0f, 0.0f, 0.4f, 1.0f }, // BMX
    { 4.0f, 0.7f, 0.2f, 5.0f, 10.0f, 1.0f, 0.3f, 0.3f }, // WATER
    { 4.0f, 0.7f, 0.2f, 5.0f, 10.0f, 0.0f, 0.0f, 1.0f }  // UNDERWATER
};

// 0x60A610
float CPlayerPed::ControlButtonSprint(eSprintType sprintType) {
    auto* const playerData = GetPlayerData();
    if (!playerData) {
        return 0.0f;
    }

    const auto* const pad       = GetPadFromPlayer();
    const auto&       sprintSet = PLAYER_SPRINT_SET[sprintType];
    float&            moveSpeed = playerData->m_fMoveSpeed;

    const bool canSprint = !playerData->m_bPlayerSprintDisabled && (moveSpeed > 0.0f || playerData->m_fTimeCanRun > 0.0f);

    if (pad->SprintJustDown() && canSprint) { // Tapping the button
        moveSpeed = std::min(sprintSet.field_0 + moveSpeed, sprintSet.field_10);
    } else if (pad->GetSprint() && canSprint) { // Holding the button
        moveSpeed = std::max(moveSpeed - CTimer::GetTimeStep() * sprintSet.field_4, 1.0f);
    } else if (moveSpeed > 0.0f) { // Slowing down
        moveSpeed = std::max(moveSpeed - CTimer::GetTimeStep() * sprintSet.field_8, 0.0f);
    }

    float progress, energyConsumption;
    if (moveSpeed > sprintSet.field_C) {
        progress          = moveSpeed / sprintSet.field_C;
        energyConsumption = sprintSet.field_18;
        if (progress <= 0.0f) {
            return 0.0f;
        }
    } else {
        if (moveSpeed <= 0.0f || !canSprint) {
            return 0.0f;
        }
        progress          = 1.0f;
        energyConsumption = sprintSet.field_14;
    }

    if (!HandleSprintEnergy(true, energyConsumption)) {
        moveSpeed = 0.0f;
        return 0.0f;
    }

    return std::max(progress - 1.0f, 0.0f) * sprintSet.field_1C + 1.0f;
}

// 0x60A820
float CPlayerPed::GetButtonSprintResults(eSprintType sprintType) {
    // The original function doesn't touch `edx`; it points to an anim blend assoc.
    // Callers at 0x60B430 rely on it, so preserve it (same pattern as Radar.cpp).
    _asm { push edx };

    float result;
    auto* playerData = GetPlayerData();
    if (playerData->m_fMoveSpeed <= PLAYER_SPRINT_THRESHOLD) {
        const auto progress = std::max(0.0f, playerData->m_fMoveSpeed / PLAYER_SPRINT_THRESHOLD - 1.0f);
        result = PLAYER_SPRINT_SET[sprintType].field_1C * progress + 1.0f;
    } else {
        result = playerData->m_fMoveSpeed > 0.0f ? 0.0f : 1.0f;
    }

    _asm { pop edx };
    return result;
}

// 0x60A8A0
void CPlayerPed::ResetPlayerBreath() {
    GetPlayerData()->m_fBreath = CStats::GetFatAndMuscleModifier(STAT_MOD_AIR_IN_LUNG);
    GetPlayerData()->m_bRequireHandleBreath = false;
}

// 0x60A8D0
void CPlayerPed::HandlePlayerBreath(bool bDecreaseAir, float fMultiplier) {
    float& breath = GetPlayerData()->m_fBreath;
    float  decreaseAmount = CTimer::GetTimeStep() * fMultiplier;
    if (!bDecreaseAir || CCheat::IsActive(CHEAT_INFINITE_OXYGEN)) {
        if (CStats::GetFatAndMuscleModifier(STAT_MOD_AIR_IN_LUNG) > breath)
            breath += decreaseAmount * 2.0f;
    } else {
        if (breath > 0.0f && bDrownsInWater)
            breath = std::max(0.0f, breath - decreaseAmount);
        else
            CWeapon::GenerateDamageEvent(this, this, eWeaponType::WEAPON_DROWNING, (int32)(decreaseAmount * 3.0f), PED_PIECE_TORSO, 0);
    }
    GetPlayerData()->m_bRequireHandleBreath = false;
}

// 0x60A9C0
void CPlayerPed::SetRealMoveAnim() {
    static auto& PLAYER_TURN_ANIM_SPEED_MULT = StaticRef<float>(0x8D24EC); // 1.0f - NOTSA name

    auto* const clump      = GetRpClump();
    auto* const playerData = GetPlayerData();

    auto* walkAssoc      = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_WALK);
    auto* runAssoc       = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_RUN);
    auto* sprintAssoc    = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_SPRINT);
    auto* walkStartAssoc = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_WALK_START);
    auto* idleAssoc      = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_IDLE);
    auto* runStopAssoc   = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_RUN_STOP);
    auto* runStopRAssoc  = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_RUN_STOPR);
    auto* turnLAssoc     = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_TURN_L);
    auto* turnRAssoc     = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_TURN_R);
    auto* idleTiredAssoc = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_IDLE_TIRED);

    if (bResetWalkAnims) {
        if (walkAssoc) {
            walkAssoc->SetCurrentTime(0.0f);
        }
        if (runAssoc) {
            runAssoc->SetCurrentTime(0.0f);
        }
        if (sprintAssoc) {
            sprintAssoc->SetCurrentTime(0.0f);
        }
        bResetWalkAnims = false;
    }

    // NOTE: The original writes `m_nMoveState` directly everywhere in here (Android uses `SetMoveState`)
    if ((runStopAssoc && runStopAssoc->IsPlaying()) || (runStopRAssoc && runStopRAssoc->IsPlaying())) {
        m_nMoveState = PEDMOVE_RUN;
        if (runStopAssoc && !runStopAssoc->IsPlaying() && runStopAssoc->m_BlendHier->m_fTotalTime > runStopAssoc->m_CurrentTime) {
            runStopAssoc->m_Flags |= ANIMATION_IS_PLAYING;
        }
    } else if ((runStopAssoc && runStopAssoc->m_BlendDelta >= 0.0f) || (runStopRAssoc && runStopRAssoc->m_BlendDelta >= 0.0f)) {
        if (auto* const stopAssoc = runStopAssoc ? runStopAssoc : runStopRAssoc) {
            stopAssoc->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
            stopAssoc->m_BlendAmount = 1.0f;
            stopAssoc->m_BlendDelta  = -8.0f;
        }
        RestoreHeadingRate();
        if (!idleAssoc) {
            idleAssoc = CAnimManager::BlendAnimation(clump, m_nAnimGroup, ANIM_ID_IDLE, 8.0f);
        }
        idleAssoc->m_BlendAmount = 0.0f;
        idleAssoc->m_BlendDelta  = 8.0f;
    } else if (playerData->m_fMoveBlendRatio == 0.0f && !sprintAssoc) { // Standing still
        const auto GetAimLeftRight = [this] { return GetPadFromPlayer()->AimWeaponLeftRight(nullptr); };

        if (GetPadFromPlayer()->GetForceCameraBehindPlayer() && GetAimLeftRight() != 0 && TheCamera.GetActiveCam().m_nMode == MODE_FOLLOWPED) {
            const auto isTurningLeft = (float)GetAimLeftRight() < 0.0f;
            const auto turnAnimId    = isTurningLeft ? ANIM_ID_TURN_L : ANIM_ID_TURN_R;
            auto*      turnAssoc     = isTurningLeft ? turnLAssoc : turnRAssoc;
            if (!turnAssoc || turnAssoc->m_BlendDelta < 0.0f || (turnAssoc->m_BlendAmount < 1.0f && turnAssoc->m_BlendDelta <= 0.0f)) {
                turnAssoc = CAnimManager::BlendAnimation(clump, ANIM_GROUP_DEFAULT, turnAnimId, 16.0f);
            }
            turnAssoc->m_Speed = std::fabs((float)GetAimLeftRight()) * PLAYER_TURN_ANIM_SPEED_MULT * (1.0f / 128.0f);
            if (idleAssoc && idleAssoc->m_BlendAmount <= 0.01f) {
                delete idleAssoc;
            }
        } else if (!idleAssoc) {
            CAnimManager::BlendAnimation(clump, m_nAnimGroup, ANIM_ID_IDLE, 4.0f);
        }

        if (playerData->m_fTimeCanRun < 0.0f
            && !GetIntelligence()->GetTaskFighting()
            && !GetIntelligence()->GetTaskUseGun()
            && !GetIntelligence()->GetTaskDuck(true)
            && !GetIntelligence()->GetTaskThrow()
            && !GetIntelligence()->GetTaskJetPack()
            && !CWorld::TestSphereAgainstWorld(GetPosition(), 0.5f, nullptr, true, false, false, false, false, false)
        ) {
            if (!idleTiredAssoc) {
                const auto tiredGroup = CClothes::GetDefaultPlayerMotionGroup() == ANIM_GROUP_FAT ? ANIM_GROUP_FAT_TIRED : ANIM_GROUP_DEFAULT;
                CAnimManager::BlendAnimation(clump, tiredGroup, ANIM_ID_IDLE_TIRED, 4.0f)->m_Flags |= ANIMATION_IS_PLAYING;
            }
        } else if (idleTiredAssoc && idleTiredAssoc->m_BlendAmount > 0.0f && idleTiredAssoc->m_BlendDelta >= 0.0f) {
            idleTiredAssoc->m_Flags &= ~ANIMATION_IS_PLAYING;
            idleTiredAssoc->m_BlendDelta = -2.0f;
        }
        m_nMoveState = PEDMOVE_STILL;
    } else { // Moving
        if (idleAssoc) {
            if (walkStartAssoc) {
                walkStartAssoc->m_BlendAmount = 1.0f;
                walkStartAssoc->m_BlendDelta  = 0.0f;
            } else {
                walkStartAssoc = CAnimManager::AddAnimation(clump, m_nAnimGroup, ANIM_ID_WALK_START);
            }
            if (walkAssoc) {
                walkAssoc->SetCurrentTime(0.0f);
            }
            if (runAssoc) {
                runAssoc->SetCurrentTime(0.0f);
            }
            delete idleAssoc;
            if (idleTiredAssoc) {
                idleTiredAssoc->m_BlendDelta = -4.0f;
            }
            if (sprintAssoc) {
                delete sprintAssoc;
            }
            sprintAssoc  = nullptr;
            m_nMoveState = PEDMOVE_WALK;
        }
        if (runStopAssoc) {
            delete runStopAssoc;
            RestoreHeadingRate();
        }
        if (runStopRAssoc) {
            delete runStopRAssoc;
            RestoreHeadingRate();
        }
        if (turnLAssoc) {
            delete turnLAssoc;
        }
        if (turnRAssoc) {
            delete turnRAssoc;
        }
        if (!walkAssoc) {
            walkAssoc = CAnimManager::AddAnimation(clump, m_nAnimGroup, ANIM_ID_WALK);
            walkAssoc->m_BlendAmount = 0.0f;
        }
        if (!runAssoc) {
            runAssoc = CAnimManager::AddAnimation(clump, m_nAnimGroup, ANIM_ID_RUN);
            runAssoc->m_BlendAmount = 0.0f;
        }
        if (walkStartAssoc) {
            if (!walkStartAssoc->IsPlaying() || walkStartAssoc->m_BlendHier->m_fTotalTime <= walkStartAssoc->m_TimeStep + walkStartAssoc->m_CurrentTime) {
                delete walkStartAssoc;
                walkAssoc->m_Flags |= ANIMATION_IS_PLAYING;
                runAssoc->m_Flags |= ANIMATION_IS_PLAYING;
                walkStartAssoc = nullptr;
            }
        }
        if (m_nMoveState == PEDMOVE_SPRINT && walkStartAssoc) {
            m_nMoveState = PEDMOVE_STILL;
        }

        if (sprintAssoc && (m_nMoveState != PEDMOVE_SPRINT || playerData->m_fMoveBlendRatio < 0.4f)) { // Blend out of the sprint
            if (sprintAssoc->m_BlendAmount == 0.0f) {
                sprintAssoc->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
                sprintAssoc->m_BlendDelta = -1000.0f;
            } else if (sprintAssoc->m_BlendDelta < 0.0f && sprintAssoc->m_BlendAmount < 0.8f) {
                if (playerData->m_fMoveBlendRatio < 1.0f) {
                    sprintAssoc->m_BlendDelta = -8.0f;
                    runAssoc->m_BlendDelta    = 8.0f;
                }
            } else if (playerData->m_fMoveBlendRatio < 0.4f) {
                const auto stopAnimId = sprintAssoc->m_CurrentTime / sprintAssoc->m_BlendHier->m_fTotalTime < 0.5f
                    ? ANIM_ID_RUN_STOP
                    : ANIM_ID_RUN_STOPR;
                auto* const stopAssoc = CAnimManager::AddAnimation(clump, ANIM_GROUP_DEFAULT, stopAnimId);
                stopAssoc->m_BlendAmount = 1.0f;
                stopAssoc->SetDeleteCallback(RestoreHeadingRateCB, this);

                m_fHeadingChangeRate = 0.0f;

                sprintAssoc->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
                sprintAssoc->m_BlendDelta = -1000.0f;

                walkAssoc->m_Flags &= ~ANIMATION_IS_PLAYING;
                runAssoc->m_Flags &= ~ANIMATION_IS_PLAYING;
                walkAssoc->m_BlendAmount = 0.0f;
                runAssoc->m_BlendAmount  = 0.0f;
                walkAssoc->m_BlendDelta  = 0.0f;
                runAssoc->m_BlendDelta   = 0.0f;
            } else if (sprintAssoc->m_BlendDelta >= 0.0f) {
                sprintAssoc->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
                sprintAssoc->m_BlendDelta = -1.0f;
                runAssoc->m_BlendDelta    = 1.0f;
            }
            m_nMoveState = playerData->m_fMoveBlendRatio <= 1.0f ? PEDMOVE_WALK : PEDMOVE_RUN;
        } else if (walkStartAssoc) {
            walkAssoc->m_Flags &= ~ANIMATION_IS_PLAYING;
            runAssoc->m_Flags &= ~ANIMATION_IS_PLAYING;
            walkAssoc->m_BlendAmount = 0.0f;
            runAssoc->m_BlendAmount  = 0.0f;
        } else if (m_nMoveState != PEDMOVE_SPRINT) {
            if (playerData->m_fMoveBlendRatio < 1.0f) {
                walkAssoc->m_BlendAmount = 1.0f;
                runAssoc->m_BlendAmount  = 0.0f;
                walkAssoc->m_BlendDelta  = 0.0f;
                runAssoc->m_BlendDelta   = 0.0f;
                m_nMoveState             = PEDMOVE_WALK;
            } else if (playerData->m_fMoveBlendRatio < 2.0f) {
                walkAssoc->m_BlendAmount = 2.0f - playerData->m_fMoveBlendRatio;
                runAssoc->m_BlendAmount  = playerData->m_fMoveBlendRatio - 1.0f;
                walkAssoc->m_BlendDelta  = 0.0f;
                runAssoc->m_BlendDelta   = 0.0f;
                m_nMoveState             = PEDMOVE_RUN;
            } else {
                walkAssoc->m_BlendAmount = 0.0f;
                runAssoc->m_BlendAmount  = 1.0f;
                walkAssoc->m_BlendDelta  = 0.0f;
                runAssoc->m_BlendDelta   = 0.0f;
                m_nMoveState             = PEDMOVE_RUN;
                CStats::UpdateStatsWhenRunning();
            }
        } else if (sprintAssoc) { // Sprinting
            if (sprintAssoc->m_BlendDelta < 0.0f) {
                sprintAssoc->m_BlendDelta = 2.0f;
                runAssoc->m_BlendDelta    = -2.0f;
            }
            CStats::UpdateStatsWhenSprinting();
        } else if (runAssoc->m_BlendAmount >= 1.0f) { // Start the sprint
            sprintAssoc = CAnimManager::BlendAnimation(clump, m_nAnimGroup, ANIM_ID_SPRINT, 2.0f);
            if (sprintAssoc) {
                CStats::UpdateStatsWhenSprinting();
            }
        } else { // Blend into run first
            if (walkAssoc->m_BlendAmount == 0.0f && runAssoc->m_BlendAmount == 0.0f) {
                walkAssoc->m_BlendAmount = 1.0f;
            }
            if (runAssoc->m_BlendDelta <= 0.0f) {
                runAssoc = CAnimManager::BlendAnimation(clump, m_nAnimGroup, ANIM_ID_RUN, 4.0f);
            }
            playerData->m_fMoveBlendRatio = runAssoc->m_BlendDelta + 1.0f;
        }
    }

    if (playerData->m_bAdrenaline) {
        float animSpeed;
        if (CTimer::GetTimeInMS() > playerData->m_nAdrenalineEndTime && !CCheat::IsActive(CHEAT_ADRENALINE_MODE)) {
            playerData->m_bAdrenaline = false;
            CTimer::ms_fTimeScale     = 1.0f;
            animSpeed                 = 1.0f;
        } else {
            CTimer::ms_fTimeScale = 1.0f / 3.0f;
            animSpeed             = 2.0f;
        }
        for (auto* const assoc : { walkStartAssoc, walkAssoc, runAssoc, sprintAssoc }) {
            if (assoc) {
                assoc->m_Speed = animSpeed;
            }
        }
    }

    if (sprintAssoc) {
        sprintAssoc->m_Speed = TheCamera.GetActiveCam().m_nMode == MODE_FIXED
            ? 0.7f
            : std::max(1.0f, GetButtonSprintResults(SPRINT_GROUND));
    }
}

// 0x60B460
void CPlayerPed::MakeChangesForNewWeapon(eWeaponType weaponType) {
    GetActiveWeapon().StopWeaponEffect();
    if (m_nPedState == PEDSTATE_SNIPER_MODE)
        TheCamera.ClearPlayerWeaponMode();

    SetCurrentWeapon(weaponType);

    GetPlayerData()->m_nChosenWeapon = m_nActiveWeaponSlot;
    GetPlayerData()->m_fAttackButtonCounter= 0.0f;

    CWeapon& wep = GetActiveWeapon();
    CWeaponInfo& wepInfo = wep.GetWeaponInfo(this);

    wep.m_AmmoInClip = std::min<uint32>(wep.m_TotalAmmo, (uint32)wepInfo.m_nAmmoClip);

    if (!wepInfo.flags.bCanAim)
        ClearWeaponTarget();

    if (!wepInfo.flags.bOnlyFreeAim)
        GetPlayerData()->m_bFreeAiming = false;


    if (auto anim = RpAnimBlendClumpGetAssociation(GetRpClump(), ANIM_ID_FIRE))
        anim->m_Flags |= ANIMATION_IS_PLAYING & ANIMATION_IS_FINISH_AUTO_REMOVE;

    TheCamera.ClearPlayerWeaponMode();
}

// 0x60B550
bool LOSBlockedBetweenPeds(CEntity* entity1, CEntity* entity2) {
    CVector origin{};
    if (entity1->GetIsTypePed()) {
        origin = entity1->AsPed()->GetBonePosition(eBoneTag::BONE_NECK, false);
        if (entity1->AsPed()->bIsDucking)
            origin.z += 0.35f;
    } else {
        origin = entity1->GetPosition();
    }

    const auto target = entity2->GetIsTypePed()
        ? entity1->AsPed()->GetBonePosition(eBoneTag::BONE_NECK, false)
        : entity1->GetPosition();

    CColPoint colPoint;
    CEntity* hitEntity;
    if (CWorld::ProcessLineOfSight(origin, target, colPoint, hitEntity, true, false, false, true, false, false, false, true))
        return hitEntity != entity2;
    return false;
}

// 0x60B650
void CPlayerPed::Compute3rdPersonMouseTarget(bool meleeWeapon) {
    CPed* target = nullptr;

    if (CCamera::m_bUseMouse3rdPerson) {
        const float   range = CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, GetWeaponSkill())->m_fTargetRange;
        const CVector pos   = GetPosition();

        CVector origin, end;
        if (meleeWeapon) {
            TheCamera.Find3rdPersonCamTargetVector(range, pos, origin, end);
        } else {
            const auto&   cam   = TheCamera.GetActiveCam();
            const CVector front = cam.m_vecFront;
            origin = cam.m_vecSource;
            // Make sure the line doesn't start behind the player
            if (const float dot = (origin - pos).Dot(front); dot < 0.0f) {
                origin -= front * dot;
            }
            end = origin + front * range;
        }

        CWorld::pIgnoreEntity  = this;
        CWorld::bIncludeBikers = true;

        CColPoint colPoint;
        CEntity*  hitEntity = nullptr;
        if (CWorld::ProcessLineOfSight(origin, end, colPoint, hitEntity, false, false, true, false, false, false, false, false)) {
            if (hitEntity != this && hitEntity->AsPed()->IsAlive()) {
                target = hitEntity->AsPed();
            }
        }
        CWorld::ResetLineTestOptions();
    }

    if (!target) {
        // Forget the old target after a while
        if (m_p3rdPersonMouseTarget && (uint32)field_7A0 < CTimer::GetTimeInMS()) {
            m_p3rdPersonMouseTarget = nullptr; // NOTE: The original doesn't clean up the reference here either
        }
        return;
    }

    if (target != m_p3rdPersonMouseTarget) {
        CEntity::ChangeEntityReference(m_p3rdPersonMouseTarget, target);
    }
    field_7A0 = CTimer::GetTimeInMS() + 1000;

    if (CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, eWeaponSkill::STD)->m_nWeaponFire == WEAPON_FIRE_MELEE) {
        return;
    }
    if (!CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, GetWeaponSkill())->flags.bCanAim) {
        return;
    }
    if (!target->GetIntelligence()->IsInSeeingRange(GetPosition())) {
        return;
    }

    // Already reacting to it?
    const auto& targetTaskMgr = target->GetTaskManager();
    CTask*      targetTask    = targetTaskMgr.GetTaskPrimary(TASK_PRIMARY_PHYSICAL_RESPONSE);
    if (!targetTask) {
        targetTask = targetTaskMgr.GetTaskPrimary(TASK_PRIMARY_EVENT_RESPONSE_TEMP);
    }
    if (!targetTask) {
        targetTask = targetTaskMgr.GetTaskPrimary(TASK_PRIMARY_EVENT_RESPONSE_NONTEMP);
    }
    if (targetTask && targetTask->GetTaskType() == TASK_COMPLEX_REACT_TO_GUN_AIMED_AT) {
        return;
    }

    if (GetActiveWeapon().m_Type != WEAPON_PISTOL_SILENCED) {
        Say(CTX_GLOBAL_PULL_GUN);
    }

    if (auto* const targetGroup = CPedGroups::GetPedsGroup(target)) {
        if (!CPedGroups::AreInSameGroup(target, this)) {
            CEventGroupEvent groupEvent{ target, new CEventGunAimedAt{ this } };
            targetGroup->GetIntelligence().AddEvent(&groupEvent);
        }
    } else {
        CEventGunAimedAt event{ this };
        target->GetIntelligence()->m_eventGroup.Add(&event, false);
    }
}

// 0x60BA80
void CPlayerPed::DrawTriangleForMouseRecruitPed() {
    const CPed* const target = m_p3rdPersonMouseTarget;
    if (!target) {
        return;
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,       RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,        RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,           RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,          RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,  RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,      RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION,  RWRSTATE(rwALPHATESTFUNCTIONALWAYS));

    // Colour depends on the health of the target (green => red)
    uint8 r = 0, g = 0, b = 0;
    if (const auto health = std::min(target->m_fHealth / target->m_fMaxHealth, 1.0f); health > 0.0f) {
        const auto invHealth = 1.0f - health;
        r = (uint8)(invHealth * 255.0f + health * 0.0f);
        g = (uint8)(health * 255.0f + invHealth * 0.0f);
        b = (uint8)(invHealth * 0.0f + health * 0.0f);
    }

    // Size depends on the distance
    const auto dist = (target->GetPosition() - GetPosition()).Magnitude();
    const auto size = std::min(std::max(dist - 10.0f, 0.0f) * 0.02f, 1.0f) * 0.825f + 0.175f;

    const CVector right = TheCamera.GetRightVector() * size;
    const CVector up    = CVector{ 0.0f, 0.0f, size };
    const CVector base  = target->GetPosition() + CVector{ 0.0f, 0.0f, 1.0f };

    CVector vertices[3];
    if (target->m_nPedType == PED_TYPE_GANG1) { // Pointing downwards
        vertices[0] = base;
        vertices[1] = base - right + up;
        vertices[2] = base + right + up;
    } else { // Pointing upwards
        vertices[0] = base + up;
        vertices[1] = base + right;
        vertices[2] = base - right;
    }

    // Move the vertices towards the camera a little
    for (auto& vertex : vertices) {
        CVector toCamera = TheCamera.GetPosition() - vertex;
        toCamera.Normalise();
        vertex += toCamera;
    }

    auto* const im3dVertices = TempBufferVertices.m_3d;
    for (auto i = 0; i < 3; i++) {
        RwIm3DVertexSetPos(&im3dVertices[i], vertices[i].x, vertices[i].y, vertices[i].z);
        RwIm3DVertexSetRGBA(&im3dVertices[i], r, g, b, i == 0 ? 255 : 0);
        aTempBufferIndices[i] = (RxVertexIndex)i;
    }

    if (RwIm3DTransform(im3dVertices, 3, nullptr, rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA)) {
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, aTempBufferIndices, 3);
        RwIm3DEnd();
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,      RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,       RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,        RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,           RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,          RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,  RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION,  RWRSTATE(rwALPHATESTFUNCTIONGREATER));
}

// 0x60C0C0
bool CPlayerPed::DoesTargetHaveToBeBroken(CEntity* target, CWeapon* weapon) {
    if (!target->GetIsVisible()) {
        return true;
    }

    if (weapon->GetWeaponRange(this, target) < (target->GetPosition() - GetPosition()).Magnitude()) {
        return true;
    }

    if (weapon->m_Type == eWeaponType::WEAPON_SPRAYCAN) {
        if (target->GetIsTypeBuilding()) {
            if (CTagManager::IsTag(*target)) {
                if (CTagManager::GetAlpha(*target) == 255) { // they probably used -1
                    return true;
                }
            }
        }
    }

    return !CanIKReachThisTarget(target->GetPosition(), weapon, false);
}

// 0x60C1E0
void CPlayerPed::KeepAreaAroundPlayerClear() {
    // Make the nearby random peds walk away (or just remove them if nobody can see that)
    for (auto i = 0; i < 16; i++) {
        auto* const entity = GetIntelligence()->GetPedEntities()[i];
        if (!entity) {
            continue;
        }
        auto* const ped = entity->AsPed();
        if (!ped->IsCreatedBy(PED_GAME) || ped->bInVehicle || !ped->IsAlive()) {
            continue;
        }
        if (CPedGroups::GetGroup(0).GetMembership().IsMember(ped)) { // NOTE: Always the group of the 1st player
            continue;
        }
        if (!ped->GetIsOnScreen() || ped->bIgnoreHeightCheckOnGotoPointTask) {
            ped->FlagToDestroyWhenNextProcessed();
            continue;
        }

        // Already fleeing from us?
        if (const auto* const fleeTask = static_cast<CTaskComplexSmartFleeEntity*>(ped->GetIntelligence()->FindTaskByType(TASK_COMPLEX_SMART_FLEE_ENTITY))) {
            if (fleeTask->m_fleeFrom == this) {
                continue;
            }
        }
        // Or about to?
        if (const auto* const scriptCmdEvent = static_cast<CEventScriptCommand*>(ped->GetIntelligence()->m_eventGroup.GetEventOfType(EVENT_SCRIPT_COMMAND))) {
            if (scriptCmdEvent->m_task && scriptCmdEvent->m_task->GetTaskType() == TASK_COMPLEX_SMART_FLEE_ENTITY) {
                continue;
            }
        }

        auto* const fleeTask = new CTaskComplexSmartFleeEntity{
            this,
            false,
            1000.0f,
            100'000,
            StaticRef<int32>(0x86F678), // CTaskComplexSmartFleeEntity::ms_iEntityPosCheckPeriod (1000)
            StaticRef<float>(0xC18CF0)  // CTaskComplexSmartFleeEntity::ms_fEntityPosChangeThreshold
        };
        fleeTask->m_moveState = PEDMOVE_WALK;

        CEventScriptCommand event{ TASK_PRIMARY_PRIMARY, fleeTask, false };
        ped->GetIntelligence()->m_eventGroup.Add(&event, false);
    }

    // Now the vehicles
    const CVector pos = bInVehicle && m_pVehicle
        ? m_pVehicle->GetPosition()
        : GetPosition();

    int16    numVehicles{};
    CEntity* vehicles[8];
    CWorld::FindObjectsInRange(GetPosition(), 15.0f, true, &numVehicles, 6, vehicles, false, true, false, false, false);
    for (int16 i = 0; i < numVehicles; i++) {
        auto* const veh = vehicles[i]->AsVehicle();
        if (veh->IsMissionVehicle()) {
            continue;
        }
        switch (veh->GetStatus()) {
        case STATUS_PLAYER:
        case STATUS_FORCED_STOP:
            continue;
        }

        auto& autoPilot = veh->m_autoPilot;
        if ((veh->GetPosition() - pos).SquaredMagnitude() > sq(5.0f)) { // Far enough, just wait
            autoPilot.m_nTempAction     = TEMPACT_WAIT;
            autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + 5000;
        } else if (const auto& vehPos = veh->GetPosition(); (pos.y - vehPos.y) * veh->GetForward().y + (pos.x - vehPos.x) * veh->GetForward().x > 0.0f) { // Player is in front, reverse away
            autoPilot.m_nTempAction     = TEMPACT_REVERSE;
            autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + 2000;
        } else { // Player is behind, drive away
            autoPilot.m_nTempAction     = TEMPACT_GOFORWARD;
            autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + 2000;
        }
        CCarCtrl::PossiblyRemoveVehicle(veh);
    }
}

// 0x60C520
void CPlayerPed::SetPlayerMoveBlendRatio(CVector* point) {
    float& moveBlendRatio = GetPlayerData()->m_fMoveBlendRatio;
    if (point) {
        moveBlendRatio = std::min(2.0f, (*point - GetPosition()).Magnitude2D() * 2.0f);
    } else {
        switch (m_nMoveState)
        {
        case PEDMOVE_WALK:
            moveBlendRatio = 1.0f;
            break;

        case PEDMOVE_RUN:
            moveBlendRatio = 1.8f;
            break;

        case PEDMOVE_SPRINT:
            moveBlendRatio = 2.5f;
            break;

        default:
            moveBlendRatio = 0.0f;
            break;
        }
    }
    SetRealMoveAnim();
}

// 0x60C5F0
CPed* CPlayerPed::FindPedToAttack() {
    CVector origin = FindPlayerCoors();
    origin.z = 0.0f;

    CVector end = origin + TheCamera.GetForward() * 100.0f;
    end.z = 0.0f;

    CPed* closestPed{};
    float closestDistance = std::numeric_limits<float>::max();

    CPedGroupMembership& membership = GetPlayerGroup().GetMembership();
    for (int i = 0; GetPedPool()->GetSize(); i++) {
        CPed* ped = GetPedPool()->GetAt(i);
        if (!ped)
            continue;
        if (ped->IsPlayer())
            continue;
        if (!ped->IsAlive())
            continue;
        if (membership.IsMember(ped))
            continue;
        if (ped->m_nPedType == PED_TYPE_GANG2)
            continue;

        CVector point = ped->GetPosition();
        point.z = 0.0f;

        float dist = CCollision::DistToLine(origin, end, point);
        float pointDist = (point - origin).Magnitude2D();
        if (pointDist > 20.0f)
            dist += (pointDist - 20.0f) / 5.0f;

        if (IsPedTypeGang(ped->m_nPedType))
            dist = std::max(0.0f, dist / 2.0f - 2.0f);

        if (dist < closestDistance) {
            closestDistance = dist;
            closestPed = ped;
        }
    }
    return closestPed;
}

// 0x60C7C0
void CPlayerPed::ForceGroupToAlwaysFollow(bool enable) {
    GetPlayerData()->m_bGroupAlwaysFollow = enable;
    if (enable)
        TellGroupToStartFollowingPlayer(true, false, true);
}

// 0x60C800
void CPlayerPed::ForceGroupToNeverFollow(bool enable) {
    GetPlayerData()->m_bGroupNeverFollow = enable;
    if (enable)
        TellGroupToStartFollowingPlayer(false, false, true);
}

// 0x609380 - Inlined on Android
static bool IsAnyRecruitCheatActive() {
    return CCheat::IsAnyActive({ CHEAT_WANNA_BE_IN_MY_GANG, CHEAT_NO_ONE_CAN_STOP_US, CHEAT_ROCKET_MAYHEM });
}

// 0x60C840
void CPlayerPed::MakeThisPedJoinOurGroup(CPed* ped) {
    if (ped->bSignalAfterKill) { // NOTE: Bit 10 of the 4th ped flags dword (0x478), the name of the flag might be off
        Say(CTX_GLOBAL_DRUGGED_IGNORE); // Yes, the player says it
        return;
    }
    if (ped->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_KILL_PED_ON_FOOT)) {
        return;
    }
    if (ped->m_nPedType != PED_TYPE_GANG2 && !IsAnyRecruitCheatActive()) {
        return;
    }

    auto& group      = GetPlayerGroup();
    auto& membership = group.GetMembership();
    if (membership.IsMember(ped)) {
        return;
    }

    CAEPedSpeechAudioEntity::SetCJMood(MOOD_UNK, 10'000, 1, -1, -1);
    Say(CTX_GLOBAL_JOIN_ME_ASK, 0, 1.0f, true);

    int32 maxNumMembers = std::min<int32>(CStats::FindMaxNumberOfGroupMembers(), GetPlayerData()->m_nScriptLimitToGangSize);
    if (CStats::GetStatValue(STAT_CITY_UNLOCKED) == 1.0f || CStats::GetStatValue(STAT_CITY_UNLOCKED) == 2.0f) {
        maxNumMembers = 0;
    }

    const bool canJoin = (IsAnyRecruitCheatActive() && membership.CountMembersExcludingLeader() < TOTAL_PED_GROUP_FOLLOWERS)
        || (membership.CountMembersExcludingLeader() < maxNumMembers && membership.CountMembersExcludingLeader() < maxNumMembers);
    if (!canJoin) {
        ped->Say(CTX_GLOBAL_JOIN_GANG_NO, 2500, 1.0f, true);

        CEventDontJoinPlayerGroup event{ this };
        ped->GetIntelligence()->m_eventGroup.Add(&event, false);
        return;
    }

    if (auto* const oldGroup = CPedGroups::GetPedsGroup(ped)) {
        oldGroup->GetMembership().RemoveMember(ped);
    }

    CEventScriptCommand scriptCmdEvent{
        TASK_PRIMARY_PRIMARY,
        new CTaskComplexBeInGroup{ (int32)FindPlayerPed()->GetPlayerData()->m_nPlayerGroup, false },
        false
    };
    ped->GetIntelligence()->m_eventGroup.Add(&scriptCmdEvent, false);

    membership.AddFollower(ped);
    group.Process();
    ped->GiveWeaponWhenJoiningGang();

    CEventGroupEvent groupEvent{ this, new CEventNewGangMember{ ped } };
    group.GetIntelligence().AddEvent(&groupEvent);

    ped->bDrownsInWater = false;

    CStats::IncrementStat(STAT_GANG_MEMBERS_RECRUITED, 1.0f);
    CStats::DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_GANG_STRENGTH, 1.0f);

    // Colour of the Grove (index 1 in the gang colour tables at 0x8D1344, 0x8D1350, 0x8D135C - see `CGangWars::GetGangColor`)
    const auto blipColor = (eBlipColour)(
          (uint32)StaticRef<uint8>(0x8D1345) << 24
        | (uint32)StaticRef<uint8>(0x8D1351) << 16
        | (uint32)StaticRef<uint8>(0x8D135D) << 8
        | 0xFF
    );
    // NOTE: The original also passes the script name ("CODEPLR") as the 5th argument
    const auto blip = CRadar::SetEntityBlip(BLIP_CHAR, GetPedPool()->GetRef(ped), blipColor, BLIP_DISPLAY_BLIPONLY);
    CRadar::ChangeBlipScale(blip, 2);
    CRadar::ChangeBlipColour(blip, blipColor);
    CRadar::SetBlipFriendly(blip, true);
    ped->bClearRadarBlipOnDeath = true;

    GetPlayerGroup().GetMembership().SetSeparationRange(120.0f);

    ped->Say(CTX_GLOBAL_JOIN_GANG_YES, 2500, 1.0f, true);
}

// 0x60CC50
bool CPlayerPed::PlayerWantsToAttack() {
    auto& group = GetPlayerGroup();
    if (group.GetMembership().CountMembersExcludingLeader() < 1) {
        return false;
    }
    if (!group.m_bMembersEnterLeadersVehicle) {
        return false;
    }
    group.GetIntelligence().ReportAllBarScriptTasksFinished();

    auto* target = m_pTargetedObject;
    if (CCamera::m_bUseMouse3rdPerson && !target) {
        target = m_p3rdPersonMouseTarget;
    }

    CPed* pedToAttack{};
    if (target && target->GetIsTypePed()) {
        pedToAttack = static_cast<CPed*>(target);
    } else {
        if (target) {
            if (CTagManager::IsTag(*target)) {
                return true;
            }
            if (target->GetIsTypeObject() && static_cast<CObject*>(target)->CanBeTargetted()) {
                return true;
            }
        }
        pedToAttack = FindPedToAttack();
    }

    if (!pedToAttack) {
        return false;
    }
    group.PlayerGaveCommand_Attack(this, pedToAttack);
    return true; // NOTE: The original function doesn't have a well-defined return value (It's whatever is left in `eax`)
}

// 0x60CD20
void CPlayerPed::SetInitialState(bool bGroupCreated) {
    CMBlur::ClearDrunkBlur();
    CTimer::ms_fTimeScale = 1.0f;

    m_bUsesCollision            = true;
    physicalFlags.bApplyGravity = true;
    ClearAimFlag();
    ClearLookFlag();
    bRenderPedInCar = true;
    if (m_pFire) {
        m_pFire->Extinguish();
    }
    SetPedState(PEDSTATE_IDLE);
    SetMoveState(PEDMOVE_STILL);
    bIsDucking        = false; // 0x46C, bit 26
    bDontRender       = false; // 0x474, bit 1
    bIsBeingArrested  = false; // 0x474, bit 6
    bCanExitCar       = true;  // 0x474, bit 26
    GetIntelligence()->FlushIntelligence();
    RpAnimBlendClumpRemoveAllAssociations(GetRpClump());
    GetTaskManager().SetTask(new CTaskSimplePlayerOnFoot{}, TASK_PRIMARY_DEFAULT, false);
    m_nAnimGroup         = ANIM_GROUP_PLAYER;
    bIsPedDieAnimPlaying = false; // 0x46C, bit 20
    if (m_pPlayerData) {
        m_pPlayerData->m_bAdrenaline = false;
    }
    SetRealMoveAnim();
    m_pStats->m_nTemper = 50;

    if (m_pAttachedTo && !m_bUsesCollision) {
        m_bUsesCollision = true;
    }
    m_pAttachedTo  = nullptr;
    m_nTurretAmmo  = 0;

    GetTaskManager().SetTaskSecondary(new CTaskComplexFacial{}, TASK_SECONDARY_FACIAL_COMPLEX);

    if (!bGroupCreated && !m_pPlayerData->m_bGroupNeverFollow) {
        auto& group = GetPlayerGroup();
        group.m_bMembersEnterLeadersVehicle = true;
        group.GetIntelligence().SetDefaultTaskAllocatorType(ePedGroupDefaultTaskAllocatorType::RANDOM);

        CEventPlayerCommandToGroupAttack playerCmdEvent{ nullptr };
        playerCmdEvent.ComputeResponseTaskType(&group);
        if (playerCmdEvent.WillRespond()) {
            auto* const gatherCmdEvent = new CEventPlayerCommandToGroupGather{ nullptr };
            gatherCmdEvent->m_TaskId   = playerCmdEvent.m_TaskId;

            CEventGroupEvent groupEvent{ this, gatherCmdEvent };
            group.GetIntelligence().AddEvent(&groupEvent);
        }
    }

    if (m_pPlayerData) {
        m_pPlayerData->SetInitialState();
    }
}

// 0x60D000
void CPlayerPed::MakeChangesForNewWeapon(uint32 weaponSlot) {
    if (weaponSlot != -1)
        MakeChangesForNewWeapon(GetWeaponInSlot(weaponSlot).m_Type);
}

static auto& PLAYER_MAX_TARGET_VIEW_ANGLE = StaticRef<float>(0x8D243C); // 140.0f

// 0x60D020
void CPlayerPed::EvaluateTarget(CEntity* target, CEntity *& outTarget, float & outTargetPriority, float maxDistance, float compensationRotRad, bool arg5) {
    const CVector dir = target->GetPosition() - GetPosition();
    const float dist = dir.Magnitude();

    if (dist > maxDistance)
        return;

    if (DoesTargetHaveToBeBroken(target, &GetActiveWeapon()))
        return;

    const float targetAngleDeg = std::fabs(RadiansToDegrees(CGeneral::LimitRadianAngle(CGeneral::GetATanOf(dir) - compensationRotRad)));

    float viewAngleMultiplier = 1.0f - targetAngleDeg / PLAYER_MAX_TARGET_VIEW_ANGLE;
    if (dist > 1.0f)
        viewAngleMultiplier /= sqrt(sqrt(dist)); // Take quad root of dist

    const float targetPriority = FindTargetPriority(target) * viewAngleMultiplier;
    if (targetPriority > outTargetPriority && !LOSBlockedBetweenPeds(this, target)) {
        outTarget = target;
        outTargetPriority = targetPriority;
    }
}

// 0x60D1C0
void CPlayerPed::EvaluateNeighbouringTarget(CEntity* target, CEntity** outTarget, float* outTargetPriority, float maxDistance, float arg4, bool arg5) {
    const auto dist = (target->GetPosition() - GetPosition()).Magnitude();
    if (dist > maxDistance) {
        return;
    }
    if (DoesTargetHaveToBeBroken(target, &GetActiveWeapon())) {
        return;
    }

    // Angle of the target relative to the current one (`arg4`), as seen from the camera
    const auto& camPos = TheCamera.GetPosition();
    auto angle = CGeneral::GetATanOfXY(target->GetPosition().x - camPos.x, target->GetPosition().y - camPos.y) - arg4;
    while (angle > PI) {
        angle -= TWO_PI;
    }
    while (angle < -PI) {
        angle += TWO_PI;
    }

    if (std::abs(angle) >= DegreesToRadians(50.0f)) {
        return;
    }

    // The closer the target is (angle wise) in the requested direction (`arg5`) the higher the priority.
    const auto priority = arg5
        ? (angle > 0.0f ? -angle : -100'000.0f)
        : (angle < 0.0f ? angle : -100'000.0f);

    if (priority > *outTargetPriority) {
        *outTarget         = target;
        *outTargetPriority = priority;
    }
}

// 0x60D350
void CPlayerPed::ProcessGroupBehaviour(CPad* pad) {
    constexpr uint16 DISBAND_GROUP_PRESS_TIME_MS = 1200;

    auto* const playerData = GetPlayerData();

    CEntity* target = m_pTargetedObject;
    if (CCamera::m_bUseMouse3rdPerson && !target) {
        target = m_p3rdPersonMouseTarget;
    }
    CPed* const targetGangMember = target && target->GetIsTypePed() && target->AsPed()->m_nPedType == PED_TYPE_GANG2
        ? target->AsPed()
        : nullptr;

    const auto GetPressTimeDelta = [] { return (uint16)(CTimer::GetTimeStepNonClipped() * 0.02f * 1000.0f); };

    // Group control forward: Tap => Recruit/Follow, Hold => Disband
    if (!FindPlayerVehicle()) {
        auto& pressTime = playerData->m_nPadUpPressedInMilliseconds;
        if (pad->GetGroupControlForward()) {
            pressTime += GetPressTimeDelta();
            if (pressTime == DISBAND_GROUP_PRESS_TIME_MS && !playerData->m_bGroupStuffDisabled) { // NOTE: Yes, `==` (Unlike below)
                DisbandPlayerGroup();
            }
        } else {
            if (pressTime != 0 && pressTime < DISBAND_GROUP_PRESS_TIME_MS) {
                if (target && target->GetIsTypePed() && (targetGangMember || IsAnyRecruitCheatActive())) {
                    if (!playerData->m_bGroupStuffDisabled) {
                        MakeThisPedJoinOurGroup(target->AsPed());
                    }
                } else {
                    TellGroupToStartFollowingPlayer(true, true, false);
                }
            }
            pressTime = 0;
        }
    }

    if (playerData->m_bGroupStuffDisabled) {
        return;
    }

    // Group control back: Tap => Recruit/Wait, Hold => Disband
    if (!FindPlayerVehicle()) {
        auto& pressTime = playerData->m_nPadDownPressedInMilliseconds;
        if (pad->GetGroupControlBack()) {
            pressTime += GetPressTimeDelta();
            if (pressTime >= DISBAND_GROUP_PRESS_TIME_MS) {
                DisbandPlayerGroup();
            }
        } else {
            if (pressTime != 0 && pressTime < DISBAND_GROUP_PRESS_TIME_MS) {
                if (targetGangMember) {
                    MakeThisPedJoinOurGroup(targetGangMember);
                } else {
                    TellGroupToStartFollowingPlayer(false, true, false);
                }
            }
            pressTime = 0;
        }
    }

    // Make the Grove hate the cops while the player is wanted
    if ((CTimer::GetFrameCounter() & 31) == 6) {
        auto* const pedPool = GetPedPool();
        for (int32 i = pedPool->GetSize(); i-- > 0;) {
            CPed* const ped = pedPool->GetAt(i);
            if (!ped || ped->m_nPedType != PED_TYPE_GANG2) {
                continue;
            }
            if ((int32)FindPlayerPed()->GetWanted()->m_WantedLevel > 0) {
                ped->m_acquaintance.SetAsAcquaintance(ACQUAINTANCE_HATE, CPedType::GetPedFlag(PED_TYPE_COP));
            } else {
                ped->m_acquaintance.ClearAsAcquaintance(ACQUAINTANCE_HATE, CPedType::GetPedFlag(PED_TYPE_COP));
            }
        }
    }
}

// 0x60D5A0
bool CPlayerPed::PlayerHasJustAttackedSomeone() {
    return PlayerWantsToAttack();
}

// 0x60D790
void CPlayerPed::SetupPlayerPed(int32 playerId) {
    auto ped = new CPlayerPed(playerId, false);
    auto& playerInfo = FindPlayerInfo(playerId);
    playerInfo.m_pPed = ped;

    if (playerId == 1)
        ped->m_nPedType = PED_TYPE_PLAYER2;

    ped->SetOrientation(0.0f, 0.0f, 0.0f);
    CWorld::Add(ped);
    ped->m_nWeaponAccuracy = 100;
    playerInfo.m_nPlayerState = ePlayerState::PLAYERSTATE_PLAYING;
}

// 0x60D850
void CPlayerPed::ProcessWeaponSwitch(CPad* pad) {
    auto* const playerData = GetPlayerData();

    const auto ProcessSwitch = [&]() -> bool { // Returns `false` if the weapon mustn't be changed at all
        if (CDarkel::FrenzyOnGoing() || m_pAttachedTo || GetIntelligence()->GetTaskJetPack()) {
            return true;
        }

        const auto weaponCamMode = TheCamera.m_PlayerWeaponMode.m_nMode;

        // NOTE: The original treats `m_nChosenWeapon` as a signed value
        auto chosenWeapon = (int8)playerData->m_nChosenWeapon;
        constexpr auto NUM_SLOTS = (int8)NUM_WEAPON_SLOTS;

        // Whenever the weapon in the given slot can be switched to
        const auto CanSwitchToSlot = [this](int8 slot) {
            auto& weapon = m_aWeapons[slot];
            if (weapon.m_Type == WEAPON_UNARMED || !weapon.HasWeaponAmmoToBeUsed()) {
                return false;
            }
            return !CGameLogic::IsCoopGameGoingOn() || weapon.CanBeUsedFor2Player();
        };

        // Cycle weapons using the pad
        if (!m_pTargetedObject && !playerData->m_bFreeAiming && !playerData->m_bDontAllowWeaponChange && !playerData->m_bInVehicleDontAllowWeaponChange) {
            if (pad->CycleWeaponRightJustDown()) {
                switch (weaponCamMode) {
                case MODE_M16_1STPERSON:
                case MODE_M16_1STPERSON_RUNABOUT:
                case MODE_SNIPER:
                case MODE_SNIPER_RUNABOUT:
                case MODE_ROCKETLAUNCHER:
                case MODE_ROCKETLAUNCHER_RUNABOUT:
                case MODE_ROCKETLAUNCHER_HS:
                case MODE_ROCKETLAUNCHER_RUNABOUT_HS:
                case MODE_CAMERA:
                    break;
                default: {
                    for (chosenWeapon = (int8)(m_nActiveWeaponSlot + 1); chosenWeapon < NUM_SLOTS; chosenWeapon++) {
                        if (CanSwitchToSlot(chosenWeapon)) {
                            break;
                        }
                    }
                    if (chosenWeapon >= NUM_SLOTS) {
                        chosenWeapon = 0;
                    }
                    break;
                }
                }
            } else if (pad->CycleWeaponLeftJustDown()) {
                switch (weaponCamMode) {
                case MODE_M16_1STPERSON:
                case MODE_SNIPER:
                case MODE_ROCKETLAUNCHER:
                case MODE_ROCKETLAUNCHER_HS:
                case MODE_CAMERA:
                    break;
                default: {
                    for (chosenWeapon = (int8)(m_nActiveWeaponSlot - 1);; chosenWeapon--) {
                        if (chosenWeapon < 0) {
                            chosenWeapon = NUM_SLOTS - 1;
                        }
                        if (chosenWeapon == 0 || CanSwitchToSlot(chosenWeapon)) {
                            break;
                        }
                    }
                    break;
                }
                }
            }
        }
        playerData->m_nChosenWeapon = (uint8)chosenWeapon;

        // Switch away from weapons that have run out of ammo
        if (CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, eWeaponSkill::STD)->m_nWeaponFire == WEAPON_FIRE_MELEE) {
            return true;
        }
        if (pad->GetWeapon(this) && GetActiveWeapon().m_Type == WEAPON_MINIGUN) {
            return true;
        }
        if ((int32)GetActiveWeapon().m_TotalAmmo > 0) {
            return true;
        }
        switch (weaponCamMode) {
        case MODE_M16_1STPERSON:
        case MODE_SNIPER:
        case MODE_ROCKETLAUNCHER:
        case MODE_ROCKETLAUNCHER_HS:
            return false;
        }

        if (GetActiveWeapon().m_Type == WEAPON_DETONATOR && m_aWeapons[8].m_Type == WEAPON_REMOTE_SATCHEL_CHARGE) {
            chosenWeapon = 8;
        } else {
            chosenWeapon = (int8)(m_nActiveWeaponSlot - 1);
        }
        for (;; chosenWeapon--) {
            if (chosenWeapon < 0) {
                chosenWeapon = 0;
                break;
            }
            if (chosenWeapon == 5 && m_aWeapons[5].m_Type == (eWeaponType)5) { // Yes, the weapon type is compared against the slot number
                break;
            }
            if ((int32)m_aWeapons[chosenWeapon].m_TotalAmmo > 0 && chosenWeapon != 18 && chosenWeapon != 17 && chosenWeapon != 16) { // Checking slot numbers that don't exist...
                break;
            }
        }
        playerData->m_nChosenWeapon = (uint8)chosenWeapon;

        return true;
    };

    if (!ProcessSwitch()) {
        return;
    }

    if (playerData->m_nChosenWeapon == m_nActiveWeaponSlot) {
        return;
    }

    // Don't change the weapon while firing/reloading
    if (const auto* const useGun = GetIntelligence()->GetTaskUseGun()) {
        switch (useGun->m_LastCmd) {
        case eGunCommand::FIRE:
        case eGunCommand::FIREBURST:
            return;
        case eGunCommand::RELOAD: {
            if (useGun->m_Anim) {
                return;
            }
            break;
        }
        }
    }

    RemoveWeaponAnims((int8)m_nActiveWeaponSlot, -1000.0f);
    if ((int8)playerData->m_nChosenWeapon != -1) {
        MakeChangesForNewWeapon(m_aWeapons[(int8)playerData->m_nChosenWeapon].m_Type);
    }
}

// 0x60DC50
bool CPlayerPed::FindWeaponLockOnTarget() {
    static auto& PLAYER_MAX_TARGET_VIEW_ANGLE_BEHIND = StaticRef<float>(0x8D2438); // 90.0f - NOTSA name
    static auto& PLAYER_TARGET_VIEW_BEHIND_DIST      = StaticRef<float>(0x8D2440); // 3.0f  - NOTSA name

    const auto* const weaponInfo = CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, GetWeaponSkill());

    // Already have a target, just check if it's still in range
    if (m_pTargetedObject) {
        const auto dist2D = (m_pTargetedObject->GetPosition() - GetPosition()).Magnitude2D();
        if (CWeapon::TargetWeaponRangeMultiplier(m_pTargetedObject, this) * weaponInfo->m_fTargetRange >= dist2D) {
            return true;
        }
        CEntity::ClearReference(m_pTargetedObject);
        return false; // NOTE: Android tries to find a new target right away
    }

    CEntity* bestTarget         = nullptr;
    float    bestTargetPriority = -10'000.0f;

    // Direction to look for targets in: Either where the player is facing, or where the stick is pushed towards
    float heading = CGeneral::GetATanOfXY(GetForward().x, GetForward().y);
    {
        const auto* const pad = GetPadFromPlayer();
        if (std::fabs((float)pad->GetPedWalkLeftRight()) > 60.0f || std::fabs((float)pad->GetPedWalkUpDown()) > 60.0f) {
            const auto upDown    = (float)pad->GetPedWalkUpDown();
            const auto leftRight = (float)(-pad->GetPedWalkLeftRight());
            heading = CGeneral::LimitRadianAngle(
                CGeneral::GetRadianAngleBetweenPoints(0.0f, 0.0f, leftRight, upDown) - TheCamera.m_fOrientation + HALF_PI
            );
        }
    }

    // With the spray can look for a tag that isn't finished yet
    if (GetActiveWeapon().m_Type == WEAPON_SPRAYCAN) {
        const float   pedHeading = GetHeading();
        const CVector center     = GetPosition() + GetForward() * 8.0f;

        int16    numEntities{};
        CEntity* entities[16];
        CWorld::FindObjectsInRange(center, 8.0f, false, &numEntities, 15, entities, true, false, false, false, false);

        CEntity* bestTag      = nullptr;
        float    bestTagAngle = PI;
        for (int16 i = 0; i < numEntities; i++) {
            CEntity* const entity = entities[i];
            if (!CTagManager::IsTag(*entity) || CTagManager::GetAlpha(*entity) >= 255) {
                continue;
            }
            const CVector dir = entity->GetPosition() - GetPosition();
            if (dir.SquaredMagnitude() >= sq(weaponInfo->m_fTargetRange)) {
                continue;
            }
            float angle = std::atan2(-dir.x, dir.y) - pedHeading;
            if (angle < -PI) {
                angle += TWO_PI;
            } else if (angle > PI) {
                angle -= TWO_PI;
            }
            if (angle < bestTagAngle || !bestTag) { // NOTE: Yes, the signed angle is used
                bestTagAngle = angle;
                bestTag      = entity;
            }
        }

        if (bestTag && !LOSBlockedBetweenPeds(this, bestTag)) {
            CEntity::ChangeEntityReference(m_pTargetedObject, bestTag);
            GetPlayerData()->m_bDontAllowWeaponChange = true;
            return true;
        }
    }

    // Returns the absolute value of the angle between the look direction and the direction `from` => `to`
    const auto GetAbsAngleFromHeading = [heading](const CVector& from, const CVector& to) {
        float angle = CGeneral::GetATanOfXY(to.x - from.x, to.y - from.y) - heading;
        while (angle > PI) {
            angle -= TWO_PI;
        }
        while (angle < -PI) {
            angle += TWO_PI;
        }
        return std::fabs(angle);
    };

    auto* const pedPool = GetPedPool();
    for (int32 i = pedPool->GetSize(); i-- > 0;) {
        CPed* const ped = pedPool->GetAt(i);
        if (!ped || ped == this) {
            continue;
        }
        if (ped->m_nPedState == PEDSTATE_DIE || ped->m_nPedState == PEDSTATE_DEAD) {
            continue;
        }
        if (!PedCanBeTargettedVehicleWise(ped)) {
            continue;
        }
        if (ped->bNeverEverTargetThisPed) {
            continue;
        }
        if (ped->IsPlayer() && CGameLogic::bPlayersCannotTargetEachOther) {
            continue;
        }
        if (CPedGroups::AreInSameGroup(ped, this)) {
            continue;
        }

        // Has to be within the view cone...
        if (GetAbsAngleFromHeading(GetPosition(), ped->GetPosition()) >= DegreesToRadians(PLAYER_MAX_TARGET_VIEW_ANGLE) / 2.0f) {
            continue;
        }

        // ...and also within the (narrower) view cone originating from a bit behind the player
        CVector forward = GetForward();
        forward.Normalise();
        const CVector behindPos = GetPosition() - forward * PLAYER_TARGET_VIEW_BEHIND_DIST;
        if (GetAbsAngleFromHeading(behindPos, ped->GetPosition()) >= DegreesToRadians(PLAYER_MAX_TARGET_VIEW_ANGLE_BEHIND) / 2.0f) {
            continue;
        }

        const float dist     = (ped->GetPosition() - GetPosition()).Magnitude();
        const float maxRange = CWeapon::TargetWeaponRangeMultiplier(ped, this) * weaponInfo->m_fTargetRange;
        if (dist >= maxRange) {
            continue;
        }

        EvaluateTarget(ped, bestTarget, bestTargetPriority, maxRange, heading, false);
    }

    auto* const objPool = GetObjectPool();
    for (int32 i = objPool->GetSize(); i-- > 0;) {
        CObject* const obj = objPool->GetAt(i);
        if (!obj || !obj->CanBeTargetted() || obj->objectFlags.bIsExploded || !obj->m_pRwObject) {
            continue;
        }
        if (!CanIKReachThisTarget(obj->GetPosition(), &GetActiveWeapon(), true)) {
            continue;
        }
        EvaluateTarget(obj, bestTarget, bestTargetPriority, weaponInfo->m_fTargetRange, heading, true);
    }

    if (CGameLogic::IsCoopGameGoingOn()) {
        auto* const vehPool = GetVehiclePool();
        for (int32 i = vehPool->GetSize(); i-- > 0;) {
            CVehicle* const veh = vehPool->GetAt(i);
            if (!veh || veh->physicalFlags.bRenderScorched || veh->IsSubBMX()) {
                continue;
            }
            if (!CanIKReachThisTarget(veh->GetPosition(), &GetActiveWeapon(), true)) {
                continue;
            }
            EvaluateTarget(veh, bestTarget, bestTargetPriority, weaponInfo->m_fTargetRange, heading, true);
        }
    }

    if (!bestTarget) {
        return false;
    }

    CEntity::ChangeEntityReference(m_pTargetedObject, bestTarget);
    GetPlayerData()->m_bDontAllowWeaponChange = true;
    return true;
}

// 0x60E530
bool CPlayerPed::FindNextWeaponLockOnTarget(CEntity* arg0, bool arg1) {
    const float targetRange = CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, GetWeaponSkill())->m_fTargetRange;

    CEntity* bestTarget         = nullptr;
    float    bestTargetPriority = -10'000.0f;

    // Heading (as seen from the camera) of the current target
    const float currTargetHeading = arg0
        ? CGeneral::GetATanOfXY(arg0->GetPosition().x - TheCamera.GetPosition().x, arg0->GetPosition().y - TheCamera.GetPosition().y)
        : CGeneral::GetATanOfXY(TheCamera.m_mCameraMatrix.GetForward().x, TheCamera.m_mCameraMatrix.GetForward().y);

    // NOTE: The original code uses the (stale) ped variable of the loop below for all the `TargetWeaponRangeMultiplier` calls,
    //       including the ones in the object and vehicle loops. (Android uses the entity being evaluated there)
    CEntity* rangeMultiplierTarget = arg0;

    auto* const pedPool = GetPedPool();
    for (int32 i = pedPool->GetSize(); i-- > 0;) {
        CPed* const ped = pedPool->GetAt(i);
        rangeMultiplierTarget = ped;
        if (!ped || ped == this || ped == arg0) {
            continue;
        }
        if (ped->m_nPedState == PEDSTATE_DIE || ped->m_nPedState == PEDSTATE_DEAD) {
            continue;
        }
        if (!PedCanBeTargettedVehicleWise(ped)) {
            continue;
        }
        if (ped->bNeverEverTargetThisPed) {
            continue;
        }
        if (CPedGroups::AreInSameGroup(ped, this)) {
            continue;
        }
        if (ped->IsPlayer() && CGameLogic::bPlayersCannotTargetEachOther) {
            continue;
        }
        if (LOSBlockedBetweenPeds(this, ped)) {
            continue;
        }
        if (!CanIKReachThisTarget(ped->GetPosition(), &GetActiveWeapon(), true)) {
            continue;
        }
        EvaluateNeighbouringTarget(
            ped,
            &bestTarget,
            &bestTargetPriority,
            CWeapon::TargetWeaponRangeMultiplier(rangeMultiplierTarget, this) * targetRange,
            currTargetHeading,
            arg1
        );
    }

    auto* const objPool = GetObjectPool();
    for (int32 i = objPool->GetSize(); i-- > 0;) {
        CObject* const obj = objPool->GetAt(i);
        if (!obj || !obj->CanBeTargetted() || obj->objectFlags.bIsExploded || !obj->m_pRwObject) {
            continue;
        }
        if (!CanIKReachThisTarget(obj->GetPosition(), &GetActiveWeapon(), true)) {
            continue;
        }
        EvaluateNeighbouringTarget(
            obj,
            &bestTarget,
            &bestTargetPriority,
            CWeapon::TargetWeaponRangeMultiplier(rangeMultiplierTarget, this) * targetRange,
            currTargetHeading,
            arg1
        );
    }

    if (CGameLogic::IsCoopGameGoingOn()) {
        auto* const vehPool = GetVehiclePool();
        for (int32 i = vehPool->GetSize(); i-- > 0;) {
            CVehicle* const veh = vehPool->GetAt(i);
            if (!veh || veh->physicalFlags.bRenderScorched || veh->IsSubBMX()) {
                continue;
            }
            if (!CanIKReachThisTarget(veh->GetPosition(), &GetActiveWeapon(), true)) {
                continue;
            }
            EvaluateNeighbouringTarget(
                veh,
                &bestTarget,
                &bestTargetPriority,
                CWeapon::TargetWeaponRangeMultiplier(rangeMultiplierTarget, this) * targetRange,
                currTargetHeading,
                arg1
            );
        }
    }

    if (!bestTarget) {
        return false;
    }

    // Let the new target know that a gun is being aimed at them
    if (bestTarget->GetIsTypePed() && CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, eWeaponSkill::STD)->m_nWeaponFire != WEAPON_FIRE_MELEE) {
        auto* const targetPed = bestTarget->AsPed();
        if (auto* const targetGroup = CPedGroups::GetPedsGroup(targetPed)) {
            if (!CPedGroups::AreInSameGroup(targetPed, this)) {
                CEventGroupEvent groupEvent{ targetPed, new CEventGunAimedAt{ this } };
                targetGroup->GetIntelligence().AddEvent(&groupEvent);
            }
        } else {
            CEventGunAimedAt event{ this };
            targetPed->GetIntelligence()->m_eventGroup.Add(&event, false);
        }
    }

    CEntity::ChangeEntityReference(m_pTargetedObject, bestTarget);
    GetPlayerData()->m_bDontAllowWeaponChange = true;
    return true;
}

// 0x60EA90
void CPlayerPed::ProcessControl() {
    if (GetPlayerData()->m_nCarDangerCounter)
        GetPlayerData()->m_nCarDangerCounter--;
    if (!GetPlayerData()->m_nCarDangerCounter)
        GetPlayerData()->m_pDangerCar = 0;
    if (GetPlayerData()->m_nFadeDrunkenness) {
        if (GetPlayerData()->m_nDrunkenness - 1 > 0) {
            --GetPlayerData()->m_nDrunkenness;
        } else {
            GetPlayerData()->m_nDrunkenness = 0;
            CMBlur::ClearDrunkBlur();
            GetPlayerData()->m_nFadeDrunkenness = 0;
        }
    }
    if (GetPlayerData()->m_nDrunkenness) 
        CMBlur::SetDrunkBlur(GetPlayerData()->m_nDrunkenness / 255.0f);
    if (GetPlayerData()->m_bRequireHandleBreath) {
        if (CStats::GetFatAndMuscleModifier(STAT_MOD_AIR_IN_LUNG) > GetPlayerData()->m_fBreath)
            GetPlayerData()->m_fBreath += CTimer::GetTimeStep() + CTimer::GetTimeStep();
        GetPlayerData()->m_bRequireHandleBreath = false;
    }
    GetPlayerData()->m_bRequireHandleBreath = true;
    CPed::ProcessControl();
    bCheckColAboveHead = true;
    float markColor = 1.0f;
    bool limitMarkColor = true;
    CVector effectPos;
    CPad* pad = CPad::GetPad(m_nPedType);
    if (!bCanPointGunAtTarget) {
        GetPlayerWanted()->Update();
        PruneReferences();
        if (GetActiveWeapon().m_Type == WEAPON_MINIGUN) {
            auto weaponInfo = CWeaponInfo::GetWeaponInfo(WEAPON_MINIGUN, eWeaponSkill::STD);
            if (GetIntelligence()->GetTaskUseGun()) {
                auto animAssoc = GetIntelligence()->GetTaskUseGun()->m_Anim;
                if (animAssoc && animAssoc->m_CurrentTime - animAssoc->m_TimeStep < weaponInfo->m_fAnimLoopEnd) {
                    if (GetPlayerData()->m_fGunSpinSpeed < 0.45f) {
                        GetPlayerData()->m_fGunSpinSpeed += CTimer::GetTimeStep() * 0.025f;
                        GetPlayerData()->m_fGunSpinSpeed = std::min(GetPlayerData()->m_fGunSpinSpeed, 0.45f);
                    }
                    if (pad->GetWeapon(this) && GetActiveWeapon().m_TotalAmmo > 0 && animAssoc->m_CurrentTime >= weaponInfo->m_fAnimLoopStart) 
                        m_weaponAudio.AddAudioEvent(AE_WEAPON_FIRE_MINIGUN_AMMO);
                    else 
                        m_weaponAudio.AddAudioEvent(AE_WEAPON_FIRE_MINIGUN_NO_AMMO);
                }
            } else {
                if (GetPlayerData()->m_fGunSpinSpeed > 0.0f) {
                    GetPlayerData()->m_fGunSpinSpeed -= CTimer::GetTimeStep() * 0.003f;
                    GetPlayerData()->m_fGunSpinSpeed = std::max(GetPlayerData()->m_fGunSpinSpeed, 0.0f);
                }
            }
        }
        if (GetActiveWeapon().m_Type == WEAPON_CHAINSAW && m_nPedState != PEDSTATE_ATTACK && !bInVehicle) {
            GetIntelligence()->GetTaskSwim(); // hmmm?
        }
        if (m_pTargetedObject) {
            ClearReference(m_p3rdPersonMouseTarget);
            if (m_pTargetedObject->GetIsTypePed()) {
                CPed* targetPed = m_pTargetedObject->AsPed();
                auto weaponInfo = CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, GetWeaponSkill());
                float targetHeadRange = weaponInfo->GetTargetHeadRange();
                markColor = targetPed->m_fHealth / targetPed->m_fMaxHealth;
                bool instantFireHit = false;
                if (targetPed->IsAlive()) {
                    auto stdWeaponInfo = CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, eWeaponSkill::STD);
                    if (stdWeaponInfo->m_nWeaponFire == WEAPON_FIRE_INSTANT_HIT) {
                        instantFireHit = true;
                        CVector distance = targetPed->GetPosition() - GetPosition();
                        if (targetHeadRange * targetHeadRange > distance.SquaredMagnitude()) {
                            GetPlayerData()->m_nTargetBone = BONE_HEAD;
                            GetPlayerData()->m_vecTargetBoneOffset.x = 0.05f;
                        }
                    }
                }
                if (!instantFireHit) {
                    GetPlayerData()->m_nTargetBone = BONE_SPINE1;
                    GetPlayerData()->m_vecTargetBoneOffset.x = 0.2f;
                }
                effectPos = GetPlayerData()->m_vecTargetBoneOffset;
                targetPed->GetTransformedBonePosition(effectPos, static_cast<eBoneTag>(GetPlayerData()->m_nTargetBone), false);
                bool targetIsInVehicle = false;
                if (markColor > 0.0f) {
                    if (!targetPed->bInVehicle && targetPed->m_nMoveState != PEDMOVE_STILL) {
                        effectPos += targetPed->m_vecMoveSpeed * CTimer::GetTimeStep();
                    }
                }
                if (targetPed->bInVehicle) {
                    auto targetVeh = targetPed->m_pVehicle;
                    if (targetVeh)
                        effectPos += (targetVeh->m_vecMoveSpeed + targetVeh->m_vecTurnSpeed) * CTimer::GetTimeStep();
                }
            } else if (m_pTargetedObject->GetIsTypeVehicle()) {
                CVehicle* targetVeh = m_pTargetedObject->AsVehicle();
                effectPos = (targetVeh->m_vecMoveSpeed + targetVeh->m_vecTurnSpeed) * CTimer::GetTimeStep();
                effectPos += targetVeh->GetPosition();
            } else if (m_pTargetedObject->GetIsTypeObject()) {
                CObject* targetObj = m_pTargetedObject->AsObject();
                effectPos = targetObj->m_vecMoveSpeed * CTimer::GetTimeStep();
                effectPos += targetObj->GetPosition();
                markColor = targetObj->m_fHealth * 0.001f;
            } else {
                effectPos = m_pTargetedObject->GetPosition();
                limitMarkColor = false; 
            }
        }
    }
    if (m_pTargetedObject) {
        uint8 r = 0, g = 0, b = 0;
        bool setRGB = true;
        if (limitMarkColor) {
            if (markColor > 0.0f)
                markColor = std::min(markColor, 1.0f);
            else
                setRGB = false;
        }
        if (setRGB) {
            r = static_cast<uint8>((1.0f - markColor) * 255.0f);
            g = static_cast<uint8>(markColor * 255.0f);
            b = static_cast<uint8>(0.0f);
        }
        CVector distance = effectPos - GetPosition();
        float size = 1.0f - distance.Magnitude() * 0.02f;
        CWeaponEffects::MarkTarget(m_nPedType, effectPos, r, g, b, 255u, size, false);
    }
    if (m_nMoveState != PEDMOVE_NONE) {
        if (m_nMoveState != PEDMOVE_RUN) {
            if (m_nMoveState != PEDMOVE_SPRINT)
                HandleSprintEnergy(false, 1.0f);
        } else if (CStats::GetFatAndMuscleModifier(STAT_MOD_TIME_CAN_RUN) > GetPlayerData()->m_fTimeCanRun)
            GetPlayerData()->m_fTimeCanRun += CTimer::GetTimeStep() * 0.15f;
    } else if (bInVehicle) {
        if (m_pVehicle && !m_pVehicle->IsSubBMX())
            HandleSprintEnergy(false, 1.0f);
    }
    GetActiveWeapon().Update(this);
    if (m_nPedState == PEDSTATE_DEAD || m_nPedState == PEDSTATE_DIE) {
        ClearWeaponTarget();
        return;
    }
    if (pad) {
        if (pad->WeaponJustDown(this)) {
            auto& activeWeapon = GetActiveWeapon();
            auto weaponType = activeWeapon.m_Type;
            if (!TheCamera.Using1stPersonWeaponMode() || activeWeapon.m_State == WEAPONSTATE_OUT_OF_AMMO) {
                if (!GetIntelligence()->GetTaskSwim()) {
                    if (weaponType == WEAPON_SNIPERRIFLE) {
                        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_FIRE_FAIL_SNIPERRIFFLE, 0.0f, 1.0f);
                    } else if (weaponType == WEAPON_RLAUNCHER || weaponType == WEAPON_RLAUNCHER_HS) {
                        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_FIRE_FAIL_ROCKET, 0.0f, 1.0f);
                    }
                }
            }
        }
        if (IsPedShootable() && this->m_nPedState != PEDSTATE_ANSWER_MOBILE) {
            int32 slot = CWorld::FindPlayerSlotWithPedPointer(this);
            if (!CWorld::Players[slot].m_pRemoteVehicle)
                ProcessWeaponSwitch(pad);
        }
    }
    ProcessAnimGroups();
    if (pad && TheCamera.GetActiveCamera().m_nMode == MODE_FOLLOWPED && !TheCamera.GetActiveCamera().m_nDirectionWasLooking) {
        auto& activeCam = TheCamera.GetActiveCamera();
        m_nLookTime = 0;
        float lookDir = CGeneral::LimitRadianAngle(atan2(-activeCam.m_vecFront.x, activeCam.m_vecFront.y));
        float angle = fabs(lookDir - m_fCurrentRotation);
        if (m_nPedState != PEDSTATE_ATTACK && angle > DegreesToRadians(30.0f) && angle < DegreesToRadians(330.0f)) {
            if (angle > DegreesToRadians(150.0f) && angle < DegreesToRadians(210.0f)) {
                float dir1 = CGeneral::LimitRadianAngle(m_fCurrentRotation - DegreesToRadians(150.0f));
                float dir2 = CGeneral::LimitRadianAngle(m_fCurrentRotation + DegreesToRadians(150.0f));
                lookDir = dir1;
                if (m_fLookDirection != 999'999.f && !bIsDucking) {
                    if (fabs(dir2 - m_fLookDirection) <= fabs(dir1 - m_fLookDirection))
                        lookDir = dir2;
                }
            }
            SetLookFlag(lookDir, true, false);
            SetLookTimer(static_cast<uint32>((CTimer::GetTimeStep() * 0.02f * 1000.0f) * 5.0f));
        } else {
            ClearLookFlag();
        }
    }
    if (m_nMoveState == PEDMOVE_SPRINT && bIsLooking) {
        ClearLookFlag();
        SetLookTimer(250);
    }
    if (m_vecMoveSpeed.Magnitude() >= 0.1f) {
        GetPlayerData()->m_nStandStillTimer = 0;
        GetPlayerData()->m_bStoppedMoving = false;
    } else if (!GetPlayerData()->m_nStandStillTimer) {
        GetPlayerData()->m_nStandStillTimer = CTimer::GetTimeInMS() + 500;
    } else if (CTimer::GetTimeInMS() > GetPlayerData()->m_nStandStillTimer) {
        GetPlayerData()->m_bStoppedMoving = true;
    }
    if (GetPlayerData()->m_bDontAllowWeaponChange) {
        if (IsPlayer()) {
            if (!CPad::GetPad(0)->GetTarget())
                GetPlayerData()->m_bDontAllowWeaponChange = false;
        }
    }
    if (m_nPedState != PEDSTATE_SNIPER_MODE && GetActiveWeapon().m_State == WEAPONSTATE_FIRING)
        GetPlayerData()->m_nLastTimeFiring = CTimer::GetTimeInMS();
    ProcessGroupBehaviour(pad);
    if (bInVehicle)
        CCarCtrl::RegisterVehicleOfInterest(m_pVehicle);
    if (!GetIsVisible())
        UpdateRpHAnim();
    if (bInVehicle) {
        CPad* pad = CPad::GetPad(0);
        if (!pad->IsDPadDownPressed()) {
            if (pad->IsDPadUpPressed())
                GetPlayerData()->m_bPlayersGangActive = true;
        } else {
            GetPlayerData()->m_bPlayersGangActive = false;
        }
    }
    if (physicalFlags.bSubmergedInWater) {
        CVector pos = GetPosition();
        pos.z += 1.5f;
        if (CWaterLevel::GetWaterLevel(pos.x, pos.y, pos.z, GetPlayerData()->m_fWaterHeight, true, nullptr)) {
            auto& box = CEntity::GetColModel()->GetBoundingBox();
            float playerMinZ = pos.z + box.m_vecMin.z;
            float playerMaxZ = pos.z + box.m_vecMax.z;
            if (GetPlayerData()->m_fWaterHeight < playerMaxZ) {
                if (GetPlayerData()->m_fWaterHeight > playerMinZ)
                    GetPlayerData()->m_nWaterCoverPerc = static_cast<uint8>((GetPlayerData()->m_fWaterHeight - playerMinZ) / (playerMaxZ - playerMinZ) * 100.0f);
                else
                    GetPlayerData()->m_nWaterCoverPerc = 0;
            } else {
                GetPlayerData()->m_nWaterCoverPerc = 100;
            }
        } else {
            physicalFlags.bSubmergedInWater = false;
        }
    } else {
        GetPlayerData()->m_nWaterCoverPerc = 0;
    }
    if ((CTimer::GetFrameCounter() & 0x7F) == 0 && !FindPlayerVehicle()) {
        auto& group = CPedGroups::GetGroup(GetPlayerData()->m_nPlayerGroup);
        if (group.m_bMembersEnterLeadersVehicle) {
            int32 memberCount = group.m_groupMembership.CountMembersExcludingLeader();
            if (memberCount > 0) {
                float distance = group.FindDistanceToNearestMember(nullptr);
                if (distance > 20.0f && distance < 100.0f && CGame::currArea == AREA_CODE_NORMAL_WORLD) {
                    if (memberCount == 1)
                        Say(CTX_GLOBAL_ORDER_KEEP_UP_ONE);
                    else
                        Say(CTX_GLOBAL_ORDER_KEEP_UP_MANY);
                    for (int32 i = 0; i < TOTAL_PED_GROUP_FOLLOWERS; ++i) {
                        CPed* member = group.m_groupMembership.GetMember(i);
                        if (member && CGeneral::GetRandomNumberInRange(0.0f, 1.0f) < 0.5f) {
                            int32 offset = CGeneral::GetRandomNumberInRange(3000, 4500);
                            member->Say(CTX_GLOBAL_FOLLOW_REPLY, offset);
                        }
                    }
                }
            }
        }
    }
    if (!bInVehicle && GetLightingTotal() <= 0.05f && !CEntryExitManager::WeAreInInteriorTransition())
        Say(CTX_GLOBAL_BREATHING);
}

// 0x609490
void CPlayerPed::SetMoveAnim() {
    //nop
}
