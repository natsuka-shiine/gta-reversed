#pragma once

#include "TaskComplex.h"
#include "Vector.h"
#include "TaskTimer.h"

class CPointRoute;

class NOTSA_EXPORT_VTABLE CTaskComplexWalkRoundObject : public CTaskComplex {
public:
    int32        m_moveState;
    CVector      m_targetPoint;
    CEntity*     m_object;
    CPointRoute* m_pointRoute;
    CTaskTimer   m_timer;          // 0x24
    CVector      m_objectPos;      // 0x30 - Position of the object when the route was computed
    CVector      m_objectForward;  // 0x3C - Forward vector of the object when the route was computed
    CVector      m_objectRight;    // 0x48 - Right vector of the object when the route was computed

public:
    static constexpr auto Type = TASK_COMPLEX_WALK_ROUND_OBJECT;

    CTaskComplexWalkRoundObject(int32 moveState, const CVector& targetPoint, CEntity* object);
    ~CTaskComplexWalkRoundObject() override;

    eTaskType GetTaskType() const override { return Type; }
    CTask* Clone() const override { return new CTaskComplexWalkRoundObject(m_moveState, m_targetPoint, m_object); }
    CTask* CreateNextSubTask(CPed* ped) override;
    CTask* CreateFirstSubTask(CPed* ped) override;
    CTask* ControlSubTask(CPed* ped) override;

    CTask* CreateRouteTask(CPed* ped);
    float  ComputeRoute(CPed* ped);                          // 0x6551D0
    CTask* CreateSubTask(eTaskType taskType, CPed* ped);     // 0x655290

private:
    friend void InjectHooksMain();
    static void InjectHooks();

    CTaskComplexWalkRoundObject* Constructor(int32 moveState, const CVector& targetPoint, CEntity* object);
};

VALIDATE_SIZE(CTaskComplexWalkRoundObject, 0x54);
