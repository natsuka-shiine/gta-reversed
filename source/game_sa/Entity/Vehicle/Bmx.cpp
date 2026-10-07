#include "StdInc.h"

#include "Bmx.h"
#include "CarAI.h"
#include "CarCtrl.h"
#include "Stats.h"
#include "Shadows.h"

void CBmx::InjectHooks() {
    RH_ScopedVirtualClass(CBmx, 0x871528, 67);
    RH_ScopedCategory("Vehicle");

    RH_ScopedInstall(Constructor, 0x6BF820);
    RH_ScopedVMTInstall(SetUpWheelColModel, 0x6BF9B0);
    RH_ScopedVMTInstall(BurstTyre, 0x6BF9C0);
    RH_ScopedVMTInstall(FindWheelWidth, 0x6C0550);
    RH_ScopedVMTInstall(ProcessControl, 0x6BFA30);
    RH_ScopedVMTInstall(ProcessDrivingAnims, 0x6BFB50);
    RH_ScopedVMTInstall(PreRender, 0x6C0810);
    RH_ScopedVMTInstall(ProcessAI, 0x6C1470);
    RH_ScopedInstall(ProcessBunnyHop, 0x6C0590);
    RH_ScopedInstall(LaunchBunnyHopCB, 0x6C0390);
}

// 0x6BF820
CBmx::CBmx(int32 modelIndex, eVehicleCreatedBy createdBy) :
    CBike(modelIndex, createdBy) 
{
    auto mi                     = CModelInfo::GetModelInfo(modelIndex);
    m_nVehicleSubType           = VEHICLE_TYPE_BMX;
    m_RideAnimData.AnimGroup = CAnimManager::GetAnimBlocks()[mi->GetAnimFileIndex()].GroupId;
    if (m_RideAnimData.AnimGroup < ANIM_GROUP_BMX || m_RideAnimData.AnimGroup > ANIM_GROUP_CHOPPA) {
        m_RideAnimData.AnimGroup = ANIM_GROUP_BMX;
    }

    m_fControlJump     = 0.0f;
    m_fControlPedaling = 0.0f;
    m_fSprintLeanAngle = 0.0f;
    m_fCrankAngle      = 0.0f;
    m_fPedalAngleL     = 0.0f;
    m_fPedalAngleR     = 0.0f;
    m_nFixLeftHand     = false;
    m_nFixRightHand    = false;
    m_bIsFreewheeling  = false;

    const auto Calc = [&](eBmxNodes node) -> float {
        RwMatrix matrix;
        RwFrame* wheelFront = m_aBikeNodes[node];
        matrix              = *RwFrameGetMatrix(wheelFront);

        auto parent = RwFrameGetParent(wheelFront);
        if (parent) {
            do {
                RwMatrixTransform(&matrix, RwFrameGetMatrix(parent), rwCOMBINEPOSTCONCAT);
                parent = RwFrameGetParent(parent);
            } while (parent != wheelFront && parent);
        }
        return matrix.pos.y;
    };
    auto wheelFrontPosY = Calc(BMX_WHEEL_FRONT);
    auto wheelRearPosY  = Calc(BMX_WHEEL_REAR);

    m_fMidWheelDistY = wheelFrontPosY - wheelRearPosY;
    m_fMidWheelFracY = wheelFrontPosY / m_fMidWheelDistY;
}

// 0x6BF9D0
CBmx::~CBmx() {
    m_vehicleAudio.Terminate();
}

// 0x6BF9B0
bool CBmx::SetUpWheelColModel(CColModel* wheelCol) {
    return false;
}

// 0x6BF9C0
bool CBmx::BurstTyre(uint8 tyreComponentId, bool bPhysicalEffect) {
    return false;
}

// 0x6BFA30
void CBmx::ProcessControl() {
    const float BMX_SPRINT_LEANSTART = FRAC_PI_2;
    const float BMX_PEDAL_LEANSTART  = 0.0f;
    const float BMX_SPRINT_LEANMULT  = 0.3f;
    const float MTB_SPRINT_LEANMULT  = 0.087f;
    const float BMX_PEDAL_LEANMULT   = 0.07f;
    const float MTB_PEDAL_LEANMULT   = 0.02f;

    CBike::ProcessControl();

    if (GetWasPostponed() || GetStatus() != STATUS_PLAYER || !m_pDriver) {
        return;
    }

    auto animBikeSprint = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_SPRINT);
    bool isMountainBike = GetModelId() == MODEL_MTBIKE;

    if (animBikeSprint && animBikeSprint->GetBlendAmount() > 0.01f) {
        float mult         = isMountainBike ? MTB_SPRINT_LEANMULT : BMX_SPRINT_LEANMULT;
        m_fSprintLeanAngle = std::sin(animBikeSprint->GetCurrentTime() / animBikeSprint->GetHier()->GetTotalTime() * TWO_PI + BMX_SPRINT_LEANSTART) * animBikeSprint->GetBlendAmount() * mult;
    } else {
        auto animBikePedal = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_PEDAL);
        if (animBikePedal && animBikePedal->GetBlendAmount() > 0.01f) {
            float mult = isMountainBike ? MTB_PEDAL_LEANMULT : BMX_PEDAL_LEANMULT;
            GetRideAnimData()->LeanAngle += std::sin(animBikePedal->GetCurrentTime() / animBikePedal->GetHier()->GetTotalTime() * TWO_PI + BMX_PEDAL_LEANSTART) * animBikePedal->GetBlendAmount() * mult;
        }
        m_fSprintLeanAngle *= 0.95f;
    }
}

