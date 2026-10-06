/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "Bike.h"

#include "Buoyancy.h"
#include "VehicleRecording.h"
#include "CarAI.h"
#include "CarCtrl.h"
#include "Stats.h"
#include "Coronas.h"
#include "PointLights.h"
#include "Shadows.h"

void CBike::InjectHooks() {
    RH_ScopedVirtualClass(CBike, 0x871360, 67);
    RH_ScopedCategory("Vehicle");

    RH_ScopedInstall(Constructor, 0x6BF430);
    RH_ScopedInstall(Destructor, 0x6B57A0);
    RH_ScopedInstall(dmgDrawCarCollidingParticles, 0x6B5A00);
    RH_ScopedInstall(DamageKnockOffRider, 0x6B5A10);
    RH_ScopedInstall(KnockOffRider, 0x6B5F40);
    RH_ScopedInstall(SetRemoveAnimFlags, 0x6B5F50);
    RH_ScopedInstall(ReduceHornCounter, 0x6B5F90);
    RH_ScopedVMTInstall(ProcessAI, 0x6BC930);
    RH_ScopedInstall(ProcessBuoyancy, 0x6B5FB0);
    RH_ScopedInstall(ResetSuspension, 0x6B6740);
    RH_ScopedInstall(GetAllWheelsOffGround, 0x6B6790);
    RH_ScopedInstall(DebugCode, 0x6B67A0);
    RH_ScopedInstall(DoSoftGroundResistance, 0x6B6D40);
    RH_ScopedInstall(PlayHornIfNecessary, 0x6B7130);
    RH_ScopedInstall(CalculateLeanMatrix, 0x6B7150);
    RH_ScopedInstall(ProcessRiderAnims, 0x6B7280);
    RH_ScopedInstall(FixHandsToBars, 0x6B7F90);
    RH_ScopedInstall(PlaceOnRoadProperly, 0x6BEEB0);
    RH_ScopedInstall(GetCorrectedWorldDoorPosition, 0x6BF230);
    RH_ScopedVMTInstall(Fix, 0x6B7050);
    RH_ScopedVMTInstall(BlowUpCar, 0x6BEA10);
    RH_ScopedVMTInstall(ProcessDrivingAnims, 0x6BF400);
    RH_ScopedVMTInstall(BurstTyre, 0x6BEB20);
    RH_ScopedVMTInstall(ProcessControlInputs, 0x6BE310);
    RH_ScopedVMTInstall(ProcessEntityCollision, 0x6BDEA0);
    RH_ScopedVMTInstall(Render, 0x6BDE20);
    RH_ScopedVMTInstall(PreRender, 0x6BD090);
    RH_ScopedVMTInstall(Teleport, 0x6BCFC0);
    RH_ScopedVMTInstall(ProcessControl, 0x6B9250);
    RH_ScopedVMTInstall(VehicleDamage, 0x6B8EC0);
    RH_ScopedVMTInstall(SetupSuspensionLines, 0x6B89B0);
    RH_ScopedVMTInstall(SetModelIndex, 0x6B8970);
    RH_ScopedVMTInstall(PlayCarHorn, 0x6B7080);
    RH_ScopedVMTInstall(SetupDamageAfterLoad, 0x6B7070);
    RH_ScopedVMTInstall(DoBurstAndSoftGroundRatios, 0x6B6950);
    RH_ScopedVMTInstall(SetUpWheelColModel, 0x6B67E0);
    RH_ScopedVMTInstall(RemoveRefsToVehicle, 0x6B67B0);
    RH_ScopedVMTInstall(ProcessControlCollisionCheck, 0x6B6620);
    RH_ScopedVMTInstall(GetComponentWorldPosition, 0x6B5990);
    RH_ScopedVMTInstall(ProcessOpenDoor, 0x6B58D0);
}

// 0x6BF430
CBike::CBike(int32 modelIndex, eVehicleCreatedBy createdBy) :
    CVehicle(createdBy) {
    auto mi = CModelInfo::GetModelInfo(modelIndex)->AsVehicleModelInfoPtr();
    if (mi->m_nVehicleType == VEHICLE_TYPE_BIKE) {
        const auto& animationStyle = CAnimManager::GetAnimBlocks()[mi->GetAnimFileIndex()].GroupId;
        m_RideAnimData.AnimGroup   = animationStyle;
        if (animationStyle < ANIM_GROUP_BIKES || animationStyle > ANIM_GROUP_WAYFARER) {
            m_RideAnimData.AnimGroup = ANIM_GROUP_BIKES;
        }
    }

    m_nVehicleSubType = VEHICLE_TYPE_BIKE;
    m_nVehicleType    = VEHICLE_TYPE_BIKE;

    m_BlowUpTimer     = 0.0f;
    m_nBrakesOn       = false;
    nBikeFlags        = 0;
    SetModelIndex(modelIndex);

    m_pHandlingData          = gHandlingDataMgr.GetVehiclePointer(mi->m_nHandlingId);
    m_BikeHandling           = gHandlingDataMgr.GetBikeHandlingPointer(mi->m_nHandlingId);
    m_nHandlingFlagsIntValue = m_pHandlingData->m_nHandlingFlags;
    m_pFlyingHandlingData    = gHandlingDataMgr.GetFlyingPointer(static_cast<uint8>(mi->m_nHandlingId));
    m_fBrakeCount            = 20.0f;
    mi->ChooseVehicleColour(m_nPrimaryColor, m_nSecondaryColor, m_nTertiaryColor, m_nQuaternaryColor, 1);
    m_fSwingArmLength       = 0.0f;
    m_fForkYOffset          = 0.0f;
    m_fForkZOffset          = 0.0f;
    m_nFixLeftHand          = false;
    m_nFixRightHand         = false;
    m_fSteerAngleTan        = std::tan(DegreesToRadians(mi->m_fBikeSteerAngle));
    m_fMass                 = m_pHandlingData->m_fMass;
    m_fTurnMass             = m_pHandlingData->m_fTurnMass;
    m_vecCentreOfMass       = m_pHandlingData->m_vecCentreOfMass;
    m_vecCentreOfMass.z     = 0.1f;
    m_fAirResistance        = GetDefaultAirResistance();
    m_fElasticity           = 0.05f;
    m_fBuoyancyConstant     = m_pHandlingData->m_fBuoyancyConstant;
    m_fSteerAngle           = 0.0f;
    m_GasPedal              = 0.0f;
    m_BrakePedal            = 0.0f;
    m_Damager               = nullptr;
    m_pWhoInstalledBombOnMe = nullptr;
    m_GasPedalAudioRevs     = 0.0f;
    m_fTyreTemp             = 1.0f;
    m_fBrakingSlide         = 0.0f;
    m_PrevSpeed             = 0.0f;

    for (auto i = 0; i < 2; ++i) {
        m_nWheelStatus[i]            = 0;
        m_aWheelSkidmarkType[i]      = eSkidmarkType::DEFAULT;
        m_bWheelBloody[i]            = false;
        m_bMoreSkidMarks[i]          = false;
        m_aWheelPitchAngles[i]       = 0.0f;
        m_aWheelAngularVelocity[i]   = 0.0f;
        m_aWheelSuspensionHeights[i] = 0.0f;
        m_aWheelOrigHeights[i]       = 0.0f;
        m_WheelStates[i]             = WHEEL_STATE_NORMAL;
    }

    for (auto i = 0; i < 4; ++i) {
        m_aWheelColPoints[i]     = {};
        m_aWheelRatios[i]        = 1.0f;
        m_aRatioHistory[i]       = 0.0f;
        m_WheelCounts[i]         = 0.0f;
        m_fSuspensionLength[i]   = 0.0f;
        m_fLineLength[i]         = 0.0f;
        m_aGroundPhysicalPtrs[i] = nullptr;
        m_aGroundOffsets[i]      = CVector{};
    }

    m_nNoOfContactWheels              = 0;
    m_NumDriveWheelsOnGround          = 0;
    m_NumDriveWheelsOnGroundLastFrame = 0;
    m_fHeightAboveRoad                = 0.0f;
    m_fExtraTractionMult              = 1.0f;

    if (!mi->m_pColModel->m_pColData->m_pLines) {
        mi->m_pColModel->m_pColData->m_nNumLines              = 4;
        mi->m_pColModel->m_pColData->m_pLines                 = static_cast<CColLine*>(CMemoryMgr::Malloc(4 * sizeof(CColLine)));
        mi->m_pColModel->m_pColData->m_pLines[1].m_vecStart.x = 99'999.99f; // todo: explain this
    }
    mi->m_pColModel->m_pColData->m_pLines[0].m_vecStart.z = 99'999.99f;
    CBike::SetupSuspensionLines();

    m_autoPilot.m_nTempAction = TEMPACT_NONE;
    m_autoPilot.SetCarMission(MISSION_NONE, 0);
    m_autoPilot.carCtrlFlags.bAvoidLevelTransitions = false;

    SetStatus(STATUS_SIMPLE);
    m_nNumPassengers         = 0;
    vehicleFlags.bLowVehicle = false;
    vehicleFlags.bIsBig      = false;
    vehicleFlags.bIsVan      = false;

    m_bLeanMatrixCalculated  = false;
    m_mLeanMatrix            = *m_matrix;
    m_vecOldSpeedForPlayback = CVector{};
    m_vehicleAudio.Initialise(this);
}

// 0x6B57A0
CBike::~CBike() {
    m_vehicleAudio.Terminate();
}

// 0x6B5A00
void CBike::dmgDrawCarCollidingParticles(const CVector& position, float power, eWeaponType weaponType) {
    // NOP
}

// 0x6B5A10
bool CBike::DamageKnockOffRider(CVehicle* vehicle, float damageIntensity, uint16 pieceType, CEntity* damager, const CVector& collisionPos, const CVector& collisionImpactVelocity) {
    const auto driver    = vehicle->m_pDriver;
    const auto passenger = vehicle->m_apPassengers[0];

    // Impact force relative to the bike's mass
    auto force = damageIntensity / vehicle->m_fMass * 800.0f;

    // A skilled rider resists being knocked off (unless flagged to always come off)
    if (vehicle->GetStatus() != STATUS_PLAYER) {
        if (driver && driver->CantBeKnockedOffBike != CANT_BE_KNOCKED_OFF_ALWAYS_NORMAL) {
            force *= 1.0f - driver->GetBikeRidingSkill() * 0.6f;
        }
    } else {
        force *= 0.75f;
        if (driver) {
            force *= 1.0f - driver->GetBikeRidingSkill() * 0.5f;
        }
    }

    // Only an actual driver gets knocked off
    if (!driver || !driver->IsStateDriving() || force <= 10.0f) {
        return false;
    }

    // A ped already reacting to a hit isn't also knocked off (cops are exempt)
    if (const auto task = driver->GetIntelligence()->GetTaskManager().GetActiveTask()) {
        if (task->GetTaskType() == TASK_SIMPLE_BE_HIT && !driver->IsCop()) {
            return false;
        }
    }

    const auto impactFwdMag   = vehicle->GetForward().Dot(collisionImpactVelocity);
    const auto impactUpMag    = vehicle->GetUp().Dot(collisionImpactVelocity);
    const auto impactRightMag = vehicle->GetRight().Dot(collisionImpactVelocity);

    // Per-axis weighting of the impact
    auto fwdWeight = 0.6f;
    if (std::abs(impactFwdMag) > 0.85f) {
        const auto vertical = collisionImpactVelocity.z < 0.85f ? 0.0f : collisionImpactVelocity.z;
        fwdWeight           = 7.0f * sq(vertical) + 0.6f;
    }
    if (vehicle->GetUp().z < 0.0f) { // bike lying on its side / upside down
        fwdWeight = 5.0f;
    }

    auto backWeight = 1.5f;
    auto upWeight   = 0.05f;
    if (vehicle->m_nModelIndex == MODEL_SANCHEZ) {
        fwdWeight *= 0.65f;
        upWeight *= 0.75f;
    } else if (vehicle->IsSubQuad()) {
        backWeight = 3.0f;
        fwdWeight *= 0.65f;
        upWeight *= 0.75f;
    }

    if (impactFwdMag > 0.0f) {
        fwdWeight *= 1.0f - driver->GetBikeRidingSkill() * 0.6f;
    }

    force *= std::abs(impactFwdMag) * fwdWeight
        + std::max(impactUpMag, 0.0f) * upWeight
        + std::abs(impactRightMag) * 0.45f
        - std::min(impactUpMag, 0.0f) * backWeight;

    // Don't knock the player off while they're on stairs
    if (driver->IsPlayer() && CCullZones::CamStairsForPlayer() && CCullZones::FindZoneWithStairsAttributeForPlayer()) {
        force = 0.0f;
    }

    // ALWAYS_HARD peds come off at a much lower force threshold
    if (force <= (driver->CantBeKnockedOffBike == CANT_BE_KNOCKED_OFF_ALWAYS_HARD ? 20.0f : 75.0f)) {
        return false;
    }

    // NEVER peds are never knocked off
    if (driver->CantBeKnockedOffBike == CANT_BE_KNOCKED_OFF_NEVER) {
        return false;
    }
    if (passenger && passenger->CantBeKnockedOffBike == CANT_BE_KNOCKED_OFF_NEVER) {
        return false;
    }

    // The driver (guaranteed present here) is thrown off, and so is the passenger, both reacting with the driver's facing
    const auto knockOffDir = (uint8)driver->GetLocalDirection(-CVector2D{ collisionImpactVelocity });

    driver->GetEventGroup().Add(CEventKnockOffBike{ vehicle, vehicle->m_vecMoveSpeed, collisionImpactVelocity, damageIntensity, 0.05f * force, KNOCK_OFF_TYPE_SKIDBACKFRONT, knockOffDir, 0, nullptr, true, false });
    if (passenger) {
        passenger->GetEventGroup().Add(CEventKnockOffBike{ vehicle, vehicle->m_vecMoveSpeed, collisionImpactVelocity, damageIntensity, 0.05f * force, KNOCK_OFF_TYPE_SKIDBACKFRONT, knockOffDir, 0, nullptr, false, false });
    }
    return true;
}

// dummy function
// 0x6B5F40
CPed* CBike::KnockOffRider(eWeaponType arg0, uint8 arg1, CPed* ped, bool arg3) {
    return ped;
}

// 0x6B5F50
void CBike::SetRemoveAnimFlags(CPed* ped) {
    if (!ped->GetIsTypePed()) {
        return;
    }
    for (auto assoc = RpAnimBlendClumpGetFirstAssociation(ped->GetRpClump(), ANIMATION_SECONDARY_TASK_ANIM); assoc; assoc = RpAnimBlendGetNextAssociation(assoc, ANIMATION_SECONDARY_TASK_ANIM)) {
        assoc->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
    }
}

// 0x6B5F90
void CBike::ReduceHornCounter() {
    if (m_HornCounter) {
        m_HornCounter -= 1;
    }
}

// 0x6B5FB0
void CBike::ProcessBuoyancy() {
    CVector vecBuoyancyTurnPoint;
    CVector vecBuoyancyForce;
    if (!mod_Buoyancy.ProcessBuoyancy(this, m_fBuoyancyConstant, &vecBuoyancyTurnPoint, &vecBuoyancyForce)) {
        vehicleFlags.bIsDrowning        = false;
        physicalFlags.bSubmergedInWater = false;
        physicalFlags.bTouchingWater    = false;
        return;
    }

    physicalFlags.bTouchingWater = true;
    ApplyMoveForce(vecBuoyancyForce);
    ApplyTurnForce(vecBuoyancyForce, vecBuoyancyTurnPoint);

    auto fTimeStep       = std::max(0.01F, CTimer::GetTimeStep());
    auto fUsedMass       = m_fMass / 125.0F;
    auto fBuoyancyForceZ = vecBuoyancyForce.z / (fTimeStep * fUsedMass);

    if (fUsedMass > m_fBuoyancyConstant) {
        fBuoyancyForceZ *= 1.05F * fUsedMass / m_fBuoyancyConstant;
    }

    if (physicalFlags.bMakeMassTwiceAsBig) {
        fBuoyancyForceZ *= 1.5F;
    }

    auto fBuoyancyForceMult = std::max(0.5F, 1.0F - fBuoyancyForceZ / 20.0F);
    auto fSpeedMult         = std::pow(fBuoyancyForceMult, CTimer::GetTimeStep());
    m_vecMoveSpeed *= fSpeedMult;
    m_vecTurnSpeed *= fSpeedMult;

    // 0x6B6443
    if (fBuoyancyForceZ > 0.8F || (fBuoyancyForceZ > 0.4F && IsAnyWheelNotMakingContactWithGround())) {
        vehicleFlags.bIsDrowning        = true;
        physicalFlags.bSubmergedInWater = true;

        m_vecMoveSpeed.z                = std::max(-0.1F, m_vecMoveSpeed.z);

        if (m_pDriver) {
            ProcessPedInVehicleBuoyancy(m_pDriver->AsPed(), true);
        } else {
            vehicleFlags.bEngineOn = false;
        }

        for (const auto passenger : GetPassengers()) {
            ProcessPedInVehicleBuoyancy(passenger, false);
        }
    } else {
        vehicleFlags.bIsDrowning        = false;
        physicalFlags.bSubmergedInWater = false;
    }
}

inline void CBike::ProcessPedInVehicleBuoyancy(CPed* ped, bool bIsDriver) {
    if (!ped) {
        return;
    }

    ped->physicalFlags.bTouchingWater = true;
    if (!ped->IsPlayer() && bikeFlags.bWaterTight) {
        return;
    }

    if (ped->IsPlayer()) {
        ped->AsPlayer()->HandlePlayerBreath(true, 1.0F);
    }

    if (IsAnyWheelMakingContactWithGround()) {
        if (!ped->IsPlayer()) {
            auto pedDamageResponseCalc = CPedDamageResponseCalculator(this, CTimer::GetTimeStep(), eWeaponType::WEAPON_DROWNING, PED_PIECE_TORSO, false);
            auto damageEvent           = CEventDamage(this, CTimer::GetTimeInMS(), eWeaponType::WEAPON_DROWNING, PED_PIECE_TORSO, 0, false, true);
            if (damageEvent.AffectsPed(ped)) {
                pedDamageResponseCalc.ComputeDamageResponse(ped, damageEvent.m_damageResponse, true);
            } else {
                damageEvent.m_damageResponse.m_bDamageCalculated = true;
            }

            ped->GetEventGroup().Add(&damageEvent, false);
        }
    } else {
        auto knockOffBikeEvent = CEventKnockOffBike(this, m_vecMoveSpeed, m_vecLastCollisionImpactVelocity, m_fDamageIntensity, 0.0F, KNOCK_OFF_TYPE_FALL, 0, 0, nullptr, bIsDriver, false);
        ped->GetEventGroup().Add(&knockOffBikeEvent);
        if (bIsDriver) {
            vehicleFlags.bEngineOn = false;
        }
    }
}

