#include "StdInc.h"

#include "Ragdoll/IKChainManager.h"
#include "ModelIndices.h"
#include "EventPassObject.h"
#include "TaskComplexGangFollower.h"
#include "TaskComplexGangLeader.h"
#include "TaskComplexFollowLeaderInFormation.h"
#include "TaskComplexFollowNodeRoute.h"
#include "TaskComplexLeaveCar.h"
#include "TaskComplexPassObject.h"
#include "TaskComplexPlayHandSignalAnim.h"
#include "TaskComplexSeekEntity.h"
#include "SeekEntity/PosCalculators/EntitySeekPosCalculatorXYOffset.h"
#include "TaskComplexSignalAtPed.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskComplexWanderGang.h"
#include "TaskSimpleCarDrive.h"
#include "TaskSimpleGoToPoint.h"
#include "TaskSimpleHoldEntity.h"
#include "TaskSimplePause.h"
#include "TaskSimpleRunAnim.h"
#include "TaskSimpleStandStill.h"

namespace {
//! NOTSA - Create the task used for following the leader (Code shared by `CreateNextSubTask` and `CreateFirstSubTask`)
CTask* CreateSeekLeaderTask(CPed* leader, const CVector& offset, bool bFaceLeaderWhenDone) {
    const auto task = new CTaskComplexSeekEntity<CEntitySeekPosCalculatorXYOffset>{
        leader,
        50'000,
        1'000,
        0.5f,
        5.f,
        2.f,
        false,
        bFaceLeaderWhenDone,
        CEntitySeekPosCalculatorXYOffset{ offset }
    };
    task->SetIsTrackingEntity(true);
    task->SetMoveState(PEDMOVE_SPRINT);
    return task;
}
} // namespace

void CTaskComplexGangFollower::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexGangFollower, 0x86F938, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x65EAA0);
    RH_ScopedInstall(Destructor, 0x65EBB0);
    RH_ScopedInstall(CalculateOffsetPosition, 0x65ED40); // Original takes a `CVector*` out param and returns it, which (for the MSVC x86 ABI) is the same as returning a `CVector` by value
    RH_ScopedInstall(Clone, 0x65ECB0);
    RH_ScopedInstall(MakeAbortable, 0x65EC30);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x665E00);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x666160);
    RH_ScopedVMTInstall(ControlSubTask, 0x662A10);
}

// 0x65EAA0
CTaskComplexGangFollower::CTaskComplexGangFollower(CPedGroup* pedGroup, CPed* ped, uint8 a4, CVector pos, float a6) : CTaskComplex() {
    m_PedGroup = pedGroup;
    m_GrpMemIdx = a4;
    m_InitialOffsetPos = pos;
    m_OffsetPos = pos;
    m_TargetRadius = a6;
    m_Leader = ped;
    m_FollowLeader = true;
    m_IsUsingStandingStillOffsets = true;
    if (ped) {
        CEntity::RegisterReference(m_Leader);
        m_LeaderInitialPos = ped->GetPosition();
    }
    m_AnimsRef = false;
    m_LeaveGroup = false;
    m_IsInPlayersGroup = m_Leader == FindPlayerPed(0);
}


// 0x65EBB0
CTaskComplexGangFollower::~CTaskComplexGangFollower() {
    if (m_Leader) {
        CEntity::CleanUpOldReference(m_Leader);
    }
    if (m_AnimsRef) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_AnimsRef = false;
    }
}

// 0x65ED40
CVector CTaskComplexGangFollower::CalculateOffsetPosition() {
    const auto& offsets = CTaskComplexFollowLeaderInFormation::ms_offsets;
    if (notsa::contains({ PEDMOVE_WALK, PEDMOVE_RUN, PEDMOVE_SPRINT }, m_Leader->m_nMoveState)) { // 0x65EE22
        m_OffsetPos.FromMultiply3x3(m_Leader->GetMatrix(), CVector{ offsets.MovingOffsets[m_GrpMemIdx] });
        m_IsUsingStandingStillOffsets = false;
    } else if (!m_IsUsingStandingStillOffsets || (m_Leader->GetPosition() - m_LeaderInitialPos).SquaredMagnitude() > sq(3.f)) { // 0x65ED8C
        m_LeaderInitialPos            = m_Leader->GetPosition();
        m_OffsetPos                   = CVector{ offsets.Offsets[m_GrpMemIdx] };
        m_IsUsingStandingStillOffsets = true;
    }
    return m_OffsetPos;
}