// 0x6BFB50
void CBmx::ProcessDrivingAnims(CPed* driver, bool blend) {
    const float BMX_PEDAL_LEAN_LIMIT       = 0.4f;       // 0x8714E0
    const float BMX_SPRINT_LEAN_LIMIT      = 0.7f;       // 0x8714E4
    const float BMX_ANIM_LEAN_DAMP         = 0.95f;      // 0x8714E8
    const float BMX_PEDAL_MIN_FWD_SPEED    = 0.01f;      // 0x8714EC
    const float BMX_PEDAL_SPEED_MULT       = 3.0f;       // 0x8714F0
    const float BMX_SPRINT_ANIM_SPEED_MAX  = 2.5f;       // 0x8714F4
    const float MTB_PEDAL_SPEED_MULT       = 5.0f;       // 0x8714F8
    const float MTB_SPRINT_ANIM_SPEED_MAX  = 2.0f;       // 0x8714FC
    const float BMX_CRANK_ANGLE_START      = 0.0f;       // 0x871500
    const float BMX_CRANK_ANGLE_LEFT       = PI;         // 0x871504
    const float BMX_CRANK_ANGLE_RIGHT      = 0.0f;       // 0x871508
    const float BMX_CRANK_ANGLE_FWD        = FRAC_PI_2;  // 0x87150C
    const float BMX_CRANK_ANGLE_DAMP       = 0.97f;      // 0x871510

    if (m_bOffscreen && !(driver && driver->IsPlayer())) {
        return;
    }

    m_nFixLeftHand  = true;
    m_nFixRightHand = true;

    const auto SetPedalAngles = [this] {
        m_fPedalAngleL = -m_fCrankAngle;
        m_fPedalAngleR = -m_fCrankAngle;
    };

    if (const auto animBunnyHop = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_BUNNYHOP)) {
        m_fCrankAngle = (1.0f - animBunnyHop->m_BlendAmount) * m_fCrankAngle + BMX_CRANK_ANGLE_FWD * animBunnyHop->m_BlendAmount;
        SetPedalAngles();
        return;
    }

    if (driver->GetPlayerData()) {
        driver->SetMoveState(PEDMOVE_NONE);
    }

    const float fwdSpeed = m_vecMoveSpeed.Dot(m_matrix->GetForward());

    auto animPedal  = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_PEDAL);
    auto animSprint = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_SPRINT);
    auto animLeft   = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_LEFT);
    auto animRight  = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_RIGHT);
    auto animFwd    = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_FWD);

    auto animDriveBy = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_DRIVEBYLHS);
    if (!animDriveBy) {
        animDriveBy = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_DRIVEBYRHS);
    }
    if (!animDriveBy) {
        animDriveBy = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_DRIVEBYFT);
    }

    const auto GetCrankAngleFromAnim = [&](CAnimBlendAssociation* anim) {
        return BMX_CRANK_ANGLE_START - anim->m_CurrentTime / anim->GetHier()->GetTotalTime() * TWO_PI;
    };
    const auto NeedsBlendingIn = [](CAnimBlendAssociation* anim) {
        return !anim || (anim->m_BlendAmount < 1.0f && anim->m_BlendDelta <= 0.0f);
    };
    const auto IsBlendedIn = [](CAnimBlendAssociation* anim) {
        return anim && !(anim->m_BlendDelta < 0.0f) && anim->m_BlendAmount > 0.0f;
    };

    const float leanLimit = m_fControlPedaling > 5.0f ? BMX_SPRINT_LEAN_LIMIT : BMX_PEDAL_LEAN_LIMIT;

    if (std::fabs(m_RideAnimData.LeanAngle) < leanLimit
        && leanLimit > m_RideAnimData.LeanFwd
        && fwdSpeed > BMX_PEDAL_MIN_FWD_SPEED
        && !animDriveBy
    ) {
        // 0x6BFCFC - Pedalling / sprinting
        float animSpeed, sprintAnimSpeedMax;
        if (GetModelId() == MODEL_MTBIKE) {
            sprintAnimSpeedMax = MTB_SPRINT_ANIM_SPEED_MAX;
            animSpeed          = m_nCurrentGear >= 1
                ? MTB_PEDAL_SPEED_MULT * fwdSpeed / ((float)m_nCurrentGear * m_pHandlingData->m_transmissionData.m_MaxFlatVelocity - 0.25f)
                : 0.0f;
        } else {
            animSpeed          = BMX_PEDAL_SPEED_MULT * fwdSpeed;
            sprintAnimSpeedMax = BMX_SPRINT_ANIM_SPEED_MAX;
        }

        bool                   bJustBlended = false;
        CAnimBlendAssociation* animCrank    = animPedal;
        if (m_fControlPedaling > 5.0f && animSpeed < sprintAnimSpeedMax) { // 0x6BFD8B - Sprinting
            if (NeedsBlendingIn(animSprint)) {
                animSprint   = CAnimManager::BlendAnimation(driver->GetRpClump(), m_RideAnimData.AnimGroup, ANIM_ID_BIKE_SPRINT, 4.0f);
                bJustBlended = true;
            }
            animSprint->SetFlag(ANIMATION_IS_PLAYING, true);
            if (animPedal) {
                animPedal->SetFlag(ANIMATION_IS_PLAYING, true);
                animPedal->m_Speed = animSpeed;
            } else {
                animSprint->m_Speed = animSpeed;
                animCrank           = animSprint;
            }
        } else if (m_GasPedal != 0.0f || m_fControlPedaling > 0.0f || GetStatus() == STATUS_SIMPLE) { // 0x6BFEDE - Pedalling
            if (NeedsBlendingIn(animCrank)) {
                animCrank    = CAnimManager::BlendAnimation(driver->GetRpClump(), m_RideAnimData.AnimGroup, ANIM_ID_BIKE_PEDAL, 4.0f);
                bJustBlended = true;
            }
            animCrank->SetFlag(ANIMATION_IS_PLAYING, true);
            animCrank->m_Speed = animSpeed;
        } else { // 0x6BFE33 - Freewheeling
            if (NeedsBlendingIn(animCrank)) {
                animCrank    = CAnimManager::BlendAnimation(driver->GetRpClump(), m_RideAnimData.AnimGroup, ANIM_ID_BIKE_PEDAL, 4.0f);
                bJustBlended = true;
            }
            animCrank->SetFlag(ANIMATION_IS_PLAYING, false);
            if (!vehicleFlags.bIsHandbrakeOn && (m_aWheelRatios[0] < 1.0f || m_aWheelRatios[1] < 1.0f || m_aWheelRatios[2] < 1.0f || m_aWheelRatios[3] < 1.0f)) {
                m_bIsFreewheeling = true;
            }
        }

        // 0x6BFF2E
        if (!animCrank) {
            m_fCrankAngle = std::pow(BMX_CRANK_ANGLE_DAMP, CTimer::GetTimeStep()) * m_fCrankAngle;
        } else {
            // Start the freshly blended anim with the cranks matching the current pose
            float startAngle = -1000.0f;
            if (bJustBlended) {
                if (animLeft && animLeft->m_BlendAmount > 0.5f) {
                    startAngle = BMX_CRANK_ANGLE_LEFT;
                } else if (animRight && animRight->m_BlendAmount > 0.5f) {
                    startAngle = BMX_CRANK_ANGLE_RIGHT;
                } else if (animFwd && animFwd->m_BlendAmount > 0.5f) {
                    startAngle = BMX_CRANK_ANGLE_FWD;
                }
            }
            if (startAngle > -1000.0f) {
                float progress = (BMX_CRANK_ANGLE_START - startAngle) * (1.0f / TWO_PI);
                if (progress < 0.0f) {
                    progress += 1.0f;
                }
                animCrank->SetCurrentTime(animCrank->GetHier()->GetTotalTime() * progress);
                m_fCrankAngle = progress; // (sic)
            } else {
                m_fCrankAngle = GetCrankAngleFromAnim(animCrank);
            }
        }

        // 0x6C0033
        if (std::fabs(m_RideAnimData.AnimLeanLeft) > 0.05f || std::fabs(m_RideAnimData.AnimLeanFwd) > 0.05f) {
            m_RideAnimData.AnimLeanLeft *= BMX_ANIM_LEAN_DAMP;
            m_RideAnimData.AnimLeanFwd  *= BMX_ANIM_LEAN_DAMP;
        }
    } else {
        // 0x6C008C - Not pedalling
        if (IsBlendedIn(animPedal) || IsBlendedIn(animSprint)) {
            if (animPedal) {
                animPedal->SetFlag(ANIMATION_IS_PLAYING, false);
                animPedal->m_BlendDelta = -8.0f;
            }
            if (animSprint) {
                animSprint->SetFlag(ANIMATION_IS_PLAYING, false);
                animSprint->m_BlendDelta = -8.0f;
            }
            m_RideAnimData.AnimLeanLeft *= BMX_ANIM_LEAN_DAMP;
            m_RideAnimData.AnimLeanFwd  *= BMX_ANIM_LEAN_DAMP;
        } else {
            CBike::ProcessRiderAnims(driver, this, &m_RideAnimData, m_BikeHandling, 0);
        }

        // 0x6C0134
        if (animPedal) {
            m_fCrankAngle = GetCrankAngleFromAnim(animPedal);
        } else if (animSprint) {
            m_fCrankAngle = GetCrankAngleFromAnim(animSprint);
        } else {
            m_fCrankAngle = 0.0f;
        }

        // 0x6C0161
        if (animLeft && animLeft->m_BlendAmount > 0.1f) {
            m_fCrankAngle     = (1.0f - animLeft->m_BlendAmount) * m_fCrankAngle + BMX_CRANK_ANGLE_LEFT * animLeft->m_BlendAmount;
            m_bIsFreewheeling = true;
        } else if (animRight && animRight->m_BlendAmount > 0.1f) {
            m_fCrankAngle     = (1.0f - animRight->m_BlendAmount) * m_fCrankAngle + BMX_CRANK_ANGLE_RIGHT * animRight->m_BlendAmount;
            m_bIsFreewheeling = true;
        } else if (animFwd && animFwd->m_BlendAmount > 0.1f) {
            m_fCrankAngle = (1.0f - animFwd->m_BlendAmount) * m_fCrankAngle + BMX_CRANK_ANGLE_FWD * animFwd->m_BlendAmount;
        } else {
            m_fCrankAngle = std::pow(BMX_CRANK_ANGLE_DAMP, CTimer::GetTimeStep()) * m_fCrankAngle;
        }

        if (animDriveBy) {
            m_nFixRightHand   = false;
            m_bIsFreewheeling = true;
        }
    }

    // 0x6C0241
    if (driver->IsPlayer()) {
        bikeFlags.bWheelieForCamera = false;

        const float fwdZ = m_matrix->GetForward().z;
        if (!(m_WheelCounts[0] > 0.0f) && !(m_WheelCounts[1] > 0.0f) && fwdZ > 0.0f && (m_WheelCounts[2] > 0.0f || m_WheelCounts[3] > 0.0f)) {
            // Wheelie
            if (m_BikeHandling->m_fWheelieAng - fwdZ < m_BikeHandling->m_fWheelieAng * 0.5f) {
                bikeFlags.bWheelieForCamera = true;
            }
        } else if (!(m_WheelCounts[2] > 0.0f) && !(m_WheelCounts[3] > 0.0f) && fwdZ < 0.0f && (m_WheelCounts[0] > 0.0f || m_WheelCounts[1] > 0.0f)) {
            // Stoppie
            if (m_BikeHandling->m_fStoppieAng - fwdZ > m_BikeHandling->m_fStoppieAng * 0.6f) {
                bikeFlags.bWheelieForCamera = true;
            }
        }
    }

    SetPedalAngles();
}

