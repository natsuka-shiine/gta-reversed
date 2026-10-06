#include "StdInc.h"

#include <extensions/utility.hpp>

#include "TaskComplexKillCriminal.h"
#include "TaskComplexKillPedOnFoot.h"
#include "TaskSimpleGangDriveBy.h"
#include "TaskComplexEnterCarAsPassenger.h"
#include "TaskComplexEnterCarAsDriver.h"
#include "TaskComplexLeaveCar.h"
#include "TaskSimpleCarDrive.h"
#include "TaskComplexCarDriveMission.h"
#include "EventAcquaintancePedHate.h"
#include "InterestingEvents.h"


void CTaskComplexKillCriminal::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexKillCriminal, 0x870a00, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x68BE70);
    RH_ScopedInstall(Destructor, 0x68BF30);

    RH_ScopedInstall(CreateSubTask, 0x68C050);
    RH_ScopedInstall(FindNextCriminalToKill, 0x68C3C0);
    RH_ScopedInstall(ChangeTarget, 0x68C6E0);

    RH_ScopedVMTInstall(Clone, 0x68CE50);
    RH_ScopedVMTInstall(GetTaskType, 0x68BF20);
    RH_ScopedVMTInstall(MakeAbortable, 0x68DAD0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x68E4F0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x68DC60);
    RH_ScopedVMTInstall(ControlSubTask, 0x68E950);
}

bool NoPedOrNoHp(CPed* ped) {
    return !ped || ped->m_fHealth <= 0.f;
}

// 0x68BE70
CTaskComplexKillCriminal::CTaskComplexKillCriminal(CPed* criminal, bool randomize) :
    m_Randomize{randomize},
    m_Criminal{criminal}
{
    if (   !m_Criminal
        || m_Criminal->IsPlayer()
        || m_Criminal->IsCreatedByMission()
        || notsa::contains({ PED_TYPE_COP, PED_TYPE_MEDIC, PED_TYPE_FIREMAN, PED_TYPE_MISSION1 }, m_Criminal->m_nPedType)
    ) {
        m_Criminal = nullptr;
    }
}

// NOTSA (For 0x68CE50)
CTaskComplexKillCriminal::CTaskComplexKillCriminal(const CTaskComplexKillCriminal& o) :
    CTaskComplexKillCriminal{o.m_Criminal, false}
{
}

// 0x68BF30
CTaskComplexKillCriminal::~CTaskComplexKillCriminal() {
    if (m_Cop) {
        m_Cop->m_nTimeTillWeNeedThisPed = CTimer::GetTimeInMS();
        m_Cop->bCullExtraFarAway = false;
        m_Cop->m_fRemovalDistMultiplier = 1.f;
        if (const auto veh = m_Cop->m_pVehicle) {
            veh->m_nExtendedRemovalRange = false;
            veh->vehicleFlags.bNeverUseSmallerRemovalRange = false;
            if (veh->IsDriver(m_Cop)) {
                const auto ap = &veh->m_autoPilot;
                ap->SetCarMission(MISSION_CRUISE);
                ap->SetDrivingStyle(DRIVING_STYLE_AVOID_CARS);
                ap->SetCruiseSpeed(10);
                if (veh->GetStatus() != STATUS_SIMPLE) {
                    CCarCtrl::JoinCarWithRoadSystem(veh);
                }
                veh->vehicleFlags.bSirenOrAlarm = false;
            }
            veh->vehicleFlags.bSirenOrAlarm = false;
        }
    }
}

