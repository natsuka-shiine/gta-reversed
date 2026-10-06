#pragma once

#include "TaskComplex.h"
#include "TaskTimer.h"
#include "Vector.h"

class CPedGroup;

class NOTSA_EXPORT_VTABLE CTaskComplexGangFollower : public CTaskComplex {
public:
    CPedGroup* m_PedGroup;
    CPed*      m_Leader;
    CVector    m_LeaderInitialPos;                       // 0x14 - Leader's position when the standing still offset was last calculated
    CVector    m_OffsetPos;                              // 0x20 - Offset (from the leader) to follow at
    CVector    m_InitialOffsetPos;                       // 0x2C
    float      m_TargetRadius;                           // 0x38
    uint8      m_GrpMemIdx;                              // 0x3C - Index into `CTaskComplexFollowLeaderInFormation::ms_offsets`
    bool       m_AnimsRef : 1 = false;                   // 0x3D (0x1) - Whenever we hold a ref to the `gangs` anim block
    bool       m_LeaveGroup : 1 = false;                 // 0x3D (0x2)
    bool       m_FollowLeader : 1 = true;                // 0x3D (0x4)
    bool       m_IsInPlayersGroup : 1 = false;           // 0x3D (0x8)
    bool       m_IsUsingStandingStillOffsets : 1 = true; // 0x3D (0x10)
    CTaskTimer m_ExhaleTimer;                            // 0x40 - Creates exhale FX when smoking cigs

public:
    static constexpr auto Type = eTaskType::TASK_COMPLEX_GANG_FOLLOWER;

    static constexpr bool ms_bUseClimbing = true; // 0x8D2EDC

    CTaskComplexGangFollower(CPedGroup* pedGroup, CPed* ped, uint8 a4, CVector pos, float a6);
    ~CTaskComplexGangFollower() override;

    eTaskType GetTaskType() const  override{ return Type; }
    CTask* Clone() const override;
    bool MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override;
    CTask* CreateNextSubTask(CPed* ped) override;
    CTask* CreateFirstSubTask(CPed* ped) override;
    CTask* ControlSubTask(CPed* ped) override;

    CVector CalculateOffsetPosition();

private:
    friend void InjectHooksMain();
    static void InjectHooks();
    CTaskComplexGangFollower* Constructor(CPedGroup* pedGroup, CPed* ped, uint8 uint8, CVector pos, float a6) { this->CTaskComplexGangFollower::CTaskComplexGangFollower(pedGroup, ped, uint8, pos, a6); return this; }
    CTaskComplexGangFollower* Destructor() { this->CTaskComplexGangFollower::~CTaskComplexGangFollower(); return this; }
};
VALIDATE_SIZE(CTaskComplexGangFollower, 0x4C);
