#include "StdInc.h"
#include "TaskSimpleCarDrive.h"
#include "TaskUtilityLineUpPedWithCar.h"
#include "TaskSimpleCarSetPedOut.h"
#include "Ragdoll/IKChainManager.h"
#include "TaskSimpleGangDriveBy.h"
#include "TaskSimpleHoldEntity.h"
#include "TaskComplexSequence.h"
#include "TaskComplexLeaveCar.h"
#include "TaskComplexDriveWander.h"
#include "EventScriptCommand.h"
#include "EventCarUpsideDown.h"
#include "EventCopCarBeingStolen.h"
#include "VehicleAnimGroupData.h"
#include "UpsideDownCarCheck.h"
#include "TheScripts.h"
#include "PedGroups.h"
#include "Cheat.h"
#include "Train.h"
#include "ModelIndices.h"
#include "CarEnterExit.h"

void CTaskSimpleCarDrive::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleCarDrive, 0x86E904, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x63C340);
    RH_ScopedInstall(Destructor, 0x63C460);

    RH_ScopedInstall(TriggerIK, 0x63C500);
    RH_ScopedInstall(UpdateBopping, 0x63C900);
    RH_ScopedInstall(StartBopping, 0x642760);
    RH_ScopedInstall(ProcessHeadBopping, 0x6428C0);
    RH_ScopedInstall(ProcessArmBopping, 0x642AE0);
    RH_ScopedInstall(ProcessBopping, 0x642E70);
    RH_ScopedInstall(StartCloseDoorRolling, 0x642700);
    RH_ScopedVMTInstall(Clone, 0x63DC20);
    RH_ScopedVMTInstall(GetTaskType, 0x63C450);
    RH_ScopedVMTInstall(MakeAbortable, 0x63C670);
    RH_ScopedVMTInstall(ProcessPed, 0x644470);
    RH_ScopedVMTInstall(SetPedPosition, 0x63C770);
}

// 0x63C340
CTaskSimpleCarDrive::CTaskSimpleCarDrive(CVehicle* vehicle, CTaskUtilityLineUpPedWithCar* utilityTask, bool updateCurrentVehicle) : CTaskSimple() {
    m_pVehicle = vehicle;
    m_pAnimCloseDoorRolling = nullptr;
    m_pTaskUtilityLineUpPedWithCar = nullptr;
    m_LeaveCarTimer = CTaskTimer{};
    m_nTimePassedSinceCarUpSideDown = 0;

    m_bUpdateCurrentVehicle = updateCurrentVehicle;
    m_b08 = true;

    CEntity::SafeRegisterRef(m_pVehicle);

    if (utilityTask) {
        m_pTaskUtilityLineUpPedWithCar = new CTaskUtilityLineUpPedWithCar(CVector{}, 0, utilityTask->m_nDoorOpenPosType, utilityTask->m_nDoorIdx);
    }

    m_fHeadBoppingFactor = 0.0f;
    m_fHeadBoppingOrientation = 0.0f;
    m_fRandomHeadBoppingMultiplier = 0.0f;
    m_nBoppingStartTime = -1;
}

// 0x63C460
CTaskSimpleCarDrive::~CTaskSimpleCarDrive() {
    CEntity::SafeCleanUpRef(m_pVehicle);

    if (m_pTaskUtilityLineUpPedWithCar) {
        delete m_pTaskUtilityLineUpPedWithCar;
        m_pTaskUtilityLineUpPedWithCar = nullptr;
    }

    if (m_b20 && m_pAnimCloseDoorRolling) {
        // TODO: FIX ME: Keeps triggering, annoying as fuck
        // Seemingly happens when getting of a motorbike (like cops getting off)
        //assert(m_pAnimCloseDoorRolling);
        m_pAnimCloseDoorRolling->SetFinishCallback(CDefaultAnimCallback::DefaultAnimCB, nullptr);
        if (m_pVehicle) {
            m_pVehicle->ClearGettingOutFlags(1);
        }
    }
}

