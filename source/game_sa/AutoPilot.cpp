#include "StdInc.h"

#include "AutoPilot.h"
#include "Curves.h"

void CAutoPilot::InjectHooks() {
    RH_ScopedClass(CAutoPilot);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(ModifySpeed, 0x41B980);
}

// 0x6D5E20
CAutoPilot::CAutoPilot() : m_aPathFindNodesInfo() {
    _smthNext = 1;
    _smthCurr = 1;

    m_nCarCtrlFlags = 0;

    m_timeToLeaveLink = 0;
    m_timeToGetToNextLink = 1000;
    m_nNextLane = 0;
    m_nCurrentLane = 0;
    m_nCarDrivingStyle = DRIVING_STYLE_STOP_FOR_CARS;
    m_nCarMission = eCarMission::MISSION_NONE;
    m_nTempAction = TEMPACT_NONE;
    SetCruiseSpeed(10);
    m_speed = 10.0F;
    m_nPathFindNodesCount = 0;
    m_TargetEntity = nullptr;

    m_nTimeToStartMission = CTimer::GetTimeInMS();
    m_nTimeSwitchedToRealPhysics = CTimer::GetTimeInMS();
    m_LastUpdateTimeMs = 0;

    m_nStraightLineDistance = 20;
    m_ucTempActionMode = 0;
    m_ucCarMissionModeCounter = 0;
    field_41 = 0;
    m_SpeedMult = 1.0f;
    m_ucHeliSpeedMult = 0;
    movementFlags.bIsStopped = false;
    movementFlags.bIsParked = false;
    field_4A = 0;
    m_ucCarFollowDist = 10;
    m_ucHeliTargetDist2 = 10;
    field_50 = CGeneral::GetRandomNumber() % 8 + 2;
    m_vehicleRecordingId = -1;
    m_bPlaneDogfightSomething = false;
    m_ObstructingEntity = nullptr;
    m_fMaxTrafficSpeed = 0.0F;
}

// 0x41B980
void CAutoPilot::ModifySpeed(float target) {
    // How far along the current curve we are (must be calculated using the old duration)
    const auto progress = static_cast<float>(CTimer::GetTimeInMS() - static_cast<uint32>(m_timeToLeaveLink)) / static_cast<float>(m_timeToGetToNextLink);

    m_speed = std::max(target, 0.01f);

    if (!ThePaths.IsAreaLoaded(m_nCurrentPathNodeInfo.m_wAreaId) || !ThePaths.IsAreaLoaded(m_nNextPathNodeInfo.m_wAreaId)) {
        return;
    }

    const auto& currLink = ThePaths.GetCarPathLink(m_nCurrentPathNodeInfo);
    const auto& nextLink = ThePaths.GetCarPathLink(m_nNextPathNodeInfo);

    const auto currDir = CVector2D{ currLink.m_dir } * static_cast<float>(_smthCurr);
    const auto nextDir = CVector2D{ nextLink.m_dir } * static_cast<float>(_smthNext);

    const auto currLaneOffset = (currLink.OneWayLaneOffset() + static_cast<float>(m_nCurrentLane)) * 5.4f;
    const auto nextLaneOffset = (nextLink.OneWayLaneOffset() + static_cast<float>(m_nNextLane)) * 5.4f;

    const auto currCoors = currLink.GetNodeCoors();
    const auto nextCoors = nextLink.GetNodeCoors();

    const CVector currPos{ currCoors.x + currLaneOffset * currDir.y, currCoors.y - currLaneOffset * currDir.x, 0.0f };
    const CVector nextPos{ nextCoors.x + nextLaneOffset * nextDir.y, nextCoors.y - nextLaneOffset * nextDir.x, 0.0f };

    m_timeToGetToNextLink = static_cast<int32>(CCurves::CalcSpeedScaleFactor(currPos, nextPos, currDir.x, currDir.y, nextDir.x, nextDir.y) * (1000.0f / m_speed));
    m_timeToLeaveLink     = static_cast<int32>(static_cast<float>(CTimer::GetTimeInMS()) - static_cast<float>(m_timeToGetToNextLink) * progress);
}

// 0x41B950
void CAutoPilot::RemoveOnePathNode() {
    assert(m_nPathFindNodesCount > 0);
    
    --m_nPathFindNodesCount;
    
    for (int16 count = 0; count < m_nPathFindNodesCount; ++count) {
        m_aPathFindNodesInfo[count] = m_aPathFindNodesInfo[count + 1]; // ?
    }
}

void CAutoPilot::SetCarMission(eCarMission carMission, uint32 timeOffsetMs) {
    m_nCarMission = carMission;
    m_nTimeToStartMission = CTimer::GetTimeInMS() + timeOffsetMs;
}

// notsa
void CAutoPilot::SetTempAction(eAutoPilotTempAction action, uint32 durMs) {
    m_nTempAction = action;
    m_nTempActionTime = CTimer::GetTimeInMS() + durMs;
}