// 0x65ECB0
CTask* CTaskComplexGangFollower::Clone() const {
    const auto clone = new CTaskComplexGangFollower{ m_PedGroup, m_Leader, m_GrpMemIdx, m_InitialOffsetPos, m_TargetRadius };
    clone->m_FollowLeader = m_FollowLeader;
    return clone;
}
// 0x65EC30
bool CTaskComplexGangFollower::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (!m_pSubTask) {
        ped->bDontAcceptIKLookAts = false;
        ped->bMoveAnimSpeedHasBeenSetByTask = false;
        return true;
    }
    if (!m_pSubTask->MakeAbortable(ped, priority, event)) {
        return false;
    }
    ped->bMoveAnimSpeedHasBeenSetByTask = false;
    ped->bDontAcceptIKLookAts = false;
    return true;
}

// 0x665E00
CTask* CTaskComplexGangFollower::CreateNextSubTask(CPed* ped) {
    if (!m_Leader) {
        ped->bMoveAnimSpeedHasBeenSetByTask = false;
        return nullptr;
    }

    const auto subTaskType = m_pSubTask->GetTaskType();

    // We've signalled at the leader that we're leaving, so do it now
    if (m_LeaveGroup && subTaskType == TASK_COMPLEX_SIGNAL_AT_PED) { // 0x665E70
        m_PedGroup->GetMembership().RemoveMember(ped);
        ped->GetTaskManager().SetTask(
            new CTaskComplexWanderGang{
                PEDMOVE_WALK,
                CGeneral::RandomNodeHeading(),
                30'000,
                true,
                0.5f
            },
            TASK_PRIMARY_DEFAULT
        );
        ped->bMoveAnimSpeedHasBeenSetByTask = false;
        return nullptr;
    }

    if (   ped->GetIntelligence()->m_AnotherStaticCounter > 30
        || (subTaskType == TASK_COMPLEX_SEEK_ENTITY && m_Leader->m_nMoveState < PEDMOVE_WALK)
    ) { // 0x666107
        if (m_Leader->IsPlayer()) {
            ped->Say(CTX_GLOBAL_FOLLOW_ARRIVE, 0, 0.3f);
        }
        return new CTaskSimpleStandStill{ 500 };
    }

    if (notsa::contains({ TASK_SIMPLE_STAND_STILL, TASK_COMPLEX_HANDSIGNAL_ANIM }, subTaskType)) { // 0x666023
        if (m_LeaveGroup) {
            return new CTaskComplexSignalAtPed{ m_Leader, -1, false };
        }
        if (CGeneral::GetRandomNumberInRange(0, 30) == 20) { // 0x66605A
            auto& membership = m_PedGroup->GetMembership();
            auto  member     = membership.GetMember(CGeneral::GetRandomNumberInRange(0, (int32)membership.CountMembers()));
            if (member == ped) {
                member = membership.GetLeader();
            }
            if (member) {
                return new CTaskComplexTurnToFaceEntityOrCoord{ member };
            }
        }
        return new CTaskSimplePause{ 50 };
    }

    if (subTaskType == TASK_SIMPLE_CAR_DRIVE) { // 0x665F6D
        return CreateFirstSubTask(ped);
    }

    if (!m_FollowLeader) { // 0x666008
        return new CTaskSimpleStandStill{ 500 };
    }

    return CreateSeekLeaderTask(m_Leader, m_OffsetPos, false); // 0x665F81
}

// 0x666160
CTask* CTaskComplexGangFollower::CreateFirstSubTask(CPed* ped) {
    if (!m_Leader) {
        ped->bMoveAnimSpeedHasBeenSetByTask = false;
        return nullptr;
    }

    if (ped->IsInVehicle()) {
        if (m_Leader->m_pVehicle == ped->m_pVehicle) { // 0x6661C7
            return new CTaskSimpleCarDrive{ ped->m_pVehicle };
        }
        return new CTaskComplexLeaveCar{ ped->m_pVehicle, 0, 0, true, false }; // 0x666214
    }

    if (m_FollowLeader && ped->GetIntelligence()->m_AnotherStaticCounter <= 30) { // 0x666286
        return CreateSeekLeaderTask(m_Leader, m_OffsetPos, true);
    }

    return new CTaskSimpleStandStill{ 500 }; // 0x66630F
}

