#include "StdInc.h"

#include "TaskComplexFollowPointRoute.h"
#include "TaskComplexWalkRoundCar.h"
#include "CarEnterExit.h"
#include "PedGeometryAnalyser.h"
#include "TaskComplexEnterCar.h"
#include "TaskSimpleStandStill.h"
#include "TaskSimpleAchieveHeading.h"

void CTaskComplexWalkRoundCar::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexWalkRoundCar, 0x86f308, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6541B0);
    RH_ScopedInstall(Destructor, 0x656B00);

    RH_ScopedInstall(SetNewVehicle, 0x654290);
    RH_ScopedInstall(CreateRouteTask, 0x6542E0);
    RH_ScopedInstall(ComputeRouteRoundSmallCar, 0x6544F0);
    RH_ScopedInstall(GoingForDoor, 0x654720);
    RH_ScopedInstall(ComputeRouteRoundBigCar, 0x656BB0);
    RH_ScopedInstall(ComputeRoute, 0x657B80);

    RH_ScopedVMTInstall(Clone, 0x655B00);
    RH_ScopedVMTInstall(GetTaskType, 0x654280);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x656B70);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x658200);
    RH_ScopedVMTInstall(ControlSubTask, 0x654370);
}

// 0x6541B0
CTaskComplexWalkRoundCar::CTaskComplexWalkRoundCar(eMoveState moveState, CVector const& targetPt, CVehicle* vehicle, bool isPedGoingForCarDoor, uint8 forceThisDirectionRoundCar) :
    m_MoveState{(uint8)moveState},
    m_bIsPedGoingForCarDoor{isPedGoingForCarDoor},
    m_ForceThisDirectionRoundCar{forceThisDirectionRoundCar},
    m_TargetPt{targetPt},
    m_Veh{vehicle},
    m_Route{new CPointRoute{}}
{
    CEntity::SafeRegisterRef(m_Veh);
}

CTaskComplexWalkRoundCar::CTaskComplexWalkRoundCar(const CTaskComplexWalkRoundCar& o) :
    CTaskComplexWalkRoundCar{
        (eMoveState)o.m_MoveState,
        o.m_TargetPt,
        o.m_Veh,
        o.m_bIsPedGoingForCarDoor,
        o.m_ForceThisDirectionRoundCar
    }
{
}

// 0x656B00
CTaskComplexWalkRoundCar::~CTaskComplexWalkRoundCar() {
    CEntity::SafeCleanUpRef(m_Veh);
    delete m_Route;
}


// 0x654290
void CTaskComplexWalkRoundCar::SetNewVehicle(CVehicle * vehicle, uint8 forceThisDirectionRoundCar) {
    if (notsa::IsFixBugs()) {
        if (vehicle == m_Veh) {
            return;
        }
    }
    CEntity::ChangeEntityReference(m_Veh, vehicle);
    m_ForceThisDirectionRoundCar    = forceThisDirectionRoundCar;
    m_bFirstSubTaskNeedsToBeCreated = true;
    m_Route->Clear();
}

// 0x6542E0
CTask* CTaskComplexWalkRoundCar::CreateRouteTask(CPed*) const {
    if (m_Route->IsEmpty()) {
        return nullptr;
    }
    return new CTaskComplexFollowPointRoute{ (eMoveState)m_MoveState, *m_Route };
}

