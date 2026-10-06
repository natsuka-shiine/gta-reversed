#include "StdInc.h"

#include "PlayerInfo.h"
#include "FireManager.h"
#include "MenuSystem.h"
#include "Hud.h"
#include "World.h"
#include "RepeatSector.h"
#include "Object.h"
#include "CarEnterExit.h"
#include "PedGeometryAnalyser.h"
#include "Cranes.h"
#include "Tasks/TaskTypes/TaskComplexLeaveCar.h"
#include "Tasks/TaskTypes/TaskComplexEnterCarAsDriver.h"
#include "Tasks/TaskTypes/TaskComplexEnterCarAsPassenger.h"
#include "Tasks/TaskTypes/TaskComplexGoPickUpEntity.h"
#include "Tasks/TaskTypes/TaskSimpleHoldEntity.h"
#include "Tasks/TaskTypes/TaskSimpleJetPack.h"
#include "Tasks/TaskTypes/TaskSimpleSwim.h"


void CPlayerInfo::InjectHooks() {
    RH_ScopedClass(CPlayerInfo);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Constructor, 0x571920, { .State = HS::RedirectToGTA, .Locked = true }); // hooking ctor will produce bugs with weapons, you will never give weapon through cheat or something
    RH_ScopedInstall(CancelPlayerEnteringCars, 0x56E860);
    RH_ScopedInstall(FindObjectToSteal, 0x56DBD0);
    RH_ScopedInstall(EvaluateCarPosition, 0x56DAD0);
    RH_ScopedInstall(Process, 0x56F8D0);
    RH_ScopedInstall(FindClosestCarSectorList, 0x56F4E0);
    RH_ScopedInstall(Clear, 0x56F330);
    RH_ScopedInstall(StreamParachuteWeapon, 0x56EB30);
    RH_ScopedInstall(AddHealth, 0x56EAB0);
    RH_ScopedInstall(ArrestPlayer, 0x56E5D0);
    RH_ScopedInstall(SetPlayerSkin, 0x5717F0);
    RH_ScopedInstall(LoadPlayerSkin, 0x56F7D0);
    RH_ScopedInstall(DeletePlayerSkin, 0x56EA80);
    RH_ScopedInstall(BlowUpRCBuggy, 0x56EA30);
    RH_ScopedInstall(MakePlayerSafe, 0x56E870);
    RH_ScopedInstall(PlayerFailedCriticalMission, 0x56E830);
    RH_ScopedInstall(WorkOutEnergyFromHunger, 0x56E610);
    RH_ScopedInstall(KillPlayer, 0x56E580);
    RH_ScopedInstall(IsRestartingAfterMissionFailed, 0x56E570);
    RH_ScopedInstall(IsRestartingAfterArrest, 0x56E560);
    RH_ScopedInstall(IsRestartingAfterDeath, 0x56E550);
    RH_ScopedInstall(IsPlayerInRemoteMode, 0x56DAB0);
    RH_ScopedInstall(GetPos_Hook, 0x56DFB0);
    RH_ScopedInstall(GetSpeed_Hook, 0x56DF50);
    RH_ScopedInstall(GivePlayerParachute, 0x56EC40);
    RH_ScopedInstall(SetLastTargetVehicle, 0x56DA80);
    RH_ScopedInstall(Load, 0x5D3B00);
    RH_ScopedInstall(Save, 0x5D3AC0);
}

// 0x571920
CPlayerInfo::CPlayerInfo() {
    plugin::CallMethod<0x571920, CPlayerInfo*>(this); // see hook
    return;

    m_PlayerData = CPlayerPedData();

    m_pSkinTexture = nullptr;
    m_bParachuteReferenced = false;
    m_nRequireParachuteTimer = 0;
}

// 0x571920
CPlayerInfo* CPlayerInfo::Constructor() {
    this->CPlayerInfo::CPlayerInfo();
    return this;
}

CVector* CPlayerInfo::GetSpeed_Hook(CVector* out) {
    *out = GetSpeed();
    return out;
}

CVector* CPlayerInfo::GetPos_Hook(CVector* outPos) {
    *outPos = GetPos();
    return outPos;
}

// 0x56E860
void CPlayerInfo::CancelPlayerEnteringCars(CVehicle* vehicle) {
    // NOP
}

// 0x56DBD0
CEntity* CPlayerInfo::FindObjectToSteal(CPed* ped) {
    const auto& pedPos = ped->GetPosition();
    const auto& pedFwd = ped->GetForward();
    const auto& pedRight = ped->GetRight();
    // Search centre is 1.5 units in front of the ped
    const CVector center = pedPos + pedFwd * (0.5f /*0x858B8C*/ * 3.0f /*0x858B3C*/);

    // Sector range over the 120x120 grid covering a 3.0 radius box around the centre
    const auto toMinSector = [](float v) {
        const auto s = (int32)std::floor(v * 0.02f /*0x858B38*/ + 60.0f /*0x858B34*/);
        return s < 1 ? 0 : s;
    };
    const auto toMaxSector = [](float v) {
        const auto s = (int32)std::floor(v * 0.02f /*0x858B38*/ + 60.0f /*0x858B34*/);
        return s < 119 ? s : 119; // 0x77
    };
    const int32 minX = toMinSector(center.x - 3.0f /*0x858B3C*/);
    const int32 minY = toMinSector(center.y - 3.0f /*0x858B3C*/);
    const int32 maxX = toMaxSector(center.x + 3.0f /*0x858B3C*/);
    const int32 maxY = toMaxSector(center.y + 3.0f /*0x858B3C*/);

    CWorld::AdvanceCurrentScanCode();
    if (maxY < minY) {
        return nullptr;
    }
    CEntity* found = nullptr;
    for (int32 y = minY; y <= maxY; y++) {
        if (minX > maxX) {
            continue;
        }
        for (int32 x = minX; x <= maxX; x++) {
            for (CObject* obj : CWorld::GetRepeatSector(x, y).Objects) {
                const CVector delta = obj->GetPosition() - center;
                if (!obj->objectFlags.bIsLiftable) { // 0x2000
                    continue;
                }
                if (obj->GetScanCode() == CWorld::GetCurrentScanCode()) {
                    continue;
                }
                if (delta.SquaredMagnitude() >= 4.5f /*0x863214*/) {
                    continue;
                }
                float alongFwd = delta.Dot(pedFwd);
                if (alongFwd < 0.0f) {
                    alongFwd = 10.0f /*0x85862C*/ - alongFwd;
                }
                if (alongFwd + std::abs(delta.Dot(pedRight)) * 3.0f /*0x858B3C*/ < 1000.0f /*0x858C4C*/) {
                    found = obj; // last match wins, no early out
                }
            }
        }
    }
    return found;
}