// 0x662A10
CTask* CTaskComplexGangFollower::ControlSubTask(CPed* ped) {
    ped->bDontAcceptIKLookAts = false;

    if (!m_Leader) { // 0x662A44
        if (m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            ped->bMoveAnimSpeedHasBeenSetByTask = false;
            return nullptr;
        }
        return m_pSubTask;
    }

    if (m_pSubTask->GetTaskType() == TASK_SIMPLE_CAR_DRIVE) {
        return m_pSubTask;
    }

    const auto walkAnim       = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_WALK);
    const auto isLeaderMoving = notsa::contains({ PEDMOVE_WALK, PEDMOVE_RUN, PEDMOVE_SPRINT }, m_Leader->GetIntelligence()->GetMoveStateFromGoToTask());

    CalculateOffsetPosition(); // Updates `m_OffsetPos`

    CVector pedToLeader2D = m_Leader->GetPosition() - ped->GetPosition();
    pedToLeader2D.z       = 0.f;

    bool hasSetMoveAnimSpeed = false;
    if (m_pSubTask->GetTaskType() == TASK_COMPLEX_SEEK_ENTITY && m_FollowLeader) { // 0x662B5B
        const auto tSeekEntity = static_cast<CTaskComplexSeekEntity<CEntitySeekPosCalculatorXYOffset>*>(m_pSubTask);
        tSeekEntity->GetSeekPosCalculator().SetOffset(m_OffsetPos);
        tSeekEntity->SetEntityMinDist2D(2.f);

        // Adjust the walk anim's speed to keep up with the leader, and update the point we're going to
        const auto tGoToPoint = ped->GetTaskManager().Find<CTaskSimpleGoToPoint>();
        if (tGoToPoint && !ped->GetTaskManager().Find<CTaskComplexFollowNodeRoute>()) { // 0x662BD3
            CVector goToPoint = m_Leader->GetPosition() + m_OffsetPos;
            CVector pedToGoToPoint2D = goToPoint - ped->GetPosition();
            pedToGoToPoint2D.z       = 0.f;
            if (isLeaderMoving && (ped->GetPosition() - goToPoint).Dot(m_Leader->GetForward()) < 0.f) { // 0x662C34 - Ped is behind the point
                if (pedToGoToPoint2D.SquaredMagnitude() > sq(tGoToPoint->m_fRadius + 1.f)) { // 0x662D0E
                    if (walkAnim) {
                        const auto prevSpeed = walkAnim->GetSpeed();
                        ped->SetMoveAnimSpeed(walkAnim);
                        const auto newSpeed = walkAnim->GetSpeed();
                        if (approxEqual(prevSpeed, newSpeed, 0.013f)) {
                            walkAnim->SetSpeed(prevSpeed);
                        } else { // Gradually change the speed
                            walkAnim->SetSpeed(prevSpeed <= newSpeed ? prevSpeed + 0.0125f : prevSpeed - 0.0125f);
                        }
                        hasSetMoveAnimSpeed = true;
                    }
                } else { // 0x662CBB - Close to the point, so slow down
                    goToPoint += m_Leader->GetForward() * 2.f;
                    if (walkAnim) {
                        walkAnim->SetSpeed(std::max(0.85f, walkAnim->GetSpeed() - 0.0125f));
                        hasSetMoveAnimSpeed = true;
                    }
                }
            }
            tGoToPoint->UpdatePoint(goToPoint, 0.5f, false); // 0x662D88
        }

        if (   ped->GetIntelligence()->m_AnotherStaticCounter > 8
            && pedToLeader2D.SquaredMagnitude() < sq(8.f)
            && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)
        ) { // 0x662D9F
            m_OffsetPos = pedToLeader2D;
            return new CTaskSimpleStandStill{ 500 };
        }
    }
    ped->bMoveAnimSpeedHasBeenSetByTask = hasSetMoveAnimSpeed; // 0x662E10

    // Make sure anims are loaded (if they can/need to be)
    if (m_AnimsRef) { // 0x662E2E
        if (!CTaskComplexGangLeader::ShouldLoadGangAnims()) {
            CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
            m_AnimsRef = false;
        }
    } else if (CTaskComplexGangLeader::ShouldLoadGangAnims()) {
        const auto blk = CAnimManager::GetAnimationBlockIndex("gangs");
        if (CAnimManager::GetAnimBlocks()[blk].IsLoaded) {
            CAnimManager::AddAnimBlockRef(blk);
            m_AnimsRef = true;
        } else {
            CStreaming::RequestModel(IFPToModelId(blk), STREAMING_KEEP_IN_MEMORY);
        }
    }

    // Randomly decide to leave the group
    if (   !m_PedGroup->m_bIsMissionGroup
        && !m_IsInPlayersGroup
        && m_PedGroup->GetMembership().CountMembers() > 3
        && CGeneral::GetRandomNumberInRange(0, 2000) == 500
    ) { // 0x662EA3
        m_LeaveGroup = true;
    }

    if (m_ExhaleTimer.IsStarted() && m_ExhaleTimer.IsOutOfTime()) { // 0x662EDD
        if (ped->GetRpClump()) {
            if (const auto matrix = RwFrameGetMatrix(RpClumpGetFrame(ped->GetRpClump()))) {
                if (const auto fx = g_fxMan.CreateFxSystem("exhale", CVector{ 0.f, 0.1f, 0.f }, matrix, false)) {
                    fx->AttachToBone(ped, BONE_HEAD);
                    fx->PlayAndKill();
                }
                m_ExhaleTimer.Stop();
            }
        }
    }

    if (!ped->IsVisible()) { // 0x662F4B
        return m_pSubTask;
    }

    // If ped isn't already looking at someone, find a random member to look at
    if (!g_ikChainMan.IsLooking(ped) && CGeneral::GetRandomNumberInRange(0, 100) > 95) { // 0x662F60
        const auto lookAtTime = CGeneral::GetRandomNumberInRange(3000, 5000);
        auto       lookAtPed  = m_PedGroup->GetMembership().GetMember(CGeneral::GetRandomNumberInRange(0, TOTAL_PED_GROUP_MEMBERS));
        if (lookAtPed == ped) {
            lookAtPed = m_Leader;
        }
        if (lookAtPed) {
            g_ikChainMan.LookAt(
                "TaskGangFollower",
                ped,
                lookAtPed,
                lookAtTime,
                BONE_HEAD,
                nullptr,
                true,
                0.15f,
                500,
                3,
                false
            );
        }
    }

    if (!m_AnimsRef || ped->IsRunningOrSprinting()) { // 0x662FFA
        return m_pSubTask;
    }

    const auto pedHeldEntity = ped->GetEntityThatThisPedIsHolding();

    if (!pedHeldEntity) { // 0x6632D4
        // If they're already playing an anim, early out
        if (ped->IsPlayingHandSignal() || ped->GetTaskManager().GetTaskSecondary(TASK_SECONDARY_PARTIAL_ANIM)) {
            return m_pSubTask;
        }

        // Otherwise (maybe) create a new partial anim task
        const auto rnd = CGeneral::GetRandomNumberInRange(0, 500);
        if (rnd > 50 && rnd < 56) { // 0x66331B
            ped->GetTaskManager().SetTaskSecondary(
                new CTaskSimpleRunAnim{ ANIM_GROUP_GANGS, CAnimManager::GetRandomGangTalkAnim() },
                TASK_SECONDARY_PARTIAL_ANIM
            );
            if (ped->m_nMoveState != PEDMOVE_STILL) {
                return m_pSubTask;
            }
        } else if (rnd == 100 && ped->m_nMoveState == PEDMOVE_STILL) { // 0x6633D3
            ped->GetTaskManager().SetTaskSecondary(new CTaskComplexPlayHandSignalAnim{}, TASK_SECONDARY_PARTIAL_ANIM);
        } else {
            return m_pSubTask;
        }

        // And say something (0x66339A and 0x663418)
        switch (CGeneral::GetRandomNumberInRange(0, 10)) {
        case 0:
        case 1:
        case 2:
            ped->Say(CTX_GLOBAL_CHAT);
            break;
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            ped->Say(CTX_GLOBAL_PCONV_GREET_MALE);
            break;
        case 8:
            ped->Say(CTX_GLOBAL_BOOZE_REQUEST);
            break;
        case 9:
            ped->Say(CTX_GLOBAL_SPLIFF_REQUEST);
            break;
        }
        return m_pSubTask;
    }

    // 0x663024
    const auto GetAnim = [ped](AnimationId id) { return RpAnimBlendClumpGetAssociation(ped->GetRpClump(), id); };

    const auto drnkBrAnim  = GetAnim(ANIM_ID_DRNKBR_PRTL);
    const auto smkCigAnim  = GetAnim(ANIM_ID_SMKCIG_PRTL);
    const auto drnkBrAnimF = GetAnim(ANIM_ID_DRNKBR_PRTL_F);
    const auto smkCigAnimF = GetAnim(ANIM_ID_SMKCIG_PRTL_F);

    const bool anyOfTheAnimsPlaying = drnkBrAnim || smkCigAnim || drnkBrAnimF || smkCigAnimF;

    // If any of the anims are playing, stop looking, start exhale timer of smkcig anims
    if (anyOfTheAnimsPlaying) { // 0x663086
        if (g_ikChainMan.IsLooking(ped)) {
            g_ikChainMan.AbortLookAt(ped, 250);
        }

        ped->bDontAcceptIKLookAts = true;

        // Start exhale timer (for smkcig anims)
        if ((smkCigAnim && smkCigAnim->m_CurrentTime < 0.5f) || (smkCigAnimF && smkCigAnimF->m_CurrentTime < 0.5f)) {
            if (!m_ExhaleTimer.IsStarted()) {
                m_ExhaleTimer.Start(2700);
            }
        }
    }

    // Now, pass on the entity held in hand (if not already)

    if (ped->GetTaskManager().Find<CTaskComplexPassObject>()) { // 0x6630FA
        return m_pSubTask;
    }

    if (CGeneral::GetRandomNumberInRange(0, 500) != 200) { // 0x663115
        if (CGeneral::GetRandomNumberInRange(0, 100) == 50) { // 0x663285
            if (const auto tHoldEntity = ped->GetTaskManager().Find<CTaskSimpleHoldEntity>()) {
                tHoldEntity->PlayAnim(CTaskComplexGangLeader::GetRandomGangAmbientAnim(ped, pedHeldEntity), ANIM_GROUP_GANGS);
            }
        }
        return m_pSubTask;
    }

    if (anyOfTheAnimsPlaying) { // 0x66312E
        return m_pSubTask;
    }

    const auto passObjTo = CTaskComplexGangLeader::TryToPassObject(ped, m_PedGroup); // 0x663156
    if (!passObjTo || passObjTo->GetEntityThatThisPedIsHolding() || !passObjTo->IsCurrentlyUnarmed()) {
        return m_pSubTask;
    }

    if (pedHeldEntity->m_nModelIndex == ModelIndices::MI_GANG_DRINK) { // 0x663195
        if (CGeneral::GetRandomNumberInRange(0, 500) >= 250) {
            ped->Say(CTX_GLOBAL_BOOZE_RECEIVE, 1500);
        } else {
            passObjTo->Say(CTX_GLOBAL_BOOZE_REQUEST);
        }
    } else if (pedHeldEntity->m_nModelIndex == ModelIndices::MI_GANG_SMOKE) { // 0x6631D5
        if (CGeneral::GetRandomNumberInRange(0, 500) >= 250) {
            ped->Say(CTX_GLOBAL_SPLIFF_RECEIVE, 1500);
        } else {
            passObjTo->Say(CTX_GLOBAL_SPLIFF_REQUEST);
        }
    }

    passObjTo->GetEventGroup().Add(CEventPassObject{ ped }); // 0x66321D
    return new CTaskComplexPassObject{ passObjTo, true };
}
