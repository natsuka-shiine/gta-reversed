#include "StdInc.h"

#include "TaskComplexProstituteSolicit.h"
#include "Ragdoll/IKChainManager.h"
#include "TaskComplexLeaveCar.h"
#include "TaskComplexEnterCarAsPassenger.h"
#include "TaskSimpleStandStill.h"
#include "TaskComplexCarDrive.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "SeekEntity/TaskComplexSeekEntityXYOffset.h"
#include "CarEnterExit.h"
#include "Messages.h"
#include "Stats.h"
#include "Cheat.h"
#include "Wanted.h"

void CTaskComplexProstituteSolicit::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexProstituteSolicit, 0x86FB88, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(CreateSubTask, 0x666360);

    RH_ScopedVMTInstall(CreateNextSubTask, 0x666780);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x6666A0);
    RH_ScopedVMTInstall(ControlSubTask, 0x6669D0);
}

// 0x661A60
CTaskComplexProstituteSolicit::CTaskComplexProstituteSolicit(CPed* client) : CTaskComplex() {
    m_nLastSavedTime = 0;
    m_nNextTimeToCheckForSecludedPlace = 0;
    m_nLastPaymentTime = 0;
    m_nCurrentTimer = 0;
    m_pClient = client;
    m_nVehicleMovementTimer = 850;
    b07 = true;
    b08 = true;
    b10 = true;
    m_pClient->RegisterReference(m_pClient);
}

// 0x661AF0
CTaskComplexProstituteSolicit::~CTaskComplexProstituteSolicit() {
    auto player = FindPlayerPed();
    if (!player)
        return;

    CEntity::ClearReference(player->GetPlayerData()->m_pCurrentProstitutePed);
    if (bMoveCameraDown) {
        bMoveCameraDown = false;
    }
}

// 0x661B80
bool CTaskComplexProstituteSolicit::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    bool aborted = m_pSubTask->MakeAbortable(ped, priority, event);
    if (aborted) {
        bMoveCameraDown = false;
    }
    return aborted;
}

// 0x661D30
void CTaskComplexProstituteSolicit::GetRidOfPlayerProstitute() {
    auto* prostitute = FindPlayerPed()->GetPlayerData()->m_pCurrentProstitutePed;
    if (!prostitute)
        return;

    const auto intel = prostitute->GetIntelligence();
    auto* task = intel->FindTaskByType(TASK_COMPLEX_PROSTITUTE_SOLICIT);
    if (!task)
        return;

    auto* t = static_cast<CTaskComplexProstituteSolicit*>(task);
    t->bTaskCanBeFinished = true;
    t->m_nCurrentTimer = 0;
}

// 0x666360
CTask* CTaskComplexProstituteSolicit::CreateSubTask(eTaskType taskType, CPed* prostitute) {
    switch (taskType) {
    case TASK_COMPLEX_SEEK_ENTITY: {
        const auto clientVeh = m_pClient->m_pVehicle;

        CMatrix invVehMat;
        Invert(clientVeh->GetMatrix(), invVehMat);

        // Go to whichever front door is closer (passenger's side preferred on a tie)
        const auto  driverDoorPos    = CCarEnterExit::GetPositionToOpenCarDoor(clientVeh, TARGET_DOOR_DRIVER);
        const auto  passengerDoorPos = CCarEnterExit::GetPositionToOpenCarDoor(clientVeh, TARGET_DOOR_FRONT_RIGHT);
        const auto& pos              = prostitute->GetPosition();
        const auto  doorPos          = (pos - passengerDoorPos).SquaredMagnitude() <= (pos - driverDoorPos).SquaredMagnitude()
            ? passengerDoorPos
            : driverDoorPos;
        const auto  offset           = invVehMat.TransformPoint(doorPos);

        const auto task = new CTaskComplexSeekEntityXYOffset{ clientVeh, 50'000, 1'000, 1.f, 2.f, 2.f, false, false };
        task->GetSeekPosCalculator().SetOffset(offset);
        return task;
    }
    case TASK_COMPLEX_TURN_TO_FACE_ENTITY:
        return new CTaskComplexTurnToFaceEntityOrCoord(m_pClient, 0.5f, 0.2f);
    case TASK_COMPLEX_CAR_DRIVE:
        bSearchingForSecludedPlace = true;
        bSexProcessStarted         = false;
        return new CTaskComplexCarDrive(m_pClient->m_pVehicle);
    case TASK_SIMPLE_STAND_STILL:
        return new CTaskSimpleStandStill(5000, false, false, 8.0f);
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER:
        return new CTaskComplexEnterCarAsPassenger(m_pClient->m_pVehicle, TARGET_DOOR_FRONT_RIGHT, false);
    case TASK_COMPLEX_LEAVE_CAR:
        return new CTaskComplexLeaveCar(m_pClient->m_pVehicle, 0, 0, true, false);
    default:
        return nullptr;
    }
}

