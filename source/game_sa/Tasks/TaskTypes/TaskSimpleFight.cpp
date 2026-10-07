#include "StdInc.h"

#include "TaskSimpleFight.h"
#include "PedStats.h"
#include "Glass.h"
#include "TaskSimpleFall.h"
#include "Localisation.h"

constexpr int32 AE_DOUBLE_HIT_DELAYS_MS[]{ 300, 400, 500 }; // 0x8D2E3C

static auto& CHAIN_COUNT_LIMIT      = StaticRef<int32, 0x8D2E48>(); // = 2 (Name from Android)
static auto& PLAYER_AUTO_FACE_RANGE = StaticRef<float, 0x8D2E8C>(); // = 2.f (NOTSA name) - See `CTaskSimpleFight::ChooseAttackPlayer`

//! Flags of the moves the ped may use from the combo set (Low byte of `CMeleeInfo::m_wFlags`, see `eMeleeComboFlags`)
static uint8 GetAvailableMoveFlags(int8 comboSet, CPed* ped) {
    // NOTE: The original doesn't clamp the index (`comboSet - 4`) here.
    // So for sets below `MELEE_COMBO_UNARMED_1` it reads whatever is in the memory before `m_aComboData` (Android uses `0` in that case).
    // That is replicated here, as `ChooseAttackAI` does get called with such sets (and uses the value).
    const auto* const info = CTaskSimpleFight::m_aComboData.data() + ((int32)comboSet - (int32)MELEE_COMBO_UNARMED_1);
    auto flags = (uint8)info->m_wFlags;
    if (comboSet > MELEE_COMBO_UNARMED_1 && comboSet <= MELEE_COMBO_UNARMED_4) {
        flags &= (uint8)ped->m_nAllowedAttackMoves;
    }
    return flags;
}

void CTaskSimpleFight::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleFight, 0x86D684, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x61C470);
    RH_ScopedInstall(Destructor, 0x61C530);

    RH_ScopedInstall(LoadMeleeData, 0x5BEDC0);
    RH_ScopedInstall(GetHitLevel, 0x5BD360);
    RH_ScopedInstall(GetHitSound, 0x5BD3B0);
    RH_ScopedInstall(GetComboType, 0x61DB30);
    RH_ScopedInstall(FinishMeleeAnimCB, 0x61DAE0);
    RH_ScopedInstall(GetRange, 0x61C1C0);
    RH_ScopedInstall(GetComboAnimGroupID, 0x4ABDA0);
    RH_ScopedInstall(IsComboSet, 0x4ABDC0);
    RH_ScopedInstall(IsHitComboSet, 0x4ABDF0);
    RH_ScopedInstall(ControlFight, 0x61C5E0);
    RH_ScopedInstall(BeHitWhileBlocking, 0x61C650);
    RH_ScopedInstall(GetStrikeDamage, 0x61C740);
    RH_ScopedInstall(GetAvailableComboSet, 0x61C7F0);
    RH_ScopedInstall(SetPlayerMoveAnim, 0x61C9B0);
    RH_ScopedInstall(FightHitPed, 0x61CBA0);
    RH_ScopedInstall(FightHitCar, 0x61D0B0);
    RH_ScopedInstall(FightHitObj, 0x61D400);
    RH_ScopedInstall(FightSetUpCol, 0x61D5F0);
    RH_ScopedInstall(StartAnim, 0x623B10);
    RH_ScopedInstall(FightStrike, 0x6240B0);
    RH_ScopedInstall(ChooseAttackPlayer, 0x624710);
    RH_ScopedInstall(ChooseAttackAI, 0x624A40);
    RH_ScopedInstall(FindTargetOnGround, 0x61D6F0);

    RH_ScopedVMTInstall(Clone, 0x622E40);
    RH_ScopedVMTInstall(GetTaskType, 0x61C520);
    RH_ScopedVMTInstall(MakeAbortable, 0x6239F0);
    RH_ScopedVMTInstall(ProcessPed, 0x629920);
}

// 0x61C470
CTaskSimpleFight::CTaskSimpleFight(CEntity* entity, int32 nCommand, uint32 nIdlePeriod) :
    CTaskSimple(),
    m_bIsFinished{ false },
    m_bIsInControl{ true },
    m_bAnimsReferenced{ false },
    m_nRequiredAnimGroup{ ANIM_GROUP_MELEE_1 },
    m_nIdlePeriod{ (uint16)std::min(nIdlePeriod, 60'000u) },
    m_nIdleCounter{ 0 },
    m_nContinueStrike{ 0 },
    m_nChainCounter{ 0 },
    m_pTargetEntity{ entity },
    m_pAnim{ nullptr },
    m_pIdleAnim{ nullptr },
    m_nComboSet{ -1 },
    m_nCurrentMove{ (eFightAttackType)-1 },
    m_nNextCommand{ (uint8)nCommand },
    m_nLastCommand{ MELEE_CMD_IDLE }
{
    CEntity::SafeRegisterRef(m_pTargetEntity);
}

// 0x61C530
CTaskSimpleFight::~CTaskSimpleFight() {
    CEntity::SafeCleanUpRef(m_pTargetEntity);

    if (m_pAnim) {
        m_pAnim->SetDefaultDeleteCallback();
    }
    if (m_pIdleAnim) {
        m_pIdleAnim->SetDefaultDeleteCallback();
    }
    if (m_bAnimsReferenced && m_nRequiredAnimGroup != ANIM_GROUP_MELEE_1) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(m_nRequiredAnimGroup));
        m_bAnimsReferenced = false;
    }
}

// 0x61C5E0
bool CTaskSimpleFight::ControlFight(CEntity* entity, uint8 command) {
    m_bIsInControl = true;

    if (entity != m_pTargetEntity) {
        CEntity::SafeCleanUpRef(m_pTargetEntity);
        m_pTargetEntity = entity;
        CEntity::SafeRegisterRef(m_pTargetEntity);
    }

    // Commands are signed bytes originally (See `eMeleeCommand`)
    if ((int8)command > (int8)m_nNextCommand) {
        m_nNextCommand = command;
    }

    return true;
}

// 0x61C650
bool CTaskSimpleFight::BeHitWhileBlocking(CPed* ped, CPed* attacker, int8 attackerComboSet, int8 attackerMove) {
    if (m_nLastCommand != MELEE_CMD_BLOCK) {
        return false;
    }

    // Must be in the middle of the block anim (and it must be paused)
    if (!m_pAnim || m_pAnim->IsPlaying() || m_pAnim->GetHier()->m_fTotalTime <= m_pAnim->m_CurrentTime) {
        return false;
    }

    // Attacks from behind can't be blocked
    const bool isAttackerBehind = DotProduct(attacker->GetPosition() - ped->GetPosition(), ped->GetMatrix().GetForward()) < 0.3f;

    const auto IsHeavyWeaponCombo = [](int8 comboSet) {
        return comboSet >= MELEE_COMBO_BBALLBAT && comboSet <= MELEE_COMBO_CHAINSAW;
    };

    switch (attackerComboSet) {
    case MELEE_COMBO_BBALLBAT:
    case MELEE_COMBO_GOLFCLUB:
    case MELEE_COMBO_SWORD:
    case MELEE_COMBO_CHAINSAW: { // Can only be blocked using a similar weapon (but not with a knife)
        if (!IsHeavyWeaponCombo(m_nComboSet) || m_nComboSet == MELEE_COMBO_KNIFE) {
            return false;
        }
        break;
    }
    case MELEE_COMBO_KNIFE: { // Can only be blocked using a weapon
        if (!IsHeavyWeaponCombo(m_nComboSet)) {
            return false;
        }
        break;
    }
    case MELEE_COMBO_UNARMED_4: {
        if (attackerMove == FIGHT_ATTACK_HIT_2) {
            return false;
        }
        break;
    }
    }

    if (isAttackerBehind) {
        return false;
    }

    m_pAnim->SetFlag(ANIMATION_IS_PLAYING, true);
    return true;
}

// 0x4ABDC0
bool CTaskSimpleFight::IsComboSet() {
    const auto move = (int32)m_nCurrentMove;
    if (move < 0 || move > 11) { // Out of the 16 bits of the flags (Original code just shifts, with the same result)
        return false;
    }
    return (GetCurrentComboData().m_wFlags & (MELEE_FLAG_FALL_1 << move)) != 0;
}

// 0x4ABDF0
bool CTaskSimpleFight::IsHitComboSet() {
    const auto move = (int32)m_nCurrentMove;
    if (move < FIGHT_ATTACK_HIT_1 || move > FIGHT_ATTACK_HIT_3) { // NOTE: Original code only checks the upper bound, lower values result in `false` anyways
        return false;
    }
    return (GetCurrentComboData().m_wFlags & (MELEE_FLAG_NOFALL_1 << move)) != 0;
}

// 0x4ABDA0
AssocGroupId CTaskSimpleFight::GetComboAnimGroupID() {
    return GetCurrentComboData().m_nAnimGroup;
}

// 0x61C1C0
float CTaskSimpleFight::GetRange() {
    return GetCurrentComboData().m_fRanges;
}

// 0x61C740
float CTaskSimpleFight::GetStrikeDamage(CPed* ped) {
    // NOTE: Original code doesn't clamp the combo set index here (`m_nComboSet - 4`)
    const auto baseDmg = (float)GetCurrentComboData().m_nDamage[m_nCurrentMove];
    if (ped->IsPlayer()) {
        return ped->GetPlayerData()->m_bAdrenaline
            ? 50.f
            : CStats::GetFatAndMuscleModifier(STAT_MOD_4) * baseDmg;
    }
    switch (ped->GetActiveWeapon().m_Type) {
    case WEAPON_BRASSKNUCKLE:
        return baseDmg * 1.5f;
    case WEAPON_UNARMED:
        return baseDmg * ped->m_pStats->m_fAttackStrength;
    default:
        return baseDmg;
    }
}