// 0x56DAD0
void CPlayerInfo::EvaluateCarPosition(CEntity* car, CPed* ped, float pedToVehDist, float* outDistance, CVehicle** outVehicle) {
    const auto& carPosn = car->GetPosition();
    const auto& pedPosn = ped->GetPosition();
    const auto& forward = ped->GetForward();

    // Find our rotation (so that is, at which angle the forward vector is)
    const auto angleFront = CGeneral::GetATanOfXY(forward.x, forward.y);

    // Find car angle, not relative to our rotation
    const auto carAngle = CGeneral::GetATanOfXY(carPosn.x - pedPosn.x, carPosn.y - pedPosn.y);

    // Make car's angle relative to our rotation (notice: it's an abs value)
    // Basically, we calculate how much the car is in our FOV.
    const auto carAngleFromFront = std::abs(CGeneral::LimitRadianAngle(angleFront - carAngle));

    // Calculate imaginary distance based on the car's angle: The higher the angle the greater the distance
    const auto distance = (1.0f - carAngleFromFront / TWO_PI) * (10.0f - pedToVehDist);
    if (distance >= *outDistance) {
        *outDistance = distance;
        *outVehicle = car->AsVehicle();
    }
}
// 0x56F4E0
void CPlayerInfo::FindClosestCarSectorList(CPtrListDoubleLink<CVehicle*>& ptrList, CPed* ped, float minX, float minY, float maxX, float maxY, float* outVehDist, CVehicle** outVehicle) {
    UNUSED(minX); UNUSED(minY); UNUSED(maxX); UNUSED(maxY); // Caller pre-filters the list; the range is unused
    for (CVehicle* vehicle : ptrList) {
        if (vehicle->IsScanCodeCurrent() || !vehicle->GetUsesCollision() || vehicle->GetType() != ENTITY_TYPE_VEHICLE) {
            continue;
        }
        vehicle->SetCurrentScanCode();
        const auto status = vehicle->GetStatus();
        if (status == STATUS_WRECKED || status == STATUS_TRAIN_MOVING) {
            continue;
        }
        if (!vehicle->m_matrix) {
            vehicle->AllocateMatrix();
            vehicle->m_placement.UpdateMatrix(vehicle->m_matrix);
        }
        // Overturned non-bikes are skipped (bikes fall over when parked, so they're exempt)
        if (vehicle->GetUp().z <= 0.3f /*0x858C24*/ && vehicle->m_nVehicleType != VEHICLE_TYPE_BIKE) {
            continue;
        }
        const CVector boundCentre = vehicle->GetBoundCentre();
        const CVector& vehPos = vehicle->GetPosition();
        const float groundZ = vehPos.z - vehicle->GetHeightAboveRoad() + 1.0f /*0x858624*/;
        const CVector& pedPos = ped->GetPosition();
        // NOTE: The original multiplies the X/Y terms by 0.0, so only the Z difference matters
        const float heightDiff = pedPos.z - groundZ;
        if (vehicle->m_nModelIndex == MODEL_AT400) {
            CVector doorPos{};
            int32 doorId = 0;
            if (CCarEnterExit::GetNearestCarDoor(ped, vehicle, doorPos, doorId)
                && std::abs(ped->GetPosition().z - doorPos.z) < 1.0f /*0x858624*/) {
                goto evaluate;
            }
        }
        if (std::abs(heightDiff) >= 2.0f /*0x858CA0*/) {
            if (vehicle->m_nVehicleType != VEHICLE_TYPE_BOAT) {
                continue;
            }
            // Boats get slack if the ped stands on them...
            if (!(groundZ < pedPos.z && groundZ >= pedPos.z - 4.0f /*0x858B90*/)
                && ped->m_pContactEntity != vehicle) {
                continue;
            }
        }
evaluate:
        if (!vehicle->vehicleFlags.bConsideredByPlayer) { // 0x42E & 0x20
            continue;
        }
        CVector doorPos{};
        int32 doorId = 0;
        if (!CCarEnterExit::GetNearestCarDoor(ped, vehicle, doorPos, doorId)) {
            continue;
        }
        const CVector& vehPos2 = vehicle->GetPosition();
        const CVector& pedPos2 = ped->GetPosition();
        const float dist2D = CVector{ pedPos2.x - vehPos2.x, pedPos2.y - vehPos2.y, 0.0f }.Magnitude();
        float pedToVehDist = dist2D;
        if (!(pedToVehDist < 10.0f /*0x85862C*/)) { // NaN-safe: NaN also goes here
            if (!(pedToVehDist > 10.0f) || !CPedGeometryAnalyser::LiesInsideBoundingBox(*ped, pedPos2, *vehicle)) {
                continue;
            }
            pedToVehDist = 10.0f;
        }
        if (CCranes::IsThisCarBeingCarriedByAnyCrane(vehicle)) {
            continue;
        }
        EvaluateCarPosition(vehicle, ped, pedToVehDist, outVehDist, outVehicle);
    }
}

// 0x56F7D0
void CPlayerInfo::LoadPlayerSkin() {
    DeletePlayerSkin();
    m_pSkinTexture = CPlayerSkin::GetSkinTexture(m_szSkinName);
}

// 0x56EA80
void CPlayerInfo::DeletePlayerSkin() {
    if (m_pSkinTexture) {
        RwTextureDestroy(m_pSkinTexture);
        m_pSkinTexture = nullptr;
    }
}

// 0x5717F0
void CPlayerInfo::SetPlayerSkin(const char* name) {
    strcpy_s(m_szSkinName, name); // NOTSA: They used `strcpy`, we use `_s` for safety
    LoadPlayerSkin();
}

// 0x56DA80
void CPlayerInfo::SetLastTargetVehicle(CVehicle* vehicle) {
    CEntity::SafeCleanUpRef(m_pLastTargetVehicle);
    m_pLastTargetVehicle = vehicle;
    CEntity::SafeRegisterRef(m_pLastTargetVehicle);
}