// data is a ptr to CBmx
// 0x6C0390
void CBmx::LaunchBunnyHopCB(CAnimBlendAssociation* assoc, void* data) {
    auto bmx = static_cast<CBmx*>(data);
    if ((bmx->m_WheelCounts[0] > 0.0f || bmx->m_WheelCounts[1] > 0.0f) &&
        (bmx->m_WheelCounts[2] > 0.0f || bmx->m_WheelCounts[3] > 0.0f)
    ) {
        auto power = std::min(bmx->m_fControlJump / 25.0f, 1.0f) + 1.0f;
        if (bmx->GetStatus() == STATUS_PLAYER) {
            power *= CStats::GetFatAndMuscleModifier(STAT_MOD_6);
        }
        if (CCheat::IsActive(CHEAT_HUGE_BUNNY_HOP)) {
            power *= 5.0f;
        }
        bmx->ApplyMoveForce(0.06f * bmx->m_fMass * power * bmx->m_matrix->GetUp());
        bmx->ApplyTurnForce(0.01f * bmx->m_fTurnMass * power * bmx->m_matrix->GetUp(), bmx->m_matrix->GetForward());
    }
}

// 0x6C0500 | inlined | see 0x6C11F3
void CBmx::GetFrameOffset(float& fZOffset, float& fAngleOffset) {
    const auto d1 = m_aWheelSuspensionHeights[0] - m_aWheelOrigHeights[0];
    const auto d2 = m_aWheelSuspensionHeights[1] - m_aWheelOrigHeights[1];

    fZOffset     = (1.0f - m_fMidWheelFracY) * d1 + d2 * m_fMidWheelFracY;
    fAngleOffset = std::atan2(d1 - d2, m_fMidWheelDistY);
}

