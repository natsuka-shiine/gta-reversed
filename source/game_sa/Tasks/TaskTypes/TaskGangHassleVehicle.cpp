#include "StdInc.h"

#include "TaskGangHassleVehicle.h"
#include "TaskComplexTrackEntity.h"
#include "TaskComplexSmartFleeEntity.h"
#include "TaskComplexLeaveCar.h"
#include "TaskComplexKillPedOnFoot.h"
#include "TaskComplexPlayHandSignalAnim.h"
#include "TaskComplexGangLeader.h"
#include "TaskGangHasslePed.h"
#include "TaskSimpleRunAnim.h"
#include "TaskSimpleShakeFist.h"
#include "TaskSimpleFight.h"
#include "Ragdoll/IKChainManager.h"

void CTaskGangHassleVehicle::InjectHooks() {
    RH_ScopedVirtualClass(CTaskGangHassleVehicle, 0x86F9D4, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x65FAC0);
    RH_ScopedInstall(Destructor, 0x65FB60);
    RH_ScopedInstall(GetTargetHeading, 0x65FDD0);
    RH_ScopedInstall(CalcTargetOffset, 0x6641A0);
    RH_ScopedInstall(Clone, 0x65FC00);
    RH_ScopedInstall(CreateNextSubTask, 0x65FC80);
    RH_ScopedInstall(CreateFirstSubTask, 0x664BA0);
    RH_ScopedInstall(ControlSubTask, 0x6637C0);
}

// 0x65FAC0
CTaskGangHassleVehicle::CTaskGangHassleVehicle(CVehicle* vehicle, int32 a3, uint8 a4, float a5, float a6) : CTaskComplex() {
    m_nTime = 0;
    dword3C = 0;
    byte40 = 0;
    byte41 = 0;
    byte18 = a4;
    dword1C = a5;
    m_Vehicle = vehicle;
    m_nHasslePosId = -1;
    m_fOffsetX = a6;
    m_bRemoveAnim = 0;
    m_pEntity = nullptr;
    CEntity::SafeRegisterRef(m_Vehicle);
}

// 0x65FB60
CTaskGangHassleVehicle::~CTaskGangHassleVehicle() {
    if (m_Vehicle) {
        if (m_nHasslePosId > -1) {
            m_Vehicle->SetHasslePosId(m_nHasslePosId, false);
        }
        CEntity::SafeCleanUpRef(m_Vehicle);
    }

    CEntity::SafeCleanUpRef(m_pEntity);

    if (m_bRemoveAnim) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_bRemoveAnim = false;
    }
}

// 0x65FDD0
float CTaskGangHassleVehicle::GetTargetHeading(CPed* ped) {
    UNUSED(ped);
    const auto& right   = m_Vehicle->GetRight();
    const auto& forward = m_Vehicle->GetForward();
    float x = right.x;
    float y = right.y;
    switch (m_nHasslePosId) {
    case 0:
    case 2:
        x = right.x;
        y = right.y;
        break;
    case 1:
    case 3:
        x = -right.x;
        y = -right.y;
        break;
    case 4:
        x = forward.x;
        y = forward.y;
        break;
    case 5:
        x = -forward.x;
        y = -forward.y;
        break;
    default:
        break;
    }
    return CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(x, y, 0.0f, 0.0f));
}

// 0x6641A0
void CTaskGangHassleVehicle::CalcTargetOffset() {
    m_vecPosn = CVector{};
    const auto& bbox = CModelInfo::ms_modelInfoPtrs[m_Vehicle->m_nModelIndex]->GetColModel()->GetBoundingBox();
    const float minX = bbox.m_vecMin.x;
    const float minY = bbox.m_vecMin.y;
    const float maxX = bbox.m_vecMax.x;
    const float maxY = bbox.m_vecMax.y;
    switch (m_nHasslePosId) {
    case 0:
        m_vecPosn.x = minX - m_fOffsetX;
        m_vecPosn.y = maxY * 0.5f;
        break;
    case 1:
        m_vecPosn.x = maxX + m_fOffsetX;
        m_vecPosn.y = maxY * 0.5f;
        break;
    case 2:
        m_vecPosn.x = minX - m_fOffsetX;
        m_vecPosn.y = minY * 0.5f;
        break;
    case 3:
        m_vecPosn.x = maxX + m_fOffsetX;
        m_vecPosn.y = minY * 0.5f;
        break;
    case 4:
        m_vecPosn.y = minY - m_fOffsetX;
        break;
    case 5:
        m_vecPosn.y = maxY + m_fOffsetX;
        break;
    default:
        break;
    }
}

