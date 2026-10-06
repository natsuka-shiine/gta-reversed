/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include "TaskSimple.h"
#include "ColModel.h"
#include "CollisionData.h"

class CAnimBlendAssociation;
class CEntity;
class CPed;
class CPlayerPed;
class CObject;
class CVehicle;

enum eFightAttackType : int8 {
    FIGHT_ATTACK_HIT_1 = 0,
    FIGHT_ATTACK_HIT_2 = 1,
    FIGHT_ATTACK_HIT_3 = 2,
    FIGHT_ATTACK_FIGHT_BLOCK = 3,
    FIGHT_ATTACK_FIGHTIDLE = 4,
};

//! Commands given to the fight task (see `CTaskSimpleFight::ControlFight`, `m_nNextCommand`, `m_nLastCommand`)
enum eMeleeCommand : int8 {
    MELEE_CMD_NONE           = -1,
    MELEE_CMD_IDLE           = 0,
    MELEE_CMD_END_SLOW       = 1, //!< Used when `MakeAbortable` is called with `LEISURE` priority
    MELEE_CMD_BLOCK          = 2,
    MELEE_CMD_MOVE_FWD       = 3,
    MELEE_CMD_MOVE_LEFT      = 4,
    MELEE_CMD_MOVE_BACK      = 5,
    MELEE_CMD_MOVE_RIGHT     = 6,
    MELEE_CMD_SHUFFLE_FWD    = 7,
    MELEE_CMD_SHUFFLE_LEFT   = 8,
    MELEE_CMD_SHUFFLE_BACK   = 9,
    MELEE_CMD_SHUFFLE_RIGHT  = 10,
    MELEE_CMD_ATTACK_1       = 11,
    MELEE_CMD_ATTACK_2       = 12,
    MELEE_CMD_ATTACK_3       = 13,
    MELEE_CMD_ATTACK_4       = 14,
    MELEE_CMD_END_QUICK      = 15,
    MELEE_CMD_END_RUNAWAY    = 16,
    MELEE_CMD_END_SPRINTAWAY = 17,
    MELEE_CMD_END_DUCK       = 18,
    MELEE_CMD_STEALTH_KILL   = 19,
};

//! Hit levels as defined in `melee.dat`
enum eMeleeHitLevel : int8 {
    MELEE_HIT_LEVEL_HIGH        = 0, // `H`
    MELEE_HIT_LEVEL_LOW         = 1, // `L`
    MELEE_HIT_LEVEL_GROUND      = 2, // `G`
    MELEE_HIT_LEVEL_BEHIND      = 3, // `B`
    MELEE_HIT_LEVEL_HIGH_LONG   = 4, // `HL`
    MELEE_HIT_LEVEL_LOW_LONG    = 5, // `LL`
    MELEE_HIT_LEVEL_GROUND_LONG = 6, // `GL`
    MELEE_HIT_LEVEL_NUM         = 7,
};

//! Bits of `CMeleeInfo::m_wFlags`
enum eMeleeComboFlags : uint16 {
    MELEE_FLAG_ATTACK_2 = 0x1,    //!< (NOTSA name) Combo has a 2nd chained move. See `CTaskSimpleFight::ChooseAttackPlayer`, `CTaskSimpleFight::ChooseAttackAI`
    MELEE_FLAG_ATTACK_3 = 0x2,    //!< (NOTSA name) Combo has a 3rd chained move
    MELEE_FLAG_GROUND   = 0x4,    //!< (NOTSA name) Combo has its own ground attack (move 3)
    MELEE_FLAG_MOVING   = 0x8,    //!< (NOTSA name) Combo has its own running attack (move 4)
    MELEE_FLAG_FALL_1   = 0x10,   //!< Shifted left by the current move (0..4) => 0x10, 0x20, 0x40, 0x80, 0x100. See `CTaskSimpleFight::IsComboSet`
    MELEE_FLAG_BLOCK    = 0x200,  //!< (NOTSA name) Combo has its own block anim. See `CTaskSimpleFight::StartAnim`
    MELEE_FLAG_OWN_IDLE = 0x400,
    MELEE_FLAG_NOFALL_1 = 0x1000, //!< Shifted left by the current move (0..2) => 0x1000, 0x2000, 0x4000. See `CTaskSimpleFight::IsHitComboSet`
};

//! Combo set data loaded from `melee.dat` (One for each `eMeleeCombo` from `MELEE_COMBO_UNARMED_1` up)
class NOTSA_EXPORT_VTABLE CMeleeInfo {
public:
    AssocGroupId         m_nAnimGroup;      // 0x00
    float                m_fRanges;         // 0x04
    std::array<float, 5> m_fHit;            // 0x08 - Anim time (seconds) the hit is fired at (per move)
    std::array<float, 5> m_fChain;          // 0x1C - Anim time (seconds) from which the next move can be chained (per move)
    std::array<float, 5> m_fRadius;         // 0x30
    float                m_fGroundLoop;     // 0x44
    float                m_fBlockLoopStart; // 0x48
    float                m_fBlockLoopEnd;   // 0x4C
    std::array<int8, 5>  m_nHitLevel;       // 0x50 - See `eMeleeHitLevel`
    std::array<uint8, 5> m_nDamage;         // 0x55
    std::array<int32, 5> m_Hit;             // 0x5C - See `eAudioEvents`
    std::array<int32, 5> m_AltHit;          // 0x70 - See `eAudioEvents`
    uint16               m_wFlags;          // 0x84 - See `eMeleeComboFlags`
};
VALIDATE_SIZE(CMeleeInfo, 0x88);
VALIDATE_OFFSET(CMeleeInfo, m_nHitLevel, 0x50);
VALIDATE_OFFSET(CMeleeInfo, m_nDamage, 0x55);
VALIDATE_OFFSET(CMeleeInfo, m_Hit, 0x5C);
VALIDATE_OFFSET(CMeleeInfo, m_wFlags, 0x84);

