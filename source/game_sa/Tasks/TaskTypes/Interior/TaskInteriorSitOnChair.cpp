#include "StdInc.h"
#include "TaskInteriorSitOnChair.h"

#include <numbers>

#include "Interior/InteriorInfo_t.h"
#include "Interior/Interior_c.h"
#include "Interior/InteriorManager_c.h"
#include "Interior/InteriorGroup_c.h"
#include "Ragdoll/IKChainManager.h"

void CTaskInteriorSitOnChair::InjectHooks() {
    RH_ScopedVirtualClass(CTaskInteriorSitOnChair, 0x870314, 9);
    RH_ScopedCategory("Tasks/TaskTypes/Interior");

    RH_ScopedInstall(Constructor, 0x675C30);
    RH_ScopedInstall(Destructor, 0x675C90);

    RH_ScopedInstall(FinishAnimCB, 0x675DD0);
    RH_ScopedVMTInstall(Clone, 0x675CF0);
    RH_ScopedVMTInstall(GetTaskType, 0x675C80);
    RH_ScopedVMTInstall(MakeAbortable, 0x675D60);
    RH_ScopedVMTInstall(ProcessPed, 0x676D30);
}

// 0x675C30
CTaskInteriorSitOnChair::CTaskInteriorSitOnChair(int32 duration, InteriorInfo_t* interiorInfo, bool bDoInstantly) :
    m_Duration{duration},
    m_InteriorInfo{interiorInfo},
    m_bDoInstantly{bDoInstantly}
{
}

// 0x675CF0
CTaskInteriorSitOnChair::CTaskInteriorSitOnChair(const CTaskInteriorSitOnChair& o) :
    CTaskInteriorSitOnChair{o.m_Duration, o.m_InteriorInfo, o.m_bDoInstantly}
{
}

// 0x675C90
CTaskInteriorSitOnChair::~CTaskInteriorSitOnChair() {
    if (m_Anim) {
        m_Anim->SetDefaultFinishCallback();
    }
}

// 0x675DD0
void CTaskInteriorSitOnChair::FinishAnimCB(CAnimBlendAssociation* anim, void* data) {
    const auto self = notsa::cast<CTaskInteriorSitOnChair>(static_cast<CTask*>(data));

    self->m_PrevAnimId = anim->GetAnimId();

    if (self->m_PrevAnimId == ANIM_ID_LOU_OUT // Last animation in the sequence
     || self->m_bTaskAborting && self->m_PrevAnimId == ANIM_ID_LOU_IN
    ) {
        anim->SetBlendDelta(-1000.f);
        self->m_bTaskFinished = true;
    }

    self->m_Anim = nullptr;
}


// 0x675D60
bool CTaskInteriorSitOnChair::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if (priority == ABORT_PRIORITY_IMMEDIATE) {
        if (g_ikChainMan.IsLooking(ped)) {
            g_ikChainMan.AbortLookAt(ped, 250);
        }
        if (m_Anim) {
            m_Anim->SetBlendDelta(-1000.f);
            m_Anim->SetDefaultDeleteCallback();
            m_Anim = nullptr;
        }
        return true;
    }
    m_bTaskAborting = true;
    return false;
}

