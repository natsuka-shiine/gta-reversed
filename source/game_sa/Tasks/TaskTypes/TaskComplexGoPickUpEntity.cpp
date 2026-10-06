#include "StdInc.h"

#include "TaskComplexGoPickUpEntity.h"
#include "TaskComplexGoToPointAndStandStill.h"
#include "TaskSimpleAchieveHeading.h"
#include "TaskSimpleHoldEntity.h"
#include "TaskSimplePickUpEntity.h"

void CTaskComplexGoPickUpEntity::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexGoPickUpEntity, 0x870B98, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6919C0);
    RH_ScopedInstall(Destructor, 0x691A50);

    RH_ScopedVMTInstall(Clone, 0x692C80);
    RH_ScopedVMTInstall(GetTaskType, 0x691A40);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x691AE0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x693610);
    RH_ScopedVMTInstall(ControlSubTask, 0x691D50);
}

// 0x6919C0
CTaskComplexGoPickUpEntity::CTaskComplexGoPickUpEntity(CEntity* entity, AssocGroupId animGroupId) :
    m_pEntity{ entity },
    m_vecPosition{ -999.0f, 0.0f, 0.0f },
    m_vecPickupPosition{ -999.0f, 0.0f, 0.0f },
    m_nTimePassedSinceLastSubTaskCreatedInMs{ 0 },
    m_nAnimGroupId{ animGroupId },
    m_bAnimBlockReferenced{ false }
{
    CEntity::SafeRegisterRef(m_pEntity);
}

// 0x691A50
CTaskComplexGoPickUpEntity::~CTaskComplexGoPickUpEntity() {
    CEntity::SafeCleanUpRef(m_pEntity);
    if (m_bAnimBlockReferenced) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(m_nAnimGroupId));
        m_bAnimBlockReferenced = false;
    }
}

// 0x691AE0
CTask* CTaskComplexGoPickUpEntity::CreateNextSubTask(CPed* ped) {
    if (!m_pEntity) {
        return nullptr;
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL: { // 0x691B3B - Arrived, now face the entity
        const auto dir = m_pEntity->GetPosition() - ped->GetPosition();
        return new CTaskSimpleAchieveHeading{ std::atan2(-dir.x, dir.y), 1.0f, 0.001f };
    }
    case TASK_SIMPLE_ACHIEVE_HEADING: { // 0x691BB8 - Facing the entity, now pick it up
        float movePedUntilAnimProgress = 0.6f;
        if (m_nAnimGroupId == ANIM_GROUP_CARRY) {
            // Pick the lift-up anim based on how high the bottom of the entity is relative to the ped
            const float entityBottomZ = m_pEntity->GetColModel()->GetBoundingBox().m_vecMin.z + m_pEntity->GetPosition().z;
            const float pedZ          = ped->GetPosition().z;
            if (entityBottomZ <= pedZ) {
                if (pedZ < entityBottomZ + 0.55f) {
                    m_nAnimGroupId           = ANIM_GROUP_CARRY05;
                    movePedUntilAnimProgress = 0.26666668f;
                }
            } else {
                m_nAnimGroupId           = ANIM_GROUP_CARRY105;
                movePedUntilAnimProgress = 0.2f;
            }
        }
        auto* const pickUp = new CTaskSimplePickUpEntity{
            m_pEntity,
            &m_vecPosition,
            PED_NODE_RIGHT_HAND,
            HOLD_ENTITY_FLAG_1,
            ANIM_ID_LIFTUP,
            m_nAnimGroupId,
            movePedUntilAnimProgress
        };
        pickUp->m_vecPickuposn = m_vecPickupPosition;
        return pickUp;
    }
    case TASK_SIMPLE_PICKUP_ENTITY: { // 0x691CC4 - Picked up, keep holding it using a secondary task
        m_nTimePassedSinceLastSubTaskCreatedInMs = CTimer::GetTimeInMS();
        auto& taskMgr = ped->GetTaskManager();
        if (!taskMgr.GetTaskSecondary(TASK_SECONDARY_PARTIAL_ANIM)) {
            taskMgr.SetTaskSecondary(
                new CTaskSimpleHoldEntity{
                    m_pEntity,
                    &m_vecPosition,
                    PED_NODE_RIGHT_HAND,
                    HOLD_ENTITY_FLAG_1,
                    ANIM_ID_CRRY_PRTIAL,
                    ANIM_GROUP_CARRY,
                    false
                },
                TASK_SECONDARY_PARTIAL_ANIM
            );
        }
        return nullptr;
    }
    default:
        return nullptr;
    }
}