// 0x5BD360
eMeleeHitLevel CTaskSimpleFight::GetHitLevel(const char* hitLevel) {
    switch (hitLevel[0]) {
    case 'H': return MELEE_HIT_LEVEL_HIGH;
    case 'L': return MELEE_HIT_LEVEL_LOW;
    case 'G': return MELEE_HIT_LEVEL_GROUND;
    case 'B': return MELEE_HIT_LEVEL_BEHIND;
    }

    // NOTE: Original code now compares the first character (as an int) to the multi-char constants 'HL', 'LL' and 'GL'.
    // That can never be true (And the first char of all those is already handled above),
    // so `MELEE_HIT_LEVEL_HIGH_LONG`, `MELEE_HIT_LEVEL_LOW_LONG`, `MELEE_HIT_LEVEL_GROUND_LONG` are never returned.

    return MELEE_HIT_LEVEL_NUM;
}

// 0x5BD3B0
eAudioEvents CTaskSimpleFight::GetHitSound(int32 hitSound) {
    switch (hitSound) {
    case 1:  return AE_PED_HIT_HIGH;
    case 3:  return AE_PED_HIT_GROUND;
    case 4:  return AE_PED_HIT_GROUND_KICK;
    case 5:  return AE_PED_HIT_HIGH_UNARMED;
    case 6:  return AE_PED_HIT_LOW_UNARMED;
    case 7:  return AE_PED_HIT_MARTIAL_PUNCH;
    case 8:  return AE_PED_HIT_MARTIAL_KICK;
    case 2:
    default: return AE_PED_HIT_LOW;
    }
}

// 0x61DB30
eMeleeCombo CTaskSimpleFight::GetComboType(const char* comboName) {
    static constexpr std::pair<std::string_view, eMeleeCombo> mapping[]{
        { "UNARMED",  MELEE_COMBO_UNARMED_1 },
        { "BBALLBAT", MELEE_COMBO_BBALLBAT  },
        { "KNIFE",    MELEE_COMBO_KNIFE     },
        { "GOLFCLUB", MELEE_COMBO_GOLFCLUB  },
        { "SWORD",    MELEE_COMBO_SWORD     },
        { "CHAINSAW", MELEE_COMBO_CHAINSAW  },
        { "DILDO",    MELEE_COMBO_DILDO     },
        { "FLOWERS",  MELEE_COMBO_FLOWERS   },
    };
    const std::string_view name{ comboName };
    for (const auto& [k, v] : mapping) {
        if (k == name) {
            return v;
        }
    }
    return MELEE_COMBO_UNARMED_1;
}

// 0x5BEDC0
void CTaskSimpleFight::LoadMeleeData() {
    ZoneScoped;

    for (auto& cs : m_aComboData) {
        cs.m_nAnimGroup      = ANIM_GROUP_MELEE_1;
        cs.m_fRanges         = 1.5f;
        cs.m_fGroundLoop     = 0.f;
        cs.m_fBlockLoopStart = 100.f;
        cs.m_fBlockLoopEnd   = 100.f;
        cs.m_wFlags          = 0;
        cs.m_fHit.fill(100.f);
        cs.m_fChain.fill(100.f);
        cs.m_fRadius.fill(1.f);
        cs.m_nHitLevel.fill(MELEE_HIT_LEVEL_NUM);
        cs.m_nDamage.fill(0);
        cs.m_Hit.fill(0);
        cs.m_AltHit.fill(0);
    }

    m_aHitOffset.fill(CVector{ 0.f, 0.75f, 0.f });

    // Combo section fields (in the order they are expected to be in the file)
    enum {
        FANIMGROUP = 1,
        FRANGES    = 2,
        FATTACK1   = 3,
        FATTACK2   = 4,
        FATTACK3   = 5,
        FAGROUND   = 6,
        FAMOVING   = 7,
        FABLOCK    = 8,
        FFLAGS     = 9,
    };

    constexpr auto FRAMES_TO_SECONDS = 1.f / 30.f;

    bool   inCombo  = false;
    bool   inLevels = false;
    size_t comboN   = 0; // Index of the combo being currently read
    size_t lineN    = 0; // Number of lines read in the current section (Original code uses the same variable for both kind of sections too)

    char fname[32]{};

    const auto file = CFileMgr::OpenFile("DATA\\melee.dat", "rb");
    for (;;) {
        const auto line = CFileLoader::LoadLine(file);
        if (!line) {
            break;
        }
        if (!line[0] || line[0] == '#') {
            continue;
        }

        const auto LineStartsWith = [line](std::string_view with) {
            return strncmp(line, with.data(), with.size()) == 0;
        };

        if (LineStartsWith("END_MELEE_DATA")) { // End of file
            break;
        }

        if (!inCombo && !inLevels) {
            if (LineStartsWith("START_COMBO")) {
                inCombo = true;
            } else if (LineStartsWith("START_LEVELS")) {
                inLevels = true;
            }
            continue;
        }

        if (LineStartsWith("END_COMBO")) { // Yes, `END_COMBO` is used for `START_LEVELS` too!
            if (inCombo) {
                comboN++;
            }
            lineN    = 0;
            inCombo  = false;
            inLevels = false;
            continue;
        }

        if (inLevels) {
            if (lineN >= m_aHitOffset.size()) { // NOTSA: Bounds check
                NOTSA_LOG_ERR("melee.dat: Too many hit levels!");
                continue;
            }

            auto* const o = &m_aHitOffset[lineN++];
            VERIFY(sscanf_s(
                line,
                "%s %f %f %f",
                SCANF_S_STR(fname), &o->x, &o->y, &o->z
            ) == 4);

            continue;
        }

        // Combo section

        const auto field = ++lineN;

        if (comboN >= m_aComboData.size()) { // NOTSA: Bounds check
            NOTSA_LOG_ERR("melee.dat: Too many combos!");
            continue;
        }
        auto* const c = &m_aComboData[comboN];

        switch (field) {
        case FANIMGROUP: {
            char animGroupName[32]{};

            VERIFY(sscanf_s(
                line,
                "%s %s",
                SCANF_S_STR(fname), SCANF_S_STR(animGroupName)
            ) == 2);

            // If there's no such group the default (`ANIM_GROUP_MELEE_1`) is kept
            for (uint32 i = 0; i < CAnimManager::GetAssocGroups().size(); i++) {
                if (strcmp(animGroupName, CAnimManager::GetAnimGroupName((AssocGroupId)i)) == 0) {
                    c->m_nAnimGroup = (AssocGroupId)i;
                    break;
                }
            }

            break;
        }
        case FRANGES: {
            VERIFY(sscanf_s(
                line,
                "%s %f",
                SCANF_S_STR(fname), &c->m_fRanges
            ) == 2);

            break;
        }
        case FATTACK1:
        case FATTACK2:
        case FATTACK3:
        case FAGROUND:
        case FAMOVING: {
            // In the same order as they appear in the file:
            float hit{};
            float chain{};
            float radius{};
            char  hitLevelName[32]{};
            int32 damage{};
            int32 hitSound{};
            int32 altHitSound{};
            float groundLoop = 0.f; // Optional

            VERIFY(sscanf_s(
                line,
                "%s %f %f %f %s %d %d %d %f",
                SCANF_S_STR(fname), &hit, &chain, &radius, SCANF_S_STR(hitLevelName), &damage, &hitSound, &altHitSound, &groundLoop
            ) >= 8);

            const auto move = field - FATTACK1; // See `eFightAttackType`

            c->m_fHit[move]      = hit * FRAMES_TO_SECONDS;
            c->m_fRadius[move]   = radius;
            c->m_fChain[move]    = chain * FRAMES_TO_SECONDS;
            c->m_nHitLevel[move] = GetHitLevel(hitLevelName);
            c->m_nDamage[move]   = (uint8)damage;
            c->m_Hit[move]       = GetHitSound(hitSound);
            c->m_AltHit[move]    = GetHitSound(altHitSound);
            if (groundLoop > 0.f) {
                c->m_fGroundLoop = groundLoop * FRAMES_TO_SECONDS;
            }

            break;
        }
        case FABLOCK: {
            float loopStart{};
            float loopEnd{};

            VERIFY(sscanf_s(
                line,
                "%s %f %f",
                SCANF_S_STR(fname), &loopStart, &loopEnd
            ) == 3);

            c->m_fBlockLoopStart = loopStart * FRAMES_TO_SECONDS;
            c->m_fBlockLoopEnd   = loopEnd * FRAMES_TO_SECONDS;

            break;
        }
        case FFLAGS: {
            uint32 flags{};

            VERIFY(sscanf_s(
                line,
                "%s %x",
                SCANF_S_STR(fname), &flags
            ) == 2);

            c->m_wFlags = (uint16)flags;

            break;
        }
        default: // Original code ignores any extra lines too
            break;
        }
    }
    CFileMgr::CloseFile(file);
}