// 0x6C0550
float CBmx::FindWheelWidth(bool bRear) {
    return 0.07f;
}

// 0x6C0560
void CBmx::BlowUpCar(CEntity* damager, bool bHideExplosion) {
    // NOP
}

// 0x6C0590
void CBmx::ProcessBunnyHop() {
    auto* anim = m_pDriver
        ? RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_BUNNYHOP)
        : nullptr;

    if (GetStatus() != STATUS_PLAYER || !m_pDriver || !m_pDriver->IsPlayer()) {
        if (anim) {
            anim->SetFlag(ANIMATION_IS_PLAYING, true);
            anim->SetFlag(ANIMATION_IS_BLEND_AUTO_REMOVE, true);
            anim->SetBlendDelta(-8.0f);
        }
        return;
    }

    auto pad = m_pDriver->AsPlayer()->GetPadFromPlayer();

    if (pad->IsLeftShoulder1Pressed() && !pad->DisablePlayerControls && m_fControlJump == 0.0f) {
        m_fControlJump += CTimer::GetTimeStep();
        anim = CAnimManager::BlendAnimation(m_pDriver->GetRpClump(), m_RideAnimData.AnimGroup, ANIM_ID_BIKE_BUNNYHOP, 8.0f);
        if (anim) {
            anim->SetCurrentTime(0.0f);
            anim->SetFlag(ANIMATION_IS_PLAYING, false);
        }
    }

    if (m_fControlJump > 0.0f) {
        if (!pad->DisablePlayerControls) {
            if (!anim) {
                m_fControlJump = 0.0f;
                return;
            }

            if (pad->IsLeftShoulder1()) {
                if (!anim->IsPlaying()) {
                    m_fControlJump = std::min(m_fControlJump + CTimer::GetTimeStep(), 25.0f);
                    anim->SetCurrentTime(m_fControlJump / 25.0f * 0.2f);
                }
            } else if (!anim->IsPlaying()) {
                if (anim->GetCurrentTime() < 0.2f) {
                    anim->SetCurrentTime((0.2f - anim->GetCurrentTime()) / 0.2f * (anim->GetHier()->GetTotalTime() - 0.2f) + 0.2f);
                }
                anim->SetFlag(ANIMATION_IS_PLAYING, true);
                anim->SetSpeed(1.5f);
                anim->SetFinishCallback(CBmx::LaunchBunnyHopCB, this);
            }
        } else {
            m_fControlJump = 0.0f;
        }
    }

    if (anim) {
        if (anim->GetBlendAmount() > 0.5f) {
            m_GasPedal                                   = 0.0f;
            FindPlayerPed()->GetPlayerData()->m_fMoveSpeed = 0.0f;
            if (!vehicleFlags.bIsHandbrakeOn && (m_aWheelRatios[0] < 1.0f || m_aWheelRatios[1] < 1.0f || m_aWheelRatios[2] < 1.0f || m_aWheelRatios[3] < 1.0f)) {
                m_bIsFreewheeling = true;
            }
        }
    }
}