// 0x6BC930
bool CBike::ProcessAI(uint32& extraHandlingFlags) {
    const auto mi = GetVehicleModelInfo();

    m_autoPilot.carCtrlFlags.bHonkAtCar = false;
    m_autoPilot.carCtrlFlags.bHonkAtPed = false;

    if (const auto recId = m_autoPilot.m_vehicleRecordingId; recId >= 0 && !CVehicleRecording::bUseCarAI[recId]) {
        extraHandlingFlags += 2;
        return false;
    }

    switch (GetStatus()) {
    case STATUS_PLAYER: { // 0x6BC994
        extraHandlingFlags += 2;
        bikeFlags.bGettingPickedUp = false;

        if (FindPlayerPed()->m_nPedState != PEDSTATE_EXIT_CAR && FindPlayerPed()->m_nPedState != PEDSTATE_DRAGGED_FROM_CAR) {
            if (m_pDriver) {
                if (m_pDriver == FindPlayerPed(0)) {
                    ProcessControlInputs(0);
                } else if (m_pDriver == FindPlayerPed(1)) {
                    ProcessControlInputs(1);
                }
            }

            const auto leanFwd = m_RideAnimData.LeanFwd;

            // Applies the lean force using the given pedal and handling multiplier
            const auto ApplyLeanForce = [&](float pedal, float forceMult) {
                const auto speed = std::min(0.1f, m_vecMoveSpeed.Magnitude());
                auto       force = (std::max(speed / 0.1f, pedal) + pedal) * (forceMult * m_fTurnMass * leanFwd * speed) * 0.5f;
                force *= CStats::GetFatAndMuscleModifier(STAT_MOD_11);
                ApplyTurnForce(
                    GetUp() * -(CTimer::GetTimeStep() * force),
                    m_vecCentreOfMass + GetForward()
                );
            };

            if (leanFwd < 0.f) { // 0x6BCA2C - Leaning back
                m_vecCentreOfMass.y = m_BikeHandling->m_fLeanBakCOM * leanFwd + m_pHandlingData->m_vecCentreOfMass.y;
                if ((m_BrakePedal == 0.f && !vehicleFlags.bIsHandbrakeOn) || !m_nNoOfContactWheels) {
                    ApplyLeanForce(m_GasPedal, m_BikeHandling->m_fLeanBakForce);
                }
            } else { // 0x6BCAFE - Leaning forward
                m_vecCentreOfMass.y = m_BikeHandling->m_fLeanFwdCOM * leanFwd + m_pHandlingData->m_vecCentreOfMass.y;
                if (m_BrakePedal < 0.f || !m_nNoOfContactWheels) {
                    ApplyLeanForce(m_BrakePedal, m_BikeHandling->m_fLeanFwdForce);
                }
            }

            PruneReferences();

            if (GetStatus() == STATUS_PLAYER) {
                DoDriveByShootings();
            }

            DoSoftGroundResistance(extraHandlingFlags);
        }

        if (CPad::GetPad(0)->CarGunJustDown()) { // 0x6BCC23
            ActivateBomb();
        }
        return true;
    }
    case STATUS_PLAYER_PLAYBACK_FROM_BUFFER: { // 0x6BCC50
        extraHandlingFlags += 2;
        return true;
    }
    case STATUS_SIMPLE: { // 0x6BCC63
        CCarAI::UpdateCarAI(this);
        CPhysical::ProcessControl();
        CCarCtrl::UpdateCarOnRails(this);

        m_NumDriveWheelsOnGroundLastFrame = m_NumDriveWheelsOnGround;
        m_nNoOfContactWheels              = 2;
        m_NumDriveWheelsOnGround          = 2;

        m_pHandlingData->GetTransmission().CalculateGearForSimpleCar(m_autoPilot.m_speed * 0.02f, m_nCurrentGear);

        {
            const auto ts = CTimer::GetTimeStep();
            m_aWheelPitchAngles[0] += ProcessWheelRotation(WHEEL_STATE_NORMAL, GetForward(), m_vecMoveSpeed, mi->m_fWheelSizeFront * 0.5f) * ts;
        }
        {
            const auto ts = CTimer::GetTimeStep();
            m_aWheelPitchAngles[1] += ProcessWheelRotation(WHEEL_STATE_NORMAL, GetForward(), m_vecMoveSpeed, mi->m_fWheelSizeRear * 0.5f) * ts;
        }

        PlayHornIfNecessary();
        ReduceHornCounter();

        bikeFlags.bWheelieForCamera       = false;
        vehicleFlags.bAudioChangingGear   = false;
        vehicleFlags.bVehicleColProcessed = false;
        return true;
    }
    case STATUS_PHYSICS:
    case STATUS_GHOST: { // 0x6BCD7B
        CCarAI::UpdateCarAI(this);
        CCarCtrl::SteerAICarWithPhysics(this);
        PlayHornIfNecessary();

        extraHandlingFlags += 2;
        bikeFlags.bWheelieForCamera = false;

        if (vehicleFlags.bIsBeingCarJacked) {
            vehicleFlags.bIsHandbrakeOn = true;
            m_GasPedal                  = 0.f;
            m_BrakePedal                = 1.f;
        } else {
            bikeFlags.bGettingPickedUp = false;
        }
        return true;
    }
    case STATUS_ABANDONED: { // 0x6BCDFB
        m_BrakePedal                = 0.f;
        vehicleFlags.bIsHandbrakeOn = m_vecMoveSpeed.SquaredMagnitude() < 0.01f || bikeFlags.bOnSideStand;
        m_GasPedal                  = 0.f;
        m_HornCounter               = 0;

        if ((m_pDriver || m_apPassengers[0] || vehicleFlags.bIsBeingCarJacked) && !bikeFlags.bOnSideStand) {
            extraHandlingFlags += 2;
        }

        m_RideAnimData.AnimLeanLeft = 0.f;
        m_RideAnimData.AnimLeanFwd  = 0.f;
        bikeFlags.bWheelieForCamera = false;

        if (vehicleFlags.bIsBeingCarJacked) {
            vehicleFlags.bIsHandbrakeOn = true;
            m_GasPedal                  = 0.f;
            m_BrakePedal                = 1.f;
        }
        return true;
    }
    case STATUS_WRECKED: { // 0x6BCF1F
        vehicleFlags.bIsHandbrakeOn = true;
        bikeFlags.bWheelieForCamera = false;
        m_BrakePedal                = 0.05f;
        m_fSteerAngle               = 0.f;
        m_GasPedal                  = 0.f;
        m_HornCounter               = 0;
        m_RideAnimData.AnimLeanLeft = 0.f;
        m_RideAnimData.AnimLeanFwd  = 0.f;
        return true;
    }
    case STATUS_FORCED_STOP: { // 0x6BCEBC
        if (m_vecMoveSpeed.SquaredMagnitude() < 0.01f) {
            vehicleFlags.bIsHandbrakeOn = true;
            m_BrakePedal                = 1.f;
        } else {
            vehicleFlags.bIsHandbrakeOn = false;
            m_BrakePedal                = 0.f;
        }
        m_fSteerAngle = 0.f;
        m_GasPedal    = 0.f;
        m_HornCounter = 0;

        extraHandlingFlags += 2;
        bikeFlags.bWheelieForCamera = false;
        return true;
    }
    }
    return true;
}

// 0x6BF400
void CBike::ProcessDrivingAnims(CPed* driver, bool blend) {
    if (m_bOffscreen && GetStatus() == STATUS_PLAYER) {
        return;
    }

    ProcessRiderAnims(driver, this, &m_RideAnimData, m_BikeHandling, 0);
}

// 0x6B7280
void CBike::ProcessRiderAnims(CPed* rider, CVehicle* vehicle, CRideAnimData* rideData, tBikeHandlingData* handling, int16 a5) {
    const bool isPlayer = rider->IsPlayer();

    CBike*       bike{};
    CAutomobile* automobile{};
    int16        numContactWheels{};
    if (vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE) {
        bike             = static_cast<CBike*>(vehicle);
        numContactWheels = bike->m_nNoOfContactWheels;
    } else if (vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE) {
        automobile       = static_cast<CAutomobile*>(vehicle);
        numContactWheels = automobile->m_nNumContactWheels;
    }

    float blendLeft = 1.0f; // Blend amount left for the lean anims
    float leanFwd   = 0.0f;

    const auto clump = rider->GetRpClump();

    auto* leftAssoc  = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_BIKE_LEFT);
    auto* rightAssoc = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_BIKE_RIGHT);
    auto* stillAssoc = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_BIKE_STILL);
    CAnimBlendAssociation *fwdAssoc{}, *backAssoc{};
    if (isPlayer) {
        fwdAssoc  = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_BIKE_FWD);
        backAssoc = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_BIKE_BACK);
    }
    auto* pushesAssoc  = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_BIKE_PUSHES);
    auto* driveByAssoc = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_BIKE_DRIVEBYLHS);
    if (!driveByAssoc) {
        driveByAssoc = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_BIKE_DRIVEBYRHS);
        if (!driveByAssoc) {
            driveByAssoc = RpAnimBlendClumpGetAssociation(clump, ANIM_ID_BIKE_DRIVEBYFT);
        }
    }

    const float fwdSpeed = vehicle->m_vecMoveSpeed.Dot(vehicle->m_matrix->GetForward());

    // 0x6B73EA
    if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_BMX && (pushesAssoc || driveByAssoc)) {
        static_cast<CBike*>(vehicle)->m_nFixRightHand = 0;
    }

    // Knock the rider (and the passenger) off the bike
    const auto KnockOffBike = [&] {
        CEventKnockOffBike riderEvent{ vehicle, vehicle->m_vecMoveSpeed, CVector{ 0.0f, 0.0f, 1.0f }, 0.0f, 0.0f, KNOCK_OFF_TYPE_SKIDBACK_FALLR, 2, 0, nullptr, true, false };
        rider->GetEventGroup().Add(&riderEvent, false);

        if (auto* const passenger = vehicle->m_apPassengers[0]; passenger && passenger != rider) {
            CEventKnockOffBike passengerEvent{ vehicle, vehicle->m_vecMoveSpeed, CVector{ 0.0f, 0.0f, 1.0f }, 0.0f, 0.0f, KNOCK_OFF_TYPE_SKIDBACK_FALLR, 2, 0, nullptr, false, false };
            passenger->GetEventGroup().Add(&passengerEvent, false);
        }
    };

    // 0x6B740A - Spinning too fast
    if (const float maxTurnSpeed = (rider->GetBikeRidingSkill() + 1.0f) * 0.3f; vehicle->m_vecTurnSpeed.SquaredMagnitude() > sq(maxTurnSpeed)) {
        KnockOffBike();
    }

    // Whenever the anim needs to be (re)started
    const auto ShouldBlendIn = [](const CAnimBlendAssociation* assoc) {
        return !assoc || (assoc->m_BlendAmount < 1.0f && assoc->m_BlendDelta <= 0.0f);
    };
    const auto BlendOutStillAndPushes = [&] {
        if (stillAssoc && !(stillAssoc->m_BlendDelta < 0.0f)) {
            stillAssoc->m_BlendDelta = -4.0f;
        }
        if (pushesAssoc && !(pushesAssoc->m_BlendDelta < 0.0f)) {
            pushesAssoc->m_BlendDelta = -4.0f;
        }
    };

    if (!driveByAssoc && std::abs(fwdSpeed) < 0.02f) { // 0x6B755C - Standing still
        if (ShouldBlendIn(stillAssoc)) {
            stillAssoc = CAnimManager::BlendAnimation(clump, rideData->AnimGroup, ANIM_ID_BIKE_STILL, 2.0f);
        }
    } else if (fwdSpeed < 0.0f) { // 0x6B75D6 - Reversing
        float maxReverseSpeedMult = (rider->GetBikeRidingSkill() + 1.0f) * 3.5f;
        if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_QUAD) {
            maxReverseSpeedMult *= 2.0f;
        }
        const float maxReverseVelocity = vehicle->m_pHandlingData->m_transmissionData.m_MaxReverseVelocity;

        const bool bFallOff = maxReverseSpeedMult * maxReverseVelocity > fwdSpeed // Going backwards way too fast
            && (numContactWheels > 0
                || (vehicle->m_matrix->GetUp().z < -0.5f && CTimer::GetTimeInMS() - vehicle->m_nLastCollisionTime < 100));
        if (bFallOff) { // 0x6B7642
            KnockOffBike();
        } else if (vehicle->m_GasPedal < 0.0f && maxReverseVelocity * 1.5 < fwdSpeed) { // 0x6B7742 - Pushing the bike backwards
            if (ShouldBlendIn(pushesAssoc)) {
                pushesAssoc = CAnimManager::BlendAnimation(clump, rideData->AnimGroup, ANIM_ID_BIKE_PUSHES, 4.0f);
            }
        } else { // 0x6B77BD
            if (isPlayer && maxReverseVelocity * 1.5 > fwdSpeed) {
                leanFwd = -1.0f;
            }
            BlendOutStillAndPushes();
        }
    } else { // 0x6B77E6 - Moving forwards
        BlendOutStillAndPushes();
    }

    // 0x6B7821 - Figure out how much blend is left for the lean anims
    const float timeScale = CTimer::GetTimeStepNonClipped() * 0.02f;
    if (stillAssoc) {
        blendLeft = 1.0f - std::min(1.0f, timeScale * stillAssoc->m_BlendDelta + stillAssoc->m_BlendAmount);
    }
    if (driveByAssoc) {
        blendLeft -= std::min(blendLeft, timeScale * driveByAssoc->m_BlendDelta + driveByAssoc->m_BlendAmount);
    }
    if (pushesAssoc) {
        blendLeft -= std::min(blendLeft, timeScale * pushesAssoc->m_BlendDelta + pushesAssoc->m_BlendAmount);
    }

    // 0x6B78D2 - Left/right lean
    const float leanLeft = leanFwd == -1.0f
        ? 0.0f
        : std::clamp(rideData->LeanAngle / handling->m_fFullAnimLean, -1.0f, 1.0f);
    {
        const float f = std::pow(0.86f, CTimer::GetTimeStep());
        rideData->AnimLeanLeft = (1.0f - f) * leanLeft + f * rideData->AnimLeanLeft;
    }

    // 0x6B795F - Forwards/backwards lean
    if (isPlayer && !vehicle->m_apPassengers[0]) {
        if (leanFwd > -1.0f) {
            leanFwd = rideData->LeanFwd;

            const float fwdZ = vehicle->m_matrix->GetForward().z;

            // If set the rider leans back to keep the wheelie balanced
            bool  bKeepWheelieBalance{};
            float wheelieAngleLeft{};
            if (bike) { // 0x6B799B
                bike->bikeFlags.bWheelieForCamera = false;

                const auto& wc = bike->m_WheelCounts;
                if (wc[0] <= 0.0f && wc[1] <= 0.0f && fwdZ > 0.0f && (wc[2] > 0.0f || wc[3] > 0.0)) { // Wheelie
                    wheelieAngleLeft = handling->m_fWheelieAng - fwdZ;
                    if (wheelieAngleLeft < handling->m_fWheelieAng * 0.5f) {
                        bike->bikeFlags.bWheelieForCamera = true;
                    }
                    bKeepWheelieBalance = true;
                } else if (wc[2] <= 0.0f && wc[3] <= 0.0f && fwdZ < 0.0f && (wc[0] > 0.0f || wc[1] > 0.0f)) { // 0x6B7A3D - Stoppie
                    if (handling->m_fStoppieAng - fwdZ > handling->m_fStoppieAng * 0.6f) {
                        bike->bikeFlags.bWheelieForCamera = true;
                    }
                }
            } else if (automobile) { // 0x6B7B14 - Quad
                const auto& wc = automobile->m_WheelCounts;
                if (wc[0] <= 0.0f && wc[1] <= 0.0f && fwdZ > 0.0f && (wc[2] > 0.0f || wc[3] > 0.0f)) {
                    wheelieAngleLeft    = handling->m_fWheelieAng - fwdZ;
                    bKeepWheelieBalance = true;
                }
            }

            if (bKeepWheelieBalance && wheelieAngleLeft < 0.15f) { // 0x6B7B90
                leanFwd = std::max(leanFwd, 0.25f);
            } else if (vehicle->m_BrakePedal > 0.5f && fwdSpeed > 0.01f) { // 0x6B7AC4 - Braking
                leanFwd = std::max(leanFwd, 0.1f);
            } else if (vehicle->m_GasPedal > 0.5f && leanFwd <= 0.0f && vehicle->m_pHandlingData->m_transmissionData.m_MaxFlatVelocity * 0.3f > fwdSpeed) { // 0x6B7BBA - Accelerating from low speed
                leanFwd = std::min(leanFwd, -0.3f);
            }

            // 0x6B7C16 - Less forwards lean the more we're leaning to the side
            if (const float absLeanLeft = std::abs(leanLeft); absLeanLeft > 0.3f) {
                leanFwd *= std::max(0.0f, 1.0f - (absLeanLeft - 0.3f) / (0.56f - 0.3f));
            }
        }
    } else { // 0x6B7C64
        leanFwd = 0.0f;
        if (bike) {
            bike->bikeFlags.bWheelieForCamera = false;
        }
    }

    // 0x6B7C7F
    if (isPlayer) {
        const float f = std::pow(0.89f, CTimer::GetTimeStep());
        rideData->AnimLeanFwd = (1.0f - f) * leanFwd + f * rideData->AnimLeanFwd;
    } else {
        rideData->AnimLeanFwd = 0.0f;
    }

    // 0x6B7CB7 - Distribute the blend between the 2 pairs of lean anims
    float leftRightWeight, fwdBackWeight;
    if (std::abs(rideData->AnimLeanLeft) > 0.56f || !isPlayer) {
        leftRightWeight = 1.0f;
        fwdBackWeight   = 0.0f;
    } else if (std::abs(rideData->AnimLeanFwd) > 0.56f) {
        leftRightWeight = 0.0f;
        fwdBackWeight   = 1.0f;
    } else {
        leftRightWeight = rideData->AnimLeanLeft;
        fwdBackWeight   = rideData->AnimLeanFwd;
        if (const float mag = std::sqrt(sq(leftRightWeight) + sq(fwdBackWeight)); mag > 0.01f) {
            const float invMag = 1.0f / mag;
            leftRightWeight *= invMag;
            fwdBackWeight   *= invMag;
        }
        leftRightWeight = std::abs(leftRightWeight);
        fwdBackWeight   = std::abs(fwdBackWeight);
    }
    const float fwdBackBlend   = fwdBackWeight * blendLeft;
    const float leftRightBlend = leftRightWeight * blendLeft;

    // Use `amount` to set the progress of the `positive` or `negative` anim (and fully blend the other out)
    const auto SetLeanAnims = [](CAnimBlendAssociation* positive, CAnimBlendAssociation* negative, float amount, float blend) {
        auto* const active   = amount < 0.0f ? negative : positive;
        auto* const inactive = amount < 0.0f ? positive : negative;

        active->m_BlendAmount = blend;
        active->SetCurrentTime(active->m_BlendHier->m_fTotalTime * std::abs(amount));
        active->m_Flags &= ~ANIMATION_IS_PLAYING;

        inactive->m_BlendAmount = 0.0f;
    };

    // 0x6B7D68
    if (isPlayer) {
        if (!fwdAssoc) {
            fwdAssoc = CAnimManager::AddAnimation(clump, rideData->AnimGroup, ANIM_ID_BIKE_FWD);
        }
        if (!backAssoc) {
            backAssoc = CAnimManager::AddAnimation(clump, rideData->AnimGroup, ANIM_ID_BIKE_BACK);
        }
        SetLeanAnims(fwdAssoc, backAssoc, rideData->AnimLeanFwd, fwdBackBlend);
    }

    // 0x6B7E0A
    if (!leftAssoc) {
        leftAssoc = CAnimManager::AddAnimation(clump, rideData->AnimGroup, ANIM_ID_BIKE_LEFT);
    }
    if (!rightAssoc) {
        rightAssoc = CAnimManager::AddAnimation(clump, rideData->AnimGroup, ANIM_ID_BIKE_RIGHT);
    }
    SetLeanAnims(rightAssoc, leftAssoc, rideData->AnimLeanLeft, leftRightBlend);

    // 0x6B7EA6 - Shake the rider's head at speed
    if (fwdSpeed > 0.3f) {
        auto* const headQuat = &rider->m_apBones[PED_NODE_HEAD]->KeyFrame->q;
        const float maxAngle = fwdSpeed * 6.0f;
        const auto  RandomAngle = [&] {
            return (maxAngle - -maxAngle) * ((float)CGeneral::GetRandomNumber() * (1.0f / 32767.0f)) + -maxAngle;
        };
        RtQuatRotate(headQuat, &CPedIK::XaxisIK, RandomAngle(), rwCOMBINEPOSTCONCAT);
        RtQuatRotate(headQuat, &CPedIK::YaxisIK, RandomAngle(), rwCOMBINEPOSTCONCAT);
        rider->bUpdateMatricesRequired = true;
    }
}

