#include "StdInc.h"

#include "TaskComplexWalkRoundObject.h"
#include "PedGeometryAnalyser.h"
#include "TaskComplexFollowPointRoute.h"
#include "TaskComplexGoToPointAndStandStill.h"
#include "TaskSimpleAchieveHeading.h"
#include "TaskSimpleStandStill.h"

void CTaskComplexWalkRoundObject::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexWalkRoundObject, 0x86F364, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x655020);
    RH_ScopedInstall(CreateRouteTask, 0x655140);
    RH_ScopedInstall(ComputeRoute, 0x6551D0);
    RH_ScopedInstall(CreateSubTask, 0x655290);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x657220);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x657380);
    RH_ScopedVMTInstall(ControlSubTask, 0x6575F0);
}

// 0x655020
CTaskComplexWalkRoundObject::CTaskComplexWalkRoundObject(int32 moveState, const CVector& targetPoint, CEntity* object) : CTaskComplex() {
    m_moveState   = moveState;
    m_targetPoint = targetPoint;
    m_object      = object;

    CEntity::SafeRegisterRef(m_object);

    m_pointRoute = new CPointRoute();
}

CTaskComplexWalkRoundObject::~CTaskComplexWalkRoundObject() {
    CEntity::SafeCleanUpRef(m_object);

    delete m_pointRoute;
    // todo: m_pointRoute = nullptr;
}

CTaskComplexWalkRoundObject* CTaskComplexWalkRoundObject::Constructor(int32 moveState, const CVector& targetPoint, CEntity* object) {
    this->CTaskComplexWalkRoundObject::CTaskComplexWalkRoundObject(moveState, targetPoint, object);
    return this;
}

// 0x6575F0
CTask* CTaskComplexWalkRoundObject::ControlSubTask(CPed* ped) {
    const auto ShouldAbort = [&] {
        if (!m_object) {
            return true;
        }
        if (m_pSubTask->GetTaskType() == TASK_COMPLEX_FOLLOW_POINT_ROUTE && m_timer.IsOutOfTime()) {
            return true;
        }
        if (ped->GetIntelligence()->m_AnotherStaticCounter > 30) { // Stuck
            return true;
        }
        if (m_pSubTask->GetTaskType() != TASK_SIMPLE_STAND_STILL) { // Check if the object has moved since the route was computed
            const auto& mat = m_object->GetMatrix();
            if ((m_objectPos - m_object->GetPosition()).SquaredMagnitude() > sq(0.25f)) {
                return true;
            }
            if (m_objectForward.Dot(mat.GetForward()) < 0.9f) {
                return true;
            }
            if (m_objectRight.Dot(mat.GetRight()) < 0.9f) {
                return true;
            }
        }
        return false;
    };

    if (ShouldAbort() && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
        return CreateSubTask(TASK_SIMPLE_STAND_STILL, ped);
    }
    return m_pSubTask;
}

// 0x657220
CTask* CTaskComplexWalkRoundObject::CreateNextSubTask(CPed* ped) {
    if (!m_object) {
        return nullptr;
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL:
        return CreateRouteTask(ped);
    case TASK_SIMPLE_ACHIEVE_HEADING: {
        if (!m_pointRoute->IsEmpty()) {
            return CreateRouteTask(ped);
        }

        CVector point;
        CPedGeometryAnalyser::ComputeClosestSurfacePoint(*ped, *m_object, point);
        if (CPedGeometryAnalyser::ComputeEntityHitSide(point, *m_object) != CPedGeometryAnalyser::ComputeEntityHitSide(m_targetPoint, *m_object)) {
            return new CTaskComplexGoToPointAndStandStill{ PEDMOVE_WALK, point, 0.5f, 2.0f, false, false };
        }

        m_pointRoute->AddUnlessFull(point);
        CPedGeometryAnalyser::ComputeClosestSurfacePoint(m_targetPoint, *m_object, point);
        m_pointRoute->AddUnlessFull(point);
        return CreateRouteTask(ped);
    }
    case TASK_COMPLEX_FOLLOW_POINT_ROUTE:
    case TASK_SIMPLE_STAND_STILL:
    default:
        return nullptr;
    }
}