// 0x63C500
void CTaskSimpleCarDrive::TriggerIK(CPed* ped) const {
    if (!m_pVehicle) {
        return;
    }

    // Made an early-out of this, as this same code is used in both possible cases.
    if (g_ikChainMan.IsLooking(ped) || CGeneral::GetRandomNumberInRange(0, 100) >= 5) {
        return;
    }

    switch (m_pVehicle->m_autoPilot.m_nCarMission) {
    case MISSION_RAMPLAYER_FARAWAY:
    case MISSION_RAMPLAYER_CLOSE:
    case MISSION_BLOCKPLAYER_FARAWAY:
    case MISSION_BLOCKPLAYER_CLOSE:
    case MISSION_BLOCKPLAYER_HANDBRAKESTOP: { // Make ped look at player ped
        g_ikChainMan.LookAt("DriveCar", ped, FindPlayerPed(0), 3000, BONE_HEAD, nullptr, false, 0.25f, 500, 3, false);
        break;
    }
    case MISSION_RAMCAR_FARAWAY:
    case MISSION_RAMCAR_CLOSE: {
        if (const auto vehTargetCar = m_pVehicle->m_autoPilot.m_TargetEntity) {
            if (vehTargetCar->GetIsTypeVehicle()) {
                if (const auto driver = vehTargetCar->m_pDriver) { // Make ped look at target car or it's driver (if any)
                    g_ikChainMan.LookAt("DriveCar", ped, driver, 3000, BONE_HEAD, nullptr, false, 0.25f, 500, 3, false);
                } else {
                    g_ikChainMan.LookAt("DriveCar", ped, vehTargetCar, 3000, BONE_UNKNOWN, nullptr, false, 0.25f, 500, 3, false);
                }
            }
        }
        break;
    }
    }
}

// 0x63C900
void CTaskSimpleCarDrive::UpdateBopping() {
    const auto timeDelta = (int32)CTimer::GetTimeInMS() - m_nBoppingStartTime;
    m_fBoppingProgress = (float)(timeDelta % m_nBoppingEndTime) / (float)m_nBoppingEndTime;
    m_nBoppingCompletedTimes = timeDelta / m_nBoppingEndTime % 2;
}