// 0x56F8D0
void CPlayerInfo::Process(uint32 playerIndex) {
    static auto& s_bPlayerHasMoved   = StaticRef<bool>(0x8CDF21);    // true
    static auto& s_bRoadNodeIsNearby = StaticRef<bool>(0x8CDF20);    // true
    static auto& s_vecOldCoors       = StaticRef<CVector>(0xB9B994); // `static CVector OldCoors` (Init guard: 0xB9B9A0)

    if (CReplay::Mode == MODE_PLAYBACK) {
        return;
    }

    CPad* const pad = CPad::GetPad(playerIndex);

    // `CTimer::ms_fTimeStep * 0.02f * 1000.0f` truncated - The amount of milliseconds this frame took
    const auto GetFrameTimeMS = [] {
        return (uint32)(CTimer::GetTimeStep() * 0.02f /*0x858B38*/ * 1000.0f /*0x858C4C*/);
    };

    // 0x56F909 - Taxi fare: 1$ for each second the player is driving a taxi with passengers
    if (m_bTaxiTimerScore
        && m_pPed->bInVehicle
        && (m_pPed->m_pVehicle->m_nModelIndex == MODEL_TAXI || m_pPed->m_pVehicle->m_nModelIndex == MODEL_CABBIE)
        && m_pPed->m_pVehicle->m_pDriver == m_pPed
        && m_pPed->m_pVehicle->m_nNumPassengers != 0
    ) {
        const uint32 elapsed = CTimer::GetTimeInMS() - m_nTaxiTimer;
        if (elapsed >= 1000) {
            const uint32 seconds = elapsed / 1000;
            m_nMoney     += seconds;
            m_nTaxiTimer += seconds * 1000;
        }
    } else {
        m_nTaxiTimer = CTimer::GetTimeInMS();
    }

    // 0x56F9A2 - Two wheels / wheelie / stoppie counters
    [&] {
        CVehicle* const veh = m_pPed->bInVehicle ? m_pPed->m_pVehicle : nullptr;

        // The counter is kept running while `m_nTempBufferCounter` (a grace period of 500ms) hasn't ran out
        const auto ContinueStunt = [&](uint32& counter, float& dist, float bufferDecay) {
            counter += GetFrameTimeMS();
            dist    += veh->m_fMovingSpeed;
            const float buffer = (float)m_nTempBufferCounter - (float)GetFrameTimeMS() * bufferDecay;
            m_nTempBufferCounter = (uint32)(0.0f <= buffer ? buffer : 0.0f);
        };
        const auto IncreaseBuffer = [&] {
            m_nTempBufferCounter -= (int32)(CTimer::GetTimeStep() * 0.02f /*0x858B38*/ * -1000.0f /*0x859948*/);
        };

        if (veh && veh->IsAutomobile()) { // 0x56F9CC
            auto* const car = veh->AsAutomobile();

            if (car->m_nNumContactWheels >= 3) {
                m_nCarLess3WheelCounter = 0;
            } else {
                m_nCarLess3WheelCounter += GetFrameTimeMS();
            }

            // 0x56FAF6 / 0x56FC79 / 0x56FCAF
            const auto FinishTwoWheels = [&] {
                if (m_nCarTwoWheelCounter >= 2000) {
                    m_fBestCarTwoWheelsDistM  = m_fCarTwoWheelDist;
                    m_nBestCarTwoWheelsTimeMs = m_nCarTwoWheelCounter;
                    CStats::SetNewRecordStat(STAT_LONGEST_2_WHEELS_TIME, (float)(m_nCarTwoWheelCounter / 1000));
                    CStats::SetNewRecordStat(STAT_LONGEST_2_WHEELS_DISTANCE, m_fCarTwoWheelDist);
                }
                m_fCarTwoWheelDist    = 0.0f;
                m_nTempBufferCounter  = 0;
                m_nCarTwoWheelCounter = 0;
            };

            const auto& ratios = car->m_fWheelsSuspensionCompressionPrev; // 0x7E4
            bool bOnTwoWheels;
            if (ratios[2] == 1.0f && ratios[3] == 1.0f) { // 0x56F9FA
                bOnTwoWheels = ratios[0] < 1.0f && ratios[1] < 1.0f && veh->m_fDamageIntensity == 0.0f;
            } else if (ratios[0] == 1.0f && ratios[1] == 1.0f) { // 0x56FB25
                bOnTwoWheels = ratios[2] < 1.0f && ratios[3] < 1.0f && veh->m_fDamageIntensity == 0.0f;
            } else { // 0x56FCA1
                if (m_nCarTwoWheelCounter != 0) {
                    FinishTwoWheels();
                }
                m_nBikeRearWheelCounter  = 0;
                m_nBikeFrontWheelCounter = 0;
                return;
            }

            if (bOnTwoWheels) {
                ContinueStunt(m_nCarTwoWheelCounter, m_fCarTwoWheelDist, 0.5f /*0x858B8C*/);
            } else if (m_nCarTwoWheelCounter != 0 && m_nTempBufferCounter < 500) { // 0x56FADA / 0x56FC3D
                IncreaseBuffer();
            } else {
                FinishTwoWheels();
            }
            m_nBikeRearWheelCounter  = 0;
            m_nBikeFrontWheelCounter = 0;
            return;
        }

        if (veh && veh->IsBike()) { // 0x56FD0B
            auto* const bike = veh->AsBike();

            // 0x56FD9E / 0x56FEB6
            const auto FinishWheelie = [&] {
                if (m_nBikeRearWheelCounter >= 5000) {
                    m_fBestBikeWheelieDistM  = m_fBikeRearWheelDist;
                    m_nBestBikeWheelieTimeMs = m_nBikeRearWheelCounter;
                    CStats::SetNewRecordStat(STAT_LONGEST_WHEELIE_TIME, (float)(m_nBikeRearWheelCounter / 1000));
                    CStats::SetNewRecordStat(STAT_LONGEST_WHEELIE_DISTANCE, m_fBikeRearWheelDist);
                }
                m_nBikeRearWheelCounter = 0;
                m_fBikeRearWheelDist    = 0.0f;
                m_nTempBufferCounter    = 0;
            };
            // 0x56FFC2 / 0x570062
            const auto FinishStoppie = [&] {
                if (m_nBikeFrontWheelCounter >= 2000) {
                    m_fBestBikeStoppieDistM  = m_fBikeFrontWheelDist;
                    m_nBestBikeStoppieTimeMs = m_nBikeFrontWheelCounter;
                    CStats::SetNewRecordStat(STAT_LONGEST_STOPPIE_TIME, (float)(m_nBikeFrontWheelCounter / 1000));
                    CStats::SetNewRecordStat(STAT_LONGEST_STOPPIE_DISTANCE, m_fBikeFrontWheelDist);
                }
                m_nBikeFrontWheelCounter = 0;
                m_fBikeFrontWheelDist    = 0.0f;
                m_nTempBufferCounter     = 0;
            };

            const auto& ratios = bike->m_aWheelRatios; // 0x720
            if (ratios[0] == 1.0f && ratios[1] == 1.0f && m_nBikeFrontWheelCounter == 0) {
                // NOTE: Unlike for cars, the damage intensity is only checked if the first wheel isn't touching the ground
                if (ratios[2] < 1.0f || (ratios[3] < 1.0f && veh->m_fDamageIntensity == 0.0f)) { // 0x56FDFB
                    ContinueStunt(m_nBikeRearWheelCounter, m_fBikeRearWheelDist, 0.2f /*0x858CC4*/);
                } else if (m_nBikeRearWheelCounter != 0 && m_nTempBufferCounter < 500) { // 0x56FD82
                    IncreaseBuffer();
                } else {
                    FinishWheelie();
                }
            } else if (m_nBikeRearWheelCounter != 0) { // 0x56FEAC
                FinishWheelie();
            } else if (ratios[2] == 1.0f && ratios[3] == 1.0f) { // 0x56FF0F
                if (ratios[0] < 1.0f || (ratios[1] < 1.0f && veh->m_fDamageIntensity == 0.0f)) { // 0x56FFF1
                    ContinueStunt(m_nBikeFrontWheelCounter, m_fBikeFrontWheelDist, 0.2f /*0x858CC4*/);
                } else if (m_nBikeFrontWheelCounter != 0 && m_nTempBufferCounter < 500) { // 0x56FF7A
                    IncreaseBuffer();
                } else {
                    FinishStoppie();
                }
            } else { // 0x570062
                FinishStoppie();
            }
            m_nCarTwoWheelCounter   = 0;
            m_nCarLess3WheelCounter = 0;
            return;
        }

        // 0x5700D0
        m_nCarLess3WheelCounter  = 0;
        m_nTempBufferCounter     = 0;
        m_nCarTwoWheelCounter    = 0;
        m_nBikeRearWheelCounter  = 0;
        m_nBikeFrontWheelCounter = 0;
    }();

    // 0x5700EE
    WorkOutEnergyFromHunger();

    // 0x5700F5 - Make the displayed money catch up with the actual amount
    if (m_nDisplayMoney != m_nMoney) {
        const int32 diff    = m_nMoney - m_nDisplayMoney;
        const int32 absDiff = diff < 0 ? -diff : diff;
        int32 step;
        if (absDiff > 100'000) {
            step = 12345;
        } else if (absDiff > 10'000) {
            step = 1234;
        } else if (absDiff > 1'000) {
            step = 123;
        } else if (absDiff > 50) {
            step = 42;
        } else {
            step = 1;
        }
        m_nDisplayMoney += diff < 0 ? -step : step;
    }

    // 0x57015B
    m_pPed->m_fMaxHealth = CStats::GetFatAndMuscleModifier(STAT_MOD_MAX_HEALTH);
    m_nMaxHealth         = (uint8)m_pPed->m_fMaxHealth;

    // 0x570180
    if (m_pPed->bInVehicle && CStats::GetStatValue(STAT_FLYING_SKILL) >= 400.0f /*0x85A700*/) {
        StreamParachuteWeapon(true);
    } else if (m_bParachuteReferenced) {
        CGameLogic::IsCoopGameGoingOn();
        if (m_bParachuteReferenced) {
            CStreaming::SetModelIsDeletable(MODEL_GUN_PARA);
            m_bParachuteReferenced   = false;
            m_nRequireParachuteTimer = 0;
        }
    }

    // 0x5701E0
    if ((CTimer::GetFrameCounter() & 0xF) == 0) {
        const CVector& pos = m_pPed->bInVehicle
            ? m_pPed->m_pVehicle->GetPosition()
            : m_pPed->GetPosition();
        m_fRoadDensityAroundPlayer = ThePaths.CalcRoadDensity(pos.x, pos.y);
    }
    {
        // NOTE: Yes, this is applied each frame to the stored value (So it converges to 1.0 between the recalculations above)
        float density = (m_fRoadDensityAroundPlayer - 1.0f /*0x858624*/) * 0.6f /*0x858CC8*/ + 1.0f /*0x858624*/;
        if (!(0.5f /*0x858B8C*/ <= density)) {
            density = 0.5f;
        }
        if (1.45f /*0x863230*/ < density) {
            density = 1.45f;
        }
        m_fRoadDensityAroundPlayer = density;
    }

    // 0x5702AA - Entering/exiting vehicles and picking up objects
    [&] {
        if (pad->GetTarget() || !m_pPed->bCanExitCar || m_pPed->bStuckUnderCar) {
            m_bTryingToExitCar = false;
            return;
        }
        if (!pad->ExitVehicleJustDown()) {
            if (!m_bTryingToExitCar || !pad->GetExitVehicle() || !m_pPed->bInVehicle) {
                m_bTryingToExitCar = false;
                return;
            }
        }

        // 0x570318
        m_bTryingToExitCar = false;

        const auto CanAbortActiveTask = [&] {
            CTask* const activeTask = m_pPed->GetTaskManager().GetActiveTask();
            return !activeTask || activeTask->MakeAbortable(m_pPed, ABORT_PRIORITY_URGENT, nullptr);
        };

        if (m_pPed->bInVehicle) { // 0x57032D - Exit the vehicle
            if (m_pRemoteVehicle) {
                return;
            }
            if (CEntity* const entityWeAreOn = m_pPed->m_pVehicle->m_pEntityWeAreOn) {
                if (CBridge::ThisIsABridgeObjectMovingUp((int32)entityWeAreOn->m_nModelIndex)) {
                    return;
                }
            }

            // 0x57035E
            {
                CVehicle* const veh = m_pPed->m_pVehicle;
                if (veh->GetStatus() == STATUS_WRECKED || veh->GetStatus() == STATUS_TRAIN_MOVING) {
                    return;
                }
                if (veh->m_nDoorLock == CARLOCK_LOCKED_PLAYER_INSIDE) {
                    return;
                }
            }

            if (!CanAbortActiveTask()) { // 0x570389
                m_bTryingToExitCar = true;
                return;
            }

            if (m_pPed->m_pVehicle->IsBoat()) { // 0x5703CA
                m_pPed->GetTaskManager().SetTask(
                    new CTaskComplexLeaveCar{ m_pPed->m_pVehicle, 0, 0, true, false },
                    TASK_PRIMARY_PRIMARY,
                    false
                );
                m_pPed->bTryingToReachDryLand = true;
                return;
            }

            if (m_pPed->m_pVehicle->CanPedStepOutCar(false) || m_pPed->m_pVehicle->CanPedJumpOutCar(m_pPed)) { // 0x57046F
                m_pPed->GetTaskManager().SetTask(
                    new CTaskComplexLeaveCar{ m_pPed->m_pVehicle, 0, 0, true, false },
                    TASK_PRIMARY_PRIMARY,
                    false
                );
                m_bTryingToExitCar = true;
                GivePlayerParachute();
            } else if (m_pPed->m_pVehicle->GetStatus() == STATUS_PLAYER) { // 0x570451
                m_bTryingToExitCar = true; // Try again next frame, once the car has slowed down enough
            }
            return;
        }

        // 0x5704D5 - On foot
        if (m_pPed->m_pAttachedTo) {
            return;
        }

        CEntity* const               objectToSteal = FindObjectToSteal(m_pPed);
        CTask* const                 simplestTask  = m_pPed->GetTaskManager().GetSimplestActiveTask();
        CTaskSimpleHoldEntity* const holdTask      = m_pPed->GetIntelligence()->GetTaskHold(false);

        if (simplestTask->GetTaskType() == TASK_SIMPLE_CLIMB) {
            return;
        }
        if (m_pPed->GetIntelligence()->GetTaskFighting()) {
            return;
        }
        if (holdTask && holdTask->m_pEntityToHold) {
            return;
        }

        if (objectToSteal) { // 0x57054C - Pick up the object
            if (!CanAbortActiveTask()) {
                return;
            }
            auto* const pickUpTask = new CTaskComplexGoPickUpEntity{ objectToSteal, ANIM_GROUP_CARRY };

            CEventScriptCommand event{ TASK_PRIMARY_PRIMARY, pickUpTask, false };
            CWorld::Players[CWorld::PlayerInFocus].m_pPed->GetEventGroup().Add(&event, false);
            return;
        }

        // 0x5705F5 - Find the closest car to get into
        CVehicle* closestVeh  = nullptr;
        float     closestDist = 0.0f;
        {
            const CVector& pedPos = m_pPed->GetPosition();
            const float minX = pedPos.x - 10.0f /*0x85862C*/;
            const float maxX = pedPos.x + 10.0f;
            const float minY = pedPos.y - 10.0f;
            const float maxY = pedPos.y + 10.0f;

            const auto ToSector = [](float v) {
                return (int32)std::floor(v * 0.02f /*0x858B38*/ + 60.0f /*0x858B34*/);
            };
            const int32 minSectorX = ToSector(minX);
            const int32 minSectorY = ToSector(minY);
            const int32 maxSectorX = ToSector(maxX);
            const int32 maxSectorY = ToSector(maxY);

            CWorld::AdvanceCurrentScanCode();

            for (int32 sy = minSectorY; sy <= maxSectorY; sy++) {
                for (int32 sx = minSectorX; sx <= maxSectorX; sx++) {
                    FindClosestCarSectorList(
                        CWorld::GetRepeatSector(sx, sy).Vehicles,
                        m_pPed,
                        minX, minY,
                        maxX, maxY,
                        &closestDist,
                        &closestVeh
                    );
                }
            }
        }

        if (!closestVeh) { // 0x570751
            return;
        }
        if (closestVeh->m_nVehicleSubType == VEHICLE_TYPE_TRAILER && closestVeh->m_pTowingVehicle) {
            closestVeh = closestVeh->m_pTowingVehicle; // Get into the vehicle that's towing the trailer
        }
        if (!closestVeh->CanBeDriven()) {
            return;
        }

        if (m_pPed->GetIntelligence()->GetTaskJetPack()) { // 0x57078D
            m_pPed->GetIntelligence()->GetTaskJetPack()->DropJetPack(m_pPed);
        }

        const auto EnterAsDriver = [&] {
            m_pPed->GetTaskManager().SetTask(new CTaskComplexEnterCarAsDriver{ closestVeh }, TASK_PRIMARY_PRIMARY, false);
        };

        if (closestVeh->IsBoat()) { // 0x570A43
            if (closestVeh->m_pDriver) {
                return;
            }
            if (!CanAbortActiveTask()) {
                return;
            }
            EnterAsDriver();
            return;
        }

        // 0x5707C0
        if (CTask* const activeTask = m_pPed->GetTaskManager().GetActiveTask()) {
            // If swimming next to a sea plane/vortex just climb onto it
            const auto TryClimbOutOfWaterOntoVehicle = [&] {
                if (activeTask->GetTaskType() != TASK_COMPLEX_IN_WATER) {
                    return false;
                }
                if (!m_pPed->GetIntelligence()->GetTaskSwim()) {
                    return false;
                }
                switch (closestVeh->m_nModelIndex) {
                case MODEL_SKIMMER:
                case MODEL_VORTEX:
                case MODEL_SEASPAR:
                case MODEL_LEVIATHN:
                    break;
                default:
                    return false;
                }
                CVector doorPos;
                int32   doorId = 0;
                if (!CCarEnterExit::GetNearestCarDoor(m_pPed, closestVeh, doorPos, doorId)) {
                    return false;
                }
                CTaskSimpleSwim* const swimTask = m_pPed->GetIntelligence()->GetTaskSwim();
                swimTask->m_vecPos    = doorPos; // 0x14
                swimTask->m_nTimeStep = 5000;    // 0x58
                return true;
            };
            if (!TryClimbOutOfWaterOntoVehicle()) {
                if (!activeTask->MakeAbortable(m_pPed, ABORT_PRIORITY_URGENT, nullptr)) { // 0x570868
                    return;
                }
            }
        }

        // 0x57087E
        if (!CWorld::Players[1].m_pPed || CGameLogic::bPlayersCanBeInSeparateCars) {
            EnterAsDriver(); // 0x5709FE
            return;
        }

        // 0x570898 - Co-op game: The players have to use the same vehicle
        CPlayerPed* const otherPlayer  = CWorld::Players[((int32)playerIndex + 1) % 2].m_pPed;
        const auto&       otherTaskMgr = otherPlayer->GetTaskManager();
        CTask* const      defaultTask  = otherTaskMgr.GetTaskPrimary(TASK_PRIMARY_DEFAULT);
        CTask* const      primaryTask  = otherTaskMgr.GetTaskPrimary(TASK_PRIMARY_PRIMARY);

        // All the tasks checked have the target vehicle as their first member
        const auto GetTargetVehicleOfTask = [](CTask* task) {
            return *reinterpret_cast<CVehicle**>(reinterpret_cast<uint8*>(task) + 0xC);
        };

        CVehicle* vehOtherIsDriverOf    = nullptr; // Vehicle the other player is driving/getting into as the driver
        CVehicle* vehOtherIsPassengerOf = nullptr; // Vehicle the other player is a passenger of/getting into as a passenger

        const int32 defaultTaskType = defaultTask ? (int32)defaultTask->GetTaskType() : 0;
        int32       primaryTaskType = 0;
        if (primaryTask) {
            primaryTaskType = (int32)primaryTask->GetTaskType();
            switch (primaryTaskType) {
            case TASK_COMPLEX_ENTER_CAR_AS_DRIVER:
            case TASK_COMPLEX_STEAL_CAR:
            case TASK_COMPLEX_DRAG_PED_FROM_CAR:
            case TASK_COMPLEX_ENTER_BOAT_AS_DRIVER:
                vehOtherIsDriverOf = GetTargetVehicleOfTask(primaryTask);
                break;
            }
        }

        if (   defaultTaskType == TASK_SIMPLE_PLAYER_IN_CAR
            || primaryTaskType == TASK_SIMPLE_PLAYER_IN_CAR
            || defaultTaskType == TASK_SIMPLE_CAR_DRIVE
            || primaryTaskType == TASK_SIMPLE_CAR_DRIVE
        ) { // 0x570949
            CVehicle* const otherVeh = otherPlayer->m_pVehicle;
            if (otherVeh->m_pDriver == otherPlayer) {
                vehOtherIsDriverOf = otherVeh;
            } else {
                vehOtherIsPassengerOf = otherVeh;
            }
        }
        if (primaryTaskType == TASK_COMPLEX_ENTER_CAR_AS_PASSENGER) { // 0x57095F
            vehOtherIsPassengerOf = GetTargetVehicleOfTask(primaryTask);
        }

        if (vehOtherIsDriverOf == closestVeh) { // 0x570972
            m_pPed->GetTaskManager().SetTask(new CTaskComplexEnterCarAsPassenger{ closestVeh, 0, false }, TASK_PRIMARY_PRIMARY, false);
        } else if (vehOtherIsPassengerOf == closestVeh || (!vehOtherIsDriverOf && !vehOtherIsPassengerOf)) { // 0x5709D6
            EnterAsDriver();
        }
    }();

    // 0x570AA5
    if (m_bAfterRemoteVehicleExplosion) {
        const uint32 prevElapsed = CTimer::GetPreviousTimeInMS() - m_nTimeOfRemoteVehicleExplosion;
        const uint32 elapsed     = CTimer::GetTimeInMS() - m_nTimeOfRemoteVehicleExplosion;

        if (prevElapsed < 1000 && elapsed >= 1000 && m_nPlayerState == PLAYERSTATE_PLAYING && m_bFadeAfterRemoteVehicleExplosion) {
            TheCamera.SetFadeColour(0, 0, 0);
            TheCamera.Fade(1.0f, eFadeFlag::FADE_IN);
        }

        if (elapsed > 2000) {
            if (m_nPlayerState == PLAYERSTATE_PLAYING && m_bFadeAfterRemoteVehicleExplosion) {
                TheCamera.RestoreWithJumpCut();
                TheCamera.SetFadeColour(0, 0, 0);
                TheCamera.Fade(1.0f, eFadeFlag::FADE_OUT);
                TheCamera.Process();
                CTimer::Stop();
                CRenderer::RequestObjectsInFrustum(nullptr, 0);
                CStreaming::LoadAllRequestedModels(false);
                CTimer::Update();
            }

            m_bAfterRemoteVehicleExplosion = false;

            auto& focusInfo = CWorld::Players[CWorld::PlayerInFocus];
            if (focusInfo.m_pRemoteVehicle) {
                focusInfo.m_pRemoteVehicle->m_bRemoveFromWorld = true;
            }
            focusInfo.m_pRemoteVehicle = nullptr;

            if (CVehicle* const veh = FindPlayerVehicle()) {
                veh->SetStatus(STATUS_PLAYER);
            }
        }
    }

    // 0x570C10 - Set the vehicle on fire if it's been upside down for too long
    if ((CTimer::GetFrameCounter() & 0x1F) == 0) {
        CVehicle* const veh = FindPlayerVehicle();
        if (   veh
            && m_pPed->bInVehicle
            && veh->GetUp().z < 0.0f
            && veh->m_vecMoveSpeed.Magnitude() < 0.05f /*0x858C28*/
            && (veh->IsAutomobile() || veh->IsBoat())
            && !veh->physicalFlags.bSubmergedInWater
        ) {
            m_nTimesUpsideDownInARow += veh->GetUp().z < -0.5f /*0x858F40*/ ? 2 : 1;
        } else {
            m_nTimesUpsideDownInARow = 0;
        }

        if (m_nTimesUpsideDownInARow > 6) { // 0x570DAF
            // NOTE: The vehicle isn't null-checked in the original either (The counter can only be `> 6` if there's one)
            CVehicle* const playerVeh = FindPlayerVehicle();
            if (playerVeh->vehicleFlags.bCanBeDamaged) {
                playerVeh->m_fHealth = playerVeh->m_fHealth <= 249.0f /*0x865040*/ ? playerVeh->m_fHealth : 249.0f;
                if (playerVeh->IsAutomobile()) {
                    playerVeh->AsAutomobile()->m_damageManager.SetEngineStatus(225);
                    playerVeh->AsAutomobile()->m_pExplosionVictim = nullptr; // 0x928
                }
            }
        }
    }

    // 0x570F19 - Distance/time stats
    if (CVehicle* const veh = FindPlayerVehicle()) {
        const auto GetVeh = [] { return FindPlayerVehicle(); }; // The original re-fetches it every time

        if (veh->m_nModelIndex == MODEL_CADDY) {
            CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_GOLF_CART, GetVeh()->m_fMovingSpeed);
        } else if (GetVeh()->m_nVehicleSubType == VEHICLE_TYPE_BMX) {
            CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_BICYCLE, GetVeh()->m_fMovingSpeed);
        } else {
            if (GetVeh()->GetVehicleAppearance() == VEHICLE_APPEARANCE_HELI) {
                CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_HELICOPTER, GetVeh()->m_fMovingSpeed);
            }
            if (GetVeh()->GetVehicleAppearance() == VEHICLE_APPEARANCE_PLANE) {
                CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_PLANE, GetVeh()->m_fMovingSpeed);
            }
            if (GetVeh()->GetVehicleAppearance() == VEHICLE_APPEARANCE_AUTOMOBILE) {
                CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_CAR, GetVeh()->m_fMovingSpeed);
            }
            if (GetVeh()->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE) {
                CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_MOTORBIKE, GetVeh()->m_fMovingSpeed);
            }
            if (GetVeh()->GetVehicleAppearance() == VEHICLE_APPEARANCE_BOAT) {
                CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_BOAT, GetVeh()->m_fMovingSpeed);
            }
            if (   GetVeh()->GetVehicleAppearance() == VEHICLE_APPEARANCE_PLANE
                || GetVeh()->GetVehicleAppearance() == VEHICLE_APPEARANCE_HELI
            ) { // 0x5712A3
                if (GetVeh()->m_vecMoveSpeed.Magnitude() > 0.2f /*0x858CC4*/) {
                    CStats::IncrementStat(STAT_FLIGHT_TIME, CTimer::GetTimeStep() * 16.0f /*0x8599D0*/);
                }
            }
        }

        // 0x5712FE
        if (!FindPlayerTrain()) {
            const CVector& speed = GetVeh()->m_vecMoveSpeed;
            if (speed.x * speed.x + speed.y * speed.y + speed.z * speed.z > 0.0f) {
                switch (GetVeh()->GetVehicleAppearance()) {
                case VEHICLE_APPEARANCE_BIKE: // 0x5713E8
                    if (GetVeh()->m_nVehicleSubType != VEHICLE_TYPE_BMX) {
                        CStats::UpdateStatsWhenOnMotorBike(static_cast<CBike*>(GetVeh()));
                    }
                    break;
                case VEHICLE_APPEARANCE_HELI:
                case VEHICLE_APPEARANCE_PLANE: // 0x5713A5
                    CStats::UpdateStatsWhenFlying(GetVeh());
                    break;
                case VEHICLE_APPEARANCE_BOAT:
                    break;
                default: // 0x57145C
                    CStats::UpdateStatsWhenDriving(GetVeh());
                    break;
                }
            }
        }
    } else if (CWorld::Players[CWorld::PlayerInFocus].m_pPed->m_fMovingSpeed > 0.0f) { // 0x57149F
        if (m_pPed->GetIntelligence()->GetTaskSwim()) {
            if (CPad::GetPad(0)->GetPedWalkLeftRight() != 0 || CPad::GetPad(0)->GetAccelerate() != 0) {
                CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_SWIMMING, CWorld::Players[CWorld::PlayerInFocus].m_pPed->m_fMovingSpeed);
            }
        } else if (m_pPed->GetIntelligence()->GetTaskJetPack()) {
            CStats::IncrementStat(STAT_TIME_ON_JETPACK, CTimer::GetTimeStep() * 16.0f /*0x8599D0*/);
        } else {
            CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_ON_FOOT, CWorld::Players[CWorld::PlayerInFocus].m_pPed->m_fMovingSpeed);
        }
    }

    // 0x571567 - Chase value (How intense the police chase is)
    if (m_pPed->GetPlayerData()->m_pWanted->GetWantedLevel() != eWantedLevel::WANTED_CLEAN && !CTheScripts::IsPlayerOnAMission()) {
        // 0x5715A9 - Every 20 seconds check if the player has moved and whenever they are near a road
        if (CTimer::GetTimeInMS() / 20'000 != CTimer::GetPreviousTimeInMS() / 20'000) {
            s_bPlayerHasMoved = false;
            if (!((s_vecOldCoors - FindPlayerCoors()).Magnitude() < 10.0f /*0x85862C*/)) {
                s_bPlayerHasMoved = true;
            }
            s_vecOldCoors = FindPlayerCoors();

            const CNodeAddress closestNode = ThePaths.FindNodeClosestToCoors(FindPlayerCoors(), PATH_TYPE_VEH, 60.0f, 1, 0, 0, 0, 0);
            s_bRoadNodeIsNearby = closestNode.m_wAreaId != (uint16)-1;
        }

        float targetChaseValue = 0.0f;
        switch ((int32)m_pPed->GetPlayerData()->m_pWanted->GetWantedLevel()) { // 0x5716A3
        case 1: targetChaseValue = 31.0f;   break; // 0x863E28
        case 2: targetChaseValue = 62.0f;   break; // 0x86503C
        case 3: targetChaseValue = 125.0f;  break; // 0x865038
        case 4: targetChaseValue = 250.0f;  break; // 0x859F80
        case 5: targetChaseValue = 500.0f;  break; // 0x858B58
        case 6: targetChaseValue = 1000.0f; break; // 0x858C4C
        }

        const float delta = (targetChaseValue - m_fCurrentChaseValue) * CTimer::GetTimeStep() * 0.0001f /*0x858FC4*/;
        if (delta < 0.0f) {
            m_fCurrentChaseValue += delta;
        } else if (
               s_bPlayerHasMoved
            && s_bRoadNodeIsNearby
            && !CCullZones::NoPolice()
            && !CCullZones::PoliceAbandonCars()
            && CGame::currArea == AREA_CODE_NORMAL_WORLD
        ) { // 0x571722 - Only gets worse if the player is actually being chased around
            m_fCurrentChaseValue += delta;
        }
    } else {
        m_fCurrentChaseValue = 0.0f;
    }

    // 0x571766 - CPlayerCrossHair::Update (The `CPlayerCrossHair` struct is `m_nCrosshairActivated` + `m_vecCrosshairTarget`)
    reinterpret_cast<CPlayerCrossHair*>(&m_nCrosshairActivated)->Update(static_cast<int32>(playerIndex), pad);

    // 0x57177B
    m_nMoney        = std::min(m_nMoney, 999'999'999);
    m_nDisplayMoney = std::min(m_nDisplayMoney, 999'999'999);
}