// 0x657380
CTask* CTaskComplexWalkRoundObject::CreateFirstSubTask(CPed* ped) {
    if (!m_object) {
        return nullptr;
    }

    m_pointRoute->Clear();
    const auto distToHitSidePlane = ComputeRoute(ped);
    if (m_pointRoute->IsEmpty()) {
        return nullptr;
    }

    m_timer.m_nStartTime = CTimer::GetTimeInMS();
    m_timer.m_nInterval  = m_moveState == PEDMOVE_WALK ? 8000 : 4000;
    m_timer.m_bStarted   = true;

    m_objectPos     = m_object->GetPosition();
    m_objectForward = m_object->GetMatrix().GetForward();
    m_objectRight   = m_object->GetMatrix().GetRight();

    const auto& firstPoint = m_pointRoute->m_Entries[0];

    CVector    dir  = firstPoint - ped->GetPosition();
    const auto dist = dir.NormaliseAndMag();
    const auto dot  = dir.Dot(ped->GetMatrix().GetForward());

    const auto threshold = [this] {
        switch (m_moveState) {
        case PEDMOVE_WALK: return 2.0f;
        case PEDMOVE_RUN:  return 4.0f;
        default:           return 6.0f;
        }
    }();

    if (dist > threshold && distToHitSidePlane > threshold && dot > 0.0f) {
        return CreateRouteTask(ped);
    }

    const auto& pedPos  = ped->GetPosition();
    const auto  heading = CGeneral::LimitRadianAngle(
        CGeneral::GetRadianAngleBetweenPoints(firstPoint.x - pedPos.x, firstPoint.y - pedPos.y, 0.0f, 0.0f)
    );
    return new CTaskSimpleAchieveHeading{ heading, 1.0f, 0.2f };
}

// 0x655140
CTask* CTaskComplexWalkRoundObject::CreateRouteTask(CPed* ped) {
    if (m_pointRoute->IsEmpty()) {
        return nullptr;
    }
    return new CTaskComplexFollowPointRoute{
        static_cast<eMoveState>(m_moveState),
        *m_pointRoute,
        CTaskComplexFollowPointRoute::Mode::ONE_WAY,
        0.5f,
        0.0f,
        true,
        false,
        false
    };
}

// 0x6551D0 - Compute the route round the object, returns the ped's distance from the bounding box plane of the side it's on.
float CTaskComplexWalkRoundObject::ComputeRoute(CPed* ped) {
    const auto prevNominalRadius = std::exchange(CPedGeometryAnalyser::ms_fPedNominalRadius, 0.7f);
    CPedGeometryAnalyser::ComputeRouteRoundEntityBoundingBox(*ped, *m_object, m_targetPoint, *m_pointRoute, 0);
    CPedGeometryAnalyser::ms_fPedNominalRadius = prevNominalRadius;

    const auto& pedPos = ped->GetPosition();

    std::array<CVector, 4> planeNormals;
    std::array<float, 4>   planeDs;
    CPedGeometryAnalyser::ComputeEntityBoundingBoxPlanes(pedPos.z, *m_object, planeNormals, planeDs);

    const auto hitSide = (size_t)CPedGeometryAnalyser::ComputeEntityHitSide(pedPos, *m_object);
    return planeNormals[hitSide].Dot(pedPos) + planeDs[hitSide];
}

// 0x655290
CTask* CTaskComplexWalkRoundObject::CreateSubTask(eTaskType taskType, CPed* ped) {
    switch (taskType) {
    case TASK_SIMPLE_STAND_STILL:
        return new CTaskSimpleStandStill{ 500, false, false, 8.0f };
    case TASK_COMPLEX_FOLLOW_POINT_ROUTE:
        return CreateRouteTask(ped);
    default:
        return nullptr;
    }
}