// 0x6C0810
void CBmx::PreRender() {
    // When set the wheels are positioned like `CBike` does (from the suspension), and the frame offset isn't applied.
    static auto& s_bUseBikeWheelPositioning = StaticRef<bool, 0xC1C83C>();

    const float BMX_WHEEL_RATIO_ON_GROUND = 1.0f;   // 0x8714BC
    const float BMX_LEAN_ROTATE_X_MULT    = -0.05f; // 0x8714C0

    CVehicle::PreRender();

    const auto cm = GetColModel();
    const auto cd = cm->m_pColData;

    if (vehicleFlags.bVehicleColProcessed) { // 0x6C0858
        DoBurstAndSoftGroundRatios();

        const auto UpdateSuspHeight = [&](int32 wheel, int32 line) {
            const auto frac = 1.0f - m_fSuspensionLength[line] / m_fLineLength[line];
            const auto t    = (std::min(m_aWheelRatios[line + 1], m_aWheelRatios[line]) - frac) / (1.0f - frac);
            auto       h    = cd->m_pLines[line].m_vecStart.z;
            if (t > 0.0f) {
                h -= t * m_fSuspensionLength[line];
            }
            m_aWheelSuspensionHeights[wheel] += (h - m_aWheelSuspensionHeights[wheel]) * 0.75f;
        };
        UpdateSuspHeight(0, 0);
        UpdateSuspHeight(1, 2);
    }

    switch (GetStatus()) { // 0x6C0962 - Wheel particles
    case STATUS_PHYSICS:
    case STATUS_PLAYER:
    case STATUS_PLAYER_PLAYBACK_FROM_BUFFER:
    case STATUS_SIMPLE: {
        const auto speed = m_vecMoveSpeed.Magnitude();
        for (int32 i = 0; i < 2; i++) {
            int32 line;
            if (i == 0) {
                line = (!(m_aRatioHistory[0] < BMX_WHEEL_RATIO_ON_GROUND) && m_aRatioHistory[1] < BMX_WHEEL_RATIO_ON_GROUND) ? 1 : 0;
            } else {
                line = (!(m_aRatioHistory[3] < BMX_WHEEL_RATIO_ON_GROUND) && m_aRatioHistory[2] < BMX_WHEEL_RATIO_ON_GROUND) ? 2 : 3;
            }

            uint32 flags = (i == 0 || m_WheelStates[1] == WHEEL_STATE_FIXED) ? 4 : 0;

            const auto leanOffset = std::sin(m_RideAnimData.LeanAngle) * GetColModel()->GetBoundingBox().m_vecMin.z * 0.8f;
            CVector    pos        = m_aWheelColPoints[line].m_vecPoint + m_matrix->GetRight() * leanOffset;

            if (m_bWheelBloody[i]) {
                flags += 1;
            }
            if (m_bMoreSkidMarks[i]) {
                flags += 2;
            }

            const auto dir = m_RideAnimData.LeanAngle > 0.0f ? -1.0f : 1.0f;

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

    // 0x6C0B10
    m_bLeanMatrixCalculated = false;
    CalculateLeanMatrix();

    CShadows::StoreShadowForVehicle(this, VEH_SHD_BIKE);

    CMatrix    mat{};
    const auto mi = GetVehicleModelInfo();

    // 0x6C0B30 - Wheel rotation
    const CVector wheelFwd = m_matrix->TransformVector(CVector{ -std::sin(m_fSteerAngle), std::cos(m_fSteerAngle), 0.0f });
    const CVector fwd      = m_matrix->GetForward();

    if (m_WheelCounts[0] > 0.0f || m_WheelCounts[1] > 0.0f) { // 0x6C0BD1 - Front
        const auto    lines = cd->m_pLines;
        const CVector wheelPos{
            0.0f,
            (lines[1].m_vecStart.y + lines[0].m_vecStart.y) * 0.5f,
            lines[0].m_vecStart.z - std::min(m_aRatioHistory[0], m_aRatioHistory[1]) * m_fSuspensionLength[0] - mi->m_fWheelSizeFront * 0.5f
        };
        const CVector wheelSpeed   = GetSpeed(wheelPos);
        m_aWheelAngularVelocity[0] = ProcessWheelRotation(WHEEL_STATE_NORMAL, wheelFwd, wheelSpeed, mi->m_fWheelSizeFront * 0.5f);
        m_aWheelPitchAngles[0] += m_aWheelAngularVelocity[0] * CTimer::GetTimeStep();
    }

    if (m_WheelCounts[2] > 0.0f || m_WheelCounts[3] > 0.0f) { // 0x6C0CCB - Rear
        const auto    lines = cd->m_pLines;
        const CVector wheelPos{
            0.0f,
            (lines[3].m_vecStart.y + lines[2].m_vecStart.y) * 0.5f,
            lines[2].m_vecStart.z - std::min(m_aRatioHistory[2], m_aRatioHistory[3]) * m_fSuspensionLength[2] - mi->m_fWheelSizeFront * 0.5f // NOTE: Original uses the front wheel's size here
        };
        const CVector wheelSpeed   = GetSpeed(wheelPos);
        m_aWheelAngularVelocity[1] = ProcessWheelRotation(m_WheelStates[1], fwd, wheelSpeed, mi->m_fWheelSizeRear * 0.5f);
        m_aWheelPitchAngles[1] += m_aWheelAngularVelocity[1] * CTimer::GetTimeStep();
    }

    CVector pos{};

    // 0x6C0DA1 - Front forks
    if (const auto forks = m_aBikeNodes[BMX_FORKS_FRONT]) {
        mat.Attach(RwFrameGetMatrix(forks), false);
        pos = mat.GetPosition();

        RwMatrix rwRot{};
        CMatrix  rot{ &rwRot, false };
        rot.SetUnity();
        rot.UpdateRW();

        // Rotation matrix with the fork as the axis
        const auto casterAngle = DegreesToRadians(mi->m_fBikeSteerAngle);
        CVector    forkAxis{ 0.0f, std::sin(casterAngle), -std::cos(casterAngle) };
        forkAxis.Normalise();

        CQuaternion q;
        q.Set(reinterpret_cast<RwV3d*>(&forkAxis), -m_RideAnimData.BarSteerAngle);
        q.Get(&rwRot);
        rot.Update();

        mat.SetUnity();
        mat = mat * rot;
        mat.GetPosition() += pos;
        mat.UpdateRW();

        if (const auto handlebars = m_aBikeNodes[BMX_HANDLEBARS]) { // 0x6C0EFB
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

    // 0x6C0FA6 - Rear forks
    if (const auto forks = m_aBikeNodes[BMX_FORKS_REAR]) {
        const auto angle = -std::asin((m_aWheelSuspensionHeights[1] - m_aWheelOrigHeights[1]) / m_fSwingArmLength);
        mat.Attach(RwFrameGetMatrix(forks), false);
        pos = mat.GetPosition();
        mat.SetRotate(angle, 0.0f, 0.0f);
        mat.GetPosition() += pos;
        mat.UpdateRW();
    }

    // 0x6C103C - Front wheel
    mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BMX_WHEEL_FRONT]), false);
    pos = mat.GetPosition();
    if (s_bUseBikeWheelPositioning) {
        pos.z = m_aWheelSuspensionHeights[0] - m_fForkZOffset;
        pos.y = (cd->m_pLines[1].m_vecStart.y + cd->m_pLines[0].m_vecStart.y) * 0.5f
            - m_fForkYOffset
            - (m_aWheelSuspensionHeights[0] - m_aWheelOrigHeights[0]) * m_fSteerAngleTan;
    }
    if (m_nWheelStatus[0] == WHEEL_STATUS_BURST) {
        mat.SetRotate(m_aWheelPitchAngles[0], 0.0f, std::sin(m_aWheelPitchAngles[0]) * 0.02f);
    } else {
        mat.SetRotateX(m_aWheelPitchAngles[0]);
    }
    mat.GetPosition() += pos;
    mat.UpdateRW();

    // 0x6C1119 - Rear wheel
    mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BMX_WHEEL_REAR]), false);
    pos = mat.GetPosition();
    if (s_bUseBikeWheelPositioning && !m_aBikeNodes[BMX_FORKS_REAR]) {
        pos.z = m_aWheelSuspensionHeights[1];
    }
    if (m_nWheelStatus[1] == WHEEL_STATUS_BURST) {
        mat.SetRotate(m_aWheelPitchAngles[1], 0.0f, std::sin(m_aWheelPitchAngles[1]) * 0.04f);
    } else {
        mat.SetRotateX(m_aWheelPitchAngles[1]);
    }
    mat.GetPosition() += pos;
    mat.UpdateRW();

    // 0x6C11CB - Chassis
    if (const auto chassis = m_aBikeNodes[BMX_CHASSIS]) {
        float zOffset = 0.0f, angleOffset = 0.0f;
        if (!s_bUseBikeWheelPositioning) {
            GetFrameOffset(zOffset, angleOffset); // 0x6C11F3 - Inlined
        }

        mat.Attach(RwFrameGetMatrix(chassis), false);
        pos   = mat.GetPosition();
        pos.z = (1.0f - std::cos(m_RideAnimData.LeanAngle)) * cm->GetBoundingBox().m_vecMin.z * 0.9f + zOffset;
        mat.SetRotateX(std::fabs(m_RideAnimData.LeanAngle) * BMX_LEAN_ROTATE_X_MULT + angleOffset);
        mat.RotateY(m_fSprintLeanAngle + m_RideAnimData.LeanAngle);
        mat.GetPosition() += pos;
        mat.UpdateRW();
    }

    // Rotates the given node around it's X axis, keeping it's position
    const auto SetNodeRotationX = [&](eBmxNodes node, float angle) {
        const auto frame = m_aBikeNodes[node];
        if (!frame) {
            return;
        }
        mat.Attach(RwFrameGetMatrix(frame), false);
        pos = mat.GetPosition();
        mat.SetRotate(angle, 0.0f, 0.0f);
        mat.GetPosition() += pos;
        mat.UpdateRW();
    };
    SetNodeRotationX(BMX_CHAINSET, m_fCrankAngle); // 0x6C12F2
    SetNodeRotationX(BMX_PEDAL_R, m_fPedalAngleL); // 0x6C1361 (sic)
    SetNodeRotationX(BMX_PEDAL_L, m_fPedalAngleR); // 0x6C13D0 (sic)
}