// 0x56F330
void CPlayerInfo::Clear() {
    // TODO: This should just use the constructor and `swap`,
    // like to just swap ourselves to a default constructed object
    // and do all the cleanup in the destructor

    m_pPed = nullptr;
    m_pRemoteVehicle = nullptr;
    if (m_pSpecCar) {
        m_pSpecCar->physicalFlags.bAddMovingCollisionSpeed = false;
        m_pSpecCar = nullptr;
    }
    m_nDisplayMoney = 0;
    m_nMoney = 0;
    m_nPlayerState = PLAYERSTATE_PLAYING;
    m_nCarDensityForCurrentZone = 0;
    m_fRoadDensityAroundPlayer = 1.0f;
    m_bAfterRemoteVehicleExplosion = false;
    m_bCreateRemoteVehicleExplosion = false;
    m_bFadeAfterRemoteVehicleExplosion = false;
    m_bTryingToExitCar = false;
    m_bTaxiTimerScore = false;
    m_nTaxiTimer = 0;
    m_nVehicleTimeCounter = CTimer::GetTimeInMS();
    m_nMaxArmour = 100;
    m_nMaxHealth = 100;
    m_bCanDoDriveBy = true;
    m_nCollectablesPickedUp = 0;
    m_nTotalNumCollectables = 3;
    m_nLastTimeEnergyLost = 0;
    m_nLastTimeArmourLost = 0;
    m_nLastTimeBigGunFired = 0;
    m_nTimesStuckInARow = 0;
    m_nTimesUpsideDownInARow = 0;
    m_nCarTwoWheelCounter = 0;
    m_fCarTwoWheelDist = 0.0f;
    m_nCarLess3WheelCounter = 0;
    m_nBikeRearWheelCounter = 0;
    m_fBikeRearWheelDist = 0.0f;
    m_nBikeFrontWheelCounter = 0;
    m_fBikeFrontWheelDist = 0.0f;
    m_nTempBufferCounter = 0;
    m_nBestCarTwoWheelsTimeMs = 0;
    m_fBestCarTwoWheelsDistM = 0.0f;
    m_nBestBikeWheelieTimeMs = 0;
    m_fBestBikeWheelieDistM = 0.0f;
    m_nBestBikeStoppieTimeMs = 0;
    m_fBestBikeStoppieDistM = 0.0f;
    m_bDoesNotGetTired = false;
    m_bFastReload = false;
    m_bFireProof = false;
    m_bGetOutOfJailFree = false;
    m_bFreeHealthCare = false;
    m_nTimeOfLastCarExplosionCaused = 0;
    m_nExplosionMultiplier = 0;
    m_nHavocCaused = 0;
    FindPlayerInfo().m_nNumHoursDidntEat = 0;
    m_nLastBustMessageNumber = 1;
    m_fCurrentChaseValue = 0.0f;
    m_nBustedAudioStatus = 0;
    m_nCrosshairActivated = 0;

    m_nRequireParachuteTimer = 0;

    if (m_bParachuteReferenced) {
        CGameLogic::IsCoopGameGoingOn();
        if (m_bParachuteReferenced) {
            CStreaming::SetModelIsDeletable(MODEL_GUN_PARA);
            m_bParachuteReferenced = false;
            m_nRequireParachuteTimer = 0;
        }
    }
}