// 0x6BEB20
bool CBike::BurstTyre(uint8 tyreComponentId, bool bPhysicalEffect) {
    if (vehicleFlags.bTyresDontBurst)
        return false;

    if (physicalFlags.bRenderScorched)
        return false;

    const auto wheel = [&]() -> eCarWheel {
        switch (tyreComponentId) {
        case CAR_PIECE_WHEEL_LF: return CAR_WHEEL_FRONT_LEFT;
        case CAR_PIECE_WHEEL_RL: return CAR_WHEEL_REAR_LEFT;
        default: NOTSA_UNREACHABLE();
        }
    }();

    const auto burst = m_nWheelStatus[wheel] == WHEEL_STATUS_OK;

    if (burst) {
        m_nWheelStatus[wheel] = WHEEL_STATUS_BURST;
        m_vehicleAudio.AddAudioEvent(AE_TYRE_BURST, 0.0f);

        if (GetStatus() == STATUS_SIMPLE) {
            CCarCtrl::SwitchVehicleToRealPhysics(this);
        }

        // 0x8D322C
        constexpr auto force = 0.02f;

        if (bPhysicalEffect) {
            ApplyMoveForce(m_matrix->GetRight() * CGeneral::GetRandomNumberInRange(-force, force) * m_fMass);
            ApplyTurnForce(
                m_matrix->GetRight() * CGeneral::GetRandomNumberInRange(-force, force) * m_fTurnMass,
                m_matrix->GetForward()
            );
        }
    }

    if ([&] {
        if (!m_pDriver) {
            return false;
        }

        const auto WereWheelsInAir = [&] (size_t wheelA, size_t wheelB) {
            return m_aRatioHistory[wheelA] >= 1.0 && m_aRatioHistory[wheelB] >= 1.0;
        };

        switch (tyreComponentId) {
            case CAR_PIECE_WHEEL_LF: {
                if (WereWheelsInAir(0, 1)) {
                    return false;
                }
                break;
            }
            case CAR_PIECE_WHEEL_RF: {
                if (WereWheelsInAir(2, 3)) {
                    return false;
                }
                break;
            }
            default: NOTSA_UNREACHABLE();
        }

        auto speed = m_vecMoveSpeed.Magnitude();

        if (speed <= 0.3f) {
            return false;
        }

        if (GetStatus() == STATUS_PLAYER && speed <= 0.55f) {
            return false;
        }

        return true;
    }()) {
        if (tyreComponentId == CAR_PIECE_WHEEL_LF) {
            const auto DoKnockOffEvent = [&] (CPed* ped, bool isDriver) {
                CEventKnockOffBike event{ this,
                    m_vecMoveSpeed,
                    m_vecLastCollisionImpactVelocity,
                    0.f,
                    0.f,
                    0x31u,
                    0,
                    0,
                    nullptr,
                    isDriver,
                    false };
                ped->GetEventGroup().Add(&event);
            };

            DoKnockOffEvent(m_pDriver, true);

            if (m_apPassengers[0]) {
                DoKnockOffEvent(m_apPassengers[0], false);
            }
        }
        else {
            ApplyTurnForce(GetRight() * m_fTurnMass * 0.02f * 2.f, GetForward());
        }
    }

    return burst;
}

// 0x6BE310
void CBike::ProcessControlInputs(uint8 playerNum) {
    const float fwdSpeed = m_vecMoveSpeed.Dot(GetForward());
    CPad* pad = CPad::GetPad(playerNum);
    if (pad->GetExitVehicle()) {
        vehicleFlags.bIsHandbrakeOn = true;
    } else {
        vehicleFlags.bIsHandbrakeOn = pad->GetHandBrake() != 0;
    }
    const float timeStep = CTimer::GetTimeStep();
    if (!CCamera::m_bUseMouse3rdPerson || !CVehicle::m_bEnableMouseSteering) {
        m_fRawSteerAngle += (float)(-(float)pad->GetSteeringLeftRight() * 0.0078125f - m_fRawSteerAngle) * timeStep * 0.2f;
        m_RideAnimData.LeanFwd += (float)(-(float)pad->GetSteeringUpDown() * 0.0078125f - m_RideAnimData.LeanFwd) * timeStep * 0.2f;
    } else if (CPad::NewMouseControllerState.m_AmountMoved.x == 0.0f && CPad::NewMouseControllerState.m_AmountMoved.y == 0.0f) {
        if (std::fabs(m_fRawSteerAngle) > 0.0f && CVehicle::m_nLastControlInput == eControllerType::MOUSE
            && pad->GetSteeringLeftRight() == 0 && pad->GetSteeringUpDown() == 0) {
            goto mouseDrift;
        }
        if (pad->GetSteeringLeftRight() == 0 && pad->GetSteeringUpDown() == 0 && CVehicle::m_nLastControlInput == eControllerType::MOUSE) {
            goto clampSteer;
        }
        CVehicle::m_nLastControlInput = eControllerType::KEYBOARD;
        m_fRawSteerAngle += (float)(-(float)pad->GetSteeringLeftRight() * 0.0078125f - m_fRawSteerAngle) * timeStep * 0.2f;
        m_RideAnimData.LeanFwd += (float)(-(float)pad->GetSteeringUpDown() * 0.0078125f - m_RideAnimData.LeanFwd) * timeStep * 0.2f;
    } else {
mouseDrift:
        CVehicle::m_nLastControlInput = eControllerType::MOUSE;
        if (pad->NewState.m_bVehicleMouseLook == 0) {
            m_fRawSteerAngle += CPad::NewMouseControllerState.m_AmountMoved.x * -0.0035f;
            m_RideAnimData.LeanFwd += CPad::NewMouseControllerState.m_AmountMoved.y * -0.0035f;
        }
        if (std::fabs(m_fRawSteerAngle) < 0.35f || pad->NewState.m_bVehicleMouseLook != 0) {
            m_fRawSteerAngle *= std::pow(0.98f, CTimer::GetTimeStep());
        }
        if (std::fabs(m_RideAnimData.LeanFwd) < 0.35f || pad->NewState.m_bVehicleMouseLook != 0) {
            m_RideAnimData.LeanFwd *= std::pow(0.98f, CTimer::GetTimeStep());
        }
    }
clampSteer:
    m_fRawSteerAngle = std::clamp(m_fRawSteerAngle, -1.0f, 1.0f);
    m_RideAnimData.LeanFwd = std::clamp(m_RideAnimData.LeanFwd, -1.0f, 1.0f);
    const float input = (float)(pad->GetAccelerate() - pad->GetBrake()) * 0.0039215689f;
    if (std::fabs(fwdSpeed) >= 0.01f) {
        if (fwdSpeed < 0.0f) {
            if (input >= 0.0f) {
                m_BrakePedal = input;
                m_GasPedal = 0.0f;
                goto updateSteer;
            }
            m_GasPedal = input;
        } else {
            if (input < 0.0f) {
                m_GasPedal = 0.0f;
                m_BrakePedal = -input;
                goto updateSteer;
            }
            m_GasPedal = input;
        }
    } else if (pad->GetAccelerate() > 150 && pad->GetBrake() > 150 && m_nVehicleSubType != VEHICLE_TYPE_BMX) {
        m_GasPedal = (float)pad->GetAccelerate() * 0.0039215689f;
        m_nBrakesOn = 1;
        m_BrakePedal = (float)pad->GetBrake() * 0.0039215689f;
        goto updateSteer;
    } else {
        m_GasPedal = input;
    }
    m_BrakePedal = 0.0f;
updateSteer: {
        float signedSquare = m_fRawSteerAngle * m_fRawSteerAngle;
        if (m_fRawSteerAngle < 0.0f) {
            signedSquare = -signedSquare;
        }
        StaticRef<float>(0xC1C804) = signedSquare; // 0xC1C804: unknown static kept for fidelity
        if (m_autoPilot.m_vehicleRecordingId < 0 || CVehicleRecording::bUseCarAI[m_autoPilot.m_vehicleRecordingId]) {
            m_fSteerAngle = m_pHandlingData->m_fSteeringLock * 0.017453292f * signedSquare;
        }
    }
    if (vehicleFlags.bComedyControls) {
        if ((CTimer::GetTimeInMS() & 0x3C00) < 0x3000) {
            m_GasPedal = 1.0f;
        }
        if (((CTimer::GetTimeInMS() >> 10) + 6 & 0xF) < 0xC) {
            m_BrakePedal = 0.0f;
        }
        vehicleFlags.bIsHandbrakeOn = false;
        if ((CTimer::GetTimeInMS() & 0x800) == 0) {
            m_fSteerAngle -= 0.03f;
        } else {
            m_fSteerAngle += 0.08f;
        }
    }
    if (CPad::GetPad(0)->DisablePlayerControls != 0 && CGameLogic::SkipState != SKIP_IN_PROGRESS) {
        m_BrakePedal = 1.0f;
        vehicleFlags.bIsHandbrakeOn = true;
        m_GasPedal = 0.0f;
        FindPlayerPed()->KeepAreaAroundPlayerClear();
        const float speed = m_vecMoveSpeed.Magnitude();
        if (speed > 0.28f) {
            m_vecMoveSpeed *= 0.28f / speed;
        }
    }
}

// 0x6BDEA0
int32 CBike::ProcessEntityCollision(CEntity* entity, CColPoint* outColPoints) {
    if (GetStatus() != STATUS_SIMPLE) {
        vehicleFlags.bVehicleColProcessed = true;
    }

    const auto tcd = GetColData(),
               ocd = entity->GetColData();

#ifdef FIX_BUGS // Text search for `FIX_BUGS@CAutomobile::ProcessEntityCollision:1`
    if (!tcd || !ocd) {
        return 0;
    }
#endif

    if (physicalFlags.bSkipLineCol || physicalFlags.bProcessingShift || entity->GetIsTypePed()) {
        tcd->m_nNumLines = 0; // Later reset back to original value
    }

    const auto ogWheelRatios = m_aWheelRatios;

    auto numColPts           = CCollision::ProcessColModels(
        GetMatrix(), *GetColModel(), entity->GetMatrix(), *entity->GetColModel(), *(std::array<CColPoint, 32>*)(outColPoints), m_aWheelColPoints.data(), m_aWheelRatios.data(), false
    );

    // Possibly add driver & entity collisions to `outColPoints`
    if (m_pDriver && m_nTestPedCollision) {
        const auto pcd = m_pDriver->GetColData();
        if (!pcd->m_nNumLines) {
            std::array<CColPoint, 32> pedCPs{};

            CMatrix driverMat = GetMatrix();
            driverMat.GetPosition() += GetDriverSeatDummyPositionWS();

            std::array<CColPoint, 32> pedEntityColPts{};
            const auto                numPedEntityColPts = CCollision::ProcessColModels(
                driverMat, *m_pDriver->GetColModel(), entity->GetMatrix(), *entity->GetColModel(), pedEntityColPts, nullptr, nullptr, false
            );

            if (numPedEntityColPts) {
                if (m_nTestPedCollision == 1) {
                    m_nTestPedCollision = 0;
                } else {
                    for (auto i = 0; i < numPedEntityColPts && numColPts < 32; i++) {
                        const auto& pedEntityCP = pedCPs[i];
                        if (pedEntityCP.m_nPieceTypeA == PED_COL_SPHERE_LEG) {
                            continue;
                        }
                        outColPoints[numColPts++] = pedEntityCP;
                    }
                }
            }
        }
    }

    size_t numProcessedLines{};
    if (tcd->m_nNumLines) {
        // Process the real wheels
        for (auto i = 0; i < NUM_SUSP_LINES; i++) {
            const auto& cp                  = m_aWheelColPoints[i];

            const auto wheelColPtsTouchDist = m_aWheelRatios[i];
            if (wheelColPtsTouchDist >= 1.f || wheelColPtsTouchDist >= ogWheelRatios[i]) {
                continue;
            }

            numProcessedLines++;

            m_anCollisionLighting[i] = cp.m_nLightingB;
            m_nContactSurface        = cp.m_nSurfaceTypeB;

            switch (entity->GetType()) {
            case ENTITY_TYPE_VEHICLE:
            case ENTITY_TYPE_OBJECT:  {
                CEntity::ChangeEntityReference(m_aGroundPhysicalPtrs[i], entity->AsPhysical());

                m_aGroundOffsets[i] = cp.m_vecPoint - entity->GetPosition();
                if (entity->GetIsTypeVehicle()) {
                    m_anCollisionLighting[i] = entity->AsVehicle()->m_anCollisionLighting[i];
                }
                break;
            }
            case ENTITY_TYPE_BUILDING: {
                m_pEntityWeAreOn    = entity;
                m_bTunnel           = entity->m_bTunnel;
                m_bTunnelTransition = entity->m_bTunnelTransition;
                break;
            }
            }
        }
    } else {
        tcd->m_nNumLines = NUM_SUSP_LINES;
    }

    if (numColPts > 0 || numProcessedLines > 0) {
        AddCollisionRecord(entity);
        if (!entity->GetIsTypeBuilding()) {
            entity->AsPhysical()->AddCollisionRecord(this);
        }
        if (numColPts > 0) {
            if (entity->GetIsTypeBuilding()
                || (entity->GetIsTypeObject() && entity->AsPhysical()->physicalFlags.bDisableCollisionForce)) {
                SetHasHitWall(true);
            }
        }
    }

    return numColPts;
}