// 0x6544F0
float CTaskComplexWalkRoundCar::ComputeRouteRoundSmallCar(CPed* ped) {
    std::array<CVector, 4> corners;
    CPedGeometryAnalyser::ComputeEntityBoundingBoxCorners(ped->GetPosition().z, *m_Veh, corners); // Result unused, but the game calls it

    std::array<CVector, 4> planeNormals;
    std::array<float, 4>   planeDs;
    CPedGeometryAnalyser::ComputeEntityBoundingBoxPlanes(ped->GetPosition().z, *m_Veh, planeNormals, planeDs);

    // Distance of the ped from the side of the vehicle's bounding box it is at
    float distToHitSide = 0.f;
    if (const auto hitSide = (int8)CPedGeometryAnalyser::ComputeEntityHitSide(*ped, *m_Veh); hitSide != -1) {
        distToHitSide = planeNormals[hitSide].Dot(ped->GetPosition()) + planeDs[hitSide];
    }

    if (m_bIsPedGoingForCarDoor) {
        // If the target point is inside the bounding box push it just outside of it
        std::array<float, 4> targetDists;
        size_t               i = 0;
        for (; i < 4; i++) {
            targetDists[i] = planeNormals[i].Dot(m_TargetPt) + planeDs[i];
            if (!(targetDists[i] < 0.f)) {
                break;
            }
        }
        if (i == 4) { // Behind all planes => inside the box
            if (const auto hitSide = (int8)CPedGeometryAnalyser::ComputeEntityHitSide(m_TargetPt, *m_Veh); hitSide != -1) {
                m_TargetPt += planeNormals[hitSide] * (0.05f - targetDists[hitSide]);
            }
        }
    }

    m_DirectionGoingRoundCar = (uint8)CPedGeometryAnalyser::ComputeRouteRoundEntityBoundingBox(
        *ped,
        *m_Veh,
        m_TargetPt,
        *m_Route,
        (int32)m_ForceThisDirectionRoundCar
    );

    return distToHitSide;
}

// 0x656BB0
float CTaskComplexWalkRoundCar::ComputeRouteRoundBigCar(CPed* ped) {
    // The game temporarily changes the nominal ped radius for calculating the surface points
    const auto ComputeWithNominalRadius = [](auto&& fn) {
        const auto originalRadius                  = CPedGeometryAnalyser::ms_fPedNominalRadius;
        CPedGeometryAnalyser::ms_fPedNominalRadius = 0.7f;
        fn();
        CPedGeometryAnalyser::ms_fPedNominalRadius = originalRadius;
    };

    CVector lastPoint; // Closest surface point to the target
    if (CPedGeometryAnalyser::ComputeEntityHitSide(*ped, *m_Veh) != CPedGeometryAnalyser::ComputeEntityHitSide(m_TargetPt, *m_Veh)) {
        CVector pedSurfacePoint, targetSurfacePoint;
        ComputeWithNominalRadius([&] {
            CPedGeometryAnalyser::ComputeClosestSurfacePoint(*ped, *m_Veh, pedSurfacePoint);
            CPedGeometryAnalyser::ComputeClosestSurfacePoint(m_TargetPt, *m_Veh, targetSurfacePoint);
        });

        // Compute the route between the 2 surface points as if the vehicle was a small one
        const auto originalPedPos   = ped->GetPosition();
        const auto originalTargetPt = m_TargetPt;
        ped->GetPosition()          = pedSurfacePoint;
        m_TargetPt                  = targetSurfacePoint;
        ComputeRouteRoundSmallCar(ped);
        m_TargetPt         = originalTargetPt;
        ped->GetPosition() = originalPedPos;

        // Now prepend the ped's surface point to the route
        const CPointRoute route{ *m_Route };
        m_Route->Clear();
        m_Route->AddUnlessFull(pedSurfacePoint);
        for (const auto& point : route.GetAll()) {
            m_Route->AddUnlessFull(point);
        }

        ComputeWithNominalRadius([&] {
            CPedGeometryAnalyser::ComputeClosestSurfacePoint(m_TargetPt, *m_Veh, lastPoint);
        });
    } else { // Same side, so just walk along it
        CVector pedSurfacePoint;
        ComputeWithNominalRadius([&] {
            CPedGeometryAnalyser::ComputeClosestSurfacePoint(*ped, *m_Veh, pedSurfacePoint);
            CPedGeometryAnalyser::ComputeClosestSurfacePoint(m_TargetPt, *m_Veh, lastPoint);
        });
        m_Route->AddUnlessFull(pedSurfacePoint);
    }
    m_Route->AddUnlessFull(lastPoint);

    return 0.f;
}

// 0x657B80
float CTaskComplexWalkRoundCar::ComputeRoute(CPed* ped) {
    return m_Veh->vehicleFlags.bIsBig
        ? ComputeRouteRoundBigCar(ped)
        : ComputeRouteRoundSmallCar(ped);
}

// 0x656B70
CTask* CTaskComplexWalkRoundCar::CreateNextSubTask(CPed* ped) {
    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_ACHIEVE_HEADING:
        return CreateRouteTask(ped);
    }
    return nullptr;
}