// 0x56EC40
void CPlayerInfo::GivePlayerParachute() const {
    if (m_nRequireParachuteTimer) {
        if (CStreaming::IsModelLoaded(MODEL_GUN_PARA)) {
            m_pPed->GiveWeapon(WEAPON_PARACHUTE, 1, true);
            m_pPed->SetSavedWeapon(WEAPON_PARACHUTE);
        }
    }
}

// 0x56EB30
void CPlayerInfo::StreamParachuteWeapon(bool unk) {
    if (CGameLogic::IsCoopGameGoingOn()) {
        if (unk) {
            return;
        }
        if (m_bParachuteReferenced) {
            CStreaming::SetModelIsDeletable(MODEL_GUN_PARA);
            m_bParachuteReferenced = false;
            m_nRequireParachuteTimer = 0;
        }
        return;
    }

    if (m_pPed && m_pPed->IsInVehicle()) {
        if (m_pPed->m_pVehicle->IsSubPlane() || m_pPed->m_pVehicle->IsSubHeli()) {
            if (m_nRequireParachuteTimer <= (uint32)CTimer::GetTimeStepInMS()) {
                const auto groundHeight = TheCamera.CalculateGroundHeight(eGroundHeightType::ENTITY_BB_BOTTOM);
                const auto vehToGroundZDist = m_pPed->m_pVehicle->GetPosition().z - groundHeight;
                m_nRequireParachuteTimer = (vehToGroundZDist <= 50.f) ? 0 : 5000;
            } else {
                m_nRequireParachuteTimer -= (uint32)CTimer::GetTimeStepInMS();
            }
        }
    }

    if (m_nRequireParachuteTimer) {
        CStreaming::RequestModel(MODEL_GUN_PARA, STREAMING_MISSION_REQUIRED);
        m_bParachuteReferenced = true;
        return;
    }

    if (m_bParachuteReferenced) {
        CStreaming::SetModelIsDeletable(MODEL_GUN_PARA);
        m_bParachuteReferenced = false;
        m_nRequireParachuteTimer = 0;
    }
}