// 0x6239F0
bool CTaskSimpleFight::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (priority != ABORT_PRIORITY_URGENT && priority != ABORT_PRIORITY_IMMEDIATE) {
        m_nNextCommand = MELEE_CMD_END_SLOW;
        return false;
    }

    if (event && (event->GetEventPriority() < 32 || event->GetEventPriority() == 60)) {
        return false;
    }

    if (m_pAnim) {
        if (priority == ABORT_PRIORITY_IMMEDIATE) {
            m_pAnim->SetBlendDelta(-1000.f);
        }
        m_pAnim->SetDefaultDeleteCallback();
        m_pAnim = nullptr;
    }

    if (m_pIdleAnim) {
        m_pIdleAnim->SetDefaultDeleteCallback();
        if (m_pIdleAnim->m_BlendAmount > 0.f && m_pIdleAnim->m_BlendDelta >= 0.f) {
            CAnimManager::BlendAnimation(
                ped->GetRpClump(),
                ped->m_nAnimGroup,
                ANIM_ID_IDLE,
                priority == ABORT_PRIORITY_IMMEDIATE
                    ? 1000.f
                    : 16.f
            );
        }
        m_pIdleAnim = nullptr;
    }

    if (ped && ped->IsPlayer()) {
        auto* const fightMovement = &ped->GetPlayerData()->m_vecFightMovement;
        fightMovement->x = 0.f;
        fightMovement->y = 0.f;
        SetPlayerMoveAnim(ped->AsPlayer());
    }

    m_bIsFinished = true;

    return true;
}

// 0x623B10
void CTaskSimpleFight::StartAnim(CPed* ped, int32 newMove) {
    if (newMove < 0) {
        m_nNextCommand = MELEE_CMD_IDLE;
        return;
    }

    if (m_pAnim) {
        m_pAnim->SetDefaultDeleteCallback();
        m_pAnim = nullptr;
    }

    auto* const clump   = ped->GetRpClump();
    const auto  command = (int8)m_nNextCommand; // NOTE: Not re-read after `SetPlayerMoveAnim` (which changes it), same as the original

    switch (command) {
    case MELEE_CMD_IDLE: { // 0x623C1C
        m_nComboSet       = MELEE_COMBO_IDLE;
        m_nCurrentMove    = FIGHT_ATTACK_HIT_1;
        m_nContinueStrike = 0;

        if (ped->m_nMoveState >= PEDMOVE_WALK && ped->IsPlayer()) {
            m_bIsFinished = true;
            break;
        }

        // 0x623C55 - The original has `GetAvailableComboSet(ped, MELEE_CMD_IDLE)` inlined here
        m_nComboSet = GetAvailableComboSet(ped, MELEE_CMD_IDLE);

        const auto BlendIdleAnim = [&] {
            return CAnimManager::BlendAnimation(clump, GetCurrentComboData().m_nAnimGroup, ANIM_ID_FIGHT_IDLE, 8.f);
        };
        if (!m_pIdleAnim) { // 0x623D38
            m_pIdleAnim = BlendIdleAnim();
            m_pIdleAnim->SetDeleteCallback(FinishMeleeAnimCB, this);
        } else if (m_pIdleAnim->m_BlendAmount < 1.f && m_pIdleAnim->m_BlendDelta <= 0.f) { // 0x623D97
            m_pIdleAnim = BlendIdleAnim();
        }

        if (ped->IsPlayer()) { // 0x623DE3
            auto* const fightMovement = &ped->GetPlayerData()->m_vecFightMovement;
            fightMovement->x = 0.f;
            fightMovement->y = 0.f;
            SetPlayerMoveAnim(ped->AsPlayer());
        }

        m_nComboSet = MELEE_COMBO_IDLE;
        break;
    }
    case MELEE_CMD_END_SLOW:
    case MELEE_CMD_END_QUICK:
    case MELEE_CMD_END_RUNAWAY:
    case MELEE_CMD_END_SPRINTAWAY:
    case MELEE_CMD_END_DUCK: { // 0x623F68
        if (auto* const playerData = ped->GetPlayerData()) {
            playerData->m_vecFightMovement.x = 0.f;
            playerData->m_vecFightMovement.y = 0.f;
            SetPlayerMoveAnim(ped->AsPlayer());
            switch (command) {
            case MELEE_CMD_END_SPRINTAWAY: ped->GetPlayerData()->m_fMoveBlendRatio = 2.f; break;
            case MELEE_CMD_END_RUNAWAY:    ped->GetPlayerData()->m_fMoveBlendRatio = 1.f; break;
            }
        } else { // 0x623FAE
            switch (command) {
            case MELEE_CMD_END_SPRINTAWAY: ped->SetMoveState(PEDMOVE_RUN);   break;
            case MELEE_CMD_END_RUNAWAY:    ped->SetMoveState(PEDMOVE_WALK);  break;
            default:                       ped->SetMoveState(PEDMOVE_STILL); break;
            }
            ped->m_nSwimmingMoveState = (int32)ped->m_nMoveState;
        }

        // 0x623FD5 - Blend into the anim the fight is left with
        switch (command) {
        case MELEE_CMD_END_SPRINTAWAY:
            CAnimManager::BlendAnimation(clump, ped->m_nAnimGroup, ANIM_ID_RUN, 8.f);
            break;
        case MELEE_CMD_END_RUNAWAY:
            CAnimManager::BlendAnimation(clump, ped->m_nAnimGroup, ANIM_ID_WALK, 8.f);
            break;
        case MELEE_CMD_END_QUICK:
            CAnimManager::BlendAnimation(clump, ped->m_nAnimGroup, ANIM_ID_IDLE, 4.f);
            break;
        default: {
            if (command == MELEE_CMD_END_DUCK && ped->bIsDucking && ped->GetIntelligence()->GetTaskDuck(true)) { // 0x624012
                CAnimManager::BlendAnimation(clump, ANIM_GROUP_DEFAULT, ANIM_ID_WEAPON_CROUCH, 4.f);
            } else { // 0x624042
                CAnimManager::BlendAnimation(clump, ped->m_nAnimGroup, ANIM_ID_IDLE, 2.f);
            }
            break;
        }
        }

        // 0x624059
        if (m_pIdleAnim) {
            m_pIdleAnim->SetFlag(ANIMATION_IS_PLAYING, false);
        } else {
            m_bIsFinished = true;
        }

        m_nNextCommand = MELEE_CMD_END_RUNAWAY; // Becomes `m_nLastCommand` below (for all the end commands)
        break;
    }
    case MELEE_CMD_BLOCK: { // 0x623EF7
        m_nCurrentMove    = FIGHT_ATTACK_HIT_1;
        m_nContinueStrike = 0;

        // NOTE: Original code doesn't clamp the combo set index here (`m_nComboSet - 4`)
        if (!(GetCurrentComboData().m_wFlags & MELEE_FLAG_BLOCK)) {
            m_nComboSet = MELEE_COMBO_UNARMED_1;
        }

        m_pAnim = CAnimManager::BlendAnimation(clump, GetCurrentComboData().m_nAnimGroup, ANIM_ID_FIGHT_FIGHT_BLOCK, 8.f);
        m_pAnim->SetFinishCallback(FinishMeleeAnimCB, this);
        break;
    }
    case MELEE_CMD_MOVE_FWD:
    case MELEE_CMD_MOVE_LEFT:
    case MELEE_CMD_MOVE_BACK:
    case MELEE_CMD_MOVE_RIGHT:
    case MELEE_CMD_SHUFFLE_FWD:
    case MELEE_CMD_SHUFFLE_LEFT:
    case MELEE_CMD_SHUFFLE_BACK:
    case MELEE_CMD_SHUFFLE_RIGHT: { // 0x623E21 - Players are handled by `SetPlayerMoveAnim`
        if (ped->IsPlayer()) {
            break;
        }

        m_nContinueStrike = 0;
        m_nComboSet       = MELEE_COMBO_MOVE;

        // The move is the direction here (0 - fwd, 1 - left, 2 - back, 3 - right)
        AnimationId animId;
        switch ((int8)m_nNextCommand) {
        case MELEE_CMD_SHUFFLE_FWD: {
            m_nCurrentMove = (eFightAttackType)0;
            animId         = ANIM_ID_FIGHTSHF;
            break;
        }
        case MELEE_CMD_SHUFFLE_BACK: {
            m_nCurrentMove = (eFightAttackType)2;
            animId         = ANIM_ID_FIGHTSHB;
            break;
        }
        default: {
            switch ((int8)m_nNextCommand) {
            case MELEE_CMD_SHUFFLE_LEFT:  m_nCurrentMove = (eFightAttackType)1;                                              break;
            case MELEE_CMD_SHUFFLE_RIGHT: m_nCurrentMove = (eFightAttackType)2;                                              break; // Yes, 2 (So the anim is `ANIM_ID_FIGHTSH_BWD`)
            default:                      m_nCurrentMove = (eFightAttackType)((int8)m_nNextCommand - MELEE_CMD_MOVE_FWD); break;
            }
            animId = (AnimationId)(ANIM_ID_FIGHTSH_FWD + (int32)m_nCurrentMove);
            break;
        }
        }

        m_pAnim = CAnimManager::BlendAnimation(clump, ANIM_GROUP_DEFAULT, animId, 8.f); // 0x623E92
        if ((int8)m_nNextCommand == MELEE_CMD_MOVE_FWD) {
            m_pAnim->SetDeleteCallback(FinishMeleeAnimCB, this);
            m_pAnim->SetFlag(ANIMATION_IS_BLEND_AUTO_REMOVE, true);
        } else { // 0x623EC9
            m_pAnim->SetFlag(ANIMATION_IS_LOOPED, false);
            m_pAnim->SetFlag(ANIMATION_IS_SYNCRONISED, false);
            m_pAnim->SetFlag(ANIMATION_IS_FINISH_AUTO_REMOVE, true);
            m_pAnim->SetFinishCallback(FinishMeleeAnimCB, this);
        }
        break;
    }
    case MELEE_CMD_ATTACK_1:
    case MELEE_CMD_ATTACK_2:
    case MELEE_CMD_ATTACK_3:
    case MELEE_CMD_ATTACK_4: { // 0x623B59
        m_nCurrentMove    = (eFightAttackType)(int8)newMove;
        m_nContinueStrike = 1;

        // NOTE: Original code doesn't clamp the combo set index here (`m_nComboSet - 4`)
        const auto& info = GetCurrentComboData();

        m_pAnim = CAnimManager::BlendAnimation(clump, info.m_nAnimGroup, (AnimationId)(ANIM_ID_FIGHT_1 + (int32)(int8)newMove), 8.f);
        m_pAnim->SetFinishCallback(FinishMeleeAnimCB, this);

        if (m_nCurrentMove == 3 && m_pAnim->m_CurrentTime != 0.f) { // 0x623BA7 - Move 3 is the ground attack, re-start its loop if it's already playing
            m_pAnim->SetCurrentTime(info.m_fGroundLoop);
        }

        if (ped->GetPlayerData()) { // 0x623BD9
            m_pAnim->m_Speed = CStats::GetFatAndMuscleModifier(STAT_MOD_3);
            if (m_nCurrentMove < 4) {
                ped->SetMoveState(PEDMOVE_STILL);
            }
        }
        break;
    }
    default: // Any other value (incl. negative ones)
        break;
    }

    m_nLastCommand = m_nNextCommand;
    m_nNextCommand = MELEE_CMD_IDLE;
}