// 0x661BB0
bool CTaskComplexProstituteSolicit::IsTaskValid(CPed* prostitute, CPed* ped) {
    if (FindPlayerPed() != ped)
        return false;

    if (!ped)
        return false;

    if (!ped->IsInVehicle())
        return false;

    if (ped->bIsBeingArrested)
        return false;

    if (ped->GetPlayerData()->m_pCurrentProstitutePed && ped->GetPlayerData()->m_pCurrentProstitutePed != prostitute)
        return false;

    if (ped->m_pVehicle->GetVehicleAppearance() != VEHICLE_APPEARANCE_AUTOMOBILE)
        return false;

    if (ped->m_pVehicle->IsUpsideDown())
        return false;

    if (ped->m_pVehicle->IsOnItsSide())
        return false;

    auto task = ped->GetTaskManager().GetSimplestActiveTask();
    if (task->GetTaskType() != TASK_SIMPLE_CAR_DRIVE)
        return false;

    if (ped->m_pVehicle->m_pDriver != ped)
        return false;

    if (prostitute->m_pVehicle) {
        if (prostitute->m_pVehicle != ped->m_pVehicle || prostitute->m_pVehicle->m_nNumPassengers != 1)
            return false;
    } else if (ped->m_pVehicle->m_nNumPassengers) {
        return false;
    }

    if (!ped->m_pVehicle->m_nMaxPassengers || ped->m_pVehicle->m_pHandlingData->m_bTandemSeats)
        return false;

    CVector out = ped->GetPosition() - prostitute->GetPosition();
    if (out.SquaredMagnitude() > 100.0f || CTheScripts::IsPlayerOnAMission() || CGameLogic::IsCoopGameGoingOn()) {
        return false;
    }
    return true;
}

// 0x6666A0
CTask* CTaskComplexProstituteSolicit::CreateFirstSubTask(CPed* ped) {
    if (!IsTaskValid(ped, m_pClient)) {
        bTaskCanBeFinished = true;
        return nullptr;
    }

    m_vecVehiclePosn = m_pClient->m_pVehicle->GetPosition();

    m_pClient->GetPlayerData()->m_pCurrentProstitutePed = ped;
    {
        auto& current = FindPlayerPed()->GetPlayerData()->m_pCurrentProstitutePed;
        current->RegisterReference(current);
    }

    if (auto& last = m_pClient->GetPlayerData()->m_pLastProstituteShagged; last != ped) {
        CEntity::SafeCleanUpRef(last);
        last = ped;
        last->RegisterReference(last);
    }

    return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
}