// 0x56EAB0
void CPlayerInfo::AddHealth(int32 amount) const {
    const auto newValue = std::min((float)m_nMaxHealth, m_pPed->m_fHealth + (float)amount); // Clamp to m_nMaxHealth
    m_pPed->m_fHealth = std::max(newValue, m_pPed->m_fHealth); // Don't change health to a lower value
}

// 0x56EA30
void CPlayerInfo::BlowUpRCBuggy(bool bExplode) const {
    if (m_pRemoteVehicle && !m_pRemoteVehicle->m_bRemoveFromWorld) {
        CRemote::TakeRemoteControlledCarFromPlayer(bExplode);
        if (bExplode)
            m_pRemoteVehicle->BlowUpCar(FindPlayerPed(), false);
    }
}

// 0x56E870
void CPlayerInfo::MakePlayerSafe(bool enable, float radius) {
    // Not quite SA, but this is the way to do it (instead of copy pasting it twice)
    auto& flags = m_pPed->physicalFlags;
    flags.bInvulnerable = enable;
    flags.bBulletProof = enable;
    flags.bFireProof = enable;
    flags.bExplosionProof = enable;
    flags.bCollisionProof = enable;
    flags.bMeleeProof = enable;
    m_PlayerData.m_bCanBeDamaged = !enable;
    m_PlayerData.m_pWanted->m_bEverybodyBackOff = enable;
    m_pPed->GetPadFromPlayer()->bPlayerSafe = enable;
    CWorld::SetAllCarsCanBeDamaged(!enable);

    if (enable) {
        CWorld::StopAllLawEnforcersInTheirTracks();
        CPad::StopPadsShaking();

        m_pPed->ClearAdrenaline();
        m_PlayerData.m_fTimeCanRun = std::max(m_PlayerData.m_fTimeCanRun, 0.f);
        m_pPed->GetIntelligence()->ClearTasks(true, false);

        gFireManager.ExtinguishPoint(GetPos(), radius);
        CExplosion::RemoveAllExplosionsInArea(GetPos(), 4000.f);
        CProjectileInfo::RemoveAllProjectiles();
        CWorld::ExtinguishAllCarFiresInArea(GetPos(), radius);
        CReplay::DisableReplays();
        m_pPed->ClearWeaponTarget();
    } else {
        CReplay::EnableReplays();
    }
}