// 0x68C050
CTask* CTaskComplexKillCriminal::CreateSubTask(eTaskType tt, CPed* ped, bool force) {
    if (!force && m_pSubTask && m_pSubTask->GetTaskType() == tt) {
        return m_pSubTask;        
    }

    switch (tt) {
    case TASK_COMPLEX_KILL_PED_ON_FOOT: {
        ped->SetCurrentWeapon(WEAPON_PISTOL);
        return new CTaskComplexKillPedOnFoot{ m_Criminal };
    }
    case TASK_FINISHED: {
        if (m_Criminal) {
            m_Criminal->SetPedDefaultDecisionMaker();
        }
        return nullptr;
    }
    case TASK_SIMPLE_GANG_DRIVEBY: {
        const auto task = new CTaskSimpleGangDriveBy{
            m_Criminal,
            nullptr,
            70.f,
            70,
            eDrivebyStyle::AI_ALL_DIRN,
            false
        };
        ped->SetCurrentWeapon(WEAPON_PISTOL); // 0x68C36A - Shared tail with `TASK_COMPLEX_KILL_PED_ON_FOOT`
        return task;
    }
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER:
        return new CTaskComplexEnterCarAsPassenger{ ped->m_pVehicle };
    case TASK_COMPLEX_ENTER_CAR_AS_DRIVER:
        return new CTaskComplexEnterCarAsDriver{ ped->m_pVehicle };
    case TASK_COMPLEX_LEAVE_CAR:
        return new CTaskComplexLeaveCar{ ped->m_pVehicle, 0, 0, true, false };
    case TASK_SIMPLE_CAR_DRIVE:
        return new CTaskSimpleCarDrive{ ped->m_pVehicle };
    case TASK_COMPLEX_CAR_DRIVE_MISSION: {
        const auto oveh = ped->m_pVehicle; // (o)ur (veh)icle
        if (!oveh) {
            return nullptr;
        }

        const auto CreateDriveMission = [&, this](eCarMission mission, float cruiseSpeed, CEntity* traget) {
            return new CTaskComplexCarDriveMission{
                ped->m_pVehicle,
                traget,
                mission,
                DRIVING_STYLE_AVOID_CARS,
                cruiseSpeed
            };
        };

        if (const auto cveh = m_Criminal->GetVehicleIfInOne()) {
            return CreateDriveMission(
                oveh->IsBike()
                    ? MISSION_FOLLOWCAR_CLOSE
                    : MISSION_BLOCKCAR_CLOSE,
                (float)(m_Criminal->m_pVehicle->m_autoPilot.m_nCruiseSpeed) + 10.f,
                cveh
            );
        } else {
            return CreateDriveMission(MISSION_KILLPED_CLOSE, 20.f, m_Criminal);
        }
    }
    default:
        NOTSA_UNREACHABLE();
    }
}

// 0x68C3C0
CPed* CTaskComplexKillCriminal::FindNextCriminalToKill(CPed* ped, bool any) {
    const auto [closest, distSq] = notsa::SpatialQuery(
        m_Cop->m_apCriminalsToKill | rng::views::filter(notsa::Not(NoPedOrNoHp)),
        m_Cop->GetPosition(),
        m_Criminal.Get(),
        !any && NoPedOrNoHp(m_Criminal)
            ? m_Criminal.Get()
            : nullptr
    );
    return closest;
}

// 0x68C6E0
bool CTaskComplexKillCriminal::ChangeTarget(CPed* newTarget) { // TODO: Figure out if `newTarget` is actually the new target, or it's just he ped that is the owner of this task
    if (newTarget == m_Criminal) {
        return true;
    }

    if (NoPedOrNoHp(newTarget)) {
        return false;
    }

    if (m_Criminal && m_Criminal->bInVehicle) {
        return false;
    }

    if (notsa::isa_and_nonnull<CTaskComplexKillPedOnFoot>(m_pSubTask) && !m_pSubTask->MakeAbortable(newTarget, ABORT_PRIORITY_URGENT, nullptr)) {
        return false;
    }

    if (!notsa::contains(m_Cop->m_apCriminalsToKill, newTarget)) { // 0x68c760
        return false;
    }

    m_Criminal = newTarget;

    // Propagate change to partner
    if (const auto partner = m_Cop->m_pCopPartner) {
        if (partner->bInVehicle) {
            if (const auto partnersTask = partner->GetTaskManager().Find<CTaskComplexKillCriminal>(false)) {
                notsa::cast<CTaskComplexKillCriminal>(partnersTask)->ChangeTarget(newTarget);
            }
        }
    }

    m_HasFinished = false;

    return true;
}

// 0x68DAD0
bool CTaskComplexKillCriminal::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if ([&, this]{
        if (!event) {
            return true;
        }

        // Code @ 0x68DB32 has been inlined into the stuff below

        switch (const auto evType = event->GetEventType()) {
        case EVENT_ACQUAINTANCE_PED_HATE:
        case EVENT_VEHICLE_DAMAGE_COLLISION: // Ignore these
            return false;

        case EVENT_DAMAGE:
        case EVENT_VEHICLE_DAMAGE_WEAPON:
        case EVENT_GUN_AIMED_AT:
        case EVENT_SHOT_FIRED: {
            const auto evSrc  = event->GetSourceEntity();
            if (m_Criminal && evSrc == m_Criminal) {
                return false; // As per 0x68DB32
            }
            if (!evSrc || !evSrc->GetIsTypePed() || evSrc->AsPed()->IsPlayer()) {
                return false;
            }
            const auto evSrcPed = evSrc->AsPed();
            if (!m_Cop || m_Cop->AddCriminalToKill(evSrcPed) == (notsa::IsFixBugs() ? -1 : 0)) {
                return false;
            }
            if (notsa::contains({ EVENT_DAMAGE, EVENT_VEHICLE_DAMAGE_WEAPON }, evType)) { // Change target immediately
                if (!m_Criminal || (m_Criminal->GetPosition() - ped->GetPosition()).SquaredMagnitude() <= sq(25.f)) {
                    ChangeTarget(evSrcPed);
                }
            }
            return false;
        }
        }
        return true;
    }()) {
        return m_pSubTask->MakeAbortable(ped, priority, event);
    } else {
        const_cast<CEvent*>(event)->m_nTimeActive++; // ???????
        return false;
    }
}