// 0x65FC80
CTask* CTaskGangHassleVehicle::CreateNextSubTask(CPed* ped) {
    UNUSED(ped);
    if (!m_Vehicle) {
        return nullptr;
    }
    if (m_pSubTask && m_pSubTask->GetTaskType() == TASK_COMPLEX_SMART_FLEE_ENTITY) {
        return nullptr;
    }
    if (m_Vehicle->m_fHealth >= 250.0f) {
        if (m_pSubTask && m_pSubTask->GetTaskType() == TASK_COMPLEX_GANG_HASSLE_PED) {
            return nullptr;
        }
        if (m_pSubTask && m_pSubTask->GetTaskType() == TASK_COMPLEX_TRACK_ENTITY) {
            return nullptr;
        }
        return new CTaskComplexTrackEntity{ m_Vehicle, m_vecPosn, 1, -1, 10.0f, 40.0f, 1 };
    }
    return new CTaskComplexSmartFleeEntity{ m_Vehicle, false, 28.0f, 1'000'000, 1000, StaticRef<float, 0xC18CF0>() };
}

// 0x664BA0
CTask* CTaskGangHassleVehicle::CreateFirstSubTask(CPed* ped) {
    if (!m_Vehicle) {
        return nullptr;
    }
    m_pEntity = m_Vehicle->m_pDriver;
    if (m_pEntity) {
        CEntity::RegisterReference(m_pEntity);
    }
    const auto& bbox = CModelInfo::ms_modelInfoPtrs[m_Vehicle->m_nModelIndex]->GetColModel()->GetBoundingBox();
    if (bbox.m_vecMax.x - bbox.m_vecMin.x > 4.0f || bbox.m_vecMax.y - bbox.m_vecMin.y > 8.0f) {
        return nullptr;
    }
    m_nHasslePosId = m_Vehicle->GetSpareHasslePosId();
    if (m_nHasslePosId == -1) {
        return nullptr;
    }
    m_Vehicle->SetHasslePosId(m_nHasslePosId, true);
    CalcTargetOffset();
    m_b31 = false;
    ped->DropEntityThatThisPedIsHolding(true);
    m_nTime = CTimer::GetTimeInMS();
    dword3C = CGeneral::GetRandomNumberInRange(150000, 250000);
    byte40 = true;
    if (!ped->bInVehicle || !ped->m_pVehicle) {
        return CreateNextSubTask(ped);
    }
    return new CTaskComplexLeaveCar{ ped->m_pVehicle, 0, 0, true, false };
}