// 0x56E830
void CPlayerInfo::PlayerFailedCriticalMission() {
    if (m_nPlayerState == PLAYERSTATE_PLAYING) {
        m_nPlayerState = PLAYERSTATE_FAILED_MISSION;
        CGameLogic::SetMissionFailed();
        CDarkel::ResetOnPlayerDeath();
    }
}

// 0x56E610
void CPlayerInfo::WorkOutEnergyFromHunger() {

    static auto& s_lastTimeHungryStateProcessedInitialized = StaticRef<bool>(0xB9B8F4); // false
    static auto& s_lastTimeHungryStateProcessed = StaticRef<uint8>(0xB9B8F2);
    static auto& s_LastHungryState = StaticRef<int8>(0xB9B8F1);
    static auto& s_bHungryMessageShown = StaticRef<bool>(0xB9B8F0);

    if (CCheat::IsActive(CHEAT_NEVER_GET_HUNGRY)) {
        return;
    }

    if (!s_lastTimeHungryStateProcessedInitialized) {
        s_lastTimeHungryStateProcessedInitialized = true;
        s_lastTimeHungryStateProcessed = CClock::GetGameClockHours();
    }

    auto pad = CPad::GetPad();
    if (   pad->ArePlayerControlsDisabled()
        || CMenuSystem::num_menus_in_use
        || TheCamera.m_bWideScreenOn
        || CCutsceneMgr::ms_running
        || CGameLogic::IsCoopGameGoingOn()
        || m_pRemoteVehicle
    ) {
        return;
    }

    if (!m_pPed)
        return;

    if (m_pPed->m_pAttachedTo)
        return;

    if (CClock::GetGameClockHours() != s_lastTimeHungryStateProcessed) {
        if (!m_nNumHoursDidntEat)
            s_LastHungryState = 0;
        m_nNumHoursDidntEat += 1;
    }

    if (m_nNumHoursDidntEat <= 48) {
        s_bHungryMessageShown = false;
    } else {
        if (CClock::GetGameClockHours() == s_lastTimeHungryStateProcessed)
            return;

        m_pPed->Say(CTX_GLOBAL_STOMACH_RUMBLE);
        pad->StartShake(400, 110u, 0);

        if (s_bHungryMessageShown) {
            bool bDecreaseHealth{};
            if (CStats::GetStatValue(STAT_FAT) > 0.0f) {
                CStats::DecrementStat(STAT_FAT, 25.0f);
                CStats::DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_FAT, 25.0f);
                bDecreaseHealth = true;
                if (!s_LastHungryState) {
                    s_LastHungryState = m_nNumHoursDidntEat + 24;
                }
            }

            if (CStats::GetStatValue(STAT_MUSCLE) <= 0.0f || m_nNumHoursDidntEat <= s_LastHungryState && s_LastHungryState) {
                if (!bDecreaseHealth) {
                    m_pPed->m_fHealth -= 2.0f;
                }
            } else {
                CStats::DecrementStat(STAT_MUSCLE, 25.0);
                CStats::DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_MUSCLE, 25.0f);
            }
        } else {
            CHud::SetHelpMessage(TheText.Get("NOTEAT"), true, false, true);
            s_bHungryMessageShown = true;
        }
    }

    if (CClock::GetGameClockHours() != s_lastTimeHungryStateProcessed) {
        s_lastTimeHungryStateProcessed = CClock::GetGameClockHours();
    }
}

// 0x56E5D0
void CPlayerInfo::ArrestPlayer() {
    if (m_nPlayerState == PLAYERSTATE_PLAYING) {
        m_nPlayerState = PLAYERSTATE_HAS_BEEN_ARRESTED;
        m_nBustedAudioStatus = 0;
        CDarkel::ResetOnPlayerDeath();
        CStats::IncrementStat(STAT_TIMES_BUSTED, 1.0f);
        CGangWars::EndGangWar(false);
    }
}