// 0x693610
CTask* CTaskComplexGoPickUpEntity::CreateFirstSubTask(CPed* ped) {
    if (!m_pEntity) {
        return nullptr;
    }

    m_nTimePassedSinceLastSubTaskCreatedInMs = CTimer::GetTimeInMS();

    const CVector pedToEntity = ped->GetPosition() - m_pEntity->GetPosition();
    const auto&   bb          = CModelInfo::GetModelInfo(m_pEntity->m_nModelIndex)->GetColModel()->GetBoundingBox();

    m_vecPosition = CVector{ -0.2f, -bb.m_vecMin.y - 0.2f, -bb.m_vecMin.z };

    // 0x6936A5 - Orient the entity so that it can be carried
    const float headingToPed = std::atan2(-pedToEntity.x, pedToEntity.y);
    if (bb.m_vecMax.y * 2.0f < bb.m_vecMax.x) { // Much wider than long
        m_pEntity->SetHeading(headingToPed);
    } else if (bb.m_vecMax.x * 2.0f < bb.m_vecMax.y) { // Much longer than wide
        m_pEntity->SetHeading(headingToPed - 1.57079637f);
    } else if (m_pEntity->GetMatrix().GetUp().z < 0.9f) { // Not upright, straighten it
        m_pEntity->SetHeading(m_pEntity->GetHeading());
    }

    // 0x69378D
    if (m_pEntity->GetIsTypeObject()) {
        auto* const object = m_pEntity->AsObject();
        object->m_vecMoveSpeed = CVector{};
        object->m_vecTurnSpeed = CVector{};
        object->SetIsStatic(true);
    }

    // 0x6937BE
    if (m_pEntity->m_pRwObject) {
        m_pEntity->UpdateRwMatrix();
    }

    // 0x6937F0 - Figure out from which side of the entity to pick it up
    const auto& mat      = m_pEntity->GetMatrix();
    const float dotRight = pedToEntity.Dot(mat.GetRight());
    const float dotFwd   = pedToEntity.Dot(mat.GetForward());
    if (dotRight > std::abs(dotFwd)) { // Right
        m_vecPickupPosition.x = bb.m_vecMax.x + 0.4f;
        m_vecPickupPosition.y = 0.0f;
    } else if (dotRight < -std::abs(dotFwd)) { // Left
        m_vecPickupPosition.x = bb.m_vecMin.x - 0.4f;
        m_vecPickupPosition.y = 0.0f;
    } else { // Front/back
        m_vecPickupPosition.x = 0.0f;
        m_vecPickupPosition.y = dotFwd <= 0.0f
            ? bb.m_vecMin.y - 0.4f
            : bb.m_vecMax.y + 0.4f;
    }
    m_vecPickupPosition.z = bb.m_vecMin.z + 1.0f;

    // 0x6938D4
    return new CTaskComplexGoToPointAndStandStill{
        PEDMOVE_WALK,
        m_pEntity->GetMatrix().TransformPoint(m_vecPickupPosition),
        0.2f,
        0.0f,
        false,
        true
    };
}

// 0x691D50
CTask* CTaskComplexGoPickUpEntity::ControlSubTask(CPed* ped) {
    // 0x691D5B - Stream in (and reference) the anims
    if (m_nAnimGroupId != ANIM_GROUP_DEFAULT && !m_bAnimBlockReferenced) {
        const auto* const block    = CAnimManager::GetAnimationBlock(m_nAnimGroupId);
        const auto        blockIdx = CAnimManager::GetAnimationBlockIndex(m_nAnimGroupId);
        if (block->IsLoaded) {
            CAnimManager::AddAnimBlockRef(blockIdx);
            m_bAnimBlockReferenced = true;
        } else {
            CStreaming::RequestModel(IFPToModelId(blockIdx), STREAMING_KEEP_IN_MEMORY);
        }
    }

    if (!m_pSubTask) {
        return m_pSubTask;
    }

    const auto AbortSubTask = [&]() -> CTask* {
        m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr);
        return nullptr;
    };

    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_PICKUP_ENTITY: { // 0x691F7C - Give up if the pick up anim didn't start in time
        const auto* const pickUp = static_cast<CTaskSimplePickUpEntity*>(m_pSubTask);
        if (!pickUp->m_pAnimBlendAssociation && !pickUp->m_bEntityDropped) {
            if (CTimer::GetTimeInMS() > m_nTimePassedSinceLastSubTaskCreatedInMs + MAX_PICKUP_TIME) {
                return AbortSubTask();
            }
        }
        break;
    }
    case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL: {
        // 0x691DDE - Took too long to get there
        if (CTimer::GetTimeInMS() > m_nTimePassedSinceLastSubTaskCreatedInMs + MAX_GOTO_TIME) {
            return AbortSubTask();
        }

        // 0x691E0F - Let the player cancel it by moving away, jumping or sprinting
        if (!ped->m_pPlayerData) {
            break;
        }
        const auto* const pad = ped->AsPlayer()->GetPadFromPlayer();
        if (!pad) {
            break;
        }

        const auto* const goTo      = static_cast<CTaskComplexGoToPointAndStandStill*>(m_pSubTask);
        const float       upDown    = (float)pad->GetPedWalkUpDown();
        const float       leftRight = (float)pad->GetPedWalkLeftRight();
        const float       stickMag  = std::sqrt(upDown * upDown + leftRight * leftRight) * (1.0f / 128.0f);

        const auto& pedPos        = ped->GetPosition();
        const float targetHeading = std::atan2(-(goTo->m_vecTargetPoint.x - pedPos.x), goTo->m_vecTargetPoint.y - pedPos.y);

        float stickHeading = CGeneral::GetRadianAngleBetweenPoints(0.0f, 0.0f, -leftRight, upDown) - TheCamera.m_fOrientation;
        if (targetHeading + 3.14159274f < stickHeading) {
            stickHeading -= 6.28318548f;
        } else if (targetHeading - 3.14159274f > stickHeading) {
            stickHeading += 6.28318548f;
        }

        if (pad->JumpJustDown() || pad->SprintJustDown()) {
            return AbortSubTask();
        }

        // NOTE: Windows compares the signed difference here (`fabs` is applied to the result of the comparison),
        //       Android does `fabsf(stickHeading - targetHeading) > 45deg`.
        if (stickMag > 0.75f && stickHeading - targetHeading > 0.785398185f) {
            return AbortSubTask();
        }
        break;
    }
    }

    return m_pSubTask;
}