// 0x642760
void CTaskSimpleCarDrive::StartBopping(CPed* ped) {
    int32 boppingStart = -1;
    int32 boppingEnd = 0;
    if (CPed* const driver = ped->m_pVehicle->m_pDriver; driver && driver != ped) {
        if (auto* const task = driver->GetTaskManager().FindActiveTaskByType(TASK_SIMPLE_CAR_DRIVE)) {
            boppingStart = static_cast<CTaskSimpleCarDrive*>(task)->m_nBoppingStartTime;
            boppingEnd = static_cast<CTaskSimpleCarDrive*>(task)->m_nBoppingEndTime;
            if (boppingStart != -1) {
                goto CopyBopping;
            }
        }
    }
    for (uint32 i = 0; i < 3; i++) {
        CPed* const passenger = ped->m_pVehicle->m_apPassengers[i];
        if (passenger && passenger != ped) {
            if (auto* const task = passenger->GetTaskManager().FindActiveTaskByType(TASK_SIMPLE_CAR_DRIVE)) {
                boppingEnd = static_cast<CTaskSimpleCarDrive*>(task)->m_nBoppingEndTime;
                boppingStart = static_cast<CTaskSimpleCarDrive*>(task)->m_nBoppingStartTime;
            }
            if (boppingStart != -1) {
                goto CopyBopping;
            }
        }
    }
    if (boppingStart == -1) {
        m_nBoppingStartTime = CTimer::GetTimeInMS();
        const int32 frames = 60 - (int)((float)(rand() & 0xFFFF) * (1.0f / 32768.0f) * -60.0f);
        m_nBoppingEndTime = (int)((1.0f / ((float)frames * (1.0f / 60.0f))) * 1000.0f);
        m_fBoppingProgress = 0.0f;
        return;
    }
CopyBopping:
    m_nBoppingStartTime = boppingStart;
    m_nBoppingEndTime = boppingEnd;
    const int32 elapsed = CTimer::GetTimeInMS() - boppingStart;
    m_fBoppingProgress = (float)(elapsed % boppingEnd) / (float)boppingEnd;
    m_nBoppingCompletedTimes = elapsed / boppingEnd % 2;
}
// 0x6428C0
void CTaskSimpleCarDrive::ProcessHeadBopping(CPed* ped, bool enable, float distSq) {
    float target = 0.0f;
    if (enable) {
        if (!m_b01) {
            if (m_fHeadBoppingFactor == 0.0f && CGeneral::GetRandomNumberInRange(0, 1000) > 995) {
                if (m_nBoppingStartTime == -1) {
                    StartBopping(ped);
                    m_nHeadBoppingStartTime = m_nBoppingStartTime;
                } else {
                    m_nHeadBoppingStartTime = CTimer::GetTimeInMS();
                }
                m_nHeadBoppingDirection = CGeneral::GetRandomNumberInRange(1, 3);
                m_fRandomHeadBoppingMultiplier = CGeneral::GetRandomNumberInRange(2.0f, 8.0f);
                m_fHeadBoppingFactor = 0;
                m_fHeadBoppingOrientation = 0.0f;
                target = 0.0f;
                goto ApplyHeadFlag;
            }
        } else {
            target = std::sin(m_fBoppingProgress * PI);
            if (CTimer::GetTimeInMS() - m_nHeadBoppingStartTime <= 5001 || CGeneral::GetRandomNumberInRange(0, 1000) <= 995) {
                goto ApplyHeadFlag;
            }
        }
        enable = false;
    }
ApplyHeadFlag:
    m_b01 = enable;
    float delta;
    if (m_b01) {
        m_fHeadBoppingFactor = std::min(m_fHeadBoppingFactor + 0.05f, 1.0f);
        delta = (target - m_fHeadBoppingOrientation) * m_fHeadBoppingFactor;
    } else {
        m_fHeadBoppingFactor = std::max(m_fHeadBoppingFactor - 0.05f, 0.0f);
        delta = (target - m_fHeadBoppingOrientation) * (1.0f - m_fHeadBoppingFactor);
    }
    m_fHeadBoppingOrientation += delta;
    if (distSq < 64.0f && m_fHeadBoppingOrientation > 0.0f) {
        float angle = m_fRandomHeadBoppingMultiplier * m_fHeadBoppingOrientation;
        auto* head = ped->m_apBones[PED_NODE_HEAD]->KeyFrame;
        if (m_nHeadBoppingDirection >= 1) {
            float a = angle;
            if (m_nBoppingCompletedTimes != 0) {
                a = -a;
            }
            RtQuatRotate(&head->q, &CPedIK::XaxisIK, a, rwCOMBINEPOSTCONCAT);
        }
        if (m_nHeadBoppingDirection != 2) {
            RtQuatRotate(&head->q, &CPedIK::ZaxisIK, -angle, rwCOMBINEPOSTCONCAT);
        }
        ped->bUpdateMatricesRequired = true;
    }
}
// 0x642AE0
void CTaskSimpleCarDrive::ProcessArmBopping(CPed* ped, bool enable, float distSq) {
    if (ped->IsPlayer()) {
        return;
    }
    CVehicle* const veh = ped->m_pVehicle;
    if (CWeather::Rain <= 0.0f) {
        uint8 door = 0;
        if (veh->m_pDriver == ped) {
            door = 10;
        } else if (veh->m_apPassengers[0] == ped) {
            door = 8;
        } else if (veh->m_apPassengers[1] == ped) {
            door = 11;
        } else if (veh->m_apPassengers[2] == ped) {
            door = 9;
        } else {
            goto FindWinAnim;
        }
        veh->SetWindowOpenFlag(door);
    } else {
        veh->ClearWindowOpenFlag(10);
        veh->ClearWindowOpenFlag(8);
        veh->ClearWindowOpenFlag(11);
        veh->ClearWindowOpenFlag(9);
        enable = false;
    }
FindWinAnim:
    const bool isFrontLeft = veh->m_apPassengers[0] == ped;
    const AnimationId winAnim = (isFrontLeft || veh->m_apPassengers[2] == ped) ? ANIM_ID_TAP_HANDP : ANIM_ID_TAP_HAND;
    CAnimBlendAssociation* const assoc = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), winAnim);
    if (!enable) {
        if (assoc) {
            assoc->m_BlendDelta = -4.0f;
        }
        m_b02 = false;
        return;
    }
    if (!m_b02) {
        if (CGeneral::GetRandomNumberInRange(0, 1000) > 995) {
            bool canBop = true;
            // OG checks `m_pVehicle->m_nVehicleSubType == 0` (AUTOMOBILE); otherwise it skips the checks and blends directly. (0x642BE8)
            if (m_pVehicle->m_nVehicleSubType == VEHICLE_TYPE_AUTOMOBILE) {
                const eTargetDoor door = (eTargetDoor)CCarEnterExit::ComputeTargetDoorToExit(m_pVehicle, ped);
                // OG bails if the door isn't perfectly OK, if there's a door left to close, or if the ped can't lean out. (0x642C08)
                if (static_cast<CAutomobile*>(m_pVehicle)->GetDamageManager().GetDoorStatus_Component((tComponent)door) != DAMSTATE_OK
                    || CCarEnterExit::CarHasDoorToClose(m_pVehicle, door)
                    || !m_pVehicle->CanPedLeanOut(ped)
                ) {
                    canBop = false;
                }
                // The original then reads three floats (the hand-rest dummy at index 10) from the vehicle model
                // struct plus the front-seat dummy height to decide if the car is tall enough for the lean-out
                // arm bop. Thresholds: 0.39/0.46, or 0.40/0.44 for truck-driving-anim groups. (0x642C3A)
                const auto* vehStruct = m_pVehicle->GetVehicleModelInfo()->GetVehicleStruct();
                if (!vehStruct->IsDummyActive(DUMMY_HAND_REST) || m_pVehicle->vehicleFlags.bLowVehicle) {
                    return;
                }
                const float height = vehStruct->m_avDummyPos[DUMMY_HAND_REST].z - vehStruct->m_avDummyPos[DUMMY_SEAT_FRONT].z;
                const bool useTruckAnims = CVehicleAnimGroupData::UsesTruckDrivingAnims(m_pVehicle->GetAnimGroupId());
                if (height < (useTruckAnims ? 0.40f : 0.39f) || height > (useTruckAnims ? 0.44f : 0.46f)) {
                    return;
                }
            }
            if (canBop) {
                CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, winAnim, 8.0f);
                if (m_nBoppingStartTime == -1) {
                    StartBopping(ped);
                    m_nArmBoppingStartTime = m_nBoppingStartTime;
                    m_b02 = true;
                    return;
                }
                m_nArmBoppingStartTime = CTimer::GetTimeInMS();
                m_b02 = true;
                return;
            }
        }
    } else {
        if (!assoc) {
            m_b02 = false;
            return;
        }
        if (distSq < 64.0f) {
            float amp = 0.0f;
            if (m_fBoppingProgress >= 0.75f) {
                amp = (m_fBoppingProgress - 0.75f) * 45.0f;
            }
            auto* arm = (winAnim == ANIM_ID_TAP_HAND ? ped->m_apBones[PED_NODE_RIGHT_ARM] : ped->m_apBones[PED_NODE_LEFT_ARM])->KeyFrame;
            RtQuatRotate(&arm->q, &CPedIK::ZaxisIK, -amp, rwCOMBINEPOSTCONCAT);
            ped->bUpdateMatricesRequired = true;
        }
        if (CTimer::GetTimeInMS() - m_nArmBoppingStartTime > 5000 && CGeneral::GetRandomNumberInRange(0, 1000) > 995) {
            assoc->m_BlendDelta = -4.0f;
            m_b02 = false;
            return;
        }
    }
}

