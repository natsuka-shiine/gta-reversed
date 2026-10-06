#include "StdInc.h"

#include "TaskComplexEvasiveCower.h"
#include "TaskSimpleCower.h"
#include "TaskSimpleAchieveHeading.h"

void CTaskComplexEvasiveCower::InjectHooks() {
    RH_ScopedClass(CTaskComplexEvasiveCower);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(CreateSubTask, 0x655570);
}

// 0x655460
CTaskComplexEvasiveCower::CTaskComplexEvasiveCower(CEntity* entity, const CVector& pos) :
    m_Pos{pos},
    m_Entity{entity}
{
    CEntity::SafeRegisterRef(m_Entity);
}

// 0x6554E0
CTaskComplexEvasiveCower::~CTaskComplexEvasiveCower() {
    CEntity::SafeCleanUpRef(m_Entity);
}

// 0x6556A0
CTask* CTaskComplexEvasiveCower::CreateNextSubTask(CPed* ped) {
    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_COWER:
        return CreateSubTask(TASK_FINISHED);
    case TASK_SIMPLE_ACHIEVE_HEADING:
        return CreateSubTask(TASK_SIMPLE_COWER);
    default:
        return nullptr;
    }
}

CTask* CTaskComplexEvasiveCower::CreateFirstSubTask(CPed* ped) {
    return CreateSubTask(TASK_SIMPLE_ACHIEVE_HEADING);
}

// 0x655570
CTask* CTaskComplexEvasiveCower::CreateSubTask(eTaskType taskType) {
    switch (taskType) {
    case TASK_SIMPLE_COWER:
        return new CTaskSimpleCower{};
    case TASK_SIMPLE_ACHIEVE_HEADING:
        return new CTaskSimpleAchieveHeading{
            CGeneral::GetRadianAngleBetweenPoints(-m_Pos.x, -m_Pos.y, 0.0f, 0.0f),
            2.0f,
            0.2f
        };
    default:
        return nullptr;
    }
}