class NOTSA_EXPORT_VTABLE CTaskSimpleFight : public CTaskSimple {
public:
    bool                   m_bIsFinished;
    bool                   m_bIsInControl;
    bool                   m_bAnimsReferenced;
    AssocGroupId           m_nRequiredAnimGroup;
    uint16                 m_nIdlePeriod;
    uint16                 m_nIdleCounter;
    int8                   m_nContinueStrike;
    int8                   m_nChainCounter;
    CEntity*               m_pTargetEntity;
    CAnimBlendAssociation* m_pAnim;
    CAnimBlendAssociation* m_pIdleAnim;
    int8                   m_nComboSet;
    eFightAttackType       m_nCurrentMove;
    uint8                  m_nNextCommand;
    uint8                  m_nLastCommand;

    static inline auto& m_aComboData      = StaticRef<std::array<CMeleeInfo, 13>>(0xC170D0); // Indexed by `eMeleeCombo - MELEE_COMBO_UNARMED_1`
    static inline auto& m_aHitOffset      = StaticRef<std::array<CVector, 7>>(0xC177D0);     // Indexed by `eMeleeHitLevel`
    static inline auto& m_sStrikeColModel = StaticRef<CColModel>(0xC17824);
    static inline auto& m_sStrikeColData  = StaticRef<CCollisionData>(0xC17854);
    static inline auto& m_sStrikeSpheres  = StaticRef<std::array<CColSphere, 1>>(0xC17884);

public:
    static constexpr auto Type = eTaskType::TASK_SIMPLE_FIGHT;

    CTaskSimpleFight(CEntity* entity, int32 nCommand, uint32 nIdlePeriod = 10000);
    ~CTaskSimpleFight() override;

    eTaskType GetTaskType() const override { return Type; }
    CTask* Clone() const override { return new CTaskSimpleFight(m_pTargetEntity, m_nLastCommand, m_nIdlePeriod); }
    bool MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override;
    bool ProcessPed(CPed* ped) override;

    static void LoadMeleeData();

    bool BeHitWhileBlocking(CPed* ped, CPed* attacker, int8 attackerComboSet, int8 attackerMove);
    int16 ChooseAttackAI(CPed* ped);     //!< Returns the move to start (see `StartAnim`)
    int16 ChooseAttackPlayer(CPed* ped); //!< Returns the move to start (see `StartAnim`)
    bool ControlFight(CEntity* entity, uint8 command);

    void  FightHitCar(CPed* attacker, CVehicle* victim, CVector& hitPt, CVector& hitDir, int16 hitPieceType, eSurfaceType hitSurfaceType);
    void  FightHitObj(CPed* attacker, CObject* victim, CVector& hitPt, CVector& hitDir, int16 hitPieceType, eSurfaceType hitSurfaceType);
    CPed* FightHitPed(CPed* attacker, CPed* victim, CVector& hitPt, CVector& hitDir, int16 hitPieceType);
    void  FightSetUpCol(float radius);
    bool  FightStrike(CPed* ped, CVector& posn); //!< Always returns `false`. NOTE: `posn` might be modified (for long hit levels)

    bool FindTargetOnGround(CPed* ped); //!< Returns whether there's a target on the ground that can be attacked
    static void FinishMeleeAnimCB(CAnimBlendAssociation* anim, void* data);

    bool IsComboSet();
    bool IsHitComboSet();

    //! Combo data for the given combo set (`eMeleeCombo`). Sets below `MELEE_COMBO_UNARMED_1` use the first entry.
    static CMeleeInfo& GetComboData(int32 comboSet) { return m_aComboData[std::max(comboSet - (int32)MELEE_COMBO_UNARMED_1, 0)]; }
    CMeleeInfo&        GetCurrentComboData() const { return GetComboData(m_nComboSet); }

    int8                   GetAvailableComboSet(CPed* ped, int8 nextCommand);
    static eMeleeCombo     GetComboType(const char* comboName);
    AssocGroupId           GetComboAnimGroupID();
    static eMeleeHitLevel  GetHitLevel(const char* hitLevel);
    static eAudioEvents    GetHitSound(int32 hitSound);
    float                  GetRange();
    float                  GetStrikeDamage(CPed* ped);

    void SetPlayerMoveAnim(CPlayerPed* player);
    void StartAnim(CPed* ped, int32 newMove);

private:
    friend void InjectHooksMain();
    static void InjectHooks();

    CTaskSimpleFight* Constructor(CEntity* entity, int32 nCommand, uint32 nIdlePeriod);
    CTaskSimpleFight* Destructor();

};
VALIDATE_SIZE(CTaskSimpleFight, 0x28);
