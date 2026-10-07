#include "StdInc.h"

#include "TaskSimpleTriggerLookAt.h"
#include "Ragdoll/IKChainManager.h"

void CTaskSimpleTriggerLookAt::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleTriggerLookAt, 0x86E3CC, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x634440);
    RH_ScopedInstall(Destructor, 0x634500);

    RH_ScopedVMTInstall(Clone, 0x634560);
    RH_ScopedVMTInstall(GetTaskType, 0x6344F0);
    RH_ScopedVMTInstall(MakeAbortable, 0x634610);
    RH_ScopedVMTInstall(ProcessPed, 0x634620);
}

// 0x634440
CTaskSimpleTriggerLookAt::CTaskSimpleTriggerLookAt(CEntity* entity, int32 time, eBoneTag32 offsetBoneTag, CVector offsetPos, bool bUseTorso, float speed, int32 blendTime, int32 priority) :
    m_pEntity{ entity },
    m_time{ time },
    m_nOffsetBoneTag{ offsetBoneTag },
    m_vOffsetPos{ offsetPos },
    m_bUseTorso{ bUseTorso },
    m_Speed{ speed },
    m_nBlendTime{ blendTime },
    m_bEntityExist{ entity != nullptr },
    m_nPriority{ static_cast<int8>(priority) }
{
    CEntity::SafeRegisterRef(m_pEntity);
}

// 0x634500
CTaskSimpleTriggerLookAt::~CTaskSimpleTriggerLookAt() {
    CEntity::SafeCleanUpRef(m_pEntity);
}

// 0x634560
CTask* CTaskSimpleTriggerLookAt::Clone() const {
    auto time    = m_time;
    auto boneTag = m_nOffsetBoneTag;
    if (boneTag.get_underlying() > -1 && !m_pEntity) { // A bone, but the entity it belongs to is gone
        boneTag = BONE_UNKNOWN;
        time    = 100;
    }
    return new CTaskSimpleTriggerLookAt{ m_pEntity, time, boneTag, m_vOffsetPos, m_bUseTorso, m_Speed, m_nBlendTime, m_nPriority };
}

// 0x634620
bool CTaskSimpleTriggerLookAt::ProcessPed(CPed* ped) {
    if (!m_bEntityExist || m_pEntity) { // Unless the entity to look at has been deleted since
        g_ikChainMan.LookAt("TaskTriggerLookAt", ped, m_pEntity, m_time, m_nOffsetBoneTag, &m_vOffsetPos, m_bUseTorso, m_Speed, m_nBlendTime, m_nPriority, false);
    }
    return true;
}