// 0x6240B0
bool CTaskSimpleFight::FightStrike(CPed* ped, CVector& posn) {
    // NOTE: Original code also calls `CWeaponInfo::GetWeaponInfo` for the active weapon here (result is unused), and has an unused `CMatrix` on the stack.
    // NOTE: Original code doesn't clamp the combo set index here (`m_nComboSet - 4`)
    const auto& info = GetCurrentComboData();

    // The original re-reads these each time they're used too
    const auto GetStrikeRadius = [&] { return info.m_fRadius[m_nCurrentMove]; };
    const auto IsLongHitLevel  = [&] { return (uint8)info.m_nHitLevel[m_nCurrentMove] >= (uint8)MELEE_HIT_LEVEL_HIGH_LONG; };

    if (ped == FindPlayerPed() && ped->GetActiveWeapon().m_Type != WEAPON_UNARMED) { // 0x624126
        CGlass::BreakGlassPhysically(posn, GetStrikeRadius());
    }

    auto* const intel = ped->GetIntelligence();

    // 0x62418C - Only the player can hit objects
    constexpr int32 MAX_OBJECTS = 16;
    CEntity*        objects[MAX_OBJECTS];
    int16           numObjects = 0;
    if (ped->IsPlayer()) {
        CWorld::FindObjectsInRange(posn, 5.f, true, &numObjects, (int16)MAX_OBJECTS, objects, false, false, false, true, false);
    }

    FightSetUpCol(GetStrikeRadius());

    CMatrix strikeMat{ *ped->m_matrix };
    strikeMat.SetTranslateOnly(posn);

    constexpr int32 NUM_SCANNER_ENTITIES = 16; // Same as `MAX_NUM_ENTITIES` of `CEntityScanner`

    CPed* hitPed      = nullptr;
    bool  hasntHitPed = true;

    // 0x624240 - First the peds, then the vehicles (both from the scanners), and finally the objects found above
    for (int32 i = 0; i < (int32)numObjects + 2 * NUM_SCANNER_ENTITIES; i++) {
        CPed*     targetPed = nullptr;
        CVehicle* targetVeh = nullptr;
        CObject*  targetObj = nullptr;
        CEntity*  entity    = nullptr;
        if (i < NUM_SCANNER_ENTITIES) {
            entity    = intel->GetPedEntities()[i];
            targetPed = static_cast<CPed*>(entity);
        } else if (i < 2 * NUM_SCANNER_ENTITIES) {
            entity    = intel->GetVehicleEntities()[i - NUM_SCANNER_ENTITIES];
            targetVeh = static_cast<CVehicle*>(entity);
        } else {
            entity    = objects[i - 2 * NUM_SCANNER_ENTITIES];
            targetObj = static_cast<CObject*>(entity);
        }

        if (!entity) {
            continue;
        }

        // 0x624287
        auto reach = entity->GetModelInfo()->GetColModel()->GetBoundRadius() + GetStrikeRadius();
        if (targetPed && IsLongHitLevel()) {
            reach += GetStrikeRadius() * 0.5f;
        }

        // 0x6242C3
        const auto CanBeHit = [&] {
            if (targetPed) {
                if (targetPed->GetUsesCollision() || !targetPed->IsAlive()) {
                    return true;
                }
                if (targetPed->bInVehicle && targetPed->m_pVehicle && targetPed->m_pVehicle->m_nVehicleType == VEHICLE_TYPE_BIKE) {
                    return true;
                }
            }
            if (targetVeh && targetVeh->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE) {
                return true;
            }
            return targetObj && targetObj->GetUsesCollision();
        };
        if (!CanBeHit()) {
            continue;
        }

        // 0x62431A - Rough check using the bounding sphere (2D only for peds)
        const auto centreToPos = entity->GetBoundCentre() - posn;
        const auto distSq      = targetPed
            ? centreToPos.SquaredMagnitude2D()
            : centreToPos.SquaredMagnitude();
        if (!(distSq < reach * reach)) {
            continue;
        }

        if (targetPed) { // 0x624393
            auto* const cd = targetPed->GetPedModelInfo()->AnimatePedColModelSkinnedWorld(targetPed->GetRpClump())->m_pColData;
            for (int32 numExtends = 0;;) {
                bool hasHit = false;
                for (int32 s = 0; s < (int32)(int16)cd->m_nNumSpheres; s++) {
                    const auto& sphere = cd->m_pSpheres[s];
                    CVector     hitDir = sphere.m_vecCenter - posn;
                    const auto  range  = sphere.m_fRadius + GetStrikeRadius();
                    if (hitDir.SquaredMagnitude() < range * range) { // 0x6244CC
                        if (auto* const damagedPed = FightHitPed(ped, targetPed, posn, hitDir, 3)) {
                            hitPed = damagedPed;
                        }
                        hasntHitPed = false;
                        hasHit      = true;
                        break;
                    }
                }
                if (hasHit) {
                    break;
                }

                // 0x624443 - Long hit levels: Move the strike position forwards, and try again (once)
                if (!IsLongHitLevel()) {
                    break;
                }
                const auto strikeRadius = GetStrikeRadius();
                posn += ped->m_matrix->GetForward() * 1.5f * strikeRadius;
                if (++numExtends >= 2) {
                    break;
                }
            }
        } else { // 0x6244FC
            const auto numColPts = CCollision::ProcessColModels(
                strikeMat,
                m_sStrikeColModel,
                entity->GetMatrix(),
                *entity->GetColModel(),
                CWorld::m_aTempColPts,
                nullptr,
                nullptr,
                false
            );
            if (numColPts > 0) {
                auto& cp = CWorld::m_aTempColPts[0];
                if (targetVeh) { // 0x62454E
                    FightHitCar(ped, targetVeh, cp.m_vecPoint, cp.m_vecNormal, (int16)cp.m_nPieceTypeB, cp.m_nSurfaceTypeB);
                } else if (targetObj) { // 0x624586
                    FightHitObj(ped, targetObj, cp.m_vecPoint, cp.m_vecNormal, (int16)cp.m_nPieceTypeB, cp.m_nSurfaceTypeB);
                }
            }
        }
    }

    // 0x6245E3 - No ped was hit (Vehicles/objects don't count)
    if (hasntHitPed && ped->IsPlayer()) {
        CEventSoundQuiet event{ ped, 40.f, (uint32)-1, CVector{ 0.f, 0.f, 0.f } };
        GetEventGlobalGroup()->Add(static_cast<CEvent*>(&event), false);
    }

    // 0x624664 - Second move of this combo is only continued if the ped hit is playing the hit anim
    if (m_nComboSet == MELEE_COMBO_UNARMED_4 && m_nCurrentMove == 1 && m_pAnim) {
        if (!hitPed || !RpAnimBlendClumpGetAssociation(hitPed->GetRpClump(), ANIM_ID_FIGHT_HIT_2)) {
            m_pAnim->m_BlendDelta = -4.f;
            m_pAnim->SetFlag(ANIMATION_IS_PLAYING, false);
            m_pAnim->SetFlag(ANIMATION_IS_BLEND_AUTO_REMOVE, true);
        }
    }

    // 0x6246AC - Last strike position (`CPed + 0x720`, `m_vecWeaponPrevPos` on Android, here it's still 3 separate `int32` fields)
    *reinterpret_cast<CVector*>(&ped->field_720) = posn;

    return false;
}