// 0x6B9250
void CBike::ProcessControl() {
    static auto& vecTestResistance = StaticRef<CVector>(0x8D3238); // { 0.9995f, 0.9f, 0.95f }
    constexpr float fDAxisX      = 1.0f;    // 0x8712D8
    constexpr float fDAxisXExtra = 100.0f;  // 0x8712DC
    constexpr float fInAirXRes   = 0.98f;   // 0x8712E0
    constexpr float fDAxisY      = 1000.0f; // 0x8712E4

    constexpr uint32 EXTRA_BALANCED_BY_RIDER = 2; // Set by `ProcessAI`
    constexpr uint32 EXTRA_STUCK_IN_SAND     = 4; // Set by `DoSoftGroundResistance`

    auto* const colData = GetColModel()->m_pColData;
    auto* const mi      = CModelInfo::GetModelInfo(m_nModelIndex)->AsVehicleModelInfoPtr();

    uint32 extraHandlingFlags = 0;

    m_vehicleAudio.Service();

    vehicleFlags.bWarnedPeds        = false;
    bikeFlags.bPlayerBoost          = false;
    vehicleFlags.bRestingOnPhysical = false;
    m_bLeanMatrixCalculated         = false;
    m_nBrakesOn                     = 0;

    if (CReplay::Mode == MODE_PLAYBACK) {
        return;
    }

    ProcessCarAlarm();
    ActivateBombWhenEntered();
    UpdateClumpAlpha();

    // 0x6B92FA
    if (m_pDriver && (m_pDriver->IsPlayer() || (m_apPassengers[0] && m_apPassengers[0]->IsPlayer()))) {
        if (m_nTestPedCollision == 1) {
            m_nTestPedCollision = 2;
        } else if (m_nTestPedCollision < 1) {
            m_nTestPedCollision = 1;
        }
    } else {
        m_nTestPedCollision = 0;
    }

    // 0x6B9345
    ProcessAI(extraHandlingFlags);
    if (GetStatus() == STATUS_SIMPLE) {
        return;
    }

    // 0x6B9363
    if (bikeFlags.bOnSideStand) {
        if (std::fabs(GetRight().z) > 0.35f || std::fabs(GetForward().z) > 0.5f) {
            bikeFlags.bOnSideStand = false;
        }
    }

    // 0x6B939E
    if ((extraHandlingFlags & EXTRA_BALANCED_BY_RIDER) || bikeFlags.bGettingPickedUp || bikeFlags.bOnSideStand) {
        float         fDx            = fDAxisX;
        CVector       res            = vecTestResistance;
        const CVector localTurnSpeed = GetMatrix().InverseTransformVector(m_vecTurnSpeed);

        if (GetStatus() == STATUS_PLAYER) {
            if (m_aWheelRatios[0] >= 1.0f && m_aWheelRatios[1] >= 1.0f) { // 0x6B944F
                fDx = fDAxisXExtra;
                const float statMult = CStats::GetFatAndMuscleModifier(STAT_MOD_13) * 0.2f;
                if ((m_aWheelRatios[2] < 1.0f || m_aWheelRatios[3] < 1.0f) && GetForward().z > 0.0f) {
                    res.x -= std::min(std::fabs(m_BikeHandling->m_fWheelieAng - GetForward().z) * statMult, 0.05f);
                } else {
                    res.x = fInAirXRes;
                }
            } else if (m_WheelCounts[2] <= 0.0f && m_WheelCounts[3] <= 0.0f) { // 0x6B9522
                fDx = fDAxisXExtra;
                const float maxMult  = CStats::GetFatAndMuscleModifier(STAT_MOD_13) * 0.075f;
                const float statMult = CStats::GetFatAndMuscleModifier(STAT_MOD_13) * 0.25f;
                if (GetForward().z < 0.0f) {
                    res.x *= std::min(std::fabs(m_BikeHandling->m_fStoppieAng - GetForward().z) * statMult, maxMult) + 0.9f;
                }
            }
        }

        // 0x6B95AD
        res.x = res.x / (sq(localTurnSpeed.x) * fDx + 1.0f);
        res.y = res.y / (fDAxisY * sq(localTurnSpeed.y) + 1.0f);
        res.x = std::pow(res.x, CTimer::GetTimeStep());
        res.y = std::pow(res.y, CTimer::GetTimeStep());
        const float turnX = localTurnSpeed.x * res.x - localTurnSpeed.x;
        const float turnY = localTurnSpeed.y * res.y - localTurnSpeed.y;

        ApplyTurnForce(GetUp() * -1.0f * turnY * m_fTurnMass, GetMatrix().TransformVector(m_vecCentreOfMass) + GetRight());
        ApplyTurnForce(GetUp() * turnX * m_fTurnMass, GetMatrix().TransformVector(m_vecCentreOfMass) + GetForward());

        // 0x6B97A6
        if (GetStatus() != STATUS_PLAYER) {
            m_vecCentreOfMass = m_pHandlingData->m_vecCentreOfMass;
        }
    } else { // 0x6B93AE
        m_vecCentreOfMass.x = m_pHandlingData->m_vecCentreOfMass.x;
        m_vecCentreOfMass.y = m_pHandlingData->m_vecCentreOfMass.y;
        m_vecCentreOfMass.z = m_BikeHandling->m_fNoPlayerCOMz;
    }

    // 0x6B97C9 - Skip physics if the bike is found to have been static recently
    bool skipPhysics = false;
    if (!GetIsStuck() && (GetStatus() == STATUS_ABANDONED || GetStatus() == STATUS_WRECKED) && !bikeFlags.bGettingPickedUp) {
        bool makeStatic = false;
        if (!vehicleFlags.bVehicleColProcessed && m_vecMoveSpeed.x == 0.0f && m_vecMoveSpeed.y == 0.0f && m_vecMoveSpeed.z == 0.0f && m_aRatioHistory[3] != 1.0f) {
            makeStatic = true;
        }

        float moveSpeedLimit, turnSpeedLimit, distanceLimit;
        if (GetStatus() == STATUS_WRECKED) {
            moveSpeedLimit = 0.006f;
            turnSpeedLimit = 0.0015f;
            distanceLimit  = 0.015f;
        } else {
            moveSpeedLimit = 0.003f;
            turnSpeedLimit = 0.0009f;
            distanceLimit  = 0.005f;
        }

        // 0x6B988C
        m_vecForce  = (m_vecForce + m_vecMoveSpeed) / 2.0f;
        m_vecTorque = (m_vecTorque + m_vecTurnSpeed) / 2.0f;

        moveSpeedLimit *= CTimer::GetTimeStep();
        turnSpeedLimit *= CTimer::GetTimeStep();

        const bool isResting = sq(moveSpeedLimit) >= m_vecForce.SquaredMagnitude()
            && sq(turnSpeedLimit) >= m_vecTorque.SquaredMagnitude()
            && m_fMovingSpeed < distanceLimit;
        if (!isResting && !makeStatic) {
            m_nFakePhysics = 0; // 0x6B9A3D
        } else {
            // 0x6B9972
            m_nFakePhysics++;
            if (m_nFakePhysics > 10 || makeStatic) {
                if (!CCarCtrl::MapCouldMoveInThisArea(GetPosition().x, GetPosition().y)) {
                    if (!makeStatic || m_nFakePhysics > 10) {
                        m_nFakePhysics = 10;
                    }
                    m_vecMoveSpeed = CVector{ 0.0f, 0.0f, 0.0f };
                    m_vecTurnSpeed = CVector{ 0.0f, 0.0f, 0.0f };
                    skipPhysics    = true;
                }
            }
        }
    }

    // 0x6B9A44
    for (auto* const groundPhysical : m_aGroundPhysicalPtrs) {
        if (!groundPhysical) {
            continue;
        }
        vehicleFlags.bRestingOnPhysical = true;
        if (CWorld::bForceProcessControl && groundPhysical->GetIsInSafePosition()) { // NOTE: Not negated here, unlike in VC [and in `CAutomobile::ProcessControl`]
            SetWasPostponed(true); // 0x6B9BD6
            return;
        }
    }

    // 0x6B9A80
    if (vehicleFlags.bRestingOnPhysical) {
        skipPhysics    = false;
        m_nFakePhysics = 0;
    }

    // 0x6B9A95
    VehicleDamage(0.0f, eVehicleCollisionComponent::DEFAULT, nullptr, nullptr, nullptr, WEAPON_RAMMEDBYCAR);

    // 0x6B9AAB
    // When set the wheels' right vectors are levelled out (z = 0) for wheel processing
    bool bLevelWheelRight = false;
    if (m_fDamageIntensity > 0.0f
        && std::fabs(DotProduct(m_vecLastCollisionImpactVelocity, GetRight())) > 0.5f
        && m_vecMoveSpeed.SquaredMagnitude() < 0.1f) {
        bLevelWheelRight = true;
    } else if (bikeFlags.bGettingPickedUp) {
        bLevelWheelRight = true;
    }

    // 0x6B9B2D
    if (skipPhysics) {
        SkipPhysics();
        vehicleFlags.bVehicleColProcessed = false;
        vehicleFlags.bAudioChangingGear   = false;

        if (bikeFlags.bOnSideStand && m_RideAnimData.BarSteerAngle < DegreesToRadians(20.0f)) {
            m_RideAnimData.BarSteerAngle += CTimer::GetTimeStep() * DegreesToRadians(1.0f);
        }
        if (bikeFlags.bOnSideStand) { // 0x6B9B90
            const float f                   = static_cast<float>(std::pow(0.97, static_cast<double>(CTimer::GetTimeStep())));
            const float leanAngle           = std::asin(std::clamp(GetRight().z, -1.0f, 1.0f)) + DegreesToRadians(15.0f);
            m_RideAnimData.DesiredLeanAngle = f * m_RideAnimData.DesiredLeanAngle - leanAngle * (1.0f - f);
            m_RideAnimData.LeanAngle        = m_RideAnimData.DesiredLeanAngle;
        }
    } else {
        // 0x6B9C3A - This has to be done if ProcessEntityCollision wasn't called
        if (!vehicleFlags.bVehicleColProcessed) {
            ProcessControlCollisionCheck(true);
        }

        // 0x6B9C4C
        if (!(extraHandlingFlags & EXTRA_BALANCED_BY_RIDER) && !bikeFlags.bGettingPickedUp && !bikeFlags.bOnSideStand) {
            if (GetRight().z < 0.0f) {
                if (m_fSteerAngle > -DegreesToRadians(25.0f)) {
                    m_fSteerAngle -= CTimer::GetTimeStep() * DegreesToRadians(0.5f);
                }
            } else {
                if (m_fSteerAngle < DegreesToRadians(25.0f)) {
                    m_fSteerAngle += CTimer::GetTimeStep() * DegreesToRadians(0.5f);
                }
            }
        }

        // 0x6B9CBB - Lean forward speed up
        const float savedAirResistance = m_fAirResistance;
        if (GetStatus() == STATUS_PLAYER && m_pDriver) {
            const auto* const assoc = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_FWD);
            if (assoc && assoc->m_BlendAmount > 0.5f && assoc->m_CurrentTime > 0.06f && assoc->m_CurrentTime < 0.14f) {
                m_fAirResistance *= CCullZones::DoExtraAirResistanceForPlayer() ? 0.85f : 0.6f;
                if (m_GasPedal > 0.5f && DotProduct(m_vecMoveSpeed, GetForward()) > 0.25f) {
                    ApplyMoveForce(CTimer::GetTimeStep() * m_fMass * 0.0016f * GetForward()); // 0.2f * GRAVITY (0.008f)
                    bikeFlags.bPlayerBoost = true;
                }
            }
        }

        // 0x6B9DCE
        const bool wasSubmergedInWater = physicalFlags.bSubmergedInWater;
        CPhysical::ProcessControl();
        m_fAirResistance = savedAirResistance;

        ProcessBuoyancy();

        // 0x6B9DEF
        if (!wasSubmergedInWater && physicalFlags.bSubmergedInWater) {
            if (m_pDriver && m_pDriver->IsPlayer()) {
                m_pDriver->AsPlayer()->ResetPlayerBreath();
            } else {
                for (auto i = 0; i < m_nMaxPassengers; i++) {
                    if (m_apPassengers[i] && m_apPassengers[i]->IsPlayer()) {
                        m_apPassengers[i]->AsPlayer()->ResetPlayerBreath();
                    }
                }
            }
        }

        // 0x6B9E55 - Rescale spring ratios, i.e. subtract wheel radius
        for (auto i = 0; i < NUM_SUSP_LINES; i++) {
            const float wheelRadius = 1.0f - m_fSuspensionLength[i] / m_fLineLength[i];
            m_aWheelRatios[i]       = (m_aWheelRatios[i] - wheelRadius) / (1.0f - wheelRadius);
        }

        CVector contactPoints[NUM_SUSP_LINES];    // Relative to the bike's position
        CVector contactSpeeds[NUM_SUSP_LINES];    // Speed at contact points
        CVector springDirections[NUM_SUSP_LINES]; // Normalised
        float   wheelSpringForces[NUM_SUSP_LINES]{};

        // 0x6B9E86 - Get points and directions if spring is compressed
        for (auto i = 0; i < NUM_SUSP_LINES; i++) {
            if (m_aWheelRatios[i] < 1.0f) {
                contactPoints[i]    = m_aWheelColPoints[i].m_vecPoint - GetPosition();
                springDirections[i] = GetMatrix().TransformVector(colData->m_pLines[i].m_vecEnd - colData->m_pLines[i].m_vecStart);
                springDirections[i].Normalise();
            }
        }

        // 0x6B9F78
        m_aWheelSkidmarkType[0] = m_aWheelSkidmarkType[1] = eSkidmarkType::DEFAULT;
        m_bMoreSkidMarks[0] = m_bMoreSkidMarks[1] = false;

        // 0x6B9FC0 - Make springs push up vehicle
        for (auto i = 0; i < NUM_SUSP_LINES; i++) {
            if (m_aWheelRatios[i] < 1.0f) {
                float bias = m_pHandlingData->m_fSuspensionBiasBetweenFrontAndRear;
                if (i == 2 || i == 3) {
                    bias = 1.0f - bias;
                }

                auto& cp = m_aWheelColPoints[i];
                if (cp.m_vecNormal.z > 0.35f) {
                    ApplySpringCollisionAlt(
                        m_pHandlingData->m_fSuspensionForceLevel,
                        springDirections[i],
                        contactPoints[i],
                        m_aWheelRatios[i],
                        bias,
                        cp.m_vecNormal,
                        wheelSpringForces[i]
                    );
                } else {
                    ApplySpringCollision(
                        m_pHandlingData->m_fSuspensionForceLevel,
                        springDirections[i],
                        contactPoints[i],
                        m_aWheelRatios[i],
                        bias,
                        wheelSpringForces[i]
                    );
                }

                // 0x6BA061
                const auto wheel            = i >= 2 ? 1 : 0;
                m_aWheelSkidmarkType[wheel] = static_cast<eSkidmarkType>(g_surfaceInfos.GetSkidmarkType(cp.m_nSurfaceTypeB));
                if (m_aWheelSkidmarkType[wheel] == eSkidmarkType::MUDDY) {
                    m_bMoreSkidMarks[wheel] = true;
                }
            } else { // 0x6BA092
                contactPoints[i] = GetMatrix().TransformVector(colData->m_pLines[i].m_vecEnd);
            }
        }

        // 0x6BA0FC - Get speed at contact points
        for (auto i = 0; i < NUM_SUSP_LINES; i++) {
            contactSpeeds[i] = GetSpeed(contactPoints[i]);
            if (m_aGroundPhysicalPtrs[i]) {
                contactSpeeds[i] -= m_aGroundPhysicalPtrs[i]->GetSpeed(m_aGroundOffsets[i]);
            }
        }

        // 0x6BA1E0 - Use the ground normal as the spring direction where the ground is flat enough
        for (const auto [a, b] : { std::pair{ 0, 1 }, std::pair{ 2, 3 } }) {
            if (m_aWheelRatios[a] < 1.0f || m_aWheelRatios[b] < 1.0f) {
                CVector normal = m_aWheelRatios[a] < 1.0f
                    ? m_aWheelColPoints[a].m_vecNormal
                    : m_aWheelColPoints[b].m_vecNormal;
                if (normal.z > 0.35f) {
                    springDirections[a] = -normal;
                }

                normal = m_aWheelRatios[b] < 1.0f
                    ? m_aWheelColPoints[b].m_vecNormal
                    : m_aWheelColPoints[a].m_vecNormal;
                if (normal.z > 0.35f) {
                    springDirections[b] = -normal;
                }
            }
        }

        // 0x6BA49A - Spring damping
        for (auto i = 0; i < NUM_SUSP_LINES; i++) {
            if (m_aWheelRatios[i] < 1.0f) {
                ApplySpringDampening(
                    m_pHandlingData->m_fSuspensionDampingLevel,
                    wheelSpringForces[i],
                    springDirections[i],
                    contactPoints[i],
                    contactSpeeds[i]
                );
            }
        }

        // 0x6BA503 - Get speed at contact points again
        for (auto i = 0; i < NUM_SUSP_LINES; i++) {
            contactSpeeds[i] = GetSpeed(contactPoints[i]);
            if (m_aGroundPhysicalPtrs[i]) {
                contactSpeeds[i] -= m_aGroundPhysicalPtrs[i]->GetSpeed(m_aGroundOffsets[i]);
            }
        }

        // 0x6BA5E1
        float fwdSpeed     = DotProduct(m_vecMoveSpeed, GetForward());
        float acceleration = m_pHandlingData->GetTransmission().CalculateDriveAcceleration(
            m_GasPedal,
            m_nCurrentGear,
            m_fGearChangeCount,
            fwdSpeed,
            nullptr,
            nullptr,
            m_NumDriveWheelsOnGround,
            CCheat::IsActive(CHEAT_PERFECT_HANDLING)
        ) / m_fVelocityFrequency;
        float brake = m_pHandlingData->m_fBrakeDeceleration * m_BrakePedal * CTimer::GetTimeStep();

        // 0x6BA64E
        float brakeBiasFront, brakeBiasRear, tractionBiasFront, tractionBiasRear;
        if (GetStatus() != STATUS_PLAYER && GetStatus() != STATUS_REMOTE_CONTROLLED && m_pHandlingData->m_bNpcNeutralHandl) {
            brakeBiasFront    = 1.0f;
            brakeBiasRear     = 1.0f;
            tractionBiasFront = 1.0f;
            tractionBiasRear  = 1.0f;
        } else {
            brakeBiasFront    = 2.0f * m_pHandlingData->m_fBrakeBias;
            brakeBiasRear     = 2.0f * (1.0f - m_pHandlingData->m_fBrakeBias);
            tractionBiasFront = 2.0f * m_pHandlingData->m_fTractionBias;
            tractionBiasRear  = 2.0f - tractionBiasFront;
        }

        // 0x6BA6E2
        m_NumDriveWheelsOnGroundLastFrame = m_NumDriveWheelsOnGround;
        m_nNoOfContactWheels              = 0;
        m_NumDriveWheelsOnGround          = 0;

        // 0x6BA6FC - Count how many wheels are touching the ground
        for (auto i = 0; i < NUM_SUSP_LINES; i++) {
            if (m_aWheelRatios[i] < 1.0f) {
                m_WheelCounts[i] = 4.0f;
            } else {
                m_WheelCounts[i] = std::max(m_WheelCounts[i] - CTimer::GetTimeStep(), 0.0f);
                if (m_WheelCounts[i] <= 0.0f) {
                    continue;
                }
            }

            m_nNoOfContactWheels++;
            if (i == 2 || i == 3) {
                m_NumDriveWheelsOnGround = 1;
            }
            if (m_nNoOfContactWheels == 1) {
                m_vecAveGroundNormal = m_aWheelColPoints[i].m_vecNormal;
            } else {
                m_vecAveGroundNormal += m_aWheelColPoints[i].m_vecNormal;
            }
        }

        // 0x6BA7D0
        if (m_nNoOfContactWheels == 0) {
            m_vecAveGroundNormal = CVector{ 0.0f, 0.0f, 1.0f };
        } else {
            m_vecAveGroundNormal *= 1.0f / static_cast<float>(m_nNoOfContactWheels);
            if (DotProduct(GetUp(), m_vecAveGroundNormal) < -0.5f) {
                m_vecAveGroundNormal *= -1.0f;
            }
        }

        // 0x6BA8AE - Find contact points for wheel processing
        const auto frontLine = m_aWheelRatios[0] < m_aWheelRatios[1] ? 0 : 1;
        CVector    frontContact{
            0.0f,
            colData->m_pLines[0].m_vecStart.y,
            colData->m_pLines[0].m_vecStart.z - m_aWheelRatios[frontLine] * m_fSuspensionLength[0] - mi->m_fWheelSizeFront * 0.5f
        };
        frontContact = GetMatrix().TransformVector(frontContact);

        // 0x6BA926
        const auto rearLine = m_aWheelRatios[2] < m_aWheelRatios[3] ? 2 : 3;
        CVector    rearContact{
            0.0f,
            colData->m_pLines[3].m_vecStart.y, // NOTE: Line 3 for `y`, but line 2 for `z` [Same on Android]
            colData->m_pLines[2].m_vecStart.z - m_aWheelRatios[rearLine] * m_fSuspensionLength[2] - mi->m_fWheelSizeRear * 0.5f
        };
        rearContact = GetMatrix().TransformVector(rearContact);

        // 0x6BA9D6
        static auto& s_fTractionScale = StaticRef<float>(0xC1C818); // Initialised at runtime (0.004f on Android)
        const float  traction         = m_pHandlingData->m_fTractionMultiplier * m_fExtraTractionMult * s_fTractionScale * 0.25f;

        // 0x6BA9F7 - Turn the handlebars
        if (GetStatus() != STATUS_PLAYER && bikeFlags.bOnSideStand && !bikeFlags.bGettingPickedUp) {
            if (m_RideAnimData.BarSteerAngle < DegreesToRadians(20.0f)) {
                m_RideAnimData.BarSteerAngle += CTimer::GetTimeStep() * DegreesToRadians(1.5f);
            }
        } else if (std::fabs(m_vecMoveSpeed.x) < 0.01f && std::fabs(m_vecMoveSpeed.y) < 0.01f && m_fSteerAngle == 0.0f) { // 0x6BAA35
            m_RideAnimData.BarSteerAngle *= static_cast<float>(std::pow(0.96, static_cast<double>(CTimer::GetTimeStep())));
        } else { // 0x6BAA88
            float f = 1.0f;
            if (fwdSpeed > 0.01f && (m_WheelCounts[0] > 0.0f || m_WheelCounts[1] > 0.0f) && GetStatus() == STATUS_PLAYER) {
                CColPoint cp{};
                cp.m_nSurfaceTypeA = SURFACE_WHEELBASE;
                cp.m_nSurfaceTypeB = SURFACE_TARMAC;
                float steer        = g_surfaceInfos.GetAdhesiveLimit(&cp) * m_BikeHandling->m_fSpeedSteer * traction * 4.0f;

                const auto rearSurface = m_aWheelColPoints[rearLine].m_nSurfaceTypeB;
                if (g_surfaceInfos.GetAdhesionGroup(rearSurface) == ADHESION_GROUP_LOOSE || g_surfaceInfos.GetAdhesionGroup(rearSurface) == ADHESION_GROUP_SAND) {
                    steer *= m_BikeHandling->m_fSlipSteer;
                }

                // 0x6BAB55
                f = std::asin(std::min(steer / sq(fwdSpeed), 1.0f)) / DegreesToRadians(m_pHandlingData->m_fSteeringLock);
                if ((m_fSteerAngle < 0.0f && m_RideAnimData.LeanAngle < 0.0f) || (m_fSteerAngle > 0.0f && m_RideAnimData.LeanAngle > 0.0f)) {
                    f *= 2.0f;
                }
                f = std::min(f, 1.0f);
            }
            if (GetStatus() != STATUS_PLAYER) {
                f = 1.0f;
            }
            m_RideAnimData.BarSteerAngle = f * m_fSteerAngle;
        }

        // 0x6BAC22
        static auto& s_WheelStates = StaticRef<std::array<tWheelState, 2>>(0xC1C26C); // Function-static `WheelState[2]`
        static auto& s_fThrust     = StaticRef<float>(0xC1C27C);                      // Function-static `fThrust`

        const CVector initialMoveSpeed = m_vecMoveSpeed;
        const bool    rearWheelsFirst  = m_pHandlingData->m_bProcRearwheelFirst;

        // Gets the wheel's forward/right vectors, projected onto the ground it stands on
        const auto CalcWheelVectors = [&](CVector& wheelFwd, CVector& wheelRight, const CVector& groundNormal, bool levelRight) {
            wheelFwd -= groundNormal * DotProduct(wheelFwd, groundNormal);
            wheelFwd.Normalise();
            wheelRight = CrossProduct(wheelFwd, groundNormal);
            wheelRight.Normalise();
            if (levelRight) {
                wheelRight.z = 0.0f;
            }
        };

        // Speed of the bike at the given point, relative to the physical the given suspension line stands on (if any)
        const auto GetContactSpeed = [&](const CVector& contactPoint, int32 line) {
            CVector speed = GetSpeed(contactPoint);
            if (m_aGroundPhysicalPtrs[line]) {
                speed -= m_aGroundPhysicalPtrs[line]->GetSpeed(m_aGroundOffsets[line]);
            }
            return speed;
        };

        // Adhesion multiplier while the brakes destabilise the bike
        const auto GetAdhesionDestab = [&](eSurfaceType surface) {
            if (m_fBrakingSlide > 0.0f) {
                switch (g_surfaceInfos.GetAdhesionGroup(surface)) {
                case ADHESION_GROUP_HARD:
                case ADHESION_GROUP_LOOSE:
                    return 0.9f; // 0x8712F8
                case ADHESION_GROUP_ROAD:
                    return 0.7f; // 0x8712F4
                default:
                    break;
                }
            }
            return 1.0f;
        };

        // Common front wheel processing (Both for the first and the second try)
        const auto ProcessFrontWheelOnGround = [&](bool levelRight) {
            auto& cp = m_aWheelColPoints[frontLine];

            CVector wheelFwd = GetMatrix().TransformVector(CVector{ -std::sin(m_RideAnimData.BarSteerAngle), std::cos(m_RideAnimData.BarSteerAngle), 0.0f });
            CVector wheelRight;
            CalcWheelVectors(wheelFwd, wheelRight, cp.m_vecNormal, levelRight);

            // 0x6BAD91 / 0x6BB6AD
            s_fThrust                  = 0.0f;
            cp.m_nSurfaceTypeA         = SURFACE_WHEELBASE;
            float       adhesion       = g_surfaceInfos.GetAdhesiveLimit(&cp) * traction;
            const float adhesionDestab = GetAdhesionDestab(cp.m_nSurfaceTypeB);
            if (GetStatus() == STATUS_PLAYER) {
                adhesion *= g_surfaceInfos.GetWetMultiplier(cp.m_nSurfaceTypeB);
            }
            if (m_nWheelStatus[0] == WHEEL_STATUS_BURST) {
                adhesion *= 0.4f;
            }

            // 0x6BAE36 / 0x6BB752
            s_WheelStates[0]     = m_WheelStates[0];
            CVector contactSpeed = GetContactSpeed(frontContact, frontLine);
            ProcessBikeWheel(
                wheelFwd,
                wheelRight,
                contactSpeed,
                frontContact,
                2,
                s_fThrust,
                brake * brakeBiasFront,
                adhesion * tractionBiasFront,
                adhesionDestab,
                0,
                &m_aWheelAngularVelocity[0],
                &s_WheelStates[0],
                0,
                m_nWheelStatus[0]
            );
            if ((extraHandlingFlags & EXTRA_STUCK_IN_SAND) && (s_WheelStates[0] == WHEEL_STATE_SPINNING || s_WheelStates[0] == WHEEL_STATE_SKIDDING)) {
                s_WheelStates[0] = WHEEL_STATE_NORMAL;
            }
        };

        // 0x6BAC4B - Process front wheel - first try
        if (!rearWheelsFirst) {
            if (m_WheelCounts[0] > 0.0f || m_WheelCounts[1] > 0.0f) { // 0x6BAC9A - Wheel on ground
                ProcessFrontWheelOnGround(bLevelWheelRight);
            } else { // 0x6BAC77 - Wheel in the air
                m_aWheelAngularVelocity[0] *= 0.95f;
                m_aWheelPitchAngles[0] += m_aWheelAngularVelocity[0]; // NOTE: No timestep here, unlike in the second try
            }
        }

        // 0x6BAF62 - Process rear wheel
        if (m_WheelCounts[2] > 0.0f || m_WheelCounts[3] > 0.0f) { // 0x6BB038 - Wheel on ground
            auto& cp = m_aWheelColPoints[rearLine];

            float rearBrake    = brake;
            float rearTraction = traction;

            CVector wheelFwd   = GetForward();
            CVector wheelRight = GetRight();
            CalcWheelVectors(wheelFwd, wheelRight, cp.m_vecNormal, bLevelWheelRight);

            // 0x6BB11A
            // NOTE: The arguments of `ApplyTurnForce` are swapped in the original code (Same as in VC)
            if (vehicleFlags.bIsHandbrakeOn) {
                rearBrake    = 20000.0f;
                m_fTyreTemp  = 1.0f;
            } else if (m_nBrakesOn) { // 0x6BB148 - Doing a burnout
                rearBrake    = 0.0f;
                rearTraction = 0.0f;
                ApplyTurnForce(contactPoints[2], GetRight() * (m_fSteerAngle * m_fTurnMass * -0.0007f) * CTimer::GetTimeStep());
            } else if (m_fTyreTemp < 1.0f && m_GasPedal > 0.75f) { // 0x6BB223
                rearTraction *= m_fTyreTemp;
                ApplyTurnForce(contactPoints[2], GetRight() * ((1.0f - m_fTyreTemp) * m_fSteerAngle * m_fTurnMass * -0.0007f) * CTimer::GetTimeStep());
            }

            // 0x6BB2D9
            if (s_fThrust > 0.0f && brake > 0.0f) {
                brake = 0.0f; // Only affects the front wheel's second try
            }

            // 0x6BB305
            s_fThrust                  = acceleration;
            cp.m_nSurfaceTypeA         = SURFACE_WHEELBASE;
            float       adhesion       = g_surfaceInfos.GetAdhesiveLimit(&cp) * rearTraction;
            const float adhesionDestab = GetAdhesionDestab(cp.m_nSurfaceTypeB);
            if (GetStatus() == STATUS_PLAYER) {
                adhesion *= g_surfaceInfos.GetWetMultiplier(cp.m_nSurfaceTypeB);
            }
            if (m_nWheelStatus[1] == WHEEL_STATUS_BURST) {
                adhesion *= 0.4f;
            }

            // 0x6BB3B2
            s_WheelStates[1]     = m_WheelStates[1];
            CVector contactSpeed = GetContactSpeed(rearContact, rearLine);
            ProcessBikeWheel(
                wheelFwd,
                wheelRight,
                contactSpeed,
                rearContact,
                2,
                s_fThrust,
                rearBrake * brakeBiasRear,
                adhesion * tractionBiasRear,
                adhesionDestab,
                1,
                &m_aWheelAngularVelocity[1],
                &s_WheelStates[1],
                1,
                m_nWheelStatus[1]
            );
            if ((extraHandlingFlags & EXTRA_STUCK_IN_SAND) && (s_WheelStates[1] == WHEEL_STATE_SPINNING || s_WheelStates[1] == WHEEL_STATE_SKIDDING)) {
                s_WheelStates[1] = WHEEL_STATE_NORMAL;
            }
        } else { // 0x6BAF90 - Wheel in the air
            if (vehicleFlags.bIsHandbrakeOn) {
                m_aWheelAngularVelocity[1] = 0.0f;
            } else if (acceleration != 0.0f) {
                if (acceleration > 0.0f) {
                    if (m_aWheelAngularVelocity[1] < 1.0f) {
                        m_aWheelAngularVelocity[1] -= 0.1f;
                    }
                } else {
                    if (m_aWheelAngularVelocity[1] > -1.0f) {
                        m_aWheelAngularVelocity[1] += 0.05f;
                    }
                }
            }
            m_aWheelPitchAngles[1] += CTimer::GetTimeStep() * m_aWheelAngularVelocity[1];
        }

        // 0x6BB4E6
        if (m_nBrakesOn && m_WheelStates[1] == WHEEL_STATE_SPINNING) {
            m_fTyreTemp -= CTimer::GetTimeStep() * 0.002f;
            if (m_fTyreTemp < 0.0f) {
                m_fTyreTemp = 0.0f;
            }
        } else if (m_fTyreTemp < 1.0f) {
            m_fTyreTemp += CTimer::GetTimeStep() * 0.005f;
        }

        // 0x6BB566 - Process front wheel - second try
        if (rearWheelsFirst) {
            if (m_WheelCounts[0] > 0.0f || m_WheelCounts[1] > 0.0f) { // 0x6BB5C4 - Wheel on ground
                ProcessFrontWheelOnGround(false); // NOTE: The wheel's right vector is never levelled here
            } else { // 0x6BB59B - Wheel in the air
                m_aWheelAngularVelocity[0] *= 0.95f;
                m_aWheelPitchAngles[0] += m_aWheelAngularVelocity[0] * CTimer::GetTimeStep();
            }
        }

        // 0x6BB87A
        m_aGroundPhysicalPtrs.fill(nullptr);

        // 0x6BB894
        float riderLeanOffset = 0.0f;
        if (m_pDriver) {
            if (const auto* const assoc = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_STILL)) {
                riderLeanOffset = assoc->m_BlendAmount * DegreesToRadians(10.0f);
            }
        }

        // 0x6BB8C4
        if (bLevelWheelRight) {
            m_vecAveGroundNormal = CVector{ 0.0f, 0.0f, 1.0f };
            CVector right        = CrossProduct(GetForward(), m_vecAveGroundNormal);
            right.Normalise();
            m_vecAveGroundNormal = CrossProduct(right, GetForward());
            m_vecAveGroundNormal.Normalise();
        }

        // 0x6BB955 - Lean
        if ((extraHandlingFlags & EXTRA_BALANCED_BY_RIDER) || bikeFlags.bGettingPickedUp) { // 0x6BB9E9
            m_vecGroundRight = CrossProduct(GetForward(), m_vecAveGroundNormal);
            m_vecGroundRight.Normalise();

            float lateralAcc;
            if (m_pAttachedTo) {
                lateralAcc = 0.0f;
            } else if (m_nNoOfContactWheels == 0) { // 0x6BBA43
                lateralAcc = m_fSteerAngle / DegreesToRadians(m_pHandlingData->m_fSteeringLock) * CTimer::GetTimeStep() * -0.004f;
            } else {
                CVector speedDiff;
                if (physicalFlags.bDisableCollisionForce) { // 0x6BBA6F
                    speedDiff                = initialMoveSpeed - m_vecOldSpeedForPlayback;
                    m_vecOldSpeedForPlayback = initialMoveSpeed;
                } else {
                    speedDiff = m_vecMoveSpeed - initialMoveSpeed;
                }
                lateralAcc = DotProduct(speedDiff, m_vecGroundRight);
            }

            // 0x6BBAD6
            float lean = lateralAcc / (std::max(CTimer::GetTimeStep(), 0.01f) * 0.008f); // GRAVITY

            const float maxLean = m_nWheelStatus[0] == WHEEL_STATUS_BURST
                ? 0.4f * m_BikeHandling->m_fMaxLean
                : m_BikeHandling->m_fMaxLean;
            lean = std::clamp(lean, -maxLean, maxLean);

            // 0x6BBB62
            const float f                   = std::pow(m_BikeHandling->m_fDesLean, CTimer::GetTimeStep());
            m_RideAnimData.DesiredLeanAngle = (std::asin(lean) - riderLeanOffset) * (1.0f - f) + f * m_RideAnimData.DesiredLeanAngle;
        } else if (bikeFlags.bOnSideStand) { // 0x6BB96E
            const float f                   = static_cast<float>(std::pow(0.97, static_cast<double>(CTimer::GetTimeStep())));
            m_RideAnimData.DesiredLeanAngle = f * m_RideAnimData.DesiredLeanAngle - (std::asin(GetRight().z) + riderLeanOffset + DegreesToRadians(15.0f)) * (1.0f - f);
        } else { // 0x6BB9CD
            m_RideAnimData.DesiredLeanAngle *= static_cast<float>(std::pow(0.95, static_cast<double>(CTimer::GetTimeStep())));
        }
        m_RideAnimData.LeanAngle = m_RideAnimData.DesiredLeanAngle; // 0x6BBBA1

        // 0x6BBBB3
        m_WheelStates = s_WheelStates;

        // 0x6BBBD0
        if (m_GasPedal < 0.0f && m_WheelStates[1] == WHEEL_STATE_SPINNING) {
            m_WheelStates[1] = WHEEL_STATE_NORMAL;
        }

        // 0x6BBBF6
        if (GetStatus() == STATUS_PLAYER) {
            ProcessSirenAndHorn(true);
        } else if (m_HornCounter > 0) { // Inlined `ReduceHornCounter`
            m_HornCounter--;
        }
    }

    // 0x6BBC18 - Bike is on fire
    if (m_fHealth < 250.0f && GetStatus() != STATUS_WRECKED) {
        if (!IsSubBMX()) {
            auto* const matrix = GetModellingMatrix();
            if (!m_pFireParticle && matrix) {
                const CVector pos = CModelInfo::GetModelInfo(m_nModelIndex)->AsVehicleModelInfoPtr()->GetModelDummyPosition(DUMMY_ENGINE);
                m_pFireParticle   = g_fxMan.CreateFxSystem("fire_bike", pos, matrix, false);
                if (m_pFireParticle) {
                    m_pFireParticle->Play();
                    GetEventGlobalGroup()->Add(CEventVehicleOnFire{ this });
                }
            }
        }

        // 0x6BBD04 - Blow up after 5 seconds
        m_BlowUpTimer += static_cast<float>(static_cast<uint32>(CTimer::GetTimeStepInMS()));
        if (m_BlowUpTimer > 5000.0f) {
            BlowUpCar(m_Damager, false);
        }
    } else { // 0x6BBD5B
        m_BlowUpTimer = 0.0f;
        if (m_pFireParticle) {
            m_pFireParticle->Kill();
            m_pFireParticle = nullptr;
        }
    }

    // 0x6BBD78
    ProcessDelayedExplosion();

    // 0x6BBD7F - Find out how much to shake the pad depending on suspension and ground surface
    float       suspShake = 0.0f;
    float       surfShake = 0.0f;
    const float speedSq   = m_vecMoveSpeed.SquaredMagnitude();
    for (auto i = 0; i < NUM_SUSP_LINES; i++) {
        const float suspChange = m_aRatioHistory[i] - m_aWheelRatios[i];
        if (suspChange > 0.3f && (i == 0 || i == 2) && speedSq > 0.04f) {
            if (GetStatus() == STATUS_PLAYER || GetStatus() == STATUS_PHYSICS) {
                if (suspChange > suspShake) {
                    suspShake = suspChange;
                }
            }
        }

        // 0x6BBE1B
        if (m_aWheelRatios[i] < 1.0f && GetStatus() == STATUS_PLAYER) {
            const float roughness = static_cast<float>(static_cast<int32>(g_surfaceInfos.GetRoughness(m_aWheelColPoints[i].m_nSurfaceTypeB))) * 0.1f;
            if (surfShake <= roughness) {
                surfShake = roughness;
            }
        }

        // 0x6BBE65
        m_aRatioHistory[i] = m_aWheelRatios[i];
        m_aWheelRatios[i]  = 1.0f;
    }

    // 0x6BBE89
    if ((CTimer::GetTimeInMS() & 0x7FF) > 800) {
        if (surfShake >= 0.29f) {
            suspShake = 0.0f;
        }
        surfShake = 0.0f;
    }

    // 0x6BBEBE - Shake pad
    if ((suspShake > 0.0f || surfShake > 0.0f) && GetStatus() == STATUS_PLAYER) {
        float speed = m_vecMoveSpeed.SquaredMagnitude();
        if (speed > sq(0.1f)) {
            speed = std::sqrt(speed);
            if (suspShake > 0.0f) {
                const auto freq = static_cast<uint8>(std::min(speed / m_fMass * suspShake * 400000.0f + 100.0f, 250.0f));
                CPad::GetPad(0)->StartShake(static_cast<int16>(CTimer::GetTimeStep() * 20000.0f / static_cast<float>(freq)), freq, 0);
            } else { // 0x6BBF9D
                const auto freq = static_cast<uint8>(std::min(speed / m_fMass * surfShake * 400000.0f + 40.0f, 150.0f));
                CPad::GetPad(0)->StartShake(static_cast<int16>(CTimer::GetTimeStep() * 5000.0f / static_cast<float>(freq)), freq, 0);
            }
        }
    }

    // 0x6BC006
    vehicleFlags.bVehicleColProcessed = false;
    vehicleFlags.bAudioChangingGear   = false;

    if (!vehicleFlags.bWarnedPeds) {
        CCarCtrl::ScanForPedDanger(this);
    }

    // 0x6BC028
    if (physicalFlags.bDisableCollisionForce && physicalFlags.bCollidable) {
        m_vecMoveSpeed         = CVector{ 0.0f, 0.0f, 0.0f };
        m_vecTurnSpeed         = CVector{ 0.0f, 0.0f, 0.0f };
        m_vecFrictionMoveSpeed = CVector{ 0.0f, 0.0f, 0.0f };
        m_vecFrictionTurnSpeed = CVector{ 0.0f, 0.0f, 0.0f };
    } else if (!skipPhysics && (m_GasPedal == 0.0f || GetStatus() == STATUS_WRECKED)) { // 0x6BC0CF
        if (std::fabs(m_vecMoveSpeed.x) < 0.005f && std::fabs(m_vecMoveSpeed.y) < 0.005f && std::fabs(m_vecMoveSpeed.z) < 0.005f) {
            if (!(m_fDamageIntensity > 0.0f && m_pDamageEntity == FindPlayerPed())) {
                m_vecTurnSpeed.z = 0.0f;
                m_vecMoveSpeed   = CVector{ 0.0f, 0.0f, 0.0f };
            }
        }
    }

    // 0x6BC192 - Balance bike
    if (!(extraHandlingFlags & EXTRA_BALANCED_BY_RIDER) && !bikeFlags.bGettingPickedUp && !bikeFlags.bOnSideStand) {
        return;
    }

    static auto& s_fStoppieSteerMult = StaticRef<float>(0x8D3280); // 0.05f
    static auto& s_fWheelieLeanMult  = StaticRef<float>(0x8D3284); // -0.1f
    static auto& s_fWheelieMoveMult  = StaticRef<float>(0x8D3288); // 0.01f

    const float   onSideness = std::clamp(DotProduct(m_vecAveGroundNormal, GetRight()), -1.0f, 1.0f);
    const CVector worldCOM   = GetMatrix().TransformVector(m_vecCentreOfMass);

    // 0x6BC23C - Keep bike upright
    if (extraHandlingFlags & EXTRA_BALANCED_BY_RIDER) {
        ApplyTurnForce(GetUp() * (onSideness * m_fTurnMass * -0.07f) * CTimer::GetTimeStep(), worldCOM + GetRight());
        bikeFlags.bOnSideStand = false;
    } else { // 0x6BC2D1
        ApplyTurnForce(GetUp() * (onSideness * m_fTurnMass * -0.1f) * CTimer::GetTimeStep(), worldCOM + GetRight());
    }

    // 0x6BC34D - Wheelie/Stoppie stabilization
    if (GetStatus() != STATUS_PLAYER) {
        return;
    }

    if (m_WheelCounts[0] <= 0.0f && m_WheelCounts[1] <= 0.0f && GetForward().z > 0.0f && (m_WheelCounts[2] > 0.0f || m_WheelCounts[3] > 0.0f)) {
        // 0x6BC3C6 - Wheelie
        float wheelie = m_BikeHandling->m_fWheelieAng - GetForward().z;
        if (wheelie > 0.15f) { // Below wheelie angle
            wheelie = std::max(0.3f - wheelie, 0.0f);
        } else if (wheelie < -0.08f) { // Above wheelie angle
            wheelie = std::min(-0.14f - wheelie, 0.0f);
        }

        // 0x6BC420
        const float wheelieStab = wheelie * m_BikeHandling->m_fWheelieStabMult * std::min(m_vecMoveSpeed.Magnitude(), 0.1f);
        ApplyTurnForce(
            GetUp() * (CStats::GetFatAndMuscleModifier(STAT_MOD_12) * wheelieStab * m_fTurnMass * CTimer::GetTimeStep() * 0.5f),
            worldCOM + GetForward()
        );

        // 0x6BC515
        ApplyTurnForce(
            GetRight() * (m_BikeHandling->m_fWheelieSteer * m_RideAnimData.BarSteerAngle * m_fTurnMass * CTimer::GetTimeStep() * 0.5f),
            worldCOM + GetForward()
        );

        // 0x6BC5AB
        const float   speed = m_vecMoveSpeed.Magnitude();
        const CVector force = GetRight() * (speed * m_fMass * m_BikeHandling->m_fWheelieSteer * m_RideAnimData.BarSteerAngle * CTimer::GetTimeStep() * s_fWheelieMoveMult);
        ApplyMoveForce(force * m_vecMoveSpeed.Magnitude());

        // 0x6BC669
        m_RideAnimData.LeanAngle += CTimer::GetTimeStep() * m_RideAnimData.BarSteerAngle * s_fWheelieLeanMult;
    } else if (m_WheelCounts[2] <= 0.0f && m_WheelCounts[3] <= 0.0f && GetForward().z < 0.0f && (m_WheelCounts[0] > 0.0f || m_WheelCounts[1] > 0.0f)) {
        // 0x6BC6FB - Stoppie
        float stoppie = m_BikeHandling->m_fStoppieAng - GetForward().z;
        if (stoppie > 0.15f) { // Below stoppie angle
            stoppie = std::max(0.3f - stoppie, 0.0f);
        } else if (stoppie < -0.15f) { // Above stoppie angle
            stoppie = std::min(-0.3f - stoppie, 0.0f);
        }

        // 0x6BC755
        const float stoppieStab = stoppie * m_BikeHandling->m_fStoppieStabMult * std::min(m_vecMoveSpeed.Magnitude(), 0.1f);
        ApplyTurnForce(
            GetUp() * (CStats::GetFatAndMuscleModifier(STAT_MOD_12) * stoppieStab * m_fTurnMass * CTimer::GetTimeStep() * 0.5f),
            worldCOM + GetForward()
        );

        // 0x6BC82C
        const float sideForce = DotProduct(m_vecMoveSpeed, GetRight()) * m_fTurnMass * CTimer::GetTimeStep() * s_fStoppieSteerMult;
        CVector     flatFwd   = CrossProduct(CVector{ 0.0f, 0.0f, 1.0f }, GetRight());
        flatFwd.Normalise();
        ApplyTurnForce(GetRight() * -sideForce, -flatFwd);
    }
}