// 0x666780
CTask* CTaskComplexProstituteSolicit::CreateNextSubTask(CPed* ped) {
    if (!m_pClient) {
        return nullptr;
    }

    if (!IsTaskValid(ped, m_pClient)) {
        bTaskCanBeFinished = true;
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_STAND_STILL: {
        if (bPlayerHasAcceptedSexProposition) {
            if (CCheat::IsActive(CHEAT_PROSTITUTES_PAY_YOU) || FindPlayerPed()->GetPlayerInfoForThisPlayerPed()->m_nMoney >= 20) {
                return CreateSubTask(TASK_COMPLEX_ENTER_CAR_AS_PASSENGER, ped);
            }
            CMessages::ClearMessages(false);
            CMessages::AddMessageQ(TheText.Get("PROS_06"), 2000, 1, true); // You've got money right?
            CMessages::AddMessageQ(TheText.Get("PROS_09"), 3000, 1, true); // Stop wasting my time!
        }
        return CreateSubTask(TASK_FINISHED, ped);
    }
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER: {
        ped->Say(CTX_GLOBAL_SOLICIT_THANKS);
        m_vecVehiclePosn = m_pClient->m_pVehicle->GetPosition();
        return CreateSubTask(TASK_COMPLEX_CAR_DRIVE, ped);
    }
    case TASK_COMPLEX_LEAVE_CAR: {
        g_ikChainMan.LookAt("TaskProzzy", ped, m_pClient, 2500, BONE_UNKNOWN, nullptr, false, 0.25f, 500, 3, false);
        return CreateSubTask(TASK_FINISHED, ped);
    }
    case TASK_COMPLEX_CAR_DRIVE:
        return CreateSubTask(TASK_COMPLEX_LEAVE_CAR, ped);
    case TASK_COMPLEX_SEEK_ENTITY: {
        g_ikChainMan.LookAt("TaskProzzy", ped, m_pClient, 5000, BONE_UNKNOWN, nullptr, false, 0.25f, 500, 3, false);
        return CreateSubTask(TASK_COMPLEX_TURN_TO_FACE_ENTITY, ped);
    }
    case TASK_COMPLEX_TURN_TO_FACE_ENTITY: {
        ped->Say(CTX_GLOBAL_SOLICIT);
        CMessages::AddMessageQ(TheText.Get("PROS_04"), 5000, 1, true); // You want a good time, honey?
        return CreateSubTask(TASK_SIMPLE_STAND_STILL, ped);
    }
    default:
        return nullptr;
    }
}