// 0x68E4F0
CTask* CTaskComplexKillCriminal::CreateNextSubTask(CPed* ped) {
    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_GANG_DRIVEBY:
        return CreateSubTask(
            m_Cop->m_isTheDriver
                ? TASK_COMPLEX_CAR_DRIVE_MISSION
                : TASK_SIMPLE_CAR_DRIVE,
            ped
        );
    case TASK_COMPLEX_KILL_PED_ON_FOOT: { // Try finding the next criminal, if none, set `m_finished` and get into *the* car (if possible)
        CPed* const nextCriminal = NoPedOrNoHp(m_Criminal)
            ? FindNextCriminalToKill(ped, true)
            : m_Criminal.Get();
        if (nextCriminal && ChangeTarget(nextCriminal)) {
            return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped, true);
        }

        // No criminal, or can't target it, so just bail, so try getting back into the car
        m_HasFinished = true;
        if (m_CantGetInCar || !ped->m_pVehicle) {
            return CreateSubTask(TASK_FINISHED, ped);
        }

        if (!m_Cop->m_isTheDriver) {
            if (NoPedOrNoHp(m_Cop->m_pCopPartner)) { // Partner is dead, we get in as the driver
                m_Cop->m_isTheDriver = true;
                m_Cop->SetPartner(nullptr);
            } else {
                return CreateSubTask(TASK_COMPLEX_ENTER_CAR_AS_PASSENGER, ped);
            }
        }

        return CreateSubTask(TASK_COMPLEX_ENTER_CAR_AS_DRIVER, ped);
    }
    case TASK_COMPLEX_ENTER_CAR_AS_DRIVER: { // 0x68E533
        if (!ped->bInVehicle) {
            return CreateSubTask(
                m_HasFinished || NoPedOrNoHp(m_Criminal)
                    ? TASK_FINISHED
                    : TASK_COMPLEX_KILL_PED_ON_FOOT,
                ped
            );
        }
        const auto copPartnerNoneOrInVeh = NoPedOrNoHp(m_Cop->m_pCopPartner) || m_Cop->m_pCopPartner->bInVehicle;
        if (!m_HasFinished && !NoPedOrNoHp(m_Criminal) && !m_Criminal->IsInVehicle()) {
            return CreateSubTask(
                !ped->m_pVehicle || m_Criminal->IsEntityInRange(ped->m_pVehicle, 25.f) // 0x68E5D8
                    ? TASK_COMPLEX_KILL_PED_ON_FOOT
                    : copPartnerNoneOrInVeh // otherwise if criminal is too far chase them with the car
                        ? TASK_COMPLEX_CAR_DRIVE_MISSION
                        : TASK_SIMPLE_CAR_DRIVE,
                ped
            );
        }
        if (const auto next = FindNextCriminalToKill(ped, true); next && ChangeTarget(next)) { // Try finding another criminal to kill
            return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped, true);
        }
        // No criminal to kill, so get into *the* vehicle and fuck off
        if (ped->IsInVehicle()) {
            ped->m_pVehicle->vehicleFlags.bSirenOrAlarm = false;
        }
        return CreateSubTask(
            copPartnerNoneOrInVeh
                ? TASK_FINISHED
                : TASK_SIMPLE_CAR_DRIVE,
            ped
        );
    }
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER: { // 0x68E6FA
        if (ped->bInVehicle) { // (Inverted)
            return CreateSubTask(TASK_SIMPLE_CAR_DRIVE, ped);
        }
        m_CantGetInCar = true;
        if (m_HasFinished || NoPedOrNoHp(m_Criminal)) {
            return CreateSubTask(TASK_FINISHED, ped);
        } 
        return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped);
    }
    case TASK_COMPLEX_LEAVE_CAR: // 0x68E77F
        return CreateSubTask(
            !ped->bInVehicle || m_CantGetInCar || (!m_HasFinished && !NoPedOrNoHp(m_Criminal) && !m_Criminal->IsInVehicle() && m_Criminal->IsEntityInRange(ped, 25.f))
                ? TASK_COMPLEX_KILL_PED_ON_FOOT     // Criminal can be killed on foot
                : TASK_COMPLEX_ENTER_CAR_AS_DRIVER, // We have to chase the criminal with a vehicle
            ped
        );
    default:
        NOTSA_UNREACHABLE();
    }
}