// 0x624710
int16 CTaskSimpleFight::ChooseAttackPlayer(CPed* ped) {
    const auto hasNoTarget = m_pTargetEntity == nullptr;
    const auto IsCommandRepeated = [this] { return (int8)m_nNextCommand == (int8)m_nLastCommand; };

    int16 move = (int8)m_nNextCommand >= MELEE_CMD_ATTACK_1 && (int8)m_nNextCommand <= MELEE_CMD_ATTACK_4 && m_nComboSet >= MELEE_COMBO_UNARMED_1
        ? -1  // To be chosen below
        : 1;

    const auto flags = GetAvailableMoveFlags(m_nComboSet, ped); // NOTE: Not updated when the combo set is changed below

    if (move < 0) {
        if (m_pAnim && m_nCurrentMove != 3 && m_nCurrentMove != 4) { // 0x624773 - In the middle of a (standing) combo
            ped->SetMoveState(PEDMOVE_STILL);

            const int16 lastMoveOfCombo = (flags & MELEE_FLAG_ATTACK_3)
                ? 2
                : (flags & MELEE_FLAG_ATTACK_2) ? 1 : 0;

            if (FindTargetOnGround(ped)) { // 0x6247B4
                move = -1;
            } else if (!IsCommandRepeated() && (int32)m_nChainCounter <= CHAIN_COUNT_LIMIT) { // 0x6247CC - Different attack button pressed
                m_nChainCounter++;

                const auto v = (int32)m_nCurrentMove - (int32)(int8)m_nLastCommand + (int32)(int8)m_nNextCommand;
                if (v & 1) {
                    move = 0;
                } else {
                    move = (v & 2) ? 1 : 2;
                }
                if (move > lastMoveOfCombo) {
                    move = 0;
                }
            } else { // 0x62481A - Continue the combo (if there are moves left)
                move = (int16)m_nCurrentMove + 1;
                if (move > lastMoveOfCombo) {
                    move = -1;
                }
            }
        } else if (ped->m_nMoveState > PEDMOVE_WALK) { // 0x624831 - Running attack
            if (!(flags & MELEE_FLAG_MOVING)) {
                m_nComboSet = MELEE_COMBO_UNARMED_1;
            }
            move = 4;
        } else if (FindTargetOnGround(ped)) { // 0x62484E - Ground attack
            // NOTE: Both of these return without updating the ped's heading
            if (m_pAnim && !IsCommandRepeated()) {
                return -1;
            }
            if (!(flags & MELEE_FLAG_GROUND)) {
                m_nComboSet = MELEE_COMBO_UNARMED_1;
            } else if (m_pTargetEntity && m_pTargetEntity->GetIsTypePed() && m_pTargetEntity->AsPed()->bIsDucking) { // 0x62487D
                m_nComboSet = MELEE_COMBO_UNARMED_1;
            }
            return 3;
        } else { // 0x6248AE
            m_nChainCounter = 0;
            move            = 0;
        }
    }

    // 0x6248B4 - No target, so turn towards the ped (close by) that requires the least amount of turning
    if (hasNoTarget) {
        constexpr auto PI_F     = 3.14159274f; // 0x858CB8
        constexpr auto TWO_PI_F = 6.28318548f; // 0x858CBC

        auto bestHeading     = -1000.f;
        auto bestHeadingDiff = 1000.f;
        auto** const pedEntities = ped->GetIntelligence()->GetPedEntities();
        for (int32 i = 0; i < 16; i++) { // 16 - Same as `MAX_NUM_ENTITIES` of `CEntityScanner`
            auto* const entity = pedEntities[i];
            if (!entity || !entity->AsPed()->IsAlive()) {
                continue;
            }

            const auto dir = entity->GetPosition() - ped->GetPosition();
            if (!(PLAYER_AUTO_FACE_RANGE * PLAYER_AUTO_FACE_RANGE > dir.SquaredMagnitude())) {
                continue;
            }

            const auto heading = std::atan2(-dir.x, dir.y);

            auto diff = heading - ped->m_fCurrentRotation;
            if (diff > PI_F) {
                diff -= TWO_PI_F;
            } else if (diff < -PI_F) {
                diff += TWO_PI_F;
            }
            diff = std::abs(diff);

            if (diff < bestHeadingDiff) {
                bestHeadingDiff = diff;
                bestHeading     = heading;
            }
        }
        if (bestHeading > -10.f) {
            ped->m_fAimingRotation = bestHeading;
        }
    }

    return move;
}

// 0x624A40
int16 CTaskSimpleFight::ChooseAttackAI(CPed* ped) {
    const auto flags = GetAvailableMoveFlags(m_nComboSet, ped);
    const auto rnd   = (float)CGeneral::GetRandomNumber() * RAND_MAX_FLOAT_RECIPROCAL; // [0, 1]

    if (rnd > 0.8f && (flags & MELEE_FLAG_ATTACK_3)) {
        // 0x624A95 - The 3rd move is always used, unless it knocks the target down (`MELEE_FLAG_FALL_1 << 2`), and the target is a wanted player with enough health
        if (!(flags & (MELEE_FLAG_FALL_1 << 2))) {
            return 2;
        }
        auto* const target = m_pTargetEntity;
        if (!target || !target->GetIsTypePed() || !target->AsPed()->GetPlayerData()) {
            return 2;
        }
        if ((int32)target->AsPed()->AsPlayer()->GetWantedLevel() <= 0) {
            return 2;
        }
        if (!(target->AsPed()->m_fHealth > 20.f)) {
            return 2;
        }
    } else if (FindTargetOnGround(ped) && (flags & MELEE_FLAG_GROUND)) { // 0x624ADE
        return 3;
    }

    // 0x624AF9
    return rnd > 0.5f && (flags & MELEE_FLAG_ATTACK_2)
        ? 1
        : 0;
}

// 0x61D6F0
bool CTaskSimpleFight::FindTargetOnGround(CPed* ped) {
    if (!CLocalisation::KickingWhenDown()) {
        return false;
    }

    // NOTE: Combo set is indexed unclamped in the original (`m_nComboSet - 4`)
    const auto  groundRange = m_aHitOffset[MELEE_HIT_LEVEL_GROUND].y + GetCurrentComboData().m_fRadius[3];
    const float sideRange   = groundRange + 0.2f;
    float       fwdRange    = groundRange + 0.4f;

    const auto& pedPos = ped->GetPosition();

    if (m_pTargetEntity) {
        if (!m_pTargetEntity->GetIsTypePed()) {
            // 0x61D8BB - Standing on the target (e.g. on top of a car)
            return m_pTargetEntity == ped->m_standingOnEntity;
        }

        auto* const target = m_pTargetEntity->AsPed();

        CVector spinePos{};
        target->GetBonePosition(&spinePos, BONE_SPINE1, false);

        // 0x61D77D - Alive targets have to be lying on the ground (or ducking)
        if (target->IsAlive() && pedPos.z - 0.2f <= spinePos.z) {
            if (!target->bIsDucking) {
                return false;
            }
            fwdRange -= 0.4f;
        }

        // 0x61D7CD - Don't kick peds that are still falling
        if (const auto* const fall = notsa::dyn_cast_if_present<CTaskSimpleFall>(target->GetTaskManager().GetSimplestActiveTask())) {
            if (fall->m_pAnim && fall->m_pAnim->m_BlendHier->m_fTotalTime > fall->m_pAnim->m_CurrentTime) {
                return false;
            }
        }

        // 0x61D80F - Has to be within the target's local box
        const auto& targetMat = target->GetMatrix();
        const auto  dir       = target->GetPosition() - pedPos;
        if (!(std::abs(dir.Dot(targetMat.GetForward())) < fwdRange)) {
            return false;
        }
        if (!(std::abs(dir.Dot(targetMat.GetRight())) < sideRange)) {
            return false;
        }
        return true;
    }

    // 0x61D8CB - No target, only the player looks for one
    if (!ped->IsPlayer()) {
        return false;
    }

    CPed* closestPed     = nullptr;
    float closestHeading = 0.f;
    float closestDiff    = 3.14159274f;
    for (auto* const entity : ped->GetIntelligence()->GetPedScanner().m_apEntities) {
        if (!entity) {
            continue;
        }
        auto* const nearby = entity->AsPed();

        CVector spinePos{};
        nearby->GetBonePosition(&spinePos, BONE_SPINE1, false);

        // 0x61D91D - Has to be dead or lying on the ground
        if (nearby->m_nPedState != PEDSTATE_DEAD && pedPos.z - 0.2f <= spinePos.z) {
            continue;
        }

        // NOTE: Original uses the matrix without checking that it's allocated
        const auto& nearbyMat = nearby->GetMatrix();
        const auto  dir       = nearby->GetPosition() - pedPos;
        if (!(std::abs(dir.Dot(nearbyMat.GetForward())) < fwdRange)) {
            continue;
        }
        if (!(std::abs(dir.Dot(nearbyMat.GetRight())) < sideRange)) {
            continue;
        }

        // 0x61D9CC
        const float heading = std::atan2(-dir.x, dir.y);
        float       diff    = heading - ped->m_fCurrentRotation;
        if (diff < -3.14159274f) {
            diff += 6.28318548f;
        } else if (diff > 3.14159274f) {
            diff -= 6.28318548f;
        }
        diff = std::abs(diff);
        if (!(diff < 1.04719758f)) { // 60 deg
            continue;
        }

        // 0x61DA2E - Prefer alive peds over dead ones, then the one we're facing the most
        if (closestPed) {
            const bool preferAlive = closestPed->m_fHealth <= 0.f && nearby->m_fHealth > 0.f;
            if (!preferAlive && !(diff < closestDiff)) {
                continue;
            }
        }
        closestPed     = nearby;
        closestHeading = heading;
        closestDiff    = diff;
    }

    if (closestPed) {
        ped->m_fAimingRotation = closestHeading;
        return true;
    }

    // 0x61DAA3 - Standing on a car
    const auto* const standingOn = ped->m_standingOnEntity;
    return standingOn && standingOn->GetIsTypeVehicle() && standingOn->AsVehicle()->IsAutomobile();
}