// 0x6637C0
CTask* CTaskGangHassleVehicle::ControlSubTask(CPed* ped) {
    // 0x6637E0 - Vehicle is gone
    if (m_pSubTask && m_pSubTask->GetTaskType() != TASK_COMPLEX_KILL_PED_ON_FOOT && !m_Vehicle) {
        return m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)
            ? nullptr
            : m_pSubTask;
    }

    // 0x66381D
    if (m_bRemoveAnim) {
        if (!CTaskComplexGangLeader::ShouldLoadGangAnims()) {
            CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
            m_bRemoveAnim = false;
        }
    } else if (CTaskComplexGangLeader::ShouldLoadGangAnims()) {
        const auto blk = CAnimManager::GetAnimationBlockIndex("gangs");
        if (CAnimManager::GetAnimBlocks()[blk].IsLoaded) {
            CAnimManager::AddAnimBlockRef(blk);
            m_bRemoveAnim = true;
        } else {
            CStreaming::RequestModel(IFPToModelId(blk), STREAMING_KEEP_IN_MEMORY);
        }
    }

    // 0x663888 - Got bored of hassling, attack the driver [Inlined `CTaskTimer::IsOutOfTime`]
    if (byte40) {
        if (byte41) {
            m_nTime = CTimer::GetTimeInMS();
            byte41  = false;
        }
        if (CTimer::GetTimeInMS() >= m_nTime + (uint32)dword3C && byte18) {
            if (m_pSubTask->GetTaskType() == TASK_COMPLEX_KILL_PED_ON_FOOT) {
                return m_pSubTask;
            }
            return new CTaskComplexKillPedOnFoot{ static_cast<CPed*>(m_pEntity) };
        }
    }

    // 0x663902 - Driver has left the vehicle, hassle them instead
    if (m_Vehicle && !m_Vehicle->m_pDriver && m_pEntity) {
        if (m_pSubTask->GetTaskType() != TASK_COMPLEX_GANG_HASSLE_PED && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            m_b31 = 3;
            return new CTaskGangHasslePed{ static_cast<CPed*>(m_pEntity), byte18 ? 2 : 1, 12'000, 20'000 };
        }
    }

    // 0x6639D1
    auto distToTargetSq = 100.f;
    if (m_pSubTask->GetTaskType() == TASK_COMPLEX_TRACK_ENTITY) {
        distToTargetSq = static_cast<CTaskComplexTrackEntity*>(m_pSubTask)->m_distToTargetSq;
    }

    // 0x6639F2 - Look at the driver
    if (ped->IsVisible() && distToTargetSq < 4.f && !g_ikChainMan.IsLooking(ped) && CGeneral::GetRandomNumberInRange(0, 100) > 60) {
        const auto lookTime = CGeneral::GetRandomNumberInRange(1000, 3000);
        if (const auto driver = m_Vehicle->m_pDriver) {
            g_ikChainMan.LookAt(
                "TaskHassleVehicle",
                ped,
                driver,
                lookTime,
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

    // 0x663A85
    if (!m_pSubTask) {
        return m_pSubTask;
    }
    if (m_pSubTask->GetTaskType() != TASK_COMPLEX_TRACK_ENTITY && m_pSubTask->GetTaskType() != TASK_COMPLEX_FOLLOW_NODE_ROUTE) {
        return m_pSubTask;
    }

    // 0x663AAF - Vehicle is about to blow up, flee
    if (m_Vehicle->m_fHealth < 250.f && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
        return CreateNextSubTask(ped);
    }

    const auto taskMgr = &ped->GetTaskManager();
    switch (m_b31) {
    case 0: { // 0x663AED - Approaching the vehicle
        if (!taskMgr->GetTaskSecondary(TASK_SECONDARY_PARTIAL_ANIM) && CGeneral::GetRandomNumberInRange(0, 100) > 60) {
            taskMgr->SetTaskSecondary(new CTaskSimpleShakeFist{}, TASK_SECONDARY_PARTIAL_ANIM);
        }
        if (distToTargetSq <= sq(dword1C)) {
            m_b31 = 1;
        }
        ped->Say(CTX_GLOBAL_CHASE_CAR);
        break;
    }
    case 1: { // 0x663B8D - Turning towards the vehicle
        const auto targetHeading = GetTargetHeading(ped);
        ped->m_fAimingRotation   = targetHeading;
        const auto heading       = ped->GetHeading();
        if (m_pSubTask->GetTaskType() == TASK_COMPLEX_TRACK_ENTITY) {
            if (distToTargetSq > sq(dword1C)) {
                m_b31 = 0;
            } else if (std::abs(heading - targetHeading) < 0.05f) {
                m_b31 = 2;
            }
        }
        break;
    }
    case 2: { // 0x663C03 - Hassling
        if (m_pSubTask->GetTaskType() == TASK_COMPLEX_TRACK_ENTITY) {
            if (distToTargetSq > sq(dword1C) + 0.05f) {
                m_b31 = 0;
            }
        }
        if (std::abs(ped->GetHeading() - GetTargetHeading(ped)) >= 0.1f) { // 0x663C31
            m_b31 = 1;
        }
        switch (CGeneral::GetRandomNumberInRange(0, 3)) { // 0x663C5B
        case 0: ped->Say(CTX_GLOBAL_DRIVE_THROUGH_TAUNT); break;
        case 1: ped->Say(CTX_GLOBAL_ATTACK_CAR);          break;
        case 2: ped->Say(CTX_GLOBAL_TIP_CAR);             break;
        }

        if (!m_bRemoveAnim || ped->m_nMoveState >= PEDMOVE_RUN) { // 0x663CA9
            break;
        }

        if (!taskMgr->GetTaskSecondary(TASK_SECONDARY_PARTIAL_ANIM)) { // 0x663CD9 - Play a random anim
            const auto rnd = (float)CGeneral::GetRandomNumberInRange(0, 200);
            if (rnd > 166.f) {
                taskMgr->SetTaskSecondary(new CTaskSimpleRunAnim{ ANIM_GROUP_GANGS, ANIM_ID_SHAKE_CARA, 4.f, false }, TASK_SECONDARY_PARTIAL_ANIM);
            } else if (rnd > 133.f) {
                taskMgr->SetTaskSecondary(new CTaskSimpleRunAnim{ ANIM_GROUP_GANGS, ANIM_ID_SHAKE_CARSH, 4.f, false }, TASK_SECONDARY_PARTIAL_ANIM);
            } else if (rnd > 100.f) {
                taskMgr->SetTaskSecondary(new CTaskSimpleRunAnim{ ANIM_GROUP_GANGS, ANIM_ID_SHAKE_CARK, 4.f, false }, TASK_SECONDARY_PARTIAL_ANIM);
            } else if (rnd > 70.f) { // 0x663DE1 - Random gang talk anim [ANIM_ID_PRTIAL_GNGTLKA, ANIM_ID_PRTIAL_GNGTLKH]
                const auto talkAnim = (AnimationId)((int32)((float)CGeneral::GetRandomNumber() * (1.f / 32768.f) * 8.f) + ANIM_ID_PRTIAL_GNGTLKA);
                taskMgr->SetTaskSecondary(new CTaskSimpleRunAnim{ ANIM_GROUP_GANGS, talkAnim, 4.f, false }, TASK_SECONDARY_PARTIAL_ANIM);
            } else if (rnd > 60.f) { // 0x663E4F
                if (!ped->IsPlayingHandSignal()) {
                    taskMgr->SetTaskSecondary(new CTaskComplexPlayHandSignalAnim{ ANIM_ID_UNDEFINED, 4.f }, TASK_SECONDARY_PARTIAL_ANIM);
                }
            } else if (rnd > 40.f) { // 0x663EAE
                m_b31 = 0;
            }
            break;
        }

        // 0x663ECB
        if (distToTargetSq > sq(dword1C)) {
            m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr);
        }

        // 0x663EED - Find the anim that's playing, and the time it hits the vehicle at
        auto hitTime = 0.5f;
        auto anim    = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_SHAKE_CARA);
        if (!anim) {
            hitTime = 0.7f;
            anim    = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_SHAKE_CARSH);
        }
        if (!anim) {
            hitTime = 0.5f;
            anim    = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_SHAKE_CARK);
        }
        if (!anim) {
            break;
        }

        // 0x663F4C - Has the anim just passed the hit time?
        if (!(hitTime < anim->m_CurrentTime) || anim->m_CurrentTime - anim->m_TimeStep > hitTime) {
            break;
        }

        // 0x663F73 - Rock the vehicle
        const auto veh   = m_Vehicle;
        const auto force = CVector{ 0.f, 0.f, veh->m_fMass * 0.02f };
        const auto point = [&]() -> CVector {
            switch (m_nHasslePosId) {
            case 0:
            case 2:  return -veh->GetRight();
            case 1:
            case 3:  return veh->GetRight();
            case 4:  return -veh->GetForward();
            case 5:  return veh->GetForward();
            default: return force;
            }
        }();
        veh->ApplyTurnForce(force, point);

        // 0x664048 - Hit the vehicle (without actually damaging it)
        const auto origHealth = m_Vehicle->m_fHealth;
        {
            CTaskSimpleFight fight{ m_Vehicle, 11, 20'000 };

            CMatrix strikeMat{ *ped->m_matrix };
            strikeMat.GetPosition() += ped->GetForward();

            fight.FightSetUpCol(0.5f);
            fight.m_nComboSet    = 4;
            fight.m_nCurrentMove = FIGHT_ATTACK_HIT_2;
            fight.m_nLastCommand = 11;

            auto& cps = CWorld::m_aTempColPts;
            if (CCollision::ProcessColModels(strikeMat, CTaskSimpleFight::m_sStrikeColModel, *m_Vehicle->m_matrix, *m_Vehicle->GetColModel(), cps, nullptr, nullptr, false) > 0) {
                fight.FightHitCar(ped, m_Vehicle, cps[0].m_vecPoint, cps[0].m_vecNormal, (int16)cps[0].m_nPieceTypeB, cps[0].m_nSurfaceTypeB);
            }

            m_Vehicle->m_fHealth = origHealth;
            m_Vehicle->m_vehicleAudio.AddAudioEvent(AE_SUSPENSION_BOUNCE, 0.f);
        }
        break;
    }
    }

    return m_pSubTask;
}