// 0x658200
CTask* CTaskComplexWalkRoundCar::CreateFirstSubTask(CPed* ped) {
    m_Route->Clear();

    const auto routeDist = m_Veh->vehicleFlags.bIsBig
        ? ComputeRouteRoundBigCar(ped)
        : ComputeRouteRoundSmallCar(ped);

    if (m_Route->IsEmpty()) {
        return nullptr;
    }

    int32 time = m_MoveState == PEDMOVE_WALK ? 20'000 : 15'000;
    if (m_Veh->m_pVehicleBeingTowed || m_Veh->m_pTowingVehicle) {
        time = (int32)((float)time * 4.f);
    }
    m_Timer.Start(time);

    m_VehPos      = m_Veh->GetPosition();
    m_VehMatFwd   = m_Veh->GetMatrix().GetForward();
    m_VehMatRight = m_Veh->GetMatrix().GetRight();

    CVector    dirToFirstPoint  = (*m_Route)[0] - ped->GetPosition();
    const auto distToFirstPoint = dirToFirstPoint.NormaliseAndMag();
    const auto dirDotPedFwd     = dirToFirstPoint.Dot(ped->GetMatrix().GetForward());

    float minDist;
    switch (m_MoveState) {
    case PEDMOVE_WALK: minDist = 2.f; break;
    case PEDMOVE_RUN:  minDist = 4.f; break;
    default:           minDist = 6.f; break;
    }

    if (ped->IsPlayer()) {
        if (const auto enterCarTask = static_cast<CTaskComplexEnterCar*>(ped->GetTaskManager().FindTaskByType(TASK_PRIMARY_PRIMARY, TASK_COMPLEX_ENTER_CAR_AS_DRIVER))) {
            m_EnterCarStartTime = enterCarTask->GetEnterCarStartTime();
        }
    }

    if (!m_Veh->vehicleFlags.bIsBig && distToFirstPoint > minDist && routeDist > minDist && dirDotPedFwd > 0.f) {
        return CreateRouteTask(ped);
    }

    // Stop, and turn towards the first point of the route
    CTaskSimpleStandStill standStill{ 0, false, false, 8.f };
    standStill.ProcessPed(ped);

    const auto firstPoint = (*m_Route)[0];
    const auto heading    = CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(
        firstPoint.x - ped->GetPosition().x,
        firstPoint.y - ped->GetPosition().y,
        0.f,
        0.f
    ));
    return new CTaskSimpleAchieveHeading{ heading, 1.f, 0.1f };
}

// 0x654370
CTask* CTaskComplexWalkRoundCar::ControlSubTask(CPed* ped) {
    if (m_bFirstSubTaskNeedsToBeCreated) {
        m_bFirstSubTaskNeedsToBeCreated = false;
        return CreateFirstSubTask(ped);
    }
    int32 waitTime = 200;
    bool  quitEntering = false;
    if (ped->IsPlayer() && m_Veh && m_EnterCarStartTime != -1
        && CCarEnterExit::IsPlayerToQuitCarEnter(ped, m_Veh, m_EnterCarStartTime, m_pSubTask)
        && m_pSubTask->GetTaskType() == TASK_COMPLEX_FOLLOW_POINT_ROUTE) {
        quitEntering = true;
        waitTime = 1302; // 0x516
    } else if (m_pSubTask->GetTaskType() == TASK_COMPLEX_FOLLOW_POINT_ROUTE && m_Timer.IsOutOfTime()) {
        waitTime = 1302; // 0x516 (quitEntering stays false here)
    }
    if (m_Veh) {
        const CVector& vehPos = m_Veh->GetPosition();
        if ((m_VehPos - vehPos).SquaredMagnitude() <= 0.0625f
            && DotProduct(m_Veh->GetMatrix().GetForward(), m_VehMatFwd) >= 0.9f
            && DotProduct(m_Veh->GetMatrix().GetRight(), m_VehMatRight) >= 0.9f
            && waitTime == 200) {
            return m_pSubTask;
        }
    }
    if (!m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
        return m_pSubTask;
    }
    if (quitEntering) {
        if (CTask* primary = ped->GetTaskManager().m_aPrimaryTasks[TASK_PRIMARY_PRIMARY]) {
            if (primary->GetTaskType() == TASK_COMPLEX_ENTER_CAR_AS_DRIVER) {
                primary->MakeAbortable(ped, ABORT_PRIORITY_LEISURE, nullptr);
            }
        }
    }
    return nullptr;
}