// 0x629920
bool CTaskSimpleFight::ProcessPed(CPed* ped) {
    if (m_bIsFinished) {
        if (m_pIdleAnim) {
            m_pIdleAnim->SetDefaultDeleteCallback();
            if (m_pIdleAnim->m_BlendAmount > 0.f && m_pIdleAnim->m_BlendDelta >= 0.f) {
                CAnimManager::BlendAnimation(ped->GetRpClump(), ped->m_nAnimGroup, ANIM_ID_IDLE, 8.f);
            }
            m_pIdleAnim = nullptr;
        }
        return true;
    }

    // NOTE: The commands are compared as signed values in the original
    const auto GetNextCommand = [this] { return (int8)m_nNextCommand; };
    const auto IsNextCommandAttack = [&] {
        return GetNextCommand() >= MELEE_CMD_ATTACK_1 && GetNextCommand() <= MELEE_CMD_ATTACK_4;
    };

    if (m_nComboSet != MELEE_COMBO_IDLE && m_bIsInControl) {
        m_nIdleCounter = 0;
    } else {
        m_nIdleCounter += (uint16)(int32)(CTimer::GetTimeStep() * 0.02f * 1000.f);
    }

    if (!m_bIsInControl) {
        if (GetNextCommand() < MELEE_CMD_END_SLOW) {
            return false;
        }
        if (m_pAnim && m_pAnim->GetAnimId() == ANIM_ID_FIGHT2IDLE) {
            return false;
        }
    }

    // 0x6299E2
    if (!m_pIdleAnim) {
        switch ((int8)m_nLastCommand) {
        case MELEE_CMD_END_SLOW:
        case MELEE_CMD_END_QUICK:
        case MELEE_CMD_END_RUNAWAY:
        case MELEE_CMD_END_SPRINTAWAY:
            m_bIsFinished = true;
            break;
        default: {
            if (m_pAnim) {
                break;
            }
            if (ped->m_nMoveState > PEDMOVE_WALK && ped->IsPlayer()) {
                break;
            }
            // 0x629A3C - Start the idle anim (The original has `GetAvailableComboSet(ped, MELEE_CMD_IDLE)` inlined here)
            m_nComboSet = GetAvailableComboSet(ped, MELEE_CMD_IDLE);
            m_pIdleAnim = CAnimManager::BlendAnimation(ped->GetRpClump(), GetComboData(m_nComboSet).m_nAnimGroup, ANIM_ID_FIGHT_IDLE, 4.f);
            m_pIdleAnim->SetDeleteCallback(FinishMeleeAnimCB, this);
            ped->SetMoveState(PEDMOVE_STILL);
            ped->m_nSwimmingMoveState = PEDMOVE_STILL;
            m_nComboSet               = MELEE_COMBO_IDLE;
            m_nLastCommand            = MELEE_CMD_IDLE;
            break;
        }
        }
    }

    // 0x629B9B - Make sure the anims are referenced
    if (m_nRequiredAnimGroup != ANIM_GROUP_MELEE_1 && !m_bAnimsReferenced) {
        GetAvailableComboSet(ped, MELEE_CMD_NONE);
    }

    // Start the move chosen by the player's input/the AI
    const auto StartChosenMove = [&](bool playerChooses) {
        StartAnim(ped, playerChooses ? ChooseAttackPlayer(ped) : m_nCurrentMove + 1);
    };

    if (!m_pAnim) { // 0x629BBB
        if (ped->IsPlayer() && m_nIdleCounter > m_nIdlePeriod && GetNextCommand() == MELEE_CMD_IDLE && m_nComboSet == MELEE_COMBO_IDLE) {
            m_nNextCommand = MELEE_CMD_END_SLOW;
        }

        if ((GetNextCommand() != MELEE_CMD_IDLE || m_nComboSet != MELEE_COMBO_IDLE) && (int8)m_nLastCommand != MELEE_CMD_END_RUNAWAY) {
            m_nComboSet = GetAvailableComboSet(ped, GetNextCommand());
            if (!ped->IsPlayer()) {
                StartAnim(ped, ChooseAttackAI(ped));
            } else if (GetNextCommand() >= MELEE_CMD_MOVE_FWD && GetNextCommand() <= MELEE_CMD_MOVE_RIGHT) {
                SetPlayerMoveAnim(ped->AsPlayer());
            } else {
                StartAnim(ped, ChooseAttackPlayer(ped));
            }
        }
    } else { // 0x629C4E
        if (ped->GetActiveWeapon().m_Type == WEAPON_CHAINSAW) {
            ped->GetWeaponAE().AddAudioEvent(AE_WEAPON_CHAINSAW_ACTIVE);
        }

        if (m_nComboSet < MELEE_COMBO_UNARMED_1) { // 0x629C8D - Movement
            const auto lastCommand = (int8)m_nLastCommand;
            if (   (lastCommand == MELEE_CMD_SHUFFLE_LEFT || lastCommand == MELEE_CMD_SHUFFLE_RIGHT)
                && m_pAnim->m_CurrentTime / m_pAnim->m_BlendHier->m_fTotalTime > 0.4f
                && m_pAnim->m_BlendDelta > -4.f
            ) {
                m_pAnim->m_BlendDelta = -4.f;
            } else if (lastCommand == MELEE_CMD_MOVE_FWD) {
                if (GetNextCommand() == MELEE_CMD_MOVE_FWD) {
                    m_nNextCommand = MELEE_CMD_IDLE;
                } else if (m_pAnim->m_BlendDelta > -4.f) {
                    m_pAnim->m_BlendDelta = -4.f;
                }
            }
        } else if ((int8)m_nLastCommand == MELEE_CMD_BLOCK) { // 0x629D13
            const auto& info = GetCurrentComboData();
            if (GetNextCommand() == MELEE_CMD_BLOCK) {
                // Still blocking, pause the anim when it reaches the start/end of the block loop
                if (m_pAnim->IsPlaying()) {
                    const auto t     = m_pAnim->m_CurrentTime;
                    const auto tNext = m_pAnim->m_TimeStep + t;
                    if (   (t < info.m_fBlockLoopStart && tNext >= info.m_fBlockLoopStart)
                        || (t < info.m_fBlockLoopEnd && tNext >= info.m_fBlockLoopEnd)
                    ) {
                        m_pAnim->m_Flags &= ~ANIMATION_IS_PLAYING;
                        m_pAnim->SetCurrentTime(info.m_fBlockLoopStart);
                    }
                }
            } else {
                if (!m_pAnim->IsPlaying() && m_pAnim->m_BlendAmount > 0.f && m_pAnim->m_BlendDelta >= 0.f) {
                    m_pAnim->m_BlendDelta = -4.f;
                }
                if (GetNextCommand() >= MELEE_CMD_ATTACK_1) {
                    m_pAnim->SetDefaultDeleteCallback();
                    m_pAnim = nullptr;
                }
            }
            if (GetNextCommand() == MELEE_CMD_BLOCK) {
                m_nNextCommand = MELEE_CMD_IDLE;
            }
        } else if (m_pAnim->m_BlendAmount > 0.9f && m_pAnim->m_BlendDelta >= 0.f) { // 0x629DCD - Attacking
            const auto& info    = GetCurrentComboData();
            const auto  move    = (int32)m_nCurrentMove;
            const auto  t       = m_pAnim->m_CurrentTime;
            const auto  hitTime = info.m_fHit[move];
            if (t > hitTime && t - m_pAnim->m_TimeStep < hitTime) { // 0x629E23 - The hit happens in this frame
                ped->GetAE().AddAudioEvent(AE_PED_SWING, 0.f, 1.f, nullptr, SURFACE_DEFAULT, info.m_Hit[move], 0);
                if (m_nComboSet == MELEE_COMBO_UNARMED_2 && m_nCurrentMove >= 0 && m_nCurrentMove <= 2) {
                    ped->GetAE().AddAudioEvent(AE_PED_SWING, 0.f, 1.f, nullptr, SURFACE_DEFAULT, info.m_Hit[m_nCurrentMove], AE_DOUBLE_HIT_DELAYS_MS[m_nCurrentMove]);
                }

                if (IsNextCommandAttack()) {
                    m_nNextCommand = MELEE_CMD_IDLE;
                }

                const auto hitLevel = info.m_nHitLevel[m_nCurrentMove];
                if (hitLevel != MELEE_HIT_LEVEL_NUM) {
                    CVector hitPos = ped->m_matrix->TransformPoint(m_aHitOffset[(uint8)hitLevel]);
                    if (m_nCurrentMove == 4) {
                        hitPos += ped->m_vecMoveSpeed * CTimer::GetTimeStep();
                    }
                    FightStrike(ped, hitPos);
                }
            } else if (t >= info.m_fChain[move] && IsNextCommandAttack()) { // 0x629F09 - Chain the next move
                switch (m_nCurrentMove) {
                case 0:
                case 1: { // 0x629FC4
                    m_nComboSet = GetAvailableComboSet(ped, GetNextCommand());
                    StartChosenMove(ped->IsPlayer());
                    break;
                }
                case 3: { // 0x629F35
                    if (ped->IsPlayer() && ChooseAttackPlayer(ped) == 3) {
                        StartAnim(ped, 3);
                    }
                    break;
                }
                case 4: { // 0x629F66
                    if (m_nComboSet == MELEE_COMBO_CHAINSAW && ped->IsPlayer()) { // Keep the chainsaw going
                        m_pAnim->SetCurrentTime(info.m_fHit[m_nCurrentMove] - 0.01f);
                    } else {
                        m_nComboSet = GetAvailableComboSet(ped, GetNextCommand());
                        StartChosenMove(ped->IsPlayer());
                    }
                    break;
                }
                }
            }
        }
    }

    // 0x629FF7 - Face the target
    if (m_pTargetEntity) {
        const auto& targetPos = m_pTargetEntity->GetPosition();
        const auto& pedPos    = ped->GetPosition();
        ped->m_fAimingRotation = std::atan2(-(targetPos.x - pedPos.x), targetPos.y - pedPos.y);
    } else if (ped->IsPlayer() && CCamera::m_bUseMouse3rdPerson && ped->AsPlayer()->GetPadFromPlayer()->GetTarget()) {
        const auto& camFront  = TheCamera.m_aCams[0].m_vecFront;
        ped->m_fAimingRotation = std::atan2(-camFront.x, camFront.y);
    }

    m_bIsInControl = false;

    return false;
}