// 0x642E70
void CTaskSimpleCarDrive::ProcessBopping(CPed* ped, bool a3) {
    if (ped->m_pVehicle->m_pDriver == FindPlayerPed(0)
        || ped->m_nPedType == PED_TYPE_COP
        || ped->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT_AND_STAND_UP)
    ) {
        return;
    }

    auto* vehicle = ped->m_pVehicle;
    if (vehicle->IsAutomobile() && !vehicle->IsSubQuad() && !ped->IsCreatedByMission()) {
        if (m_nBoppingStartTime != -1) { // IsBopping
            UpdateBopping();
        }

        const auto dist = DistanceBetweenPointsSquared(TheCamera.GetPosition(), ped->GetPosition());
        ProcessHeadBopping(ped, a3, dist);
        ProcessArmBopping(ped, a3, dist);
        if (m_nBoppingStartTime != -1 && !m_b01 && !m_b02) {
            m_nBoppingStartTime = -1;
        }
    }
}

// 0x63DC20


CTask* CTaskSimpleCarDrive::Clone() const {
    auto task = new CTaskSimpleCarDrive(m_pVehicle);
    task->m_bUpdateCurrentVehicle = m_bUpdateCurrentVehicle;
    return task;
}

// 0x63C670
bool CTaskSimpleCarDrive::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if (priority != ABORT_PRIORITY_IMMEDIATE) {
        m_b08 = true;
        return !m_b20;
    }
    if (ped->bInVehicle && ped->m_pVehicle) {
        CTaskSimpleCarSetPedOut setPedOutTask{
            ped->m_pVehicle,
            (eTargetDoor)CCarEnterExit::ComputeTargetDoorToExit(ped->m_pVehicle, ped),
            false
        };
        if (ped->m_pVehicle->m_nVehicleType == VEHICLE_TYPE_BIKE && event) {
            setPedOutTask.m_bFallingOutOfCar = true;
        }
        setPedOutTask.ProcessPed(ped);
    }
    if (g_ikChainMan.IsLooking(ped)) {
        g_ikChainMan.AbortLookAt(ped, 250);
    }
    return true;
}

