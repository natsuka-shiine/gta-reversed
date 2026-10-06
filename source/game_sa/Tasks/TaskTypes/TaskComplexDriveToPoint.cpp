#include "StdInc.h"

#include "TaskComplexDriveToPoint.h"
#include "TaskComplexGoToPointAnyMeans.h"

void CTaskComplexDriveToPoint::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexDriveToPoint, 0x86E9DC, 14);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedOverloadedInstall(IsTargetBlocked, "Entities", 0x6432A0, bool (CTaskComplexDriveToPoint::*)(CPed*, CEntity**, int32) const);
    RH_ScopedOverloadedInstall(IsTargetBlocked, "Ped", 0x6452C0, bool (CTaskComplexDriveToPoint::*)(CPed*) const);
    RH_ScopedVMTInstall(Drive, 0x645420);
}

// 0x63CE00
CTaskComplexDriveToPoint::CTaskComplexDriveToPoint(CVehicle* vehicle, const CVector& point, float speed, int32 arg4, eModelID carModelIndexToCreate, float radius, eCarDrivingStyle drivingStyle) :
      CTaskComplexCarDrive(vehicle, speed, carModelIndexToCreate, drivingStyle),
      m_Point{ point },
      field_30{ arg4 },
      m_Radius{ radius },
      field_38{ false }
{

}

// 0x63CE80
CTask* CTaskComplexDriveToPoint::CreateSubTaskCannotGetInCar(CPed* ped) {
    return new CTaskComplexGoToPointAnyMeans(PEDMOVE_RUN, m_Point, 0.5f, m_DesiredCarModel);
}

// 0x63CF00
void CTaskComplexDriveToPoint::SetUpCar() {
    m_OriginalDrivingStyle = m_Veh->m_autoPilot.m_nCarDrivingStyle;
    m_OriginalMission         = m_Veh->m_autoPilot.m_nCarMission;
    m_OriginalSpeed              = m_Veh->m_autoPilot.m_nCruiseSpeed;

    m_bIsCarSetUp = true;

    if (m_CruiseSpeed > 0.0f) {
        assert(m_CruiseSpeed < 255.0f);
        m_Veh->m_autoPilot.SetCruiseSpeed((uint8)m_CruiseSpeed);
    }
    m_Veh->m_autoPilot.m_nCarDrivingStyle    = static_cast<eCarDrivingStyle>(m_CarDrivingStyle);
    m_Veh->m_autoPilot.m_nTimeToStartMission = CTimer::GetTimeInMS();
}

// 0x645420
CTask* CTaskComplexDriveToPoint::Drive(CPed* ped) {
    const auto subTask = m_pSubTask;
    auto&      ap      = m_Veh->m_autoPilot;

    const auto dist = DistanceBetweenPoints(m_Point, m_Veh->GetPosition());
    if (dist < m_Radius) {
        ap.SetCarMission(MISSION_NONE);
        field_38 = true;
        return CTaskComplexCarDrive::CreateSubTask(TASK_FINISHED, ped);
    }

    if (dist < 3.0f && ap.m_nCarMission == MISSION_NONE) {
        field_38 = true;
        return CTaskComplexCarDrive::CreateSubTask(TASK_FINISHED, ped);
    }

    if (!ap.m_nCruiseSpeed) {
        ap.m_nCruiseSpeed = static_cast<uint8>(static_cast<int32>(m_CruiseSpeed));
    }

    if (IsTargetBlocked(ped)) {
        field_38 = true;
        return CTaskComplexCarDrive::CreateSubTask(TASK_FINISHED, ped);
    }

    switch (field_30) {
    case field_30_enum::DEFAULT:       CCarAI::GetCarToGoToCoors(m_Veh, m_Point, m_CarDrivingStyle, false); break;
    case field_30_enum::ACCURATE:      CCarAI::GetCarToGoToCoorsAccurate(m_Veh, m_Point, m_CarDrivingStyle, false); break;
    case field_30_enum::STRAIGHT_LINE: CCarAI::GetCarToGoToCoorsStraightLine(m_Veh, m_Point, m_CarDrivingStyle, false); break;
    case field_30_enum::RACING:        CCarAI::GetCarToGoToCoorsRacing(m_Veh, m_Point, m_CarDrivingStyle, false); break;
    default:                           break; // Original does nothing for other values
    }
    return subTask;
}

// 0x6452C0
bool CTaskComplexDriveToPoint::IsTargetBlocked(CPed* ped) const {
    if (DistanceBetweenPointsSquared(ped->GetPosition(), m_Point) > sq(6.0f)) {
        return false;
    }

    const auto intel = ped->GetIntelligence();
    return IsTargetBlocked(ped, intel->GetPedEntities(), 16)
        || IsTargetBlocked(ped, intel->GetVehicleEntities(), 16);
}

// 0x6432A0
bool CTaskComplexDriveToPoint::IsTargetBlocked(CPed* ped, CEntity** entities, int32 numEntities) const {
    const auto veh = ped->m_pVehicle;
    if (!veh) {
        return false;
    }

    const auto vehRadius        = veh->GetModelInfo()->GetColModel()->GetBoundRadius();
    const auto vehDistToPointSq = (veh->GetPosition() - m_Point).SquaredMagnitude();

    for (auto i = 0; i < numEntities; ++i) {
        const auto entity = entities[i];
        if (!entity || entity == veh) {
            continue;
        }

        // Is the target point inside the entity's bounding sphere?
        const auto entityRadius = entity->GetModelInfo()->GetColModel()->GetBoundRadius();
        if (sq(entityRadius) <= (entity->GetPosition() - m_Point).SquaredMagnitude()) {
            continue;
        }

        // And are we close enough to it?
        if (static_cast<float>(static_cast<double>(sq(entityRadius + vehRadius)) * 1.5) > vehDistToPointSq) {
            return true;
        }
    }
    return false;
}

void CTaskComplexDriveToPoint::GoToPoint(const CVector& point) {
    m_Point = point;
}