// 0x68DC60
CTask* CTaskComplexKillCriminal::CreateFirstSubTask(CPed* ped) {
    if (!m_Criminal || m_Criminal->IsPlayer()) {
        return nullptr;
    }
    if (FindPlayerPed()->GetPlayerData()->m_pWanted->GetWantedLevel() != eWantedLevel::WANTED_CLEAN || !g_LoadMonitor.IsAmbientCrimeEnabled()) {
        return nullptr;
    }
    if (ped->m_nPedType != PED_TYPE_COP) {
        return nullptr;
    }

    // 0x68DCF1 - Don't go after anyone that is in the same vehicle as a player
    if (const auto criminalVeh = m_Criminal->m_pVehicle) {
        if (criminalVeh->m_pDriver && criminalVeh->m_pDriver->IsPlayer()) {
            return nullptr;
        }
        for (auto i = 0; i < m_Criminal->m_pVehicle->m_nMaxPassengers; i++) {
            const auto psgr = m_Criminal->m_pVehicle->m_apPassengers[i];
            if (psgr && psgr->IsPlayer()) {
                return nullptr;
            }
        }
    }

    // 0x68DD55
    m_Cop = ped->AsCop();

    if (m_Randomize) { // 0x68DD6A
        if (!m_Criminal->bCanClimbOntoBoat) { // NOTE: Bit 11 of the 4th flag dword (`*(uint8*)(ped + 0x479) & 8`)
            const auto criminalVeh = m_Criminal->bInVehicle ? m_Criminal->m_pVehicle : nullptr;
            if (!criminalVeh || !criminalVeh->vehicleFlags.bMadDriver) {
                return nullptr;
            }
            if (CGeneral::GetRandomNumberInRange(0, 3) != 0) {
                return nullptr;
            }
        }
    }

    // 0x68DDBF
    m_Cop->AddCriminalToKill(m_Criminal);
    if (m_Criminal->bInVehicle) {
        m_Criminal->GetIntelligence()->SetPedDecisionMakerType(eDecisionMakerType::PED_EMPTY);
    }

    const auto AddKillCriminalEventToPartner = [this] {
        CEventAcquaintancePedHate event{ m_Criminal, TASK_COMPLEX_KILL_CRIMINAL };
        m_Cop->m_pCopPartner->GetEventGroup().Add(&event, false);
    };

    // What to do when we (or our partner) are in a vehicle [0x68E036]
    const auto CreateSubTaskForCriminal = [&, this]() -> CTask* {
        if (!m_Criminal->bInVehicle || !m_Criminal->m_pVehicle) {
            return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped);
        }
        return CreateSubTask(
            m_Cop->m_isTheDriver
                ? TASK_COMPLEX_CAR_DRIVE_MISSION
                : TASK_SIMPLE_CAR_DRIVE,
            ped
        );
    };

    CTask* subTask{};
    if (!m_Cop->bInVehicle || !m_Cop->m_pVehicle) { // 0x68E205
        if (NoPedOrNoHp(m_Cop->m_pCopPartner)) {
            m_Cop->m_isTheDriver = true;
        }
        subTask = CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped);
    } else if (const auto driver = m_Cop->m_pVehicle->m_pDriver; driver == ped) { // 0x68DE11
        m_Cop->m_isTheDriver = true;

        // Find a partner (if we don't have one yet)
        if (!m_Cop->m_pCopPartner) {
            for (auto i = 0; i < m_Cop->m_pVehicle->m_nMaxPassengers; i++) {
                const auto psgr = m_Cop->m_pVehicle->m_apPassengers[i];
                if (psgr && psgr->m_nPedType == PED_TYPE_COP) {
                    m_Cop->SetPartner(psgr->AsCop());
                    break;
                }
            }
        }

        if (m_Cop->m_pCopPartner) {
            AddKillCriminalEventToPartner();
        }

        subTask = CreateSubTaskForCriminal();
    } else { // 0x68DEC3
        if (driver && driver->m_nPedType == PED_TYPE_COP) {
            m_Cop->SetPartner(driver->AsCop());
            m_Cop->m_pVehicle->m_pDriver->AsCop()->m_isTheDriver = true;
        }

        const auto partner = m_Cop->m_pCopPartner;
        if (!partner) { // 0x68DFE1
            m_Cop->m_isTheDriver = true;
            subTask = CreateSubTask(TASK_COMPLEX_LEAVE_CAR, ped);
            if (!subTask) {
                subTask = CreateSubTaskForCriminal();
            }
        } else if (!partner->m_isTheDriver) {
            subTask = CreateSubTaskForCriminal();
        } else {
            const auto partnersTask = static_cast<CTaskComplexKillCriminal*>(partner->GetIntelligence()->FindTaskByType(TASK_COMPLEX_KILL_CRIMINAL));
            if (!partnersTask || partnersTask->m_Criminal.Get() != m_Criminal.Get()) { // 0x68DF7B - Wait for the partner to go after the same criminal
                AddKillCriminalEventToPartner();
                return nullptr;
            }
            m_Cop->m_isTheDriver     = false;
            m_Cop->bDontDragMeOutCar = true;
            subTask = CreateSubTaskForCriminal();
        }
    }

    // 0x68E2DE - Save the vehicle's state, so that it can be restored later
    if (ped->m_pVehicle && m_Cop->m_isTheDriver) {
        const auto& ap = ped->m_pVehicle->m_autoPilot;
        m_OrigDrivingMode = (int8)ap.m_nCarDrivingStyle;
        m_OrigMission     = (int8)ap.m_nCarMission;
        m_OrigCruiseSpeed = ap.m_nCruiseSpeed;
        m_IsSetUp         = true;
    }

    // 0x68E31E - Arm the criminal(s)
    if (const auto criminalVeh = m_Criminal->m_pVehicle) {
        if (criminalVeh->vehicleFlags.bMadDriver) {
            if (criminalVeh->m_nNumPassengers > 0) { // 0x68E3B2
                m_Criminal->GiveWeapon(WEAPON_PISTOL, 1000, true);
                m_Criminal->SetCurrentWeapon(WEAPON_PISTOL);
                for (auto i = 0; i < m_Criminal->m_pVehicle->m_nMaxPassengers; i++) {
                    const auto psgr = m_Criminal->m_pVehicle->m_apPassengers[i];
                    if (!psgr) {
                        continue;
                    }
                    psgr->GiveWeapon(WEAPON_PISTOL, 1000, true);
                    psgr->SetCurrentWeapon(WEAPON_PISTOL);
                    m_Cop->AddCriminalToKill(psgr);
                }
            } else if (CGeneral::GetRandomNumber() & 1) {
                m_Criminal->GiveWeapon(WEAPON_PISTOL, 1000, true);
                m_Criminal->SetCurrentWeapon(WEAPON_PISTOL);
            }
        }
    } else if (CGeneral::GetRandomNumber() & 1) { // 0x68E334
        m_Criminal->GiveWeapon(WEAPON_PISTOL, 1000, true);
        m_Criminal->SetCurrentWeapon(WEAPON_PISTOL);

        // Make the criminal fight back
        CEventAcquaintancePedHate event{ ped, TASK_COMPLEX_KILL_PED_ON_FOOT };
        m_Criminal->GetEventGroup().Add(&event, false);
    }

    // 0x68E43C - Make sure we (and our vehicle) won't be removed for a while
    ped->m_nTimeTillWeNeedThisPed = CTimer::GetTimeInMS() + 300'000;
    ped->bCullExtraFarAway        = true;
    ped->m_fRemovalDistMultiplier = 0.3f;
    if (const auto veh = ped->m_pVehicle) {
        veh->m_nExtendedRemovalRange                   = 255;
        veh->vehicleFlags.bNeverUseSmallerRemovalRange = true;
        if (ped->bInVehicle) {
            veh->vehicleFlags.bSirenOrAlarm = true;
        }
    }

    g_InterestingEvents.Add(CInterestingEvents::INTERESTING_EVENT_25, ped);

    return subTask;
}