// 0x6B6740
void CBike::ResetSuspension() {
    for (auto i = 0; i < 2; i++) {
        m_aWheelPitchAngles[i] = 0.0f;
        m_WheelStates[i]       = WHEEL_STATE_NORMAL;
    }
    for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
        m_aWheelRatios[i] = 1.0f;
        m_WheelCounts[i]  = 0.0f;
    }
}

// 0x6B6790
bool CBike::GetAllWheelsOffGround() const {
    return m_nNoOfContactWheels == 0;
}

// 0x6B67A0
void CBike::DebugCode() {
    // NOP
}

// 0x6B6D40
void CBike::DoSoftGroundResistance(uint32& extraHandlingFlags) {
    // Any wheel on sand loses its grip
    for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
        if (m_aWheelRatios[i] < 1.0f && g_surfaceInfos.GetAdhesionGroup(m_aWheelColPoints[i].m_nSurfaceTypeB) == ADHESION_GROUP_SAND) {
            const auto up  = GetUp();
            const auto fwd = GetForward();

            auto vel       = m_vecMoveSpeed;
            vel -= up * vel.Dot(up); // Only the horizontal velocity is affected

            if (m_GasPedal > 0.3f) {
                if (vel.SquaredMagnitude() < sq(0.3f)) {
                    extraHandlingFlags += 4;
                }
                vel -= fwd * vel.Dot(fwd); // The wheels can't grip, so no forward push either
            }
            ApplyMoveForce(vel * -(CTimer::ms_fTimeStep * m_fMass * 0.02f));
            return;
        }
    }

    // Any wheel on rails
    for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
        if (m_aWheelRatios[i] < 1.0f && m_aWheelColPoints[i].m_nSurfaceTypeB == SURFACE_RAILTRACK) {
            const auto up  = GetUp();

            const auto vel = m_vecMoveSpeed - up * m_vecMoveSpeed.Dot(up);
            ApplyMoveForce(vel * -(CTimer::ms_fTimeStep * m_fMass * ms_fRailTrackResistance));
            return;
        }
    }
}