// 0x61DAE0
void CTaskSimpleFight::FinishMeleeAnimCB(CAnimBlendAssociation* anim, void* data) {
    auto* const self = static_cast<CTaskSimpleFight*>(data);

    if (self->m_pAnim == anim) {
        self->m_pAnim = nullptr;
    } else if (self->m_pIdleAnim == anim) {
        self->m_pIdleAnim = nullptr;
    }

    if (anim->GetAnimId() == ANIM_ID_FIGHT2IDLE) {
        self->m_bIsFinished = true;
    }

    if (!self->m_pIdleAnim) {
        switch (self->m_nLastCommand) {
        case MELEE_CMD_END_RUNAWAY:
        case MELEE_CMD_END_SPRINTAWAY:
        case MELEE_CMD_END_QUICK:
        case MELEE_CMD_END_SLOW:
            self->m_bIsFinished = true;
            break;
        }
    }
}

// 0x61C7F0
int8 CTaskSimpleFight::GetAvailableComboSet(CPed* ped, int8 nextCommand) {
    // Same as the (inlined) original: Use the group's block, or look it up by name if it has none yet.
    const auto GetAnimBlockOfGroup = [](AssocGroupId group) {
        if (auto* const block = CAnimManager::GetAnimationBlock(group)) {
            return block;
        }
        return CAnimManager::GetAnimationBlock(CAnimManager::GetAnimBlockName(group));
    };

    // Add a ref to the required anim block if it's loaded
    const auto TryReferenceRequiredAnims = [&]() -> CAnimBlock* {
        auto* const block = GetAnimBlockOfGroup(m_nRequiredAnimGroup);
        if (block->IsLoaded) {
            CAnimManager::AddAnimBlockRef(CAnimManager::GetAnimationBlockIndex(block));
            m_bAnimsReferenced = true;
        }
        return block;
    };

    const auto IsAttackCommand = [](int8 cmd) {
        return cmd >= MELEE_CMD_ATTACK_1 && cmd <= MELEE_CMD_ATTACK_4;
    };

    if (nextCommand < 0) { // 0x61C7FA - No command, just make sure the anims are referenced if possible
        if (m_nRequiredAnimGroup != ANIM_GROUP_MELEE_1 && !m_bAnimsReferenced) {
            TryReferenceRequiredAnims();
        }
        return MELEE_COMBO_IDLE;
    }

    if (nextCommand != MELEE_CMD_BLOCK && nextCommand != MELEE_CMD_IDLE && !IsAttackCommand(nextCommand)) {
        return MELEE_COMBO_IDLE;
    }

    // 0x61C860
    auto comboSet = (int8)CWeaponInfo::GetWeaponInfo(ped->GetActiveWeapon().m_Type, eWeaponSkill::STD)->m_nBaseCombo;
    const auto CheckOwnIdle = [&] { // 0x61C8B4 - Returns false if the idle command can't use this combo set
        return nextCommand != MELEE_CMD_IDLE || (GetComboData(comboSet).m_wFlags & MELEE_FLAG_OWN_IDLE) != 0;
    };
    if (nextCommand == MELEE_CMD_ATTACK_2) {
        comboSet = (int8)ped->m_nFightingStyle;
    } else if (comboSet == MELEE_COMBO_UNARMED_1) {
        if (nextCommand == MELEE_CMD_BLOCK || nextCommand == MELEE_CMD_IDLE) {
            comboSet = (int8)ped->m_nFightingStyle;
            if (!CheckOwnIdle()) {
                return MELEE_COMBO_UNARMED_1;
            }
        }
    } else if (!CheckOwnIdle()) {
        return MELEE_COMBO_UNARMED_1;
    }

    // 0x61C8D1 - Make sure anims for this combo are loaded
    const auto newAnimGroup = GetComboData(comboSet).m_nAnimGroup;

    // `ANIM_GROUP_MELEE_1` is always loaded
    if (newAnimGroup == ANIM_GROUP_MELEE_1) {
        return comboSet;
    }

    if (newAnimGroup != m_nRequiredAnimGroup) { // 0x61C8F9 - Different group, release old one
        if (m_bAnimsReferenced) {
            CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(m_nRequiredAnimGroup));
            m_bAnimsReferenced = false;
        }
        m_nRequiredAnimGroup = newAnimGroup;
    } else if (m_bAnimsReferenced) { // 0x61C928 - Same group, and already referenced
        return comboSet;
    }

    // 0x61C92F - Try referencing the anims
    auto* const block = TryReferenceRequiredAnims();
    if (m_bAnimsReferenced) {
        return comboSet;
    }

    // 0x61C97A - Not loaded yet, request it, and use the default combo in the meantime
    CStreaming::RequestModel(IFPToModelId(CAnimManager::GetAnimationBlockIndex(block)), STREAMING_KEEP_IN_MEMORY);
    if (IsAttackCommand((int8)m_nNextCommand)) {
        m_nNextCommand = MELEE_CMD_ATTACK_1;
    }
    return MELEE_COMBO_UNARMED_1;
}

// 0x61C9B0
void CTaskSimpleFight::SetPlayerMoveAnim(CPlayerPed* player) {
    auto* const clump = player->GetRpClump();

    auto* fwd   = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_FIGHTSH_FWD);
    auto* left  = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_FIGHTSH_LEFT);
    auto* back  = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_FIGHTSH_BWD);
    auto* right = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_FIGHTSH_RIGHT);

    const auto& movement = player->GetPlayerData()->m_vecFightMovement;

    // 0x61C9F3 - If not moving anymore, blend out all anims
    if (   (m_nNextCommand == MELEE_CMD_IDLE && m_nComboSet == MELEE_COMBO_IDLE)
        || std::sqrt(movement.x * movement.x + movement.y * movement.y) < 0.1f
    ) {
        for (auto* const a : { fwd, left, back, right }) {
            if (a) {
                a->SetBlendDelta(-8.f);
            }
        }

        m_nComboSet    = MELEE_COMBO_IDLE;
        m_nLastCommand = MELEE_CMD_IDLE;
        m_nNextCommand = MELEE_CMD_IDLE;

        return;
    }

    // Set blend amount of the `toBlendIn` anim (it's created if necessary), and zero out the opposing anim
    const auto UpdateAnims = [&](CAnimBlendAssociation* opposing, CAnimBlendAssociation*& toBlendIn, AnimationId toBlendInAnimId, float blendAmount) {
        if (opposing) {
            opposing->SetBlendAmount(0.f);
        }
        if (!toBlendIn) {
            toBlendIn = CAnimManager::AddAnimation(clump, ANIM_GROUP_DEFAULT, toBlendInAnimId);
        }
        toBlendIn->SetBlendAmount(blendAmount);
    };

    // 0x61CA6C - Calculate blend values from the movement vector
    const auto invSum = 1.f / (std::abs(movement.y) + std::abs(movement.x));
    const auto blendX = invSum * movement.x;
    const auto blendY = invSum * movement.y;

    // 0x61CA92 - Process left/right movement (x axis)
    if (blendX > 0.f) {
        UpdateAnims(left, right, ANIM_ID_FIGHTSH_RIGHT, blendX);
    } else if (blendX < 0.f) {
        UpdateAnims(right, left, ANIM_ID_FIGHTSH_LEFT, -blendX);
    }

    // 0x61CB06 - Process forward/backward movement (y axis)
    if (blendY < 0.f) {
        UpdateAnims(back, fwd, ANIM_ID_FIGHTSH_FWD, -blendY);
    } else if (blendY > 0.f) {
        UpdateAnims(fwd, back, ANIM_ID_FIGHTSH_BWD, blendY);
    }

    // 0x61CB7A
    m_nComboSet    = MELEE_COMBO_MOVE;
    m_nLastCommand = m_nNextCommand;
    m_nNextCommand = MELEE_CMD_IDLE;
}