// 0x68E950
CTask* CTaskComplexKillCriminal::ControlSubTask(CPed* ped) {
    const auto origSubTask = m_pSubTask;

    // 0x68E96B
    if (m_Criminal) {
        if (   m_Criminal->IsPlayer()
            || notsa::contains({ PED_TYPE_COP, PED_TYPE_MEDIC, PED_TYPE_FIREMAN }, m_Criminal->m_nPedType)
            || m_Criminal->m_nPedType >= PED_TYPE_MISSION1
            || m_Criminal->IsCreatedByMission()
        ) {
            return nullptr;
        }
    }

    // 0x68E9C4 - Player became wanted, stop bothering with ambient criminals
    if (FindPlayerPed()->GetPlayerData()->m_pWanted->GetWantedLevel() != eWantedLevel::WANTED_CLEAN) {
        if (FindPlayerWanted()->CanCopJoinPursuit(ped->AsCop()) && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            return nullptr;
        }
    }

    // 0x68EA26
    if (!g_LoadMonitor.IsAmbientCrimeEnabled()) {
        return nullptr;
    }

    auto tt        = TASK_NONE;
    auto bothAlive = false;

    // NOTE: This pointer is used by the original code even after `SetPartner(nullptr)` below
    CCopPed* const partner = m_Cop->m_pCopPartner;

    // 0x68EA2F - Partner is dead (or we have none), so we're the driver from now on
    if (!m_Cop->m_isTheDriver && NoPedOrNoHp(partner)) {
        m_Cop->m_isTheDriver = true;
        m_Cop->SetPartner(nullptr);
        if (ped->bInVehicle && ped->m_pVehicle) {
            tt = TASK_COMPLEX_LEAVE_CAR;
        }
    }

    const auto PartnerGoneOrInVeh = [&] {
        return !partner || partner->m_fHealth <= 0.f || partner->bInVehicle;
    };

    if (ped->m_fHealth <= 0.f) { // 0x68EA8F
        tt = TASK_FINISHED;
    } else if (NoPedOrNoHp(m_Criminal)) { // 0x68F476 - Criminal is dead, try finding another one
        if (m_pSubTask->GetTaskType() != TASK_COMPLEX_KILL_PED_ON_FOOT) {
            const auto next = FindNextCriminalToKill(ped, true);
            if (   !(next && ChangeTarget(next))
                && m_pSubTask->GetTaskType() != TASK_COMPLEX_ENTER_CAR_AS_PASSENGER
                && m_pSubTask->GetTaskType() != TASK_COMPLEX_ENTER_CAR_AS_DRIVER
            ) {
                if (ped->m_pVehicle && !ped->bInVehicle && !m_CantGetInCar) { // 0x68F4DB - Get back into the car
                    tt = m_Cop->m_isTheDriver
                        ? TASK_COMPLEX_ENTER_CAR_AS_DRIVER
                        : TASK_COMPLEX_ENTER_CAR_AS_PASSENGER;
                } else if (!ped->bInVehicle) { // 0x68F4FE
                    tt = TASK_FINISHED;
                } else if (m_pSubTask->GetTaskType() != TASK_SIMPLE_CAR_DRIVE || PartnerGoneOrInVeh()) { // 0x68F506
                    tt = TASK_FINISHED;
                }
            }
        }
    } else if (!m_Cop->m_isTheDriver) { // 0x68EAD9 - We're the passenger
        bothAlive = true;
        tt        = TASK_COMPLEX_KILL_PED_ON_FOOT;
        if (partner->bInVehicle || partner->GetIntelligence()->FindTaskByType(TASK_COMPLEX_ENTER_CAR_AS_DRIVER)) {
            if (const auto veh = ped->m_pVehicle; veh && !m_CantGetInCar) {
                if (!ped->bInVehicle) {
                    tt = TASK_COMPLEX_ENTER_CAR_AS_PASSENGER;
                } else if (!partner->bInVehicle) {
                    tt = TASK_SIMPLE_CAR_DRIVE;
                } else { // 0x68EB42
                    tt = (m_Criminal->GetPosition() - veh->GetPosition()).Magnitude() < 60.f
                        ? TASK_SIMPLE_GANG_DRIVEBY
                        : TASK_SIMPLE_CAR_DRIVE;
                }
            }
        }
    } else { // 0x68EBB4 - We're the driver
        bothAlive = true;
        if (ped->m_pVehicle && ped->bInVehicle) {
            ped->m_pVehicle->vehicleFlags.bSirenOrAlarm = true;
        }

        CPed* const criminal = m_Criminal;
        if (criminal->bInVehicle && criminal->m_pVehicle) { // 0x68EBF1 - Criminal is in a vehicle
            const auto criminalVeh = criminal->m_pVehicle;

            // Make the occupants of the criminal's vehicle react
            auto makeOccupantsReact = criminalVeh->m_fHealth < 250.f;

            if (ped->bInVehicle && ped->m_pVehicle) { // 0x68EC28
                const auto subTaskType = m_pSubTask->GetTaskType();
                if (subTaskType == TASK_COMPLEX_CAR_DRIVE_MISSION || subTaskType == TASK_SIMPLE_GANG_DRIVEBY) { // 0x68EC8C
                    if (m_Criminal->m_pVehicle->GetMoveSpeed().Magnitude() >= 0.12f) {
                        m_TimeToGetOutOfCar = 1.f;
                    } else {
                        m_TimeToGetOutOfCar = m_TimeToGetOutOfCar - CTimer::GetTimeStep() * 0.02f;
                        if (   m_TimeToGetOutOfCar <= 0.f
                            && m_Criminal->GetIntelligence()->GetEventHandler().GetCurrentEventType() != EVENT_ACQUAINTANCE_PED_HATE
                            && (m_Criminal->m_pVehicle->GetPosition() - ped->m_pVehicle->GetPosition()).SquaredMagnitude() < sq(15.f)
                        ) {
                            tt = TASK_COMPLEX_KILL_PED_ON_FOOT;
                            if (const auto next = FindNextCriminalToKill(ped, false)) {
                                ChangeTarget(next);
                            }
                        }
                    }
                } else if (subTaskType == TASK_SIMPLE_CAR_DRIVE) { // 0x68EC59
                    if (PartnerGoneOrInVeh()) {
                        tt = TASK_COMPLEX_CAR_DRIVE_MISSION;
                    }
                }
            } else if (criminalVeh->GetMoveSpeed().Magnitude() < 0.2f) { // 0x68ED9C - Criminal's vehicle has stopped and we (or our partner) are on foot next to it
                if ((criminal->GetPosition() - ped->GetPosition()).SquaredMagnitude() < sq(6.f)) {
                    makeOccupantsReact = true;
                } else if (partner && !partner->bInVehicle && (criminal->GetPosition() - partner->GetPosition()).SquaredMagnitude() < sq(6.f)) {
                    makeOccupantsReact = true;
                }
            } else if ( // 0x68EE6A - They're getting away, get back in the car
                   ped->m_pVehicle
                && !m_CantGetInCar
                && m_pSubTask->GetTaskType() != TASK_COMPLEX_ENTER_CAR_AS_DRIVER
                && m_pSubTask->GetTaskType() != TASK_COMPLEX_ENTER_CAR_AS_PASSENGER
            ) {
                const auto distSq = (m_Criminal->m_pVehicle->GetPosition() - ped->m_pVehicle->GetPosition()).SquaredMagnitude();
                if (m_Criminal->m_pVehicle->GetMoveSpeed().Magnitude() >= 0.2f || distSq > sq(20.f)) {
                    tt = m_Cop->m_isTheDriver
                        ? TASK_COMPLEX_ENTER_CAR_AS_DRIVER
                        : TASK_COMPLEX_ENTER_CAR_AS_PASSENGER;
                }
            }

            if (makeOccupantsReact) { // 0x68EF56
                CEventAcquaintancePedHate event{ ped };
                event.m_TaskId = m_Criminal->m_pVehicle->m_fHealth < 250.f
                    ? TASK_COMPLEX_LEAVE_CAR
                    : TASK_COMPLEX_KILL_PED_AND_REENTER_CAR; // (Original code has a [always true] null-check of the active weapon here, otherwise it would be `TASK_COMPLEX_FOLLOW_LEADER_IN_FORMATION`)

                m_Criminal->GetEventGroup().Add(&event, false);
                m_Criminal->SetPedDefaultDecisionMaker();

                if (const auto driver = m_Criminal->m_pVehicle->m_pDriver; driver && driver != m_Criminal && !driver->IsPlayer()) {
                    m_Criminal->m_pVehicle->m_pDriver->GetEventGroup().Add(&event, false);
                }

                for (int32 i = 0; i < m_Criminal->m_pVehicle->m_nMaxPassengers; i++) {
                    const auto psgr = m_Criminal->m_pVehicle->m_apPassengers[i];
                    if (!psgr || psgr == m_Criminal || psgr->IsPlayer()) {
                        continue;
                    }
                    psgr->GetEventGroup().Add(&event, false);
                    psgr->SetPedDefaultDecisionMaker();
                }
            }
        } else if (const auto veh = ped->GetVehicleIfInOne()) { // 0x68F0C3 - Criminal is on foot, we're in a vehicle
            const auto distSq = (criminal->GetPosition() - veh->GetPosition()).SquaredMagnitude();
            const auto partnerGoneOrInVeh = PartnerGoneOrInVeh();
            if (distSq > sq(15.f)) {
                if (partnerGoneOrInVeh) {
                    tt = TASK_COMPLEX_CAR_DRIVE_MISSION;
                }
            } else { // 0x68F15B
                const auto getOutDistSq = veh->IsBike()
                    ? 5.f
                    : 16.f;
                if (notsa::contains({ MISSION_KILLPED_CLOSE, MISSION_KILLPED_FARAWAY }, veh->m_autoPilot.m_nCarMission)) {
                    if (distSq < getOutDistSq) {
                        tt = TASK_COMPLEX_KILL_PED_ON_FOOT;
                    }
                } else {
                    tt = TASK_COMPLEX_KILL_PED_ON_FOOT;
                }
            }
        } else if (!m_CantGetInCar && ped->m_pVehicle) { // 0x68F1B1 - Both of us are on foot
            if (   (criminal->GetPosition() - ped->GetPosition()).SquaredMagnitude() > sq(25.f)
                || (ped->m_pVehicle->GetPosition() - ped->GetPosition()).SquaredMagnitude() > 250.f
            ) { // 0x68F277 - Criminal (or our car) is too far, try another one, or get back into the car
                const auto next = FindNextCriminalToKill(ped, false);
                tt = next && ChangeTarget(next)
                    ? TASK_COMPLEX_KILL_PED_ON_FOOT
                    : TASK_COMPLEX_ENTER_CAR_AS_DRIVER;
            }
        }
    }

    // 0x68F2A1 - Bikes: Do driveby while following the criminal
    // (Only reached if both us and the criminal were alive above)
    if (bothAlive) {
        if (const auto veh = ped->m_pVehicle; veh && veh->IsBike()) {
            const auto DoDriveBy = [&] {
                const auto subTaskType = m_pSubTask->GetTaskType();
                const auto origTT      = tt;
                if (subTaskType == TASK_COMPLEX_CAR_DRIVE_MISSION && (tt == TASK_NONE || tt == subTaskType)) {
                    if ((m_Criminal->GetPosition() - ped->m_pVehicle->GetPosition()).Magnitude() < 60.f) {
                        tt = TASK_SIMPLE_GANG_DRIVEBY;
                        return true;
                    }
                }
                return origTT == TASK_SIMPLE_GANG_DRIVEBY || m_pSubTask->GetTaskType() == TASK_SIMPLE_GANG_DRIVEBY;
            };
            if (DoDriveBy()) { // 0x68F35E
                const auto ap = &ped->m_pVehicle->m_autoPilot;
                ped->m_pVehicle->SetStatus(STATUS_PHYSICS);
                if (m_Criminal->bInVehicle && m_Criminal->m_pVehicle) {
                    ap->m_nCarMission      = MISSION_FOLLOWCAR_CLOSE;
                    ap->m_nCruiseSpeed     = (uint8)((float)m_Criminal->m_pVehicle->m_autoPilot.m_nCruiseSpeed + 10.f);
                    ap->m_speed            = (float)ap->m_nCruiseSpeed;
                    ap->m_nCarDrivingStyle = DRIVING_STYLE_AVOID_CARS;
                    ap->m_TargetEntity     = m_Criminal->m_pVehicle;
                } else {
                    ap->m_nCarMission      = MISSION_KILLPED_CLOSE;
                    ap->m_nCruiseSpeed     = 20;
                    ap->m_speed            = (float)ap->m_nCruiseSpeed;
                    ap->m_nCarDrivingStyle = DRIVING_STYLE_AVOID_CARS;
                    ap->m_TargetEntity     = reinterpret_cast<CVehicle*>(m_Criminal.Get()); // NOTE: No reference is registered by the original code
                }
            }
        }
    }

    // 0x68F53C - Make the passengers of the criminal's vehicle shoot at us
    if (m_Criminal && m_Criminal->bInVehicle) {
        if (const auto criminalVeh = m_Criminal->m_pVehicle; criminalVeh && criminalVeh->m_nNumPassengers > 0) {
            if ((criminalVeh->GetPosition() - ped->GetPosition()).Magnitude() < 60.f) {
                for (int32 i = 0; i < m_Criminal->m_pVehicle->m_nMaxPassengers; i++) {
                    const auto psgr = m_Criminal->m_pVehicle->m_apPassengers[i];
                    if (!psgr || !psgr->bInVehicle || psgr->GetIntelligence()->FindTaskByType(TASK_SIMPLE_GANG_DRIVEBY)) {
                        continue;
                    }
                    psgr->GetTaskManager().SetTask(
                        new CTaskSimpleGangDriveBy{
                            ped,
                            nullptr,
                            70.f,
                            70,
                            eDrivebyStyle::AI_ALL_DIRN,
                            false
                        },
                        TASK_PRIMARY_PRIMARY
                    );
                }
            }
        }
    }

    // 0x68F691
    if (tt == TASK_NONE || !m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
        return origSubTask;
    }
    return CreateSubTask(tt, ped); // Inlined in the original code
}