// 0x6B7130
void CBike::PlayHornIfNecessary() {
    if (m_autoPilot.carCtrlFlags.bHonkAtCar || m_autoPilot.carCtrlFlags.bHonkAtPed) {
        PlayCarHorn();
    }
}

// 0x6B7150
void CBike::CalculateLeanMatrix() {
    if (m_bLeanMatrixCalculated) {
        return;
    }

    CMatrix mat;
    mat.SetRotateX(fabs(m_RideAnimData.LeanAngle) * -0.05f);
    mat.RotateY(m_RideAnimData.LeanAngle);
    m_mLeanMatrix = GetMatrix();
    m_mLeanMatrix = m_mLeanMatrix * mat;
    // place wheel back on ground
    m_mLeanMatrix.GetPosition() += GetUp() * (1.0f - cos(m_RideAnimData.LeanAngle)) * GetColModel()->GetBoundingBox().m_vecMin.z;
    m_bLeanMatrixCalculated = true;
}

// 0x6B7F90
void CBike::FixHandsToBars(CPed* rider) {
    static auto& vecBmxHandleBarPos     = StaticRef<CVector>(0x8D3244);
    static auto& vecMtbHandleBarPos     = StaticRef<CVector>(0x8D3250);
    static auto& vecChopperHandleBarPos = StaticRef<CVector>(0x8D325C);
    static auto& vecTweakHandleBarPos2  = StaticRef<CVector>(0x8D3274);

    if (!m_nFixRightHand && !m_nFixLeftHand) {
        return;
    }

    const auto chassis = m_aBikeNodes[BIKE_CHASSIS];
    if (!chassis) {
        return;
    }

    CMatrix chassisMat{};
    chassisMat.Attach(RwFrameGetMatrix(chassis), false);

    CMatrix mat{};
    mat = *m_matrix;
    mat *= chassisMat;

    const auto hier     = GetAnimHierarchyFromSkinClump(rider->GetRpClump());
    const auto GetBonePos = [&](eBoneTag bone) -> RwV3d& {
        return RpHAnimHierarchyGetMatrixArray(hier)[RpHAnimIDGetIndex(hier, bone)].pos;
    };
    const auto MoveBone = [&](eBoneTag bone, const CVector& by) {
        auto& pos = GetBonePos(bone);
        pos.x += by.x;
        pos.y += by.y;
        pos.z += by.z;
    };

    CVector handPos = GetVehicleModelInfo()->GetModelDummyPosition(DUMMY_HAND_REST);
    if (handPos.x != 0.f || handPos.y != 0.f || handPos.z != 0.f) { // 0x6B8122
        CMatrix    handMat{ &RpHAnimHierarchyGetMatrixArray(hier)[RpHAnimIDGetIndex(hier, BONE_R_HAND)], false };
        const auto tweak = mat.InverseTransformVector(handMat.TransformVector(vecTweakHandleBarPos2));
        handPos          = tweak + handPos;
    } else {
        switch (GetModelIndex()) {
        case MODEL_MTBIKE: handPos = vecMtbHandleBarPos;     break;
        case MODEL_BIKE:   handPos = vecChopperHandleBarPos; break;
        default:           handPos = vecBmxHandleBarPos;     break;
        }
    }

    if (m_nFixRightHand) { // 0x6B81E0
        const auto  target = mat.TransformPoint(handPos);
        const auto& bone   = GetBonePos(BONE_R_HAND);
        const auto  delta  = CVector{ target.x - bone.x, target.y - bone.y, target.z - bone.z };

        MoveBone(BONE_R_HAND, delta);
        MoveBone(BONE_R_FINGER, delta);
        MoveBone(BONE_R_FINGER_01, delta);
        MoveBone(BONE_R_FORE_ARM, delta * 0.667f);
        MoveBone(BONE_R_UPPER_ARM, delta * 0.333f);
        if (rider->IsPlayer()) {
            MoveBone(BONE_R_BREAST, delta * 0.333f);
        }
    }

    if (m_nFixLeftHand) { // 0x6B8413
        handPos.x *= -1.f;

        const auto  target = mat.TransformPoint(handPos);
        const auto& bone   = GetBonePos(BONE_L_HAND);
        const auto  delta  = CVector{ target.x - bone.x, target.y - bone.y, target.z - bone.z };

        MoveBone(BONE_L_HAND, delta);
        MoveBone(BONE_L_FINGER, delta);
        MoveBone(BONE_L_FINGER_01, delta);
        MoveBone(BONE_L_FORE_ARM, delta * 0.75f);
        MoveBone(BONE_L_UPPER_ARM, delta * 0.4f);
        if (rider->IsPlayer()) {
            MoveBone(BONE_L_BREAST, delta * 0.4f);
        }
    }

    m_nFixLeftHand  = 0;
    m_nFixRightHand = 0;
}


// 0x6BF230
void CBike::GetCorrectedWorldDoorPosition(CVector& out, CVector arg1, CVector arg2) {
    const auto forward = GetForward();

    // Rebuild the bike's orientation from its forward vector
    const auto right       = forward.Cross(CVector{ 0.0f, 0.0f, 1.0f });
    const auto up          = right.Cross(forward);

    const auto rightUpSkew = right.Dot(GetUp());

    // The bounding box may stick out past the bounding sphere - Take that into account
    const auto cm       = GetColModel();
    const auto overhang = cm->m_boundSphere.m_fRadius < cm->m_boundBox.m_vecMin.x
        ? cm->m_boundBox.m_vecMin.x - cm->m_boundSphere.m_fRadius
        : 0.0f;

    out.Set(0.0f, 0.0f, 0.0f);
    out += forward * (arg2.y - arg1.y);
    out += right * (overhang * rightUpSkew + (arg2.x - arg1.x));
    out += up * (arg2.z - arg1.z);
    out += GetPosition();
}

// 0x6BEA10
void CBike::BlowUpCar(CEntity* damager, bool bHideExplosion) {
    if (!vehicleFlags.bCanBeDamaged) {
        return;
    }

    m_vecMoveSpeed.z += 0.13f;
    SetStatus(STATUS_WRECKED);
    physicalFlags.bRenderScorched = true;
    CVisibilityPlugins::SetClumpForAllAtomicsFlag(GetRpClump(), eAtomicComponentFlag::ATOMIC_PIPE_NO_EXTRA_PASSES);
    m_fHealth = 0.0f;
    m_wBombTimer = 0;

    TheCamera.CamShake(0.4f, GetPosition());
    KillPedsInVehicle();

    m_nOverrideLights = eVehicleOverrideLightsState::NO_CAR_LIGHT_OVERRIDE;
    vehicleFlags.bEngineOn = false;
    vehicleFlags.bLightsOn = false;
    ChangeLawEnforcerState(false);

    CExplosion::AddExplosion(this, damager, eExplosionType::EXPLOSION_CAR, GetPosition(), 0, 1, -1.0F, bHideExplosion);
    CDarkel::RegisterCarBlownUpByPlayer(*this, 0);
}