// 0x644470
bool CTaskSimpleCarDrive::ProcessPed(CPed* ped) {
    const auto hasRadioRetuneJustStarted = AudioEngine.HasRadioRetuneJustStarted();

    if (m_bUpdateCurrentVehicle && ped->bInVehicle && ped->m_pVehicle && ped->m_pVehicle != m_pVehicle) {
        CEntity::ChangeEntityReference(m_pVehicle, ped->m_pVehicle);
    }

    if (!m_pVehicle || !ped->bInVehicle) {
        ped->bInVehicle = false;
        if (g_ikChainMan.IsLooking(ped)) {
            g_ikChainMan.AbortLookAt(ped, 250);
        }
        return true;
    }

    auto* const group = CPedGroups::GetPedsGroup(ped);

    // Car upside down?
    if (!CTheScripts::UpsideDownCars.IsCarUpsideDown(m_pVehicle)) {
        m_nTimePassedSinceCarUpSideDown = 0;
    } else {
        if (ped != FindPlayerPed() && FindPlayerPed()->bInVehicle && FindPlayerPed()->m_pVehicle == m_pVehicle) {
            ped->Say(CTX_GLOBAL_CAR_FLIPPED);
        }
        if (!group || group->GetMembership().IsLeader(ped)) {
            m_nTimePassedSinceCarUpSideDown += (int32)(CTimer::ms_fTimeStep * 0.02f * 1000.0f);
            if (m_nTimePassedSinceCarUpSideDown > 2000) {
                CEventCarUpsideDown event{ m_pVehicle };
                ped->GetEventGroup().Add(&event, false);
            }
        }
    }

    // Player is driving a cop car?
    if (ped->IsPlayer() && m_pVehicle->IsLawEnforcementVehicle()) {
        if (!m_copCarStolenTimer.m_bStarted || m_copCarStolenTimer.IsOutOfTime()) {
            m_copCarStolenTimer.m_nStartTime = CTimer::GetTimeInMS();
            m_copCarStolenTimer.m_nInterval  = 2000;
            m_copCarStolenTimer.m_bStarted   = true;

            CEventCopCarBeingStolen event{ ped, m_pVehicle };
            GetEventGlobalGroup()->Add(&event, false);
        }
    }

    if (m_pVehicle->m_nVehicleType == VEHICLE_TYPE_BIKE || m_pVehicle->m_nVehicleSubType == VEHICLE_TYPE_QUAD || m_pVehicle->m_nModelIndex == MODEL_TRACTOR) {
        ped->bTestForShotInVehicle = true;
    }

    const auto GiveDriveByTask = [&](bool seatRHS) {
        CEventScriptCommand event{
            TASK_PRIMARY_PRIMARY,
            new CTaskSimpleGangDriveBy{ nullptr, nullptr, 100.0f, 100, eDrivebyStyle::AI_ALL_DIRN, seatRHS },
            false
        };
        ped->GetEventGroup().Add(&event, false);
    };
    const auto HasDriveByWeapon = [&] {
        const auto& wep = ped->m_aWeapons[4];
        return wep.m_Type != WEAPON_UNARMED && (int32)wep.m_TotalAmmo > 0;
    };

    // What to do after the driving anims/door closing has been dealt with
    enum class Next {
        DRIVING_ANIMS, //< Process driving anims + bopping
        COMMON,        //< Just the common part
    } next = Next::COMMON;

    if (ped->IsPlayer() && m_pVehicle->m_pDriver == ped) { // Player driving
        auto* const pad = ped->AsPlayer()->GetPadFromPlayer();

        if (CCheat::IsActive(CHEAT_WEAPON_AIMING_WHILE_DRIVING) && HasDriveByWeapon() && m_pVehicle->CanPedLeanOut(ped)) {
            GiveDriveByTask(false);
        }

        const auto IsPadIdle = [&] {
            return pad->GetAccelerate() == 0 && pad->GetSteeringLeftRight() == 0 && pad->GetBrake() == 0;
        };
        const auto GetRollDoorAnimGroup = [&](AnimationId animId) {
            return CVehicleAnimGroupData::GetGroupForAnim((AssocGroupId)m_pVehicle->m_pHandlingData->m_nAnimGroup, animId);
        };

        auto* const pedVeh = ped->m_pVehicle;
        if (pedVeh->m_nModelIndex != MODEL_STREAKC && pedVeh->m_nVehicleType == VEHICLE_TYPE_TRAIN && !pedVeh->AsTrain()->m_aDoors[0].IsClosed()) {
            // Close the train's door
            if (!pad || IsPadIdle()) {
                pedVeh->ProcessOpenDoor(ped, TARGET_DOOR_DRIVER, GetRollDoorAnimGroup(ANIM_ID_CAR_ROLLDOOR), ANIM_ID_CAR_ROLLDOOR, 1.0f);
            }
        } else if (pedVeh->m_nVehicleType != VEHICLE_TYPE_AUTOMOBILE || pedVeh->AsAutomobile()->m_damageManager.GetDoorStatus(DOOR_LEFT_FRONT) != DAMSTATE_OPENED) {
            next = Next::DRIVING_ANIMS;
        } else if (!pedVeh->IsDoorMissing(DOOR_LEFT_FRONT)) {
            // The driver's door is open, so close it
            auto* const rollDoorAnim = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_CAR_ROLLDOOR);
            if ((pedVeh->m_nGettingOutFlags & 1) || (pedVeh->m_nGettingInFlags & 1) || rollDoorAnim) {
                if (rollDoorAnim) {
                    pedVeh->ProcessOpenDoor(ped, TARGET_DOOR_DRIVER, GetRollDoorAnimGroup(rollDoorAnim->GetAnimId()), ANIM_ID_CAR_ROLLDOOR, rollDoorAnim->m_CurrentTime);
                }
                ProcessBopping(ped, false);
            } else if (pad && !IsPadIdle()) {
                ProcessBopping(ped, false);
            } else {
                pedVeh->SetGettingOutFlags(1);
                m_b20 = true;
                StartCloseDoorRolling(ped);
                return false;
            }
        }
    } else if (m_pVehicle->m_pDriver == ped) { // AI driver
        next = Next::DRIVING_ANIMS;
    } else if (m_pVehicle->IsPassenger(ped)) {
        if (!m_b08 && !m_b10) {
            if (m_pVehicle->GetRideAnimData()) {
                CAnimManager::BlendAnimation(ped->GetRpClump(), m_pVehicle->GetRideAnimData()->AnimGroup, ANIM_ID_BIKE_PASSENGER, 8.0f);
            }
            m_b10 = true;
        }
        ProcessBopping(ped, true);

        if (   ped->IsPlayer()
            && CGameLogic::IsCoopGameGoingOn()
            && HasDriveByWeapon()
            && m_pVehicle->CanPedLeanOut(ped)
            && TheCamera.m_aCams[TheCamera.m_nActiveCam].m_nMode == MODE_TWOPLAYER_IN_CAR_AND_SHOOTING
        ) {
            GiveDriveByTask(m_pVehicle->m_apPassengers[1] != ped);
        }
    }

    if (next == Next::DRIVING_ANIMS) {
        m_pVehicle->ProcessDrivingAnims(ped, hasRadioRetuneJustStarted);
        ProcessBopping(ped, true);
    }

    // Smoking
    if (auto* const heldEntity = ped->GetEntityThatThisPedIsHolding()) {
        if (heldEntity->m_nModelIndex == ModelIndices::MI_GANG_SMOKE && !g_ikChainMan.IsLooking(ped) && CGeneral::GetRandomNumberInRange(0, 60) == 15) {
            if (auto* const holdTask = static_cast<CTaskSimpleHoldEntity*>(ped->GetTaskManager().FindActiveTaskByType(TASK_SIMPLE_HOLD_ENTITY))) {
                holdTask->PlayAnim(
                    m_pVehicle->IsPassenger(ped) ? ANIM_ID_PASS_SMOKE_IN_CAR : ANIM_ID_SMOKE_IN_CAR,
                    ANIM_GROUP_DEFAULT
                );
            }
        }
    }
    if (!RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_SMOKE_IN_CAR)) {
        TriggerIK(ped);
    } else {
        if (g_ikChainMan.IsLooking(ped)) {
            g_ikChainMan.AbortLookAt(ped, 250);
        }
        m_nBoppingStartTime = -1;
    }

    // Random peds sitting in the front passenger seat of a driverless car will take over the wheel after a while
    if (ped->GetCreatedBy() == PED_GAME && !CPedGroups::GetPedsGroup(ped)) {
        if (m_pVehicle->m_pDriver || m_pVehicle->m_apPassengers[0] != ped) {
            m_LeaveCarTimer.m_bStarted = false;
        } else {
            if (!m_LeaveCarTimer.m_bStarted) {
                m_LeaveCarTimer.m_nStartTime = CTimer::GetTimeInMS();
                m_LeaveCarTimer.m_nInterval  = 4000;
                m_LeaveCarTimer.m_bStarted   = true;
            }
            if (m_LeaveCarTimer.IsOutOfTime()) {
                auto* const sequence = new CTaskComplexSequence{};
                sequence->AddTask(new CTaskComplexLeaveCar{ m_pVehicle, 0, 0, true, false });
                sequence->AddTask(new CTaskComplexCarDriveWander{ m_pVehicle, DRIVING_STYLE_STOP_FOR_CARS, 10.0f });

                CEventScriptCommand event{ TASK_PRIMARY_PRIMARY, sequence, false };
                ped->GetEventGroup().Add(&event, false);
            }
        }
    }

    // Passenger speech
    if (((ped->m_nRandomSeed + CTimer::m_FrameCounter) & 0x3FFF) == 0) {
        if (auto* const driver = m_pVehicle->m_pDriver; driver && driver != ped && driver->IsPlayer()) {
            if (m_pVehicle->m_vecMoveSpeed.Magnitude2D() > 0.7f) {
                ped->Say(CTX_GLOBAL_CAR_FAST);
            }
            if (m_pVehicle->m_vecMoveSpeed.Magnitude2D() < 0.1f) {
                ped->Say(CTX_GLOBAL_CAR_SLOW);
            }
            ped->Say(CTX_GLOBAL_CAR_SINGALONG);
        }
    }

    return false;
}