// 0x56E580
void CPlayerInfo::KillPlayer() {
    if (m_nPlayerState == PLAYERSTATE_PLAYING) {
        m_nPlayerState = PLAYERSTATE_HAS_DIED;
        CDarkel::ResetOnPlayerDeath();
        CMessages::AddBigMessage(TheText.Get("DEAD"), 4000, STYLE_WHITE_MIDDLE);
        CStats::IncrementStat(STAT_NUMBER_OF_HOSPITAL_VISITS, 1.0f);
        CGangWars::EndGangWar(false);
    }
}

// 0x56E570
bool CPlayerInfo::IsRestartingAfterMissionFailed() const {
    return m_nPlayerState == PLAYERSTATE_FAILED_MISSION;
}

// 0x56E560
bool CPlayerInfo::IsRestartingAfterArrest() const {
    return m_nPlayerState == PLAYERSTATE_HAS_BEEN_ARRESTED;
}

// 0x56E550
bool CPlayerInfo::IsRestartingAfterDeath() const {
    return m_nPlayerState == PLAYERSTATE_HAS_DIED;
}

// 0x56DAB0
bool CPlayerInfo::IsPlayerInRemoteMode() const {
    return m_pRemoteVehicle || m_bAfterRemoteVehicleExplosion;
}

// 0x56DFB0
// Return occupied vehicle's (if in any) or player's ped position
CVector CPlayerInfo::GetPos() const {
    return m_pPed->IsInVehicle() ? m_pPed->m_pVehicle->GetPosition() : m_pPed->GetPosition();
}

// 0x56DF50
// Return occupied vehicle's (if in any) or player's ped move speed
CVector CPlayerInfo::GetSpeed() const {
    return m_pPed->IsInVehicle() ? m_pPed->m_pVehicle->GetMoveSpeed() : m_pPed->GetMoveSpeed();
}

// 0x5D3B00
bool CPlayerInfo::Load() {
    CGenericGameStorage::LoadDataFromWorkBuffer<int32>(); // Discarded
    auto data = CGenericGameStorage::LoadDataFromWorkBuffer<CPlayerInfoSaveStructure>();
    data.Extract(this);
    return true;
}

// 0x5D3AC0
bool CPlayerInfo::Save() {
    CPlayerInfoSaveStructure data;
    data.Construct(this);
    CGenericGameStorage::SaveDataToWorkBuffer(sizeof(CPlayerInfoSaveStructure));
    CGenericGameStorage::SaveDataToWorkBuffer(data);
    return true;
}

// 0x45DEF0
CPlayerInfo& CPlayerInfo::operator=(const CPlayerInfo& rhs) {
    m_pPed                             = rhs.m_pPed;
    m_PlayerData                       = rhs.m_PlayerData;
    m_pRemoteVehicle                   = rhs.m_pRemoteVehicle;
    m_pSpecCar                         = rhs.m_pSpecCar;
    m_nMoney                           = rhs.m_nMoney;
    m_nDisplayMoney                    = rhs.m_nDisplayMoney;
    m_nCollectablesPickedUp            = rhs.m_nCollectablesPickedUp;
    m_nTotalNumCollectables            = rhs.m_nTotalNumCollectables;
    m_nLastBumpPlayerCarTimer          = rhs.m_nLastBumpPlayerCarTimer;
    m_nTaxiTimer                       = rhs.m_nTaxiTimer;
    m_nVehicleTimeCounter              = rhs.m_nVehicleTimeCounter;
    m_bTaxiTimerScore                  = rhs.m_bTaxiTimerScore;
    m_bTryingToExitCar                 = rhs.m_bTryingToExitCar;
    m_pLastTargetVehicle               = rhs.m_pLastTargetVehicle;
    m_nPlayerState                     = rhs.m_nPlayerState;
    m_bAfterRemoteVehicleExplosion     = rhs.m_bAfterRemoteVehicleExplosion;
    m_bCreateRemoteVehicleExplosion    = rhs.m_bCreateRemoteVehicleExplosion;
    m_bFadeAfterRemoteVehicleExplosion = rhs.m_bFadeAfterRemoteVehicleExplosion;
    m_nTimeOfRemoteVehicleExplosion    = rhs.m_nTimeOfRemoteVehicleExplosion;
    m_nLastTimeEnergyLost              = rhs.m_nLastTimeEnergyLost;
    m_nLastTimeArmourLost              = rhs.m_nLastTimeArmourLost;
    m_nLastTimeBigGunFired             = rhs.m_nLastTimeBigGunFired;
    m_nTimesUpsideDownInARow           = rhs.m_nTimesUpsideDownInARow;
    m_nTimesStuckInARow                = rhs.m_nTimesStuckInARow;
    m_nCarTwoWheelCounter              = rhs.m_nCarTwoWheelCounter;
    m_fCarTwoWheelDist                 = rhs.m_fCarTwoWheelDist;
    m_nCarLess3WheelCounter            = rhs.m_nCarLess3WheelCounter;
    m_nBikeRearWheelCounter            = rhs.m_nBikeRearWheelCounter;
    m_fBikeRearWheelDist               = rhs.m_fBikeRearWheelDist;
    m_nBikeFrontWheelCounter           = rhs.m_nBikeFrontWheelCounter;
    m_fBikeFrontWheelDist              = rhs.m_fBikeFrontWheelDist;
    m_nTempBufferCounter               = rhs.m_nTempBufferCounter;
    m_nBestCarTwoWheelsTimeMs          = rhs.m_nBestCarTwoWheelsTimeMs;
    m_fBestCarTwoWheelsDistM           = rhs.m_fBestCarTwoWheelsDistM;
    m_nBestBikeWheelieTimeMs           = rhs.m_nBestBikeWheelieTimeMs;
    m_fBestBikeWheelieDistM            = rhs.m_fBestBikeWheelieDistM;
    m_nBestBikeStoppieTimeMs           = rhs.m_nBestBikeStoppieTimeMs;
    m_fBestBikeStoppieDistM            = rhs.m_fBestBikeStoppieDistM;
    m_nCarDensityForCurrentZone        = rhs.m_nCarDensityForCurrentZone;
    m_fRoadDensityAroundPlayer         = rhs.m_fRoadDensityAroundPlayer;
    m_nTimeOfLastCarExplosionCaused    = rhs.m_nTimeOfLastCarExplosionCaused;
    m_nExplosionMultiplier             = rhs.m_nExplosionMultiplier;
    m_nHavocCaused                     = rhs.m_nHavocCaused;
    m_nNumHoursDidntEat                = rhs.m_nNumHoursDidntEat;
    m_fCurrentChaseValue               = rhs.m_fCurrentChaseValue;
    m_bDoesNotGetTired                 = rhs.m_bDoesNotGetTired;
    m_bFastReload                      = rhs.m_bFastReload;
    m_bFireProof                       = rhs.m_bFireProof;
    m_nMaxHealth                       = rhs.m_nMaxHealth;
    m_nMaxArmour                       = rhs.m_nMaxArmour;
    m_bGetOutOfJailFree                = rhs.m_bGetOutOfJailFree;
    m_bFreeHealthCare                  = rhs.m_bFreeHealthCare;
    m_bCanDoDriveBy                    = rhs.m_bCanDoDriveBy;
    m_nBustedAudioStatus               = rhs.m_nBustedAudioStatus;
    m_nLastBustMessageNumber           = rhs.m_nLastBustMessageNumber;
    m_nCrosshairActivated              = rhs.m_nCrosshairActivated;
    m_vecCrosshairTarget               = rhs.m_vecCrosshairTarget;
    m_pSkinTexture                     = rhs.m_pSkinTexture;
    m_bParachuteReferenced             = rhs.m_bParachuteReferenced;
    m_nRequireParachuteTimer           = rhs.m_nRequireParachuteTimer;
    strcpy_s(m_szSkinName, rhs.m_szSkinName);
    return *this;
}