// 0x6B7050
void CBike::Fix() {
    vehicleFlags.bIsDamaged = false;
    bikeFlags.bEngineOnFire = false;
    m_nWheelStatus[0]       = 0;
    m_nWheelStatus[1]       = 0;
}

// 0x6BD090
void CBike::PreRender() {
    // Unknown statics, kept for fidelity
    static auto& s_SpeedToExhaustSpeedDiv = StaticRef<float>(0xC1C81C);
    static auto& s_SirenBrightness        = StaticRef<float>(0xB7C4E4);

    CVehicle::PreRender();

    const auto cm  = GetColModel();
    const auto cd  = cm->m_pColData;
    const auto mi  = GetVehicleModelInfo();

    if (vehicleFlags.bVehicleColProcessed) { // 0x6BD0D8
        DoBurstAndSoftGroundRatios();

        const auto UpdateSuspHeight = [&](int32 wheel, int32 line) {
            const auto frac = 1.f - m_fSuspensionLength[line] / m_fLineLength[line];
            const auto t    = (std::min(m_aWheelRatios[line + 1], m_aWheelRatios[line]) - frac) / (1.f - frac);
            auto       h    = cd->m_pLines[line].m_vecStart.z;
            if (t > 0.f) {
                h -= t * m_fSuspensionLength[line];
            }
            m_aWheelSuspensionHeights[wheel] += (h - m_aWheelSuspensionHeights[wheel]) * 0.75f;
        };
        UpdateSuspHeight(0, 0);
        UpdateSuspHeight(1, 2);
    }

    switch (GetStatus()) { // 0x6BD1E2 - Wheel particles
    case STATUS_PHYSICS:
    case STATUS_PLAYER:
    case STATUS_PLAYER_PLAYBACK_FROM_BUFFER:
    case STATUS_SIMPLE: {
        const auto bRearWheelFixed = m_WheelStates[1] == WHEEL_STATE_FIXED;
        const auto speed           = m_vecMoveSpeed.Magnitude();
        for (int32 i = 0; i < 2; i++) {
            int32 line;
            if (i == 0) {
                line = (!(m_aRatioHistory[0] < 1.f) && m_aRatioHistory[1] < 1.f) ? 1 : 0;
            } else {
                line = (!(m_aRatioHistory[3] < 1.f) && m_aRatioHistory[2] < 1.f) ? 2 : 3;
            }

            uint32 flags = (i == 0 && !bRearWheelFixed) ? 4 : 0;

            const auto leanOffset = std::sin(m_RideAnimData.LeanAngle) * GetColModel()->GetBoundingBox().m_vecMin.z * 0.8f;
            CVector    pos        = m_aWheelColPoints[line].m_vecPoint + GetRight() * leanOffset;

            if (m_bWheelBloody[i]) {
                flags += 1;
            }
            if (m_bMoreSkidMarks[i]) {
                flags += 2;
            }

            const auto dir = m_RideAnimData.LeanAngle <= 0.f ? 1.f : -1.f;

            AddSingleWheelParticles(
                m_WheelStates[i],
                m_nWheelStatus[i],
                m_aRatioHistory[line],
                speed,
                &m_aWheelColPoints[line],
                &pos,
                dir,
                i,
                static_cast<uint32>(m_aWheelSkidmarkType[i]),
                &m_bWheelBloody[i],
                flags
            );
        }
        break;
    }
    default:
        break;
    }

    m_bLeanMatrixCalculated = false;
    CalculateLeanMatrix();

    // 0x6BD3AD - Exhaust
    const auto fwdSpeed = DotProduct(GetForward(), m_vecMoveSpeed) / s_SpeedToExhaustSpeedDiv;
    if (vehicleFlags.bEngineOn && !m_pHandlingData->m_bNoExhaust) {
        if (fwdSpeed < 130.f && !vehicleFlags.bIsDrowning) {
            AddExhaustParticles();
        }
    }

    AddDamagedVehicleParticles();

    // 0x6BD40F - Cop bike siren
    if (GetModelIndex() == MODEL_COPBIKE && vehicleFlags.bSirenOrAlarm && vehicleFlags.bEngineOn && !physicalFlags.bRenderScorched) {
        const auto t          = CTimer::GetTimeInMS() & 0x1FF;
        const auto brightness = static_cast<uint8>(static_cast<int32>(s_SirenBrightness * 25.5f));

        CVector posBlue{ 0.28f, 0.6f, 0.3f };
        CVector posRed{ -0.28f, 0.6f, 0.3f };

        uint8 red, blue;
        if (t < 0x100) {
            red  = static_cast<uint8>(t);
            blue = static_cast<uint8>(0xFF - red);
            CCoronas::RegisterCorona(
                reinterpret_cast<uint32>(this) + 21,
                this,
                brightness, 0, 0, 255,
                posRed,
                0.4f, 40.f,
                CORONATYPE_SHINYSTAR, eCoronaFlareType{}, eCoronaReflType{}, eCoronaLOSCheck{}, eCoronaTrail{},
                0.f, false,
                1.5f, false,
                30.f, false, true
            );
        } else {
            red  = static_cast<uint8>(0u - static_cast<uint8>(t));
            blue = static_cast<uint8>(0xFF - red);
            CCoronas::RegisterCorona(
                reinterpret_cast<uint32>(this) + 22,
                this,
                0, 0, brightness, 255,
                posBlue,
                0.4f, 40.f,
                CORONATYPE_SHINYSTAR, eCoronaFlareType{}, eCoronaReflType{}, eCoronaLOSCheck{}, eCoronaTrail{},
                0.f, false,
                1.5f, false,
                30.f, false, true
            );
        }

        const auto& mat = *m_matrix;
        CPointLights::AddLight(
            PLTYPE_POINTLIGHT,
            GetPosition() + mat.GetForward() + mat.GetUp() * 0.5f,
            CVector{ 0.f, 0.f, 0.f },
            10.f,
            static_cast<float>(red) * (1.f / 1024.f),
            0.f,
            static_cast<float>(blue) * (1.f / 1024.f),
            0,
            true,
            nullptr
        );
    }

    DoVehicleLights(m_mLeanMatrix, VEHICLE_LIGHTS_IGNORE_DAMAGE);
    CShadows::StoreShadowForVehicle(this, VEH_SHD_BIKE);

    // 0x6BD674 - Wheel rotation
    const auto wheelFwd = m_matrix->TransformVector(CVector{ -std::sin(m_fSteerAngle), std::cos(m_fSteerAngle), 0.f });
    const auto fwd      = GetForward();

    if (m_WheelCounts[0] > 0.f || m_WheelCounts[1] > 0.f) { // 0x6BD719 - Front
        const auto lines = cd->m_pLines;
        const CVector wheelPos{
            0.f,
            (lines[1].m_vecStart.y + lines[0].m_vecStart.y) * 0.5f,
            lines[0].m_vecStart.z - std::min(m_aRatioHistory[0], m_aRatioHistory[1]) * m_fSuspensionLength[0] - mi->m_fWheelSizeFront * 0.5f
        };
        const auto wheelSpeed      = GetSpeed(wheelPos);
        m_aWheelAngularVelocity[0] = ProcessWheelRotation(WHEEL_STATE_NORMAL, wheelFwd, wheelSpeed, mi->m_fWheelSizeFront * 0.5f);
        m_aWheelPitchAngles[0] += m_aWheelAngularVelocity[0] * CTimer::GetTimeStep();
    }

    if (m_WheelCounts[2] > 0.f || m_WheelCounts[3] > 0.f) { // 0x6BD816 - Rear
        const auto lines = cd->m_pLines;
        const CVector wheelPos{
            0.f,
            (lines[3].m_vecStart.y + lines[2].m_vecStart.y) * 0.5f,
            lines[2].m_vecStart.z - std::min(m_aRatioHistory[2], m_aRatioHistory[3]) * m_fSuspensionLength[2] - mi->m_fWheelSizeFront * 0.5f // NOTE: Original uses the front wheel's size here
        };
        const auto wheelSpeed      = GetSpeed(wheelPos);
        m_aWheelAngularVelocity[1] = ProcessWheelRotation(m_WheelStates[1], fwd, wheelSpeed, mi->m_fWheelSizeRear * 0.5f);
        m_aWheelPitchAngles[1] += m_aWheelAngularVelocity[1] * CTimer::GetTimeStep();
    }

    CMatrix mat{};
    CVector pos{};

    // 0x6BD8EF - Front forks
    if (const auto forks = m_aBikeNodes[BIKE_FORKS_FRONT]) {
        mat.Attach(RwFrameGetMatrix(forks), false);
        pos = mat.GetPosition();

        RwMatrix rwRot{};
        CMatrix  rot{ &rwRot, false };
        rot.SetUnity();
        rot.UpdateRW();

        // Rotation matrix with the fork as the axis
        const auto casterAngle = DegreesToRadians(mi->m_fBikeSteerAngle);
        CVector    forkAxis{ 0.f, std::sin(casterAngle), -std::cos(casterAngle) };
        forkAxis.Normalise();

        CQuaternion q;
        q.Set(reinterpret_cast<RwV3d*>(&forkAxis), -m_RideAnimData.BarSteerAngle);
        q.Get(&rwRot);
        rot.Update();

        mat.SetUnity();
        mat = mat * rot;
        mat.GetPosition() += pos;
        mat.UpdateRW();

        if (const auto handlebars = m_aBikeNodes[BIKE_HANDLEBARS]) { // 0x6BDA66
            mat.Attach(RwFrameGetMatrix(handlebars), false);
            pos = mat.GetPosition();
            switch (GetStatus()) {
            case STATUS_ABANDONED:
            case STATUS_WRECKED: {
                mat.SetUnity();
                mat *= rot;
                mat.GetPosition() += pos;
                break;
            }
            default: {
                mat.SetTranslate(pos);
                break;
            }
            }
            mat.UpdateRW();
        }
    }

    // 0x6BDB03 - Rear forks
    if (const auto forks = m_aBikeNodes[BIKE_FORKS_REAR]) {
        const auto angle = -std::asin((m_aWheelSuspensionHeights[1] - m_aWheelOrigHeights[1]) / m_fSwingArmLength);
        mat.Attach(RwFrameGetMatrix(forks), false);
        pos = mat.GetPosition();
        mat.SetRotate(angle, 0.f, 0.f);
        mat.GetPosition() += pos;
        mat.UpdateRW();
    }

    // 0x6BDB99 - Front wheel
    mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BIKE_WHEEL_FRONT]), false);
    pos.x = mat.GetPosition().x;
    pos.z = m_aWheelSuspensionHeights[0] - m_fForkZOffset;
    pos.y = (cd->m_pLines[1].m_vecStart.y + cd->m_pLines[0].m_vecStart.y) * 0.5f
        - m_fForkYOffset
        - (m_aWheelSuspensionHeights[0] - m_aWheelOrigHeights[0]) * m_fSteerAngleTan;
    if (m_nWheelStatus[0] == WHEEL_STATUS_BURST) {
        mat.SetRotate(m_aWheelPitchAngles[0], 0.f, std::sin(m_aWheelPitchAngles[0]) * 0.05f);
    } else {
        mat.SetRotateX(m_aWheelPitchAngles[0]);
    }
    mat.GetPosition() += pos;
    mat.UpdateRW();

    // 0x6BDC6E - Mudguard
    mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BIKE_MUDGUARD]), false);
    mat.GetPosition() = pos;
    mat.UpdateRW();

    // 0x6BDCA3 - Rear wheel
    mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BIKE_WHEEL_REAR]), false);
    pos = mat.GetPosition();
    if (m_nWheelStatus[1] == WHEEL_STATUS_BURST) {
        mat.SetRotate(m_aWheelPitchAngles[1], 0.f, std::sin(m_aWheelPitchAngles[1]) * 0.07f);
    } else {
        mat.SetRotateX(m_aWheelPitchAngles[1]);
    }
    mat.GetPosition() += pos;
    mat.UpdateRW();

    // 0x6BDD3A - Chassis
    if (const auto chassis = m_aBikeNodes[BIKE_CHASSIS]) {
        mat.Attach(RwFrameGetMatrix(chassis), false);
        pos   = mat.GetPosition();
        pos.z = (1.f - std::cos(m_RideAnimData.LeanAngle)) * cm->GetBoundingBox().m_vecMin.z * 0.9f;
        mat.SetRotateX(std::fabs(m_RideAnimData.LeanAngle) * -0.05f);
        mat.RotateY(m_RideAnimData.LeanAngle);
        mat.GetPosition() += pos;
        mat.UpdateRW();
    }
}

// 0x6BEEB0
void CBike::PlaceOnRoadProperly() {
    const auto cm = GetColModel();
    const auto startX = cm->m_pColData->m_pLines[0].m_vecStart.x;
    const auto endX   = -cm->m_pColData->m_pLines[0].m_vecStart.y;

    const auto& pos = GetPosition();

    auto frontCheck = pos + GetForward() * startX;
    frontCheck.z    = pos.z;

    auto rearCheck = pos - GetForward() * endX;
    rearCheck.z    = pos.z;

    CColPoint colPoint{};
    CEntity* colEntity = nullptr;

    bool foundFront = false;
    float frontZ;
    if (CWorld::ProcessVerticalLine(frontCheck, frontCheck.z + 5.0f, colPoint, colEntity, true)) {
        foundFront = true;
        frontZ     = colPoint.m_vecPoint.z;
        m_pEntityWeAreOn = colEntity;
        m_bTunnel = colEntity->m_bTunnel;
        m_bTunnelTransition = colEntity->m_bTunnelTransition;
    }
    if (CWorld::ProcessVerticalLine(frontCheck, frontCheck.z - 5.0f, colPoint, colEntity, true)) {
        if (!foundFront || std::fabs(frontCheck.z - colPoint.m_vecPoint.z) < std::fabs(frontCheck.z - frontZ)) {
            m_pEntityWeAreOn = colEntity;
            m_bTunnel = colEntity->m_bTunnel;
            m_bTunnelTransition = colEntity->m_bTunnelTransition;
            frontZ    = colPoint.m_vecPoint.z;
            foundFront = true;
        }
        m_FrontCollPoly.ligthing = colPoint.m_nLightingB;
        frontCheck.z = frontZ;
    } else if (foundFront) {
        m_FrontCollPoly.ligthing = colPoint.m_nLightingB;
        frontCheck.z = frontZ;
    }

    bool foundRear = false;
    float rearZ;
    if (CWorld::ProcessVerticalLine(rearCheck, rearCheck.z + 5.0f, colPoint, colEntity, true)) {
        foundRear = true;
        rearZ     = colPoint.m_vecPoint.z;
        m_pEntityWeAreOn = colEntity;
        m_bTunnel = colEntity->m_bTunnel;
        m_bTunnelTransition = colEntity->m_bTunnelTransition;
    }
    if (CWorld::ProcessVerticalLine(rearCheck, rearCheck.z - 5.0f, colPoint, colEntity, true)) {
        if (!foundRear || std::fabs(rearCheck.z - colPoint.m_vecPoint.z) < std::fabs(rearCheck.z - rearZ)) {
            m_pEntityWeAreOn = colEntity;
            m_bTunnel = colEntity->m_bTunnel;
            m_bTunnelTransition = colEntity->m_bTunnelTransition;
            rearZ     = colPoint.m_vecPoint.z;
            foundRear = true;
        }
        m_RearCollPoly.ligthing = colPoint.m_nLightingB;
        rearCheck.z = rearZ;
    } else if (foundRear) {
        m_RearCollPoly.ligthing = colPoint.m_nLightingB;
        rearCheck.z = rearZ;
    }

    const auto length = endX + startX;
    const auto pitch  = std::atan2(frontZ - rearZ, length);
    const auto cosPitch = std::cos(pitch);

    GetRight().Set((frontCheck.y - rearCheck.y) / length, -((frontCheck.x - rearCheck.x) / length), 0.0f);
    GetForward().Set(-cosPitch * GetRight().y, cosPitch * GetRight().x, 0.0f);
    GetForward().z = std::sin(pitch);
    GetUp() = CrossProduct(GetRight(), GetForward());

    const CVector newPos = (frontCheck * endX + rearCheck * startX) * (1.0f / length);
    const auto height = GetHeightAboveRoad() + newPos.z;
    if (m_matrix) {
        m_matrix->GetPosition().Set(newPos.x, newPos.y, height);
    } else {
        m_placement.m_vPosn.Set(newPos.x, newPos.y, height);
    }
}

// 0x6BDE20
void CBike::Render() {
    auto savedRef = 0;
    RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &savedRef);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(1));

    m_nTimeTillWeNeedThisCar = CTimer::GetTimeInMS() + 3'000;
    CVehicle::Render();

    if (m_renderLights.m_bRightFront) {
        CalculateLeanMatrix();
        CVehicle::DoHeadLightBeam(eVehicleLightId::MAIN, m_mLeanMatrix, true);
    }

    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(savedRef));
}

// 0x6BCFC0
void CBike::Teleport(CVector destination, bool resetRotation) {
    CWorld::Remove(this);

    GetPosition() = destination;
    if (resetRotation) {
        SetOrientation(0.0f, 0.0f, 0.0f);
    }

    ResetMoveSpeed();
    ResetTurnSpeed();
    ResetSuspension();

    CWorld::Add(this);
}