// 0x63C840 - Finish callback of the `ANIM_ID_CAR_ROLLDOOR` anim (Not reversed yet, so it's referenced by address)
static const auto FinishAnimCloseDoorRollingCB = reinterpret_cast<void(__cdecl*)(CAnimBlendAssociation*, void*)>(0x63C840);

// 0x642700
void CTaskSimpleCarDrive::StartCloseDoorRolling(CPed* ped) {
    const auto groupId = CVehicleAnimGroupData::GetVehicleAnimGroup(ped->m_pVehicle->m_pHandlingData->m_nAnimGroup).GetGroup(ANIM_ID_CAR_ROLLDOOR);
    m_pAnimCloseDoorRolling = CAnimManager::AddAnimation(ped->GetRpClump(), groupId, ANIM_ID_CAR_ROLLDOOR);
    m_pAnimCloseDoorRolling->SetFinishCallback(FinishAnimCloseDoorRollingCB, this);
}

// 0x63C770

// 0x0
bool CTaskSimpleCarDrive::SetPedPosition(CPed* ped) {
    if (m_b08) {
        if (auto* const task = ped->GetTaskManager().FindTaskByType(TASK_PRIMARY_DEFAULT, TASK_SIMPLE_CAR_DRIVE)) {
            if (task != this) {
                if (auto* const drive = static_cast<CTaskSimpleCarDrive*>(task); drive->m_pTaskUtilityLineUpPedWithCar) {
                    m_pTaskUtilityLineUpPedWithCar = drive->m_pTaskUtilityLineUpPedWithCar;
                    drive->m_pTaskUtilityLineUpPedWithCar = nullptr;
                }
            }
        }
        m_b08 = false;
    }
    if (!m_pTaskUtilityLineUpPedWithCar) {
        if (m_pVehicle && ped->m_pVehicle == m_pVehicle && ped->bInVehicle) {
            ped->SetPedPositionInCar();
        }
        return true;
    }
    static constexpr AnimationId s_GetInAnims[] = {
        ANIM_ID_CAR_GETIN_LHS_0,
        ANIM_ID_CAR_GETIN_RHS_0,
        ANIM_ID_CAR_GETIN_LHS_1,
        ANIM_ID_CAR_GETIN_RHS_1,
        ANIM_ID_CAR_GETIN_BIKE_FRONT,
        ANIM_ID_CAR_CLOSEDOOR_LHS_0,
        ANIM_ID_CAR_CLOSEDOOR_RHS_0,
        ANIM_ID_CAR_CLOSEDOOR_LHS_1,
        ANIM_ID_CAR_CLOSEDOOR_RHS_1,
        ANIM_ID_CAR_SHUFFLE_RHS_0,
        ANIM_ID_CAR_SHUFFLE_RHS_1,
        ANIM_ID_CAR_ROLLDOOR,
    };
    CAnimBlendAssociation* assoc = nullptr;
    for (const auto animId : s_GetInAnims) {
        if ((assoc = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), animId))) {
            break;
        }
    }
    m_pTaskUtilityLineUpPedWithCar->ProcessPed(ped, m_pVehicle, assoc);
    delete m_pTaskUtilityLineUpPedWithCar;
    m_pTaskUtilityLineUpPedWithCar = nullptr;
    return true;
}