// 0x676D30
bool CTaskInteriorSitOnChair::ProcessPed(CPed* ped) {
    // Offset of the ped (in the ped's space) to be applied when getting on/off the chair
    // NOTE: Zero in the executable's image, and no writer of it was found - It's read from the game's memory to stay faithful
    static auto& s_ChairPedOffset = StaticRef<CVector, 0xC18C78>();

    const auto currAnimId = m_Anim ? m_Anim->GetAnimId() : ANIM_ID_UNDEFINED;

    const auto BlendAnim = [&](AnimationId animId, float blendDelta) {
        m_Anim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_INT_HOUSE, animId, blendDelta);
        m_Anim->SetFinishCallback(FinishAnimCB, this);
    };

    ped->SetMoveState(PEDMOVE_STILL);

    if (m_bTaskFinished) {
        if (!RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_LOU_OUT)) {
            return true;
        }
    }

    if (m_bTaskAborting) {
        if (!g_interiorMan.AreAnimsLoaded(0)) { // 0 => "int_house"
            return true;
        }
        switch (currAnimId) {
        case ANIM_ID_LOU_IN: {
            m_Anim->SetBlendDelta(-8.f);
            break;
        }
        case ANIM_ID_LOU_LOOP: {
            if (!m_bUpdatePedPos) {
                m_Anim->SetDefaultDeleteCallback();
                BlendAnim(ANIM_ID_LOU_OUT, 1000.f);
                m_bUpdatePedPos = true;
                return false;
            }
            break;
        }
        case ANIM_ID_LOU_OUT: {
            m_Anim->SetSpeed(3.f);
            break;
        }
        }
    }

    if (!m_Anim) {
        if (g_interiorMan.AreAnimsLoaded(0)) {
            const auto StartLoop = [&] {
                m_TaskTimer.Start(m_Duration);
                BlendAnim(ANIM_ID_LOU_LOOP, 1000.f);
                m_bUpdatePedPos = true;
            };
            if (m_PrevAnimId == ANIM_ID_UNDEFINED) {
                if (m_bDoInstantly) {
                    StartLoop();
                } else {
                    BlendAnim(ANIM_ID_LOU_IN, 4.f);
                }
            } else if (m_PrevAnimId == ANIM_ID_LOU_IN) {
                StartLoop();
            }
        }
    } else {
        if (m_bUpdatePedPos) {
            CVector    pos  = ped->GetPosition();
            const auto oldZ = pos.z;
            switch (currAnimId) {
            case ANIM_ID_LOU_LOOP: {
                pos = ped->GetMatrix().TransformPoint(s_ChairPedOffset); // Must be calculated before the heading is changed

                const auto heading      = CGeneral::LimitRadianAngle(ped->m_fCurrentRotation + 3.14159274f);
                ped->m_fAimingRotation  = heading;
                ped->m_fCurrentRotation = heading;
                ped->SetHeading(heading);
                break;
            }
            case ANIM_ID_LOU_OUT: {
                pos = ped->GetMatrix().TransformPoint(s_ChairPedOffset);
                break;
            }
            }
            pos.z = oldZ;
            ped->SetPosn(pos);
            m_bUpdatePedPos = false;
        }

        if (m_TaskTimer.m_bStarted && m_TaskTimer.IsOutOfTime()) {
            if (m_Anim->GetAnimId() != ANIM_ID_LOU_OUT) {
                m_Anim->SetDefaultDeleteCallback();
                BlendAnim(ANIM_ID_LOU_OUT, 1000.f);
                m_bUpdatePedPos = true;
            }
        }

        if (m_Anim->GetAnimId() == ANIM_ID_LOU_IN) { // Slide the ped towards the chair while sitting down
            const auto pedToChair = m_InteriorInfo->Pos - ped->GetPosition();
            const auto dist       = pedToChair.Magnitude();
            const auto step       = pedToChair * (1.f / dist) * std::min(dist, 0.02f); // NOTE: No zero check in the original either
            const auto& mat       = ped->GetMatrix();
            ped->m_vecAnimMovingShiftLocal.x = DotProduct(step, mat.GetRight());
            ped->m_vecAnimMovingShiftLocal.y = DotProduct(step, mat.GetForward());

            ped->m_fAimingRotation = CGeneral::LimitRadianAngle(
                CGeneral::GetRadianAngleBetweenPoints(m_InteriorInfo->Dir.x, m_InteriorInfo->Dir.y, 0.f, 0.f)
            );
        }
    }

    if (m_Anim && m_Anim->GetAnimId() == ANIM_ID_LOU_LOOP) { // Look around randomly while sitting
        if (g_ikChainMan.IsLooking(ped)) {
            return false;
        }
        if (CGeneral::GetRandomNumberInRange(0, 1000) <= 980) {
            return false;
        }
        const auto intGrp = g_interiorMan.GetPedsInteriorGroup(ped);
        if (!intGrp) {
            return false;
        }
        InteriorInfo_t* lookAtInfo{};
        Interior_c*     lookAtInterior{};
        float           lookAtDist;
        intGrp->FindClosestInteriorInfo(0, ped->GetPosition(), 10.f, &lookAtInfo, &lookAtInterior, &lookAtDist);
        if (!lookAtInfo) {
            return false;
        }
        const auto time = CGeneral::GetRandomNumberInRange(10'000, 20'000);
        g_ikChainMan.LookAt(
            "TaskSitInChair",
            ped,
            nullptr,
            time,
            BONE_UNKNOWN,
            &lookAtInfo->Pos,
            false,
            0.25f,
            500,
            3,
            false
        );
    } else if (g_ikChainMan.IsLooking(ped)) {
        g_ikChainMan.AbortLookAt(ped, 250);
    }
    return false;
}