// 0x6669D0
CTask* CTaskComplexProstituteSolicit::ControlSubTask(CPed* ped) {
    bMoveCameraDown = bSexProcessStarted;

    if (!IsTaskValid(ped, m_pClient)) {
        bMoveCameraDown    = false;
        bTaskCanBeFinished = true;
    }

    const auto timeStepMs = (int16)(CTimer::GetTimeStep() * 0.02f * 1000.0f);

    if (bTaskCanBeFinished) {
        if (m_nCurrentTimer == 0) {
            if (m_pSubTask->GetTaskType() != TASK_COMPLEX_LEAVE_CAR && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
                return CreateSubTask(TASK_COMPLEX_LEAVE_CAR, ped);
            }
        } else {
            m_nCurrentTimer -= timeStepMs;
            if (m_nCurrentTimer <= 0) {
                CAEPedSpeechAudioEntity::SetCJMood(MOOD_WR, 120'000, -1, -1, -1);
                m_pClient->Say(CTX_GLOBAL_AFTER_SEX);
                m_nCurrentTimer = 0;
            }
        }
        return m_pSubTask;
    }

    if (m_pSubTask->GetTaskType() == TASK_SIMPLE_STAND_STILL) {
        const auto pad = CPad::GetPad(0);
        if (pad->ConversationYesJustDown()) {
            bPlayerHasAcceptedSexProposition = true;
            if (m_pClient) {
                m_pClient->Say(CTX_GLOBAL_SOLICIT_PRO_YES);
            }
            return CreateNextSubTask(ped);
        }
        if (pad->ConversationNoJustDown()) {
            if (m_pClient) {
                m_pClient->Say(CTX_GLOBAL_SOLICIT_PRO_NO);
            }
            bTaskCanBeFinished = true;
        }
        return m_pSubTask;
    }

    if (m_pSubTask->GetTaskType() == TASK_COMPLEX_SEEK_ENTITY || m_pSubTask->GetTaskType() == TASK_COMPLEX_ENTER_CAR_AS_PASSENGER) {
        // Client drove away
        if ((m_pClient->m_pVehicle->GetPosition() - m_vecVehiclePosn).SquaredMagnitude() > sq(4.0f)) {
            bTaskCanBeFinished = true;
        }
        return m_pSubTask;
    }

    if (m_pSubTask->GetTaskType() != TASK_COMPLEX_CAR_DRIVE) {
        return m_pSubTask;
    }

    const auto timeMs = CTimer::GetTimeInMS();

    if (TheCamera.m_nWhoIsInControlOfTheCamera == 1) { // Script is in control
        m_nLastSavedTime = timeMs;
        m_nCurrentTimer  = 8000;
        bMoveCameraDown  = false;
        b08              = true;
        bVehicleShifted  = true;
        return m_pSubTask;
    }

    const auto isVehStopped = (m_pClient->m_pVehicle->m_vecMoveSpeed * 50.0f).SquaredMagnitude() < sq(0.75f);
    if (!isVehStopped || b08) {
        b08              = false;
        m_nLastSavedTime = timeMs;
    }

    const auto wanted = FindPlayerWanted();

    if (timeMs > (uint32)m_nNextTimeToCheckForSecludedPlace) {
        bPedsCanPotentiallySeeThis         = false;
        bPedsCanSeeThis                    = false;
        bCopsCanSeeThis                    = false;
        m_nNextTimeToCheckForSecludedPlace = timeMs + 1000;

        const auto entities = ped->GetIntelligence()->GetPedEntities();
        for (auto i = 0; i < 16; i++) {
            const auto other = static_cast<CPed*>(entities[i]);
            if (!other || other == m_pClient || other->m_nPedType == PED_TYPE_PROSTITUTE) {
                continue;
            }

            const auto distSq = (other->GetPosition() - ped->GetPosition()).SquaredMagnitude();
            if (distSq < sq(7.5f)) {
                bPedsCanSeeThis = true;
            }
            if (distSq < sq(20.0f)) {
                bPedsCanPotentiallySeeThis = true;
            }

            if (other->m_nPedType == PED_TYPE_COP && bSexProcessStarted && wanted && (int32)wanted->m_WantedLevel < 1) {
                const auto clientVeh        = m_pClient->m_pVehicle;
                const auto vehUsesCollision = clientVeh->m_bUsesCollision;
                clientVeh->m_bUsesCollision = false;
                const auto isLOSClear = CWorld::GetIsLineOfSightClear(other->GetPosition(), m_pClient->GetPosition(), true, true, false, true, false, true, false);
                m_pClient->m_pVehicle->m_bUsesCollision = vehUsesCollision;
                if (isLOSClear) {
                    bCopsCanSeeThis = true;
                }
            }
        }
    }

    if (bSearchingForSecludedPlace) {
        if (isVehStopped && timeMs - (uint32)m_nLastSavedTime > 4000) {
            if (!bPedsCanPotentiallySeeThis) {
                bSearchingForSecludedPlace = false;
                bSexProcessStarted         = true;
                m_nLastPaymentTime         = timeMs;
                CMessages::AddMessageQ(TheText.Get("PROS_02"), 2000, 1, true);
            } else if (!bSecludedPlaceMessageShown) {
                CMessages::AddMessageQ(TheText.Get("PROS_01"), 3000, 1, true);
                bSecludedPlaceMessageShown = true;
            }
        }
        return m_pSubTask;
    }

    if (!bSexProcessStarted) {
        return m_pSubTask;
    }

    if (b07) {
        b07             = false;
        m_nCurrentTimer = 15'000;
        CStats::IncrementStat(STAT_NUMBER_OF_PROSTITUTES_VISITED, 1.0f);
    }

    const auto pad                = CPad::GetPad(0);
    const auto isPlayerDrivingOff = pad->GetAccelerate() || pad->GetBrake();
    const auto rnd                = rand(); // NOTE: On PC this is in range [0, 0x7FFF], so some of the checks below can never pass

    auto wasSeenByCop = false;
    if (bCopsCanSeeThis && wanted && (int32)wanted->m_WantedLevel < 1) {
        FindPlayerWanted()->SetWantedLevel(eWantedLevel::WANTED_LEVEL_1);
        wasSeenByCop = true;
    }

    if (isPlayerDrivingOff || bPedsCanSeeThis || wasSeenByCop) {
        m_nLastSavedTime           = timeMs;
        bSexProcessStarted         = false;
        bSearchingForSecludedPlace = true;
        bVehicleShifted            = true;

        if (!isPlayerDrivingOff) {
            if (m_nCurrentTimer >= 3000) {
                CMessages::AddMessageQ(TheText.Get("PROS_01"), 3000, 1, true);
                m_nCurrentTimer = 8000;
            } else {
                bTaskCanBeFinished = true;
                m_nCurrentTimer    = 0;
            }
        } else if (rnd >= 0x1FFFFFFF && m_nCurrentTimer >= 3000) {
            m_nCurrentTimer = 8000;
        } else {
            bTaskCanBeFinished = true;
            m_nCurrentTimer    = 0;
            CMessages::AddMessageQ(TheText.Get("PROS_09"), 3000, 1, true);
        }
        return m_pSubTask;
    }

    // Rock the vehicle
    m_nVehicleMovementTimer -= timeStepMs;
    if (m_nVehicleMovementTimer <= 0) {
        auto forceMult = std::lerp(-0.5f, -0.9f, CGeneral::GetRandomNumberInRange(0.0f, 1.0f)); // = GetRandomNumberInRange(-0.5f, -0.9f)

        if (m_nCurrentTimer > 10'000) {
            m_nVehicleMovementTimer = 850;
        } else if (m_nCurrentTimer > 5000) {
            m_nVehicleMovementTimer = 450;
        } else if (m_nCurrentTimer > 1000) {
            m_nVehicleMovementTimer = 120;
        } else {
            forceMult *= 0.5f;
            m_nVehicleMovementTimer = 850;
            CPad::GetPad(0)->StartShake(1000, 120, 0);
        }

        const auto clientVeh = m_pClient->m_pVehicle;
        const auto vehPos    = clientVeh->GetPosition();
        const auto clientPos = m_pClient->GetPosition();
        const auto force     = std::max(std::min(150.0f, clientVeh->m_fMass / 15.0f), m_pClient->m_fMass);
        const auto point     = (rnd & 1) ? clientPos : ped->GetPosition();
        clientVeh->ApplyTurnForce(
            CVector{ 0.0f, 0.0f, force * forceMult },
            CVector{ point.x - vehPos.x, point.y - vehPos.y, 0.0f }
        );
        m_pClient->m_pVehicle->m_vehicleAudio.AddAudioEvent(AE_SUSPENSION_BOUNCE, 0.0f);

        if (b10 && rnd > 0x0FFFFFFF) {
            ped->Say((rnd & 0xFFFF) >= 0xFF ? CTX_GLOBAL_HAVING_SEX : CTX_GLOBAL_GIVING_HEAD, 0, 0.5f);
        }
    }

    m_nCurrentTimer -= timeStepMs;
    if (m_nCurrentTimer <= 0) {
        m_nCurrentTimer    = 3000;
        bSexProcessStarted = false;
        bTaskCanBeFinished = true;
    }

    if (timeMs - (uint32)m_nLastPaymentTime > 1000) {
        m_nLastPaymentTime = timeMs;

        const auto playerInfo = m_pClient->AsPlayer()->GetPlayerInfoForThisPlayerPed();
        if (CCheat::IsActive(CHEAT_PROSTITUTES_PAY_YOU)) {
            playerInfo->m_nMoney += 2;
        } else if (playerInfo->m_nMoney < 2) {
            playerInfo->m_nMoney = 0;
            m_nCurrentTimer      = 0;
            bSexProcessStarted   = false;
            bTaskCanBeFinished   = true;
            CMessages::ClearMessages(false);
            CMessages::AddMessageQ(TheText.Get("PROS_06"), 2000, 1, true); // You've got money right?
            CMessages::AddMessageQ(TheText.Get("PROS_09"), 3000, 1, true); // Stop wasting my time!
        } else {
            playerInfo->m_nMoney -= 2;
            CStats::IncrementStat(STAT_PROSTITUTE_BUDGET, 2.0f);
            ped->m_nMoneyCount++;
        }

        if (!bVehicleShifted) {
            playerInfo->AddHealth(2);
        }
    }

    return m_pSubTask;
}