// 0x61CBA0
CPed* CTaskSimpleFight::FightHitPed(CPed* attacker, CPed* victim, CVector& hitPt, CVector& hitDir, int16 hitPieceType) {
    // Player can't be hit while getting up
    if (victim->IsPlayer()) {
        const auto* const activeTask = victim->GetTaskManager().GetActiveTask();
        if (activeTask && activeTask->GetTaskType() == TASK_SIMPLE_GET_UP) { // NOTE: Original code doesn't null check
            return nullptr;
        }
    }

    const auto move = (int32)m_nCurrentMove;

    const auto IsDoubleHit = [&] { // Moves of this combo set hit twice (so the sound is played twice too)
        return m_nComboSet == MELEE_COMBO_UNARMED_2 && move >= FIGHT_ATTACK_HIT_1 && move <= FIGHT_ATTACK_HIT_3;
    };

    const auto PlayHitSound = [&](int32 audioEvent, float volume, uint32 delayMs) {
        attacker->GetAE().AddAudioEvent((eAudioEvents)audioEvent, volume, 1.f, victim, SURFACE_DEFAULT, 0, delayMs);
    };

    // 0x61CBFC - Check if victim has blocked the attack
    if (auto* const victimFight = victim->GetIntelligence()->GetTaskFighting()) {
        if (victimFight->BeHitWhileBlocking(victim, attacker, m_nComboSet, (int8)m_nCurrentMove)) {
            const auto altHitSound = GetCurrentComboData().m_AltHit[move];
            PlayHitSound(altHitSound, -9.f, 0);
            if (IsDoubleHit()) {
                PlayHitSound(altHitSound, -9.f, AE_DOUBLE_HIT_DELAYS_MS[move]);
            }
            return nullptr;
        }
    }

    // 0x61CCBF
    const auto* const combo      = &GetCurrentComboData();
    const auto        weaponType = attacker->GetActiveWeapon().m_Type;
    const auto        damage     = (int32)GetStrikeDamage(attacker);

    m_nContinueStrike = -1;

    const auto victimHitSide    = CPedGeometryAnalyser::ComputePedShotSide(*victim, attacker->GetPosition());
    const auto hasDamagedVictim = CWeapon::GenerateDamageEvent(
        victim,
        attacker,
        weaponType,
        damage,
        (ePedPieceTypes)hitPieceType,
        (uint8)victimHitSide
    );

    if (weaponType == WEAPON_CHAINSAW) { // 0x61CD63
        attacker->GetWeaponAE().AddAudioEvent(AE_WEAPON_CHAINSAW_CUTTING);
    }

    if (victimHitSide == eDirection::FORWARD) { // 0x61CD7C - Hit from the front
        const auto hitSound = combo->m_Hit[move];
        if (IsDoubleHit()) {
            PlayHitSound(hitSound, 0.f, 0);
            PlayHitSound(hitSound, 0.f, AE_DOUBLE_HIT_DELAYS_MS[move]);
        } else if (m_nComboSet == MELEE_COMBO_UNARMED_4 && move == FIGHT_ATTACK_HIT_2) { // 0x61CDC5
            PlayHitSound(hitSound, 0.f, AE_DOUBLE_HIT_DELAYS_MS[move]);
            PlayHitSound(hitSound, 0.f, (uint32)((float)AE_DOUBLE_HIT_DELAYS_MS[move] * 2.8f));
        } else { // 0x61CE31
            PlayHitSound(hitSound, 0.f, 0);
        }
    } else { // 0x61CE4A
        const auto altHitSound = combo->m_AltHit[move];
        PlayHitSound(altHitSound, 0.f, 0);
        if (IsDoubleHit()) {
            PlayHitSound(altHitSound, 0.f, AE_DOUBLE_HIT_DELAYS_MS[move]);
        }
    }

    if (attacker->IsPlayer()) { // 0x61CEA4
        attacker->Say(CTX_GLOBAL_FIGHT);
    }

    // 0x61CEC5
    {
        CEventSoundQuiet event{ attacker, 55.f, (uint32)-1, CVector{ 0.f, 0.f, 0.f } };
        GetEventGlobalGroup()->Add(static_cast<CEvent*>(&event), false);
    }

    // 0x61CF0E - Add blood fx
    const auto IsBladedOrHeavyCombo = [&] {
        switch (m_nComboSet) {
        case MELEE_COMBO_BBALLBAT:
        case MELEE_COMBO_KNIFE:
        case MELEE_COMBO_GOLFCLUB:
        case MELEE_COMBO_SWORD:
        case MELEE_COMBO_CHAINSAW:
            return true;
        }
        return false;
    };
    const auto bloodChance = [&]() -> int32 {
        if (IsBladedOrHeavyCombo()) {
            return 100;
        }
        if (m_nComboSet == MELEE_COMBO_UNARMED_1 && move == FIGHT_ATTACK_FIGHTIDLE) {
            return -1;
        }
        return (int32)(100.f - victim->m_fHealth);
    }();
    if ((int32)((float)CGeneral::GetRandomNumber() * (1.f / 32768.f) * 100.f) < bloodChance) { // 0x61CF52
        const CVector fxPos = hitPt;

        auto fxDir = attacker->GetPosition() - victim->GetPosition();
        fxDir.Normalise();
        if (!victim->IsAlive()) {
            fxDir = CVector{ 0.f, 0.f, 2.f };
        }

        int32 amount = 8;
        if (IsBladedOrHeavyCombo()) {
            amount = 16;
            if (victim->IsAlive()) {
                fxDir *= 1.5f;
            }
        }

        g_fx.AddBlood(fxPos, fxDir, amount, victim->m_fContactSurfaceBrightness);
    }

    return hasDamagedVictim
        ? victim
        : nullptr;
}

// 0x61D0B0
void CTaskSimpleFight::FightHitCar(CPed* attacker, CVehicle* victim, CVector& hitPt, CVector& hitDir, int16 hitPieceType, eSurfaceType hitSurfaceType) {
    const auto originalVehHealth = victim->m_fHealth;
    const auto strikeDmg         = GetStrikeDamage(attacker);
    const auto weaponType        = attacker->GetActiveWeapon().m_Type;

    const auto DoVehicleDamage = [&](float dmgFactor) {
        victim->VehicleDamage(
            victim->m_pHandlingData->m_fMass * strikeDmg * dmgFactor,
            (eVehicleCollisionComponent)hitPieceType,
            attacker,
            &hitPt,
            &hitDir,
            weaponType
        );
    };

    if (weaponType == WEAPON_CHAINSAW) { // 0x61D112
        g_fx.AddSparks(
            hitPt,
            CVector{ *RwMatrixGetAt(attacker->GetBoneMatrix(BONE_R_HAND)) },
            5.f,
            32,
            CVector{ 0.f, 0.f, 0.f },
            SPARK_PARTICLE_SPARK,
            0.3f,
            1.f
        );
        DoVehicleDamage(0.00075f);
    } else { // 0x61D1EC
        DoVehicleDamage(0.01f);
    }

    CCrime::ReportCrime(CRIME_HIT_CAR, victim, attacker);

    // 0x61D233 - Notify occupants
    const auto AddDamageEventTo = [&](CPed* occupant) {
        CEventVehicleDamageWeapon event{ victim, attacker, WEAPON_BASEBALLBAT };
        occupant->GetEventGroup().Add(static_cast<CEvent*>(&event), false);
    };
    if (victim->m_pDriver) {
        AddDamageEventTo(victim->m_pDriver);
    }
    for (auto i = 0u; i < victim->m_nMaxPassengers; i++) {
        if (auto* const passenger = victim->m_apPassengers[i]) {
            AddDamageEventTo(passenger);
        }
    }

    if (victim->m_fHealth < originalVehHealth) { // 0x61D342
        victim->m_nLastWeaponDamageType = (uint8)weaponType;
        victim->m_pLastDamageEntity     = attacker;
        attacker->RegisterReference(&victim->m_pLastDamageEntity);
    }

    if (attacker->GetActiveWeapon().m_Type == WEAPON_CHAINSAW) { // 0x61D370
        attacker->GetWeaponAE().AddAudioEvent(AE_WEAPON_CHAINSAW_CUTTING);
    }
    attacker->GetAE().AddAudioEvent(
        (eAudioEvents)GetCurrentComboData().m_Hit[m_nCurrentMove],
        0.f,
        1.f,
        victim,
        hitSurfaceType
    );
    g_fx.AddPunchImpact(hitPt, hitDir, 4);
}

// 0x61D400
void CTaskSimpleFight::FightHitObj(CPed* attacker, CObject* victim, CVector& hitPt, CVector& hitDir, int16 hitPieceType, eSurfaceType hitSurfaceType) {
    const CVector dir       = hitDir; // Original code makes a copy too
    const auto    strikeDmg = GetStrikeDamage(attacker);

    if (   victim->m_nColDamageEffect < 200
        && !victim->physicalFlags.bDisableCollisionForce
        && victim->m_pObjectInfo->m_fColDamageMultiplier < 99.9f
    ) {
        if (victim->GetIsStatic() && victim->m_pObjectInfo->m_fUprootLimit <= 0.f) { // 0x61D466
            victim->SetIsStatic(false);
            victim->AddToMovingList();
        }
        if (!victim->GetIsStatic()) { // 0x61D494
            victim->ApplyForce(
                dir * (victim->physicalFlags.bDisableZ ? -0.1f : -0.5f),
                hitPt - victim->GetPosition(),
                true
            );
        }
    }

    // 0x61D54F
    victim->ObjectDamage(
        strikeDmg * 10.f,
        &hitPt,
        &dir,
        attacker,
        attacker->GetActiveWeapon().m_Type
    );

    if (attacker->GetActiveWeapon().m_Type == WEAPON_CHAINSAW) {
        attacker->GetWeaponAE().AddAudioEvent(AE_WEAPON_CHAINSAW_CUTTING);
    }
    attacker->GetAE().AddAudioEvent(
        (eAudioEvents)GetCurrentComboData().m_Hit[m_nCurrentMove],
        0.f,
        1.f,
        victim,
        hitSurfaceType
    );
    g_fx.AddPunchImpact(hitPt, dir, 4);
}

// 0x61D5F0
void CTaskSimpleFight::FightSetUpCol(float radius) {
    auto* const cm = &m_sStrikeColModel;
    auto* const cd = &m_sStrikeColData;

    if (!cm->m_pColData) {
        cm->m_pColData    = cd;
        cd->m_pSpheres    = m_sStrikeSpheres.data();
        cd->m_nNumSpheres = (uint16)m_sStrikeSpheres.size();
    }

    cd->m_pSpheres[0].Set(radius, CVector{ 0.f, 0.f, 0.f }, SURFACE_DEFAULT, 0, tColLighting{ 0xFF });

    cm->m_boundSphere.m_vecCenter = CVector{ 0.f, 0.f, 0.f };
    cm->m_boundSphere.m_fRadius   = radius;

    cm->m_boundBox.m_vecMin = CVector{ -radius, -radius, -radius };
    cm->m_boundBox.m_vecMax = CVector{ radius, radius, radius };
}

CTaskSimpleFight* CTaskSimpleFight::Constructor(CEntity* entity, int32 nCommand, uint32 nIdlePeriod) {
    this->CTaskSimpleFight::CTaskSimpleFight(entity, nCommand, nIdlePeriod);
    return this;
}

CTaskSimpleFight* CTaskSimpleFight::Destructor() {
    this->CTaskSimpleFight::~CTaskSimpleFight();
    return this;
}
