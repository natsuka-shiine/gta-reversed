#pragma once

#include "TaskComplex.h"

class CTaskComplexSequence;

enum ePartnerState : int8 {
    PARTNER_STATE_UNK_1 = 1,              //< Initial state, target positions are calculated (by the lead)
    PARTNER_STATE_GOT_TARGET_POS = 2,     //< NOTSA name - Target positions are known
    PARTNER_STATE_GOING_TO_POINT = 3,     //< NOTSA name - Going to the target point, then turning to face the partner
    PARTNER_STATE_AT_POINT = 4,           //< NOTSA name - At the target point, waiting for the partner
    PARTNER_STATE_FINE_TUNING = 5,        //< NOTSA name - Both at the target point, fine tuning the position
    PARTNER_STATE_READY = 6,              //< NOTSA name - Doing the partner sequence(s)
};

class NOTSA_EXPORT_VTABLE CTaskComplexPartner : public CTaskComplex {
public:
    int32         field_C;
    int32         field_10;
    char          m_commandName[32];
    int32         m_taskId;
    CPed*         m_partner;
    float         m_distanceMultiplier;
    CVector       m_point;
    CVector       m_targetPoint;
    bool          m_leadSpeaker;
    ePartnerState m_partnerState;
    int8          m_firstToTargetFlag;
    int8          m_updateDirectionCount;
    bool          m_taskCompleted;
    bool          m_makePedAlwaysFacePartner;
    char          m_animBlockName[16];
    bool          m_requiredAnimsStreamedIn;

public:
    static constexpr auto Type = TASK_COMPLEX_PARTNER;

    CTaskComplexPartner(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, bool makePedAlwaysFacePartner, int8 updateDirectionCount, CVector point);
    ~CTaskComplexPartner() override;

    eTaskType GetTaskType() const override { return Type; }
    CTask*       CreateNextSubTask(CPed* ped) override;
    CTask*       CreateFirstSubTask(CPed* ped) override;
    CTask*       ControlSubTask(CPed* ped) override;
    virtual void StreamRequiredAnims();
    virtual void RemoveStreamedAnims();
    virtual CTaskComplexSequence* GetPartnerSequence() = 0; // Vtable slot 13 - Implemented by all derived classes

    void CalcTargetPositions(CPed* ped, CVector& outPoint, CVector& outPartnerPoint); // 0x681FE0
    int8 GetPartnerState();                                                            // 0x6822B0

    //! NOTSA: The game's class has a `int16 m_timeout` at 0x70 (Used by `ControlSubTask`), but here
    //! all derived classes declare it themselves (as `field_70`), so it's accessed this way.
    int16& GetTimeout() { return *reinterpret_cast<int16*>(reinterpret_cast<uint8*>(this) + 0x70); }

private:
    friend void InjectHooksMain();
    static void InjectHooks();

    CTaskComplexPartner* Constructor(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, bool makePedAlwaysFacePartner, int8 updateDirectionCount, CVector point);
};

VALIDATE_SIZE(CTaskComplexPartner, 0x70);