// 0x6B8EC0
void CBike::VehicleDamage(float damageIntensity, eVehicleCollisionComponent component, CEntity* damager, CVector* vecCollisionCoors, CVector* vecCollisionDirection, eWeaponType weapon) {
    // inverted
    if (damageIntensity > 0.f || m_fDamageIntensity < 1.0f || !vehicleFlags.bCanBeDamaged)
        return;

    auto playerDamageIntensity = m_fDamageIntensity;

    if (GetStatus() == STATUS_PLAYER && CStats::GetPercentageProgress() >= 100.0f) {
        playerDamageIntensity *= 0.5f;
    }

    if (bikeFlags.bOnSideStand && playerDamageIntensity > 20.f) {
        bikeFlags.bOnSideStand = false;
    }

    DamageKnockOffRider(this, m_fDamageIntensity, m_nPieceType, m_pDamageEntity, m_vecLastCollisionPosn, m_vecLastCollisionImpactVelocity);

    if (m_pDamageEntity && m_pDamageEntity->GetType() == ENTITY_TYPE_VEHICLE) {
        m_nLastWeaponDamageType = eWeaponType::WEAPON_RAMMEDBYCAR;
        m_pLastDamageEntity = m_pDamageEntity;
        RegisterReference(m_pDamageEntity);
    }

    // inverted
    if (physicalFlags.bCollisionProof)
        return;

    // inverted
    if (m_pDamageEntity &&
        GetType() == eEntityType::ENTITY_TYPE_BUILDING &&
        DotProduct(m_vecLastCollisionImpactVelocity, m_matrix->GetUp()) > 0.6f)
        return;

    // inverted
    if (playerDamageIntensity <= 25.0 || GetStatus() == STATUS_WRECKED)
        return;

    if (!vehicleFlags.bIsLawEnforcer) {
        auto playerVehicle = FindPlayerVehicle();

        if (playerVehicle &&
            (CVehicle*)m_pDamageEntity == playerVehicle &&
            GetStatus() != STATUS_ABANDONED &&
            playerVehicle->m_vecMoveSpeed.Magnitude() >= m_vecMoveSpeed.Magnitude() &&
            playerVehicle->m_vecMoveSpeed.Magnitude() > 0.1) {
            FindPlayerPed()->SetWantedLevelNoDrop(eWantedLevel::WANTED_LEVEL_1);
        }
    }

    auto collisionDamageForce = (playerDamageIntensity - 25.0f) * m_pHandlingData->m_fCollisionDamageMultiplier;

    if (collisionDamageForce > 0.f) {
        auto damageVehicle = (CVehicle*)m_pDamageEntity;
        if (collisionDamageForce > 5.f &&
            m_pDriver &&
            m_pDamageEntity &&
            m_pDamageEntity->GetType() == ENTITY_TYPE_VEHICLE &&
            (FindPlayerVehicle() != this || damageVehicle->GetCreatedBy() != MISSION_VEHICLE) &&
            damageVehicle->m_pDriver) {
            m_pDriver->Say(CTX_GLOBAL_CRASH_BIKE, 0, 1.0, false, false, false);
        }

        auto vehicleDamageForce = collisionDamageForce;

        if (this == FindPlayerVehicle()) {
            if (vehicleFlags.bTakeLessDamage) {
                vehicleDamageForce *= 0.16f;
            } else {
                vehicleDamageForce *= 0.5f;
            }
        } else if (vehicleFlags.bTakeLessDamage) {
            vehicleDamageForce *= 0.083f;
        } else if (damageVehicle && damageVehicle == FindPlayerVehicle()) {
            vehicleDamageForce *= 0.6f;
        } else {
            vehicleDamageForce *= 0.25f;
        }

        auto currentHealth = m_fHealth;
        m_fHealth -= vehicleDamageForce;

        if (m_fHealth <= 1.0 && currentHealth > 1.0) {
            m_fHealth = 1.0f;
        }

        if (m_fHealth >= 250.f) {
            return;
        }

        if (!bikeFlags.bEngineOnFire) {
            bikeFlags.bEngineOnFire = true;
            m_BlowUpTimer = 0.f;
            m_Damager = m_pDamageEntity;

            if (m_pDamageEntity) {
                RegisterReference(m_pDamageEntity);
            }
        }
    }
}

// 0x6B89B0
void CBike::SetupSuspensionLines() {
    const auto mi = GetVehicleModelInfo();
    auto& cm      = *mi->GetColModel();
    auto& cd      = *cm.m_pColData;

    const auto& handling = *m_pHandlingData;

    const auto GetNodeWorldPos = [&](RwFrame* node, CVector& out) {
        RwMatrix matrix = *RwFrameGetMatrix(node);
        for (auto parent = RwFrameGetParent(node); parent && parent != m_aBikeNodes[BIKE_CHASSIS]; parent = RwFrameGetParent(parent)) {
            RwMatrixTransform(&matrix, RwFrameGetMatrix(parent), rwCOMBINEPOSTCONCAT);
        }
        out = *RwMatrixGetPos(&matrix);
    };

    const bool hasValidLines = cd.m_pLines[0].m_vecStart.x != 99999.99f && cd.m_pLines[0].m_vecStart.y != 99999.99f;
    for (auto i = 0; i < NUM_SUSP_LINES; i++) {
        auto& line = cd.m_pLines[i];

        CVector wheelPos;
        float height;
        if (!hasValidLines) {
            GetNodeWorldPos(i < 2 ? m_aBikeNodes[BIKE_WHEEL_FRONT] : m_aBikeNodes[BIKE_WHEEL_REAR], wheelPos);
            if (i == 0) {
                height = mi->m_fWheelSizeFront * 0.25f;
            } else if (i == 1) {
                height = mi->m_fWheelSizeFront * -0.25f;
            } else if (i == 2) {
                height = mi->m_fWheelSizeRear * 0.25f;
            } else {
                height = mi->m_fWheelSizeRear * -0.25f;
            }
        } else {
            wheelPos = i < 2 ? line.m_vecStart : line.m_vecEnd;
            height   = i < 2 ? m_fForkYOffset : m_fForkZOffset;
        }

        wheelPos.z += height;
        if (i == 0) {
            m_fForkYOffset = wheelPos.z;
        } else if (i == 2) {
            m_fForkZOffset = wheelPos.z;
            if (!m_aBikeNodes[BIKE_MISC_A]) {
                m_fSwingArmLength = 0.0f;
            } else {
                CVector miscPos;
                GetNodeWorldPos(m_aBikeNodes[BIKE_MISC_A], miscPos);
                const auto dx = wheelPos.x - miscPos.x;
                const auto dy = wheelPos.y - miscPos.y;
                m_fSwingArmLength = std::sqrt(dx * dx + dy * dy);
            }
        }
        wheelPos.z += handling.m_fSuspensionUpperLimit;

        line.m_vecStart = wheelPos;
        line.m_vecEnd   = CVector{ wheelPos.x, wheelPos.y, wheelPos.z + handling.m_fSuspensionLowerLimit - (i < 2 ? mi->m_fWheelSizeFront : mi->m_fWheelSizeRear) * 0.5f };

        m_fSuspensionLength[i] = handling.m_fSuspensionUpperLimit - handling.m_fSuspensionLowerLimit;
        m_fLineLength[i]       = line.m_vecStart.z - line.m_vecEnd.z;
    }

    if (!m_aBikeNodes[BIKE_MISC_A]) {
        CVector chassisPos;
        GetNodeWorldPos(m_aBikeNodes[BIKE_CHASSIS], chassisPos);
        m_fHeightAboveRoad   = chassisPos.z;
        m_fExtraTractionMult = chassisPos.z;
    }

    m_fHeightAboveRoad = mi->m_fWheelSizeFront * 0.5f - cd.m_pLines[0].m_vecStart.z
        + (1.0f - 1.0f / (handling.m_fSuspensionForceLevel * 4.0f)) * m_fSuspensionLength[0];

    for (auto i = 0; i < 2; i++) {
        m_aWheelSuspensionHeights[i] = (i == 0 ? mi->m_fWheelSizeFront : mi->m_fWheelSizeRear) * 0.5f - m_fHeightAboveRoad;
    }

    if (cd.m_pLines[0].m_vecEnd.z < cm.m_boundBox.m_vecMin.z) {
        cm.m_boundBox.m_vecMin.z = cd.m_pLines[0].m_vecEnd.z;
    }
    cm.m_boundSphere.m_fRadius = std::max({ cm.m_boundSphere.m_fRadius, cm.m_boundBox.m_vecMin.Magnitude(), cm.m_boundBox.m_vecMax.Magnitude() });

    if ((m_nHandlingFlagsIntValue & VEHICLE_HANDLING_STREET_RACER) && cd.m_pLines[0].m_vecStart.x == 99999.99f) {
        const auto clearance = 0.25f - m_fHeightAboveRoad;
        const auto numVerts  = *reinterpret_cast<const uint16*>(cd.m_pVertices);
        const auto verts     = reinterpret_cast<CVector*>(reinterpret_cast<uint16*>(cd.m_pVertices) + 8);
        for (auto i = 0; i < numVerts; i++) {
            if (verts[i].y - verts[i].z < clearance) {
                if (verts[i].z > 0.4f) {
                    verts[i].z = std::max(0.4f, verts[i].y - clearance);
                }
                verts[i].y = clearance + verts[i].z;
            }
        }
    }
}
// 0x6B8970
void CBike::SetModelIndex(uint32 index) {
    CVehicle::SetModelIndex(index);
    SetupModelNodes();
}

// 0x6B5960
void CBike::SetupModelNodes() {
    std::ranges::fill(m_aBikeNodes, nullptr);
    CClumpModelInfo::FillFrameArray(GetRpClump(), m_aBikeNodes.data());
}

// 0x6B7080
void CBike::PlayCarHorn() {
    if (m_nAlarmState && m_nAlarmState != -1 && GetStatus() != STATUS_WRECKED || m_HornCounter) {
        return;
    }

    if (m_nCarHornTimer) {
        m_nCarHornTimer -= 1;
        return;
    }

    m_nCarHornTimer = CGeneral::GetRandomNumber() % 128 - 106; // TODO: GetRandomNumberInRange
    const uint32 chance = m_nCarHornTimer % 8;

    if (chance < 4) {
        if (chance >= 2) {
            if (m_pDriver && m_autoPilot.carCtrlFlags.bHonkAtCar) {
                m_pDriver->Say(CTX_GLOBAL_BLOCKED);
            }
        }
        m_HornCounter = 45;
    } else {
        if (m_pDriver) {
            m_pDriver->Say(CTX_GLOBAL_BLOCKED);
        }
    }
}

// 0x6B7070
void CBike::SetupDamageAfterLoad() {
    // NOP
}

// 0x6B6950
void CBike::DoBurstAndSoftGroundRatios() {
    const auto mi = GetVehicleModelInfo();

    // Wheels that aren't burst/aren't on rails (Only these can sink into soft ground)
    std::array<bool, NUM_SUSP_LINES> wheelIntact;
    rng::fill(wheelIntact, true);

    const auto fwdSpeed = std::abs(m_vecMoveSpeed.Dot(GetForward()));

    for (auto i = 0u; i < 2u; i++) {
        const auto wheelA = 2 * i; // Wheels come in pairs (Front, Rear) that share a `m_nWheelStatus`
        const auto wheelB = 2 * i + 1;

        switch (m_nWheelStatus[i]) {
        case WHEEL_STATUS_MISSING:
            m_aWheelRatios[wheelA] = 1.0f;
            m_aWheelRatios[wheelB] = 1.0f;
            break;
        case WHEEL_STATUS_BURST: {
            // NOTE: The original generates these two values, but never uses them
            (void)(CGeneral::GetRandomNumber());
            (void)((float)CGeneral::GetRandomNumber() + 0x62);
            if (CGeneral::GetRandomNumber() < 100) { // The rim burrows itself into the ground a bit
                const auto sinkAmount  = (m_fLineLength[wheelA] - m_fSuspensionLength[wheelA]) / m_fLineLength[wheelA] * 0.2f;
                m_aWheelRatios[wheelA] = std::min(1.0f, m_aWheelRatios[wheelA] + sinkAmount);
                m_aWheelRatios[wheelB] = std::min(1.0f, m_aWheelRatios[wheelB] + sinkAmount);
            }
            break;
        }
        default:
            if ((m_aWheelRatios[wheelA] < 1.0f && m_aWheelColPoints[wheelA].m_nSurfaceTypeB == SURFACE_RAILTRACK)
                || (m_aWheelRatios[wheelB] < 1.0f && m_aWheelColPoints[wheelB].m_nSurfaceTypeB == SURFACE_RAILTRACK)) {
                // Stepping over a rail compresses the suspension
                const auto wheelSize       = 1.5f / (mi->m_fWheelSizeFront * 0.5f); // NOTE: Uses the front size for both pairs, same as the original
                const auto pitchAngleScale = [&] {
                    auto scale = wheelSize;
                    if (fwdSpeed > 0.3f) {
                        scale *= fwdSpeed / 0.3f;
                    }
                    return 1.0f / scale;
                }();
                const auto prevAngle = [&] {
                    const auto angle = m_aWheelPitchAngles[i] * pitchAngleScale;
                    return angle - std::floor(angle);
                }();
                const auto currAngle = [&] {
                    const auto angle = (CTimer::ms_fTimeStep * m_aWheelAngularVelocity[i] + m_aWheelPitchAngles[i]) * pitchAngleScale;
                    return angle - std::floor(angle);
                }();
                if (m_aWheelAngularVelocity[i] > 0.0f ? currAngle < prevAngle : prevAngle < currAngle) {
                    const auto compression = (m_fLineLength[wheelA] - m_fSuspensionLength[wheelA]) / m_fLineLength[wheelA] * 0.3f;
                    m_aWheelRatios[wheelA] = std::max(m_aWheelRatios[wheelA] - compression, 0.2f);
                    m_aWheelRatios[wheelB] = std::max(m_aWheelRatios[wheelB] - compression, 0.2f);
                }
            } else {
                continue;
            }
        }
        wheelIntact[wheelA] = false;
        wheelIntact[wheelB] = false;
    }

    // Sink the intact wheels into soft ground
    for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
        if (!wheelIntact[i]
            || m_aWheelRatios[i] >= 1.0f
            || g_surfaceInfos.GetAdhesionGroup(m_aWheelColPoints[i].m_nSurfaceTypeB) != ADHESION_GROUP_SAND
            || GetModelId() == MODEL_RHINO) {
            continue;
        }
        const auto sinkMult = m_nHandlingFlagsIntValue & VEHICLE_HANDLING_OFFROAD_ABILITY2 ? 0.1f
            : m_nHandlingFlagsIntValue & VEHICLE_HANDLING_OFFROAD_ABILITY                  ? 0.15f
                                                                                           : 0.25f;
        const auto sinkage  = std::max(0.4f, (1.0f - (fwdSpeed / 0.3f) * 0.7f) - CWeather::WetRoads * 0.7f);
        m_aWheelRatios[i]   = std::min(1.0f, ((m_fLineLength[i] - m_fSuspensionLength[i]) / m_fLineLength[i]) * sinkage * sinkMult + m_aWheelRatios[i]);
    }
}

// 0x6B67E0
bool CBike::SetUpWheelColModel(CColModel* wheelCol) {
    const auto mi               = GetVehicleModelInfo();
    const auto wcm              = GetColModel();
    const auto wcd              = wheelCol->m_pColData;

    wheelCol->m_boundBox        = wcm->m_boundBox;
    wheelCol->m_boundSphere     = wcm->m_boundSphere;

    const auto SetupWheelSphere = [&](CColSphere& sphere, eBikeNodes wheelNode, float wheelSize, uint8 pieceType) {
        // Get the wheel's position in world space by walking up the frame hierarchy (Up to the chassis)
        RwMatrix matrix = *RwFrameGetMatrix(m_aBikeNodes[wheelNode]);
        for (auto parent = RwFrameGetParent(m_aBikeNodes[wheelNode]); parent && parent != m_aBikeNodes[BIKE_CHASSIS]; parent = RwFrameGetParent(parent)) {
            RwMatrixTransform(&matrix, RwFrameGetMatrix(parent), rwCOMBINEPOSTCONCAT);
        }
        sphere.Set(wheelSize * 0.5f, CVector{ matrix.pos.x, matrix.pos.y, matrix.pos.z }, SURFACE_RUBBER, pieceType, tColLighting(0xFF));
    };

    SetupWheelSphere(wcd->m_pSpheres[0], BIKE_WHEEL_FRONT, mi->m_fWheelSizeFront, 0xD);
    SetupWheelSphere(wcd->m_pSpheres[1], BIKE_WHEEL_REAR, mi->m_fWheelSizeRear, 0xF);

    wcd->m_nNumSpheres = 2;
    return true;
}

// 0x6B67B0
void CBike::RemoveRefsToVehicle(CEntity* entityToRemove) {
    for (auto& entity : m_aGroundPhysicalPtrs) {
        if (entity == entityToRemove) {
            entity = nullptr;
        }
    }
}

// 0x6B6620
void CBike::ProcessControlCollisionCheck(bool applySpeed) {
    const CMatrix oldMat = GetMatrix();
    SetIsStuck(false);
    SkipPhysics();
    physicalFlags.bSkipLineCol     = false;
    physicalFlags.bProcessingShift = false;
    m_fMovingSpeed                 = 0.0f;
    rng::fill(m_aWheelRatios, 1.0f);

    if (applySpeed) {
        ApplyMoveSpeed();
        ApplyTurnSpeed();

        for (auto i = 0; CheckCollision() && i < 5; i++) {
            GetMatrix() = oldMat;
            ApplyMoveSpeed();
            ApplyTurnSpeed();
        }
    } else {
        const auto usesCollision = GetUsesCollision();
        SetUsesCollision(false);
        CheckCollision();
        SetUsesCollision(usesCollision);
    }

    SetIsStuck(false);
    SetIsInSafePosition(true);
}

// 0x6B5990
void CBike::GetComponentWorldPosition(int32 componentId, CVector& outPos) {
    if (IsComponentPresent(componentId)) {
        outPos = RwFrameGetLTM(m_aBikeNodes[componentId])->pos;
    } else {
        NOTSA_LOG_DEBUG("BikeNode missing: model={}, nodeIdx={}", m_nModelIndex, componentId);
    }
}

// 0x6B58D0
void CBike::ProcessOpenDoor(CPed* ped, uint32 doorComponentId, uint32 animGroup, uint32 animId, float fTime) {
    // NOP
}