// 0x6C1470
bool CBmx::ProcessAI(uint32& extraHandlingFlags) {
    const float BMX_STEER_TURN_FORCE_MULT   = 0.002f; // 0x8714C4
    const float BMX_STEER_TURN_SPEED_LIMIT  = 0.04f;  // 0x8714C8
    const float BMX_SPRINT_FORCE_MULT       = 0.3f;   // 0x8714CC
    const float BMX_PEDALING_PLAYER_DECAY   = 0.4f;   // 0x8714D0
    const float BMX_SPRINT_BUTTON_THRESHOLD = 1.2f;   // 0x8714D4
    const float BMX_SPRINT_AI_DECAY         = 0.02f;  // 0x8714D8
    const float BMX_PEDALING_AI_DECAY       = 0.01f;  // 0x8714DC

    const auto mi = GetVehicleModelInfo();

    m_autoPilot.carCtrlFlags.bHonkAtCar = false;
    m_autoPilot.carCtrlFlags.bHonkAtPed = false;

    m_bIsFreewheeling = false;

    const auto ApplyBeingCarJackedControls = [this] {
        vehicleFlags.bIsHandbrakeOn = true;
        m_GasPedal                  = 0.0f;
        m_BrakePedal                = 1.0f;
    };

    switch (GetStatus()) {
    case STATUS_PLAYER: { // 0x6C14B2
        extraHandlingFlags += 2;
        bikeFlags.bGettingPickedUp = false;

        if (!m_pDriver || !m_pDriver->IsPlayer()) {
            break;
        }

        ProcessControlInputs((uint8)m_pDriver->m_nPedType);

        const auto pad     = m_pDriver->AsPlayer()->GetPadFromPlayer();
        const auto player  = m_pDriver->AsPlayer();
        const auto leanFwd = m_RideAnimData.LeanFwd;

        const auto ApplyLeanForce = [&](float force) {
            ApplyTurnForce(
                -(CTimer::GetTimeStep() * force) * m_matrix->GetUp(),
                m_vecCentreOfMass + m_matrix->GetForward()
            );
        };

        if (leanFwd < 0.0f) { // 0x6C151D - Leaning back
            m_vecCentreOfMass.y = m_BikeHandling->m_fLeanBakCOM * leanFwd + m_pHandlingData->m_vecCentreOfMass.y;
            if ((m_BrakePedal == 0.0f && !vehicleFlags.bIsHandbrakeOn) || !m_nNoOfContactWheels) {
                const auto speed     = std::min(0.1f, m_vecMoveSpeed.Magnitude());
                const auto pedalMult = GetModelId() == MODEL_SANCHEZ
                    ? m_GasPedal * 0.7f + 0.3f
                    : (m_GasPedal + 1.0f) * 0.5f;
                ApplyLeanForce(pedalMult * m_BikeHandling->m_fLeanBakForce * m_fTurnMass * leanFwd * speed);
            }
        } else { // 0x6C161F - Leaning forward
            m_vecCentreOfMass.y = m_BikeHandling->m_fLeanFwdCOM * leanFwd + m_pHandlingData->m_vecCentreOfMass.y;
            if (m_BrakePedal < 0.0f || !m_nNoOfContactWheels) {
                const auto speed = std::min(0.1f, m_vecMoveSpeed.Magnitude());
                ApplyLeanForce(m_BikeHandling->m_fLeanFwdForce * m_fTurnMass * speed * leanFwd);
            }
        }

        // 0x6C1704
        PruneReferences();

        if (GetStatus() == STATUS_PLAYER) {
            DoDriveByShootings();
        }

        DoSoftGroundResistance(extraHandlingFlags);

        // 0x6C1724 - Steering (spinning) while all wheels are in the air
        if (m_aWheelRatios[0] == 1.0f && m_aWheelRatios[1] == 1.0f && m_aWheelRatios[2] == 1.0f && m_aWheelRatios[3] == 1.0f) {
            const auto turnSpeedUp = DotProduct(m_vecTurnSpeed, m_matrix->GetUp());
            if ((turnSpeedUp < BMX_STEER_TURN_SPEED_LIMIT && (float)pad->GetSteeringLeftRight() < 0.0f)
                || (-BMX_STEER_TURN_SPEED_LIMIT < turnSpeedUp && (float)pad->GetSteeringLeftRight() > 0.0f)
            ) {
                const auto ts    = CTimer::GetTimeStep();
                const auto force = (float)pad->GetSteeringLeftRight() * (1.0f / 128.0f) * m_fTurnMass * ts * BMX_STEER_TURN_FORCE_MULT;
                ApplyTurnForce(force * m_matrix->GetRight(), m_matrix->GetForward());
            }
        }

        // 0x6C184E
        ProcessBunnyHop();

        const auto animSprint = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_SPRINT);
        if (player->ControlButtonSprint(SPRINT_BMX) > BMX_SPRINT_BUTTON_THRESHOLD
            || (player->GetButtonSprintResults(SPRINT_BMX) > 1.0f && animSprint && animSprint->m_BlendAmount > 0.5f)
        ) { // 0x6C19AD
            bikeFlags.bPlayerBoost = true;
            m_fControlPedaling     = player->GetPlayerData()->m_fMoveSpeed;
        } else { // 0x6C18B6
            player->HandleSprintEnergy(false, std::max(0.5f, 1.0f - (float)pad->GetAccelerate() * (1.0f / 255.0f) * 0.5f));
            if (player->GetButtonSprintResults(SPRINT_BMX) > 0.0f) {
                m_fControlPedaling = 4.9f;
                if (m_GasPedal == 0.0f && m_BrakePedal == 0.0f) {
                    m_GasPedal = 1.0f;
                }
            } else if (player->GetPlayerData()->m_fTimeCanRun < 0.0f) {
                if (pad->GetAccelerateJustDown()) {
                    m_fControlPedaling = 4.9f;
                } else {
                    m_fControlPedaling = std::max(0.0f, m_fControlPedaling - BMX_PEDALING_PLAYER_DECAY);
                }
            } else {
                m_fControlPedaling = 0.0f;
            }
        }

        // 0x6C19CE
        CStats::UpdateStatsWhenCycling(bikeFlags.bPlayerBoost, this);

        if (pad->CarGunJustDown()) {
            ActivateBomb();
        }
        break;
    }
    case STATUS_PLAYER_PLAYBACK_FROM_BUFFER: { // 0x6C1B65
        extraHandlingFlags += 2;
        break;
    }
    case STATUS_SIMPLE: { // 0x6C1B71
        CCarAI::UpdateCarAI(this);
        CPhysical::ProcessControl();
        CCarCtrl::UpdateCarOnRails(this);

        m_NumDriveWheelsOnGroundLastFrame = m_NumDriveWheelsOnGround;
        m_nNoOfContactWheels              = 2;
        m_NumDriveWheelsOnGround          = 2;

        m_pHandlingData->GetTransmission().CalculateGearForSimpleCar(m_autoPilot.m_speed * 0.02f, m_nCurrentGear);

        m_aWheelPitchAngles[0] += ProcessWheelRotation(WHEEL_STATE_NORMAL, m_matrix->GetForward(), m_vecMoveSpeed, mi->m_fWheelSizeFront * 0.5f);
        m_aWheelPitchAngles[1] += ProcessWheelRotation(WHEEL_STATE_NORMAL, m_matrix->GetForward(), m_vecMoveSpeed, mi->m_fWheelSizeRear * 0.5f);

        PlayHornIfNecessary();
        ReduceHornCounter();

        vehicleFlags.bVehicleColProcessed = false;
        vehicleFlags.bAudioChangingGear   = false;
        bikeFlags.bWheelieForCamera       = false;
        m_fControlPedaling                = 0.0f;
        break;
    }
    case STATUS_PHYSICS: { // 0x6C1C5A
        CCarAI::UpdateCarAI(this);
        CCarCtrl::SteerAICarWithPhysics(this);
        PlayHornIfNecessary();

        extraHandlingFlags += 2;
        bikeFlags.bWheelieForCamera = false;

        if (vehicleFlags.bIsBeingCarJacked) {
            ApplyBeingCarJackedControls();
        } else {
            bikeFlags.bGettingPickedUp = false;
        }

        // 0x6C1CBB
        if (m_fControlPedaling > 0.0f) {
            float decay;
            if (m_fControlPedaling > 5.0f) {
                decay = CTimer::GetTimeStep() * BMX_SPRINT_AI_DECAY;
            } else {
                decay = CTimer::GetTimeStep() * BMX_PEDALING_AI_DECAY;
                if (vehicleFlags.bUseCarCheats) {
                    decay += decay;
                }
            }
            m_fControlPedaling -= decay;
            if (m_fControlPedaling < 0.0f) {
                m_fControlPedaling = 0.0f;
            }
        }
        break;
    }
    case STATUS_ABANDONED: { // 0x6C1D33
        m_BrakePedal                = 0.0f;
        vehicleFlags.bIsHandbrakeOn = m_vecMoveSpeed.SquaredMagnitude() < 0.01f || bikeFlags.bOnSideStand;
        m_GasPedal                  = 0.0f;
        m_HornCounter               = 0;

        if ((m_pDriver || m_apPassengers[0] || vehicleFlags.bIsBeingCarJacked) && !bikeFlags.bOnSideStand) {
            extraHandlingFlags += 2;
        }

        m_RideAnimData.AnimLeanLeft = 0.0f;
        m_RideAnimData.AnimLeanFwd  = 0.0f;
        bikeFlags.bWheelieForCamera = false;
        m_fControlPedaling          = 0.0f;

        if (vehicleFlags.bIsBeingCarJacked) {
            ApplyBeingCarJackedControls();
        }
        break;
    }
    case STATUS_WRECKED: { // 0x6C1E87
        vehicleFlags.bIsHandbrakeOn = true;
        bikeFlags.bWheelieForCamera = false;
        m_BrakePedal                = 0.05f;
        m_fSteerAngle               = 0.0f;
        m_GasPedal                  = 0.0f;
        m_HornCounter               = 0;
        m_fControlPedaling          = 0.0f;
        m_RideAnimData.AnimLeanLeft = 0.0f;
        m_RideAnimData.AnimLeanFwd  = 0.0f;
        break;
    }
    case STATUS_FORCED_STOP: { // 0x6C1E0E
        if (m_vecMoveSpeed.SquaredMagnitude() < 0.01f) {
            m_BrakePedal                = 1.0f;
            vehicleFlags.bIsHandbrakeOn = true;
        } else {
            m_BrakePedal                = 0.0f;
            vehicleFlags.bIsHandbrakeOn = false;
        }
        m_fSteerAngle = 0.0f;
        m_GasPedal    = 0.0f;
        m_HornCounter = 0;

        extraHandlingFlags += 2;
        bikeFlags.bWheelieForCamera = false;
        m_fControlPedaling          = 0.0f;
        break;
    }
    default:
        break;
    }

    // 0x6C19FE - Rider is leaning or doing a drive-by => can't pedal
    if (m_pDriver) {
        const auto IsBlendedAtLeastHalf = [](CAnimBlendAssociation* anim) {
            return anim && anim->m_BlendAmount >= 0.5f;
        };

        auto animLean = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_FWD);
        if (!IsBlendedAtLeastHalf(animLean)) {
            animLean = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_BACK);
        }

        auto animDriveBy = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_DRIVEBYLHS);
        if (!IsBlendedAtLeastHalf(animDriveBy)) {
            animDriveBy = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_DRIVEBYRHS);
        }
        if (!IsBlendedAtLeastHalf(animDriveBy)) {
            animDriveBy = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_DRIVEBYFT);
        }

        if ((animLean && animLean->m_BlendAmount > 0.5f) || (animDriveBy && animDriveBy->m_BlendAmount > 0.5f)) {
            m_GasPedal = 0.0f;
            if (!vehicleFlags.bIsHandbrakeOn && (m_aWheelRatios[0] < 1.0f || m_aWheelRatios[1] < 1.0f || m_aWheelRatios[2] < 1.0f || m_aWheelRatios[3] < 1.0f)) {
                m_bIsFreewheeling = true;
            }
            return true;
        }
    }

    // 0x6C1ED7 - Sprinting: extra push while the rear wheel is on the ground
    if (m_fControlPedaling > 5.0f && (m_aWheelRatios[2] < 1.0f || m_aWheelRatios[3] < 1.0f)) {
        const auto fwdSpeed = m_vecMoveSpeed.Dot(m_matrix->GetForward());

        float mult = std::clamp(2.4f - fwdSpeed / m_pHandlingData->m_transmissionData.m_MaxFlatVelocity * 1.5f, 0.0f, 2.0f);
        if (GetStatus() == STATUS_PLAYER) {
            mult *= CStats::GetFatAndMuscleModifier(STAT_MOD_5);
        } else if (vehicleFlags.bUseCarCheats) {
            mult *= 1.25f;
        }

        ApplyMoveForce(CTimer::GetTimeStep() * m_fMass * BMX_SPRINT_FORCE_MULT * mult * 0.008f * m_matrix->GetForward());
    }

    return true;
}
