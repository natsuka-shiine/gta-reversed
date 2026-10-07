#include "StdInc.h"

#include "Plane.h"
#include "CarCtrl.h"
#include "FireManager.h"
#include "Shadows.h"
#include "MotionBlurStreaks.h"
#include "Events/EventDanger.h"

auto& HARRIER_NOZZLE_ROTATERATE = StaticRef<float, 0x8D33DC>();       // 25.0f
auto& PLANE_DAMAGE_WAVE_COUNTER_VAR = StaticRef<float, 0x8D33E0>();   // 0.75f
auto& PLANE_DAMAGE_THRESHHOLD = StaticRef<float, 0x8D33E4>();         // 500.0f
auto& PLANE_DAMAGE_SCALE_MASS = StaticRef<float, 0x8D33E8>();         // 10000.0f
auto& PLANE_DAMAGE_DESTROY_THRESHHOLD = StaticRef<float, 0x8D33EC>(); // 5000.0f
auto& vecRCBaronGunPos = StaticRef<CVector, 0x8D33F0>();            // <0.0f, 0.45f, 0.0f>
auto& PLANE_PANEL_SPRING = StaticRef<float, 0x8D33FC>();              // 0.03f
auto& PLANE_PANEL_DAMPING = StaticRef<float, 0x8D3400>();             // 0.98f
auto& VORTEX_SKIRT_MAX_SCALE = StaticRef<float, 0x8D3404>();          // 1.3f
auto& VORTEX_SKIRT_ROLL_LIMIT = StaticRef<float, 0x8D3408>();         // 0.1f
auto& VORTEX_SKIRT_PITCH_LIMIT = StaticRef<float, 0x8D340C>();        // 0.1f
auto& VORTEX_SKIRT_SMOOTHING = StaticRef<float, 0x8D3410>();          // 0.9f
auto& VORTEX_SKIRT_SPEED_LIMIT = StaticRef<float, 0x8D3414>();        // 0.3f
auto& VORTEX_SKIRT_SPEED_MULT = StaticRef<float, 0x8D3418>();         // 2.0f
auto& HARRIER_HOVER_THRUST_MULT = StaticRef<float, 0x8D341C>();       // 0.25f
auto& PLANE_DAMAGE_AILERON_PANEL_FEEDBACK = StaticRef<float, 0x8D3420>();  // 10.0f
auto& PLANE_DAMAGE_AILERON_PANEL_SHAKE = StaticRef<float, 0x8D3424>();     // 0.002f
auto& PLANE_DAMAGE_AILERON_CONTROL_SHAKE = StaticRef<float, 0x8D3428>();   // 0.05f
auto& PLANE_DAMAGE_ELEVATOR_PANEL_FEEDBACK = StaticRef<float, 0x8D342C>(); // 10.0f
auto& PLANE_DAMAGE_ELEVATOR_PANEL_SHAKE = StaticRef<float, 0x8D3430>();    // 0.002f
auto& PLANE_DAMAGE_ELEVATOR_CONTROL_SHAKE = StaticRef<float, 0x8D3434>();  // 0.05f
auto& PLANE_DAMAGE_RUDDER_PANEL_FEEDBACK = StaticRef<float, 0x8D3438>();   // 10.0f
auto& PLANE_DAMAGE_RUDDER_PANEL_SHAKE = StaticRef<float, 0x8D343C>();      // 0.002f
auto& PLANE_DAMAGE_RUDDER_CONTROL_SHAKE = StaticRef<float, 0x8D3440>();    // 0.05f
auto& PLANE_DAMAGE_PROP_THRUST_MULT = StaticRef<float, 0x8D3444>();        // 0.2f
auto& PLANE_DAMAGE_WAVE_PERIOD = StaticRef<uint32, 0x8D3448>();            // 2500
auto& PLANE_DAMAGE_PROP_THRUST_VAR = StaticRef<float, 0x8D344C>();         // 0.8f

void CPlane::InjectHooks() {
    RH_ScopedVirtualClass(CPlane, 0x871948, 71);
    RH_ScopedCategory("Vehicle");

    RH_ScopedInstall(Constructor, 0x6C8E20);
    RH_ScopedInstall(InitPlaneGenerationAndRemoval, 0x6CAD90);
    RH_ScopedVMTInstall(SetUpWheelColModel, 0x6C9140);
    RH_ScopedVMTInstall(BurstTyre, 0x6C9150);
    RH_ScopedVMTInstall(PreRender, 0x6C94A0);
    RH_ScopedVMTInstall(Render, 0x6CAB70);
    RH_ScopedInstall(IsAlreadyFlying, 0x6CAB90);
    RH_ScopedVMTInstall(Fix, 0x6CABB0);
    RH_ScopedVMTInstall(SetupDamageAfterLoad, 0x6CAC10);
    RH_ScopedInstall(SetGearUp, 0x6CAC20);
    RH_ScopedInstall(SetGearDown, 0x6CAC70);
    RH_ScopedVMTInstall(OpenDoor, 0x6CACB0);
    RH_ScopedVMTInstall(ProcessControl, 0x6C9260);
    RH_ScopedVMTInstall(ProcessControlInputs, 0x6CADD0);
    RH_ScopedVMTInstall(ProcessFlyingCarStuff, 0x6CB7C0);
    RH_ScopedVMTInstall(VehicleDamage, 0x6CC4B0);
    RH_ScopedInstall(CountPlanesAndHelis, 0x6CCA50);
    RH_ScopedInstall(AreWeInNoPlaneZone, 0x6CCAA0);
    RH_ScopedInstall(AreWeInNoBigPlaneZone, 0x6CCBB0);
    RH_ScopedInstall(SwitchAmbientPlanes, 0x6CCC50);
    RH_ScopedVMTInstall(BlowUpCar, 0x6CCCF0);
    RH_ScopedInstall(FindPlaneCreationCoors, 0x6CD090);
    RH_ScopedInstall(DoPlaneGenerationAndRemoval, 0x6CD2F0);
}

// 0x6C8E20
CPlane::CPlane(int32 modelIndex, eVehicleCreatedBy createdBy) : CAutomobile(modelIndex, createdBy, true) {
    m_nVehicleSubType = VEHICLE_TYPE_PLANE;

    m_fLeftRightSkid               = 0.0f;
    m_fSteeringUpDown              = 0.0f;
    m_fSteeringLeftRight           = 0.0f;
    m_fAccelerationBreakStatus     = 0.0f;
    m_fAccelerationBreakStatusPrev = 1.0f;
    m_fPropSpeed                   = 0.0f;
    field_9C8                      = 0.0f;
    m_LandingGearAngle           = 0.0f;
    m_StallCounter                = 0;
    m_planeCreationHeading         = 0.0f;
    m_planeHeading                 = 0.0f;
    m_planeHeadingPrev             = 0.0f;
    m_maxAltitude                  = 15.0f;
    m_altitude                     = 25.0f;
    m_minAltitude                  = 20.0f;
    m_forwardZ                     = 0;
    m_nStartedFlyingTime           = 0;
    m_fSteeringFactor              = 0.0f;

    if (m_nModelIndex != MODEL_VORTEX)
        physicalFlags.bDontCollideWithFlyers = true;

    m_nExtendedRemovalRange = 255;
    vehicleFlags.bNeverUseSmallerRemovalRange = true;
    vehicleFlags.bIsBig = true;

    auto& leftDoor = m_doors[DOOR_LEFT_FRONT];
    switch (modelIndex) {
    case MODEL_HYDRA:
    case MODEL_RUSTLER:
    case MODEL_CROPDUST:
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_OK);
        leftDoor.Init((3.0f * PI) / 5.0f, 0.0f, DOOR_AXIS_NEG_X, DOOR_AXIS_Y, DOOR_EXTRA_BASED);
        break;
    case MODEL_SHAMAL:
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_OK);
        leftDoor.Init(-((3.0f * PI) / 4.0f), 0.0f, DOOR_AXIS_Z, DOOR_AXIS_Y, DOOR_EXTRA_BASED);
        rwObjectSetFlags(GetFirstObject(m_aCarNodes[PLANE_WHEEL_LF]), 0);
        break;
    case MODEL_NEVADA:
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_OK);
        leftDoor.Init(-TWO_PI / 5.0f, 0.0f, DOOR_AXIS_NEG_Y, DOOR_AXIS_Z, DOOR_EXTRA_BASED);
        break;
    case MODEL_VORTEX:
        if (m_panels[FRONT_LEFT_PANEL].m_nFrameId == (uint16)-1)
            m_panels[FRONT_LEFT_PANEL].SetPanel(PLANE_GEAR_L, 1, -0.25f);
        break;
    case MODEL_STUNT:
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_OK);
        leftDoor.Init((3.0f * PI) / 5.0f, 0.0f, DOOR_AXIS_NEG_X, DOOR_AXIS_Y, DOOR_EXTRA_BASED);
        rwObjectSetFlags(GetFirstObject(m_aCarNodes[PLANE_WHEEL_LB]), 0);
        rwObjectSetFlags(GetFirstObject(m_aCarNodes[PLANE_WHEEL_RB]), 0);
        break;
    }

    CVector modelPos, localPos;
    for (auto wheelId = 0; wheelId < 4; wheelId++) {
        GetVehicleModelInfo()->GetWheelPosn(wheelId, modelPos, false);
        GetVehicleModelInfo()->GetWheelPosn(wheelId, localPos, true);
        m_wheelPosition[wheelId] = m_wheelPosition[wheelId] - modelPos.z + localPos.z;
    }

    m_planeDamageWave = 0;
    m_pGunParticles = nullptr;
    m_nFiringMultiplier = 16;
    field_9DC = 0;
    field_9E0 = 0;
    m_apJettrusParticles.fill(nullptr);

    m_pSmokeParticle = nullptr;

    if (m_nModelIndex == MODEL_HYDRA)
        m_wMiscComponentAngle = HARRIER_NOZZLE_ROTATE_LIMIT;

    m_bSmokeEjectorEnabled = false;
}

// 0x6C9160
CPlane::~CPlane() {
    if (m_pGunParticles) {
        for (auto i = 0; i < CVehicle::GetPlaneNumGuns(); i++) {
            if (auto& particle = m_pGunParticles[i]) {
                particle->Kill();
                g_fxMan.DestroyFxSystem(particle);
            }
        }
        delete[] m_pGunParticles;
        m_pGunParticles = nullptr;
    }

    for (auto particle : m_apJettrusParticles) {
        if (particle) {
            FxSystem_c::KillAndClear(particle);
        }
    }

    FxSystem_c::SafeKillAndClear(m_pSmokeParticle);

    m_vehicleAudio.Terminate();
}

// 0x6CAD90
void CPlane::InitPlaneGenerationAndRemoval() {
    GenPlane_Status = 0;
    GenPlane_LastTimeGenerated = 0;
    GenPlane_Active = true;
}

// 0x6CCCF0
void CPlane::BlowUpCar(CEntity* damager, bool bHideExplosion) {
    if (!vehicleFlags.bCanBeDamaged)
        return;

    // Check if this is a mission plane that should crash-and-burn instead of exploding immediately.
    // (The actual binary checks raw bytes here: `TEST [ESI+0x36],0xF8` etc, which don't map cleanly
    // onto the `m_nStatus` bitfield - matching the observable behavior instead. See disasm @ 0x6CCD03.)
    if (GetStatus() != STATUS_PLAYER && m_autoPilot.m_nCarMission != MISSION_PLANE_CRASH_AND_BURN && m_nModelIndex != MODEL_RCBARON) {
        m_autoPilot.SetCarMission(MISSION_PLANE_CRASH_AND_BURN);
        m_fHealth = 0.0f;
        return;
    }

    if (damager == FindPlayerPed() || damager == FindPlayerVehicle()) {
        FindPlayerInfo().m_nHavocCaused += 20;
        FindPlayerInfo().m_fCurrentChaseValue += 10.0f;
        CStats::IncrementStat(STAT_COST_OF_PROPERTY_DAMAGED, (float)CGeneral::GetRandomNumberInRange(4000, 10'000));
    }

    if (GetStatus() == STATUS_PLAYER) {
        if (m_pDriver) {
            m_pDriver->bDontRender = true;
        }
        for (auto& passenger : m_apPassengers) {
            if (passenger) {
                passenger->bDontRender = true;
            }
        }
        // NOTSA: assembly `AND [ESI+0x1C],0xFFFFFF7E` - m_nFlags @ 0x1C
        *(uint32*)((uint8*)this + 0x1C) &= 0xFFFFFF7E;
        ResetMoveSpeed();
        ResetTurnSpeed();
    }

    // NOTSA: inlined `SetStatus(STATUS_WRECKED)` - assembly:
    //   MOV DL,[ESI+0x36] ; AND DL,0x7 ; OR DL,0x28 ; MOV [ESI+0x36],DL
    *(uint8*)((uint8*)this + 0x36) = (*(uint8*)((uint8*)this + 0x36) & 7) | 0x28;
    physicalFlags.bRenderScorched = true;
    m_nTimeWhenBlowedUp = CTimer::GetTimeInMS();
    CVisibilityPlugins::SetClumpForAllAtomicsFlag(GetRpClump(), eAtomicComponentFlag::ATOMIC_PIPE_NO_EXTRA_PASSES_LOD);
    m_damageManager.FuckCarCompletely(false);
    if (m_nModelIndex != MODEL_RCBARON) {
        CAutomobile::SetBumperDamage(FRONT_BUMPER, false);
        CAutomobile::SetBumperDamage(REAR_BUMPER, false);
        CAutomobile::SetDoorDamage(DOOR_BONNET, false);
        CAutomobile::SetDoorDamage(DOOR_BOOT, false);
        CAutomobile::SetDoorDamage(DOOR_LEFT_FRONT, false);
        CAutomobile::SetDoorDamage(DOOR_RIGHT_FRONT, false);
        CAutomobile::SetDoorDamage(DOOR_LEFT_REAR, false);
        CAutomobile::SetDoorDamage(DOOR_RIGHT_REAR, false);
        CAutomobile::SpawnFlyingComponent(static_cast<eCarNodes>(PLANE_WHEEL_LF), 1);

        if (auto node = m_aCarNodes[PLANE_WHEEL_LF]) {
            RwObject* atomic = nullptr;
            RwFrameForAllObjects(node, GetCurrentAtomicObjectCB, &atomic);
            if (atomic) {
                rwObjectSetFlags(atomic, 0);
            }
        }
    }
    // NOTSA: raw writes matching the binary (`MOV [ESI+0x4DE],BX`, `AND [ESI+0x4A8],0xF8`) -
    // these offsets overlap bitfields with no clean named mapping, so match the bytes exactly.
    *(uint16*)((uint8*)this + 0x4DE) = 0;
    *(uint8*)((uint8*)this + 0x4A8) &= 0xF8;
    m_fHealth = 0.0f;
    m_wBombTimer = 0;

    TheCamera.CamShake(0.4f, GetPosition());
    KillPedsInVehicle();
    // NOTSA: more raw flag writes from the disasm (`AND [ESI+0x428],0xAF`, `AND [ESI+0x4A8],0xE7`,
    // `AND [ESI+0x42D],0x7F`, `AND [ESI+0x868],0xFE`) - no clean named fields, matching bytes.
    *(uint8*)((uint8*)this + 0x428) &= 0xAF;
    *(uint8*)((uint8*)this + 0x4A8) &= 0xE7;
    *(uint8*)((uint8*)this + 0x42D) &= 0x7F;
    *(uint8*)((uint8*)this + 0x868) &= 0xFE;
    m_bSmokeEjectorEnabled = false;

    if (vehicleFlags.bIsAmbulanceOnDuty) {
        vehicleFlags.bIsAmbulanceOnDuty = false;
        --CCarCtrl::NumAmbulancesOnDuty;
    }

    if (vehicleFlags.bIsFireTruckOnDuty) {
        vehicleFlags.bIsFireTruckOnDuty = false;
        --CCarCtrl::NumFireTrucksOnDuty;
    }

    ChangeLawEnforcerState(false);
    gFireManager.StartFire(this, damager, 0.8f, 1, 7000, 0);
    CDarkel::RegisterCarBlownUpByPlayer(*this, 0);
    if (m_nModelIndex == MODEL_RCBARON) {
        CExplosion::AddExplosion(this, damager, EXPLOSION_RC_VEHICLE, GetPosition(), 0, 1, -1.0f, 0);
    } else {
        CExplosion::AddExplosion(this, damager, EXPLOSION_AIRCRAFT, GetPosition(), 0, 1, -1.0f, 0);
    }
}

// 0x6CABB0
void CPlane::Fix() {
    m_damageManager.ResetDamageStatus();
    if (m_pHandlingData->m_bNoDoors) {
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_NOTPRESENT);
        m_damageManager.SetDoorStatus(DOOR_RIGHT_FRONT, DAMSTATE_NOTPRESENT);
        m_damageManager.SetDoorStatus(DOOR_LEFT_REAR, DAMSTATE_NOTPRESENT);
        m_damageManager.SetDoorStatus(DOOR_RIGHT_REAR, DAMSTATE_NOTPRESENT);
    }
    SetupDamageAfterLoad();
}

// 0x6CACB0
void CPlane::OpenDoor(CPed* ped, int32 componentId, eDoors door, float doorOpenRatio, bool playSound) {
    CAutomobile::OpenDoor(ped, componentId, door, doorOpenRatio, playSound);

    if (m_nModelIndex != MODEL_STUNT)
        return;

    // Unfinished code R*, which removed in Android
    if (false) // byte_C1CAFC
    {
        CMatrix matrix(RwFrameGetMatrix(m_aCarNodes[componentId]), false);
        const auto y = m_doors[door].m_angle - m_doors[door].m_prevAngle + matrix.GetPosition().y;
        matrix.SetTranslate({matrix.GetPosition().x, y, matrix.GetPosition().z});
        matrix.UpdateRW();
    }
}

// 0x6CAC10
void CPlane::SetupDamageAfterLoad() {
    vehicleFlags.bIsDamaged = false;
}

// 0x6CC4B0
void CPlane::VehicleDamage(float damageIntensity, eVehicleCollisionComponent component, CEntity* damager, CVector* vecCollisionCoors, CVector* vecCollisionDirection, eWeaponType weapon) {
    if (m_nModelIndex == MODEL_VORTEX) {
        CAutomobile::VehicleDamage(damageIntensity, component, damager, vecCollisionCoors, vecCollisionDirection, weapon);
        return;
    }

    if (!vehicleFlags.bCanBeDamaged) {
        return;
    }

    if (GetStatus() != STATUS_PLAYER && physicalFlags.bInvulnerable) {
        if (m_pDamageEntity != FindPlayerPed() && m_pDamageEntity != FindPlayerVehicle()) {
            return;
        }
    }

    auto damageThreshold  = PLANE_DAMAGE_THRESHHOLD;
    auto destroyThreshold = PLANE_DAMAGE_DESTROY_THRESHHOLD;
    auto particleForceMult = 0.333f;
    auto isWeaponDamage    = false;

    if (damageIntensity != 0.0f) { // Called directly (Not by collision)
        bool damagedDueToFireOrExplosionOrBullet = false;
        if (!CanVehicleBeDamaged(damager, weapon, damagedDueToFireOrExplosionOrBullet)) {
            return;
        }
        switch (weapon) {
        case WEAPON_ROCKET:
        case WEAPON_ROCKET_HS:
        case WEAPON_FREEFALL_BOMB:
        case WEAPON_EXPLOSION:
            break;
        default:
            return;
        }
        isWeaponDamage  = true;
        damageIntensity = 0.0f;
    } else { // Collision
        const auto massRatio = m_fMass / PLANE_DAMAGE_SCALE_MASS;

        damageIntensity   = m_fDamageIntensity;
        particleForceMult = 1.0f;
        damageThreshold  *= std::max(massRatio, 1.0f);
        destroyThreshold *= massRatio;
        vecCollisionCoors = &m_vecLastCollisionPosn;

        if (!(damageIntensity > 0.0f)) {
            return;
        }
        if (physicalFlags.bCollisionProof) {
            return;
        }
        if (m_pDamageEntity && m_pDamageEntity->GetIsTypePed()) {
            return;
        }
    }

    // 0x6CC63C
    auto       healthLoss = damageIntensity * m_pHandlingData->m_fCollisionDamageMultiplier;
    const auto prevHealth = m_fHealth;
    if (this == FindPlayerVehicle()) {
        healthLoss *= vehicleFlags.bTakeLessDamage ? 1.0f / 6.0f : 0.5f;
    } else if (vehicleFlags.bTakeLessDamage) {
        healthLoss *= 1.0f / 12.0f;
    } else {
        healthLoss *= m_pDamageEntity && m_pDamageEntity == FindPlayerVehicle() ? 2.0f / 3.0f : 0.25f;
    }
    m_fHealth -= healthLoss;
    if (m_fHealth <= 0.0f && prevHealth > 0.0f) {
        m_fHealth = 1.0f;
    }

    if (damageIntensity > destroyThreshold) {
        BlowUpCar(this, false);
        return;
    }
    if (m_fHealth <= 1.0f && prevHealth > 0.0f) {
        BlowUpCar(this, false);
        return;
    }

    if (!(damageIntensity > damageThreshold) && !isWeaponDamage) {
        return;
    }
    if (GetStatus() == STATUS_WRECKED) {
        return;
    }
    if (!(m_fMass > 1000.0f)) {
        return;
    }

    // 0x6CC775 - Find the component closest to the collision point
    const auto colPosLocal = GetMatrix().InverseTransformPoint(*vecCollisionCoors);

    auto closestDist = 1000.0f;
    auto closestNode = -1;
    for (auto node = (int32)PLANE_STATIC_PROP; node < (int32)PLANE_NUM_NODES; node++) {
        const auto frame = m_aCarNodes[node];
        if (!frame) {
            continue;
        }
        const auto distSq = (CVector{ RwFrameGetMatrix(frame)->pos } - colPosLocal).SquaredMagnitude();
        if (distSq < sq(closestDist)) {
            closestNode = node;
            closestDist = std::sqrt(distSq);
        }
    }
    if (closestNode <= -1) {
        return;
    }

    if (closestNode == PLANE_RUDDER && (rand() & 1)) {
        if (m_aCarNodes[PLANE_ELEVATOR_L] && (rand() & 1)) {
            closestNode = PLANE_ELEVATOR_L;
        } else if (m_aCarNodes[PLANE_ELEVATOR_R]) {
            closestNode = PLANE_ELEVATOR_R;
        }
    }

    if (m_damageManager.ProgressAeroplaneDamage((uint8)closestNode)) {
        if (m_damageManager.GetAeroplaneCompStatus((uint8)closestNode) == 1 && closestNode >= PLANE_RUDDER && closestNode <= PLANE_GEAR_R) {
            // Make the component flap around
            // NOTE: The game iterates 4 panels here, but `CAutomobile` has only 3 (So it reads into `m_swingingChassis`) - Not doing that.
            for (auto& panel : m_panels) {
                if (panel.m_nFrameId == (uint16)-1) {
                    panel.SetPanel((int16)closestNode, 0, -0.02f);
                    break;
                }
                if (panel.m_nFrameId == (uint16)closestNode) {
                    break;
                }
            }
        } else if (const auto status = m_damageManager.GetAeroplaneCompStatus((uint8)closestNode); status == 2) {
            SetComponentVisibility(m_aCarNodes[closestNode], status);
        }
    }

    dmgDrawCarCollidingParticles(*vecCollisionCoors, particleForceMult * damageIntensity, weapon);

    // 0x6CC92E - (Re)start the fire on the damaged component
    const CVector nodePos{ RwFrameGetMatrix(m_aCarNodes[closestNode])->pos };
    if (m_pSmokeParticle) {
        m_pSmokeParticle->Kill();
        m_pSmokeParticle = nullptr;
    }
    if (const auto modellingMat = GetModellingMatrix()) {
        m_pSmokeParticle = g_fxMan.CreateFxSystem("fire_med", nodePos, modellingMat, false);
        if (m_pSmokeParticle) {
            m_pSmokeParticle->Play();
            m_pSmokeParticle->SetVelAdd(-m_vecMoveSpeed * 5.0f);
            m_pSmokeParticle->SetLocalParticles(true);
            m_nSmokeTimer = CGeneral::GetRandomNumberInRange(2000, 4000);
        }
    }
}

// 0x6CAB90
void CPlane::IsAlreadyFlying() {
    m_nStartedFlyingTime = CTimer::GetTimeInMS() - 20000;
}

// 0x6CAC20
void CPlane::SetGearUp() {
    m_LandingGearAngle = 1.0f;
    m_fAirResistance = m_pHandlingData->m_fDragMult / 1000.0f / 2.0f * m_pFlyingHandlingData->m_fGearUpR;
    m_damageManager.SetWheelStatus(CAR_WHEEL_FRONT_LEFT,  WHEEL_STATUS_MISSING);
    m_damageManager.SetWheelStatus(CAR_WHEEL_REAR_LEFT,   WHEEL_STATUS_MISSING);
    m_damageManager.SetWheelStatus(CAR_WHEEL_FRONT_RIGHT, WHEEL_STATUS_MISSING);
    m_damageManager.SetWheelStatus(CAR_WHEEL_REAR_RIGHT,  WHEEL_STATUS_MISSING);
}

// 0x6CAC70
void CPlane::SetGearDown() {
    m_LandingGearAngle = 0.0f;
    m_fAirResistance = m_pHandlingData->m_fDragMult / 1000.0f / 2.0f;
    m_damageManager.SetWheelStatus(CAR_WHEEL_FRONT_LEFT,  WHEEL_STATUS_OK);
    m_damageManager.SetWheelStatus(CAR_WHEEL_REAR_LEFT,   WHEEL_STATUS_OK);
    m_damageManager.SetWheelStatus(CAR_WHEEL_FRONT_RIGHT, WHEEL_STATUS_OK);
    m_damageManager.SetWheelStatus(CAR_WHEEL_REAR_RIGHT,  WHEEL_STATUS_OK);
}

// 0x6CCA50
uint32 CPlane::CountPlanesAndHelis() {
    uint32 counter = 0;
    for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
        if (vehicle.IsSubHeli() || vehicle.IsSubPlane()) {
            counter++;
        }
    }
    return counter;
}

// 0x6CCAA0
bool CPlane::AreWeInNoPlaneZone() {
    const auto& camPos = TheCamera.GetPosition();
    constexpr CVector vec1 = { -1073.0f, -675.0f, 50.0f };

    return DistanceBetweenPoints(vec1, camPos) < 200.0f ||
           camPos.x > -2743.0f && camPos.x < -2626.0f && camPos.y > 1300.0f && camPos.y < 2200.0f || // todo: Is point inside
           camPos.x > -1668.0f && camPos.x < -1122.0f && camPos.y > 541.0f && camPos.y < 1118.0f;
}

// 0x6CCBB0
bool CPlane::AreWeInNoBigPlaneZone() {
    // untested
    const auto& camPos = TheCamera.GetPosition();
    return DistanceBetweenPoints2D({ +1522.0f, -1237.0f }, camPos) < 800.0f ||
           DistanceBetweenPoints2D({ -1836.0f, +659.0f }, camPos) < 800.0f;
}
// 0x6CCC50
void CPlane::SwitchAmbientPlanes(bool enable) {
    if (!GenPlane_Active) {
        GenPlane_Active = enable;
        return;
    }
    if (!enable) {
        for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
            if (vehicle.IsSubHeli() || vehicle.IsSubPlane()) {
                if (vehicle.GetCreatedBy() == RANDOM_VEHICLE) {
                    CWorld::Remove(&vehicle);
                    delete &vehicle;
                }
            }
        }
    }
    GenPlane_Active = enable;
}

// 0x6CD090
void CPlane::FindPlaneCreationCoors(CVector* outCoors, CVector* outTargetCoors, float* outPlaneOrientation, float* outFlightHeight, bool isBigPlane) {
    float spawnRadius, baseHeight;
    if (!isBigPlane) {
        spawnRadius = 140.0f;
        baseHeight = 25.0f;
    } else {
        spawnRadius = 340.0f;
        baseHeight = 200.0f;
    }
    int32 attempt = 0;
    for (; attempt < 500; attempt += 10) {
        const float angle = (float)(CGeneral::GetRandomNumber() % 360) * DEG_TO_RAD;
        *outCoors = FindPlayerCoors();
        *outFlightHeight = (float)(CGeneral::GetRandomNumber() & 0xF) + (float)attempt + baseHeight;
        outCoors->x += std::cos(angle) * spawnRadius;
        outCoors->y += std::sin(angle) * spawnRadius;
        outCoors->z += *outFlightHeight;
        *outTargetCoors = FindPlayerCoors();
        const float targetDist = (float)(CGeneral::GetRandomNumber() & 0x1F) + 20.0f;
        // NOTSA: raw static @ 0xB6F9AC - camera/plane target direction vector, no clean named mapping
        const auto dir = *reinterpret_cast<CVector*>(0xB6F9AC);
        outTargetCoors->x += dir.x * targetDist;
        outTargetCoors->y += dir.y * targetDist;
        outTargetCoors->z += dir.z * targetDist + *outFlightHeight;
        *outPlaneOrientation = CGeneral::GetATanOfXY(outTargetCoors->x - outCoors->x, outTargetCoors->y - outCoors->y);
        CVector lineEnd = *outTargetCoors + (*outTargetCoors - *outCoors);
        CWorld::AdvanceCurrentScanCode();
        CColSphere sphere(*outCoors, 16.0f);
        if (CWorld::GetIsLineOfSightClear(*outCoors, lineEnd, true, false, false, false, false, false, false)) {
            // NOTSA: the binary builds raw sphere/box structs on the stack and calls
            // `CCollision::CheckCameraCollisionBuildings` with them - no clean high-level mapping,
            // matching the observable behavior (break iff no building blocks the spawn line).
            CColSphere spS(lineEnd, 16.0f), spA(lineEnd, 16.0f), spB(lineEnd, 16.0f);
            CColBox box(spA.m_vecCenter - CVector(16.0f, 16.0f, 16.0f), spA.m_vecCenter + CVector(16.0f, 16.0f, 16.0f));
            if (!CCollision::CheckCameraCollisionBuildings(0, 0, box, spS, spA, spB)) {
                break;
            }
        }
    }
    *outFlightHeight += 20.0f;
    outTargetCoors->z += 20.0f;
}

// 0x6CD2F0
void CPlane::DoPlaneGenerationAndRemoval() {
    if ((CTimer::m_FrameCounter & 31) != 30 || !GenPlane_Active) {
        return;
    }

    const auto numPlanesAndHelis = CountPlanesAndHelis();
    const auto timeMs            = [] { return CTimer::GetTimeInMS(); };

    const auto RequestGeneration = [](int32 modelIndex) {
        GenPlane_ModelIndex = modelIndex;
        GenPlane_Status     = 1;
    };

    if (GenPlane_Status == 1) { // 0x6CD33E - Create the requested plane/heli (Once it's loaded)
        if (!CStreaming::IsModelLoaded(GenPlane_ModelIndex)) {
            CStreaming::RequestModel(GenPlane_ModelIndex, STREAMING_KEEP_IN_MEMORY);
            return;
        }

        // 0x6C4130 - Makes the vehicle face in the direction of `heading` (Translation is kept)
        const auto SetVehicleHeading = [](CVehicle* veh, float heading) {
            auto&      mat = *veh->m_matrix;
            const auto s   = std::sin(heading);
            const auto c   = std::cos(heading);
            mat.GetRight()   = CVector{ s, -c, 0.0f };
            mat.GetForward() = CVector{ c, s, 0.0f };
            mat.GetUp()      = CVector{ 0.0f, 0.0f, 1.0f };
        };

        // 0x41BDD0
        const auto SetVehicleEngineOn = [](CVehicle* veh, bool on) {
            veh->vehicleFlags.bEngineOn = veh->vehicleFlags.bEngineBroken ? false : on;
        };

        CVector pos, targetPos;
        float   heading, flightHeight;

        if (GenPlane_ModelIndex == MODEL_POLMAV) { // 0x6CD385
            FindPlaneCreationCoors(&pos, &targetPos, &heading, &flightHeight, false);

            auto* const heli = new CHeli(GenPlane_ModelIndex, RANDOM_VEHICLE);
            heli->SetPosn(pos);
            SetVehicleHeading(heli, heading);
            CCarCtrl::JoinCarWithRoadSystem(heli);
            heli->m_fHeliRotorSpeed = 0.22f;
            heli->SetStatus(STATUS_PHYSICS);
            CWorld::Add(heli);

            heli->m_fMinAltitude            = flightHeight;
            heli->m_autoPilot.m_nCarMission = MISSION_HELI_FLYINDIRECTION;
            heli->field_9B4                 = heading;
            heli->m_fMaxAltitude            = flightHeight;
            heli->SetVelocity(CVector{ std::cos(heading) * (2.0f / 3.0f), std::sin(heading) * (2.0f / 3.0f), 0.0f });
            heli->vehicleFlags.bNeverUseSmallerRemovalRange = true;
            heli->m_nExtendedRemovalRange                   = 255;
            heli->m_autoPilot.m_nCruiseSpeed                = 50;
            SetVehicleEngineOn(heli, true);

            // 0x6CD48B - Sometimes go after some car or ped
            const auto SetTarget = [heli](CEntity* target) {
                heli->m_autoPilot.m_nCarMission = MISSION_HELI_FOLLOW_ENTITY;
                heli->m_autoPilot.m_TargetEntity = reinterpret_cast<CVehicle*>(target);
                target->RegisterReference(reinterpret_cast<CEntity**>(&heli->m_autoPilot.m_TargetEntity));
                heli->m_nHeliFlags                |= 2;
                heli->m_autoPilot.m_nCarCtrlFlags |= 0xA0; // bDoTargetCatchupCheck | bHeliFollowTarget
            };
            switch (rand() % 2) {
            case 0: { // 0x6CD5EA
                if (auto* const car = CWorld::FindUnsuspectingTargetCar(heli->GetPosition(), FindPlayerCoors())) {
                    SetTarget(car);
                }
                break;
            }
            case 1: { // 0x6CD4A7
                if (auto* const ped = CWorld::FindUnsuspectingTargetPed(heli->GetPosition(), FindPlayerCoors())) {
                    SetTarget(ped);

                    // Scare the ped (or it's group)
                    auto* const group = CPedGroups::GetPedsGroup(ped);
                    if (group && !group->m_bIsMissionGroup) {
                        CEventDanger event{ heli, 200.0f };
                        event.m_TaskId = static_cast<eTaskType>(1505);
                        group->GetIntelligence().AddEvent(&event);
                    } else if (!ped->IsPlayer() && !ped->IsCreatedByMission() && !ped->IsCop()) {
                        CEventDanger event{ heli, 200.0f };
                        event.m_TaskId = static_cast<eTaskType>(911);
                        ped->GetIntelligence()->GetEventGroup().Add(&event, false);
                    }
                }
                break;
            }
            }
        } else { // 0x6CD67A
            const auto isBigPlane = GenPlane_ModelIndex == MODEL_AT400 || GenPlane_ModelIndex == MODEL_ANDROM;
            FindPlaneCreationCoors(&pos, &targetPos, &heading, &flightHeight, isBigPlane);

            auto* const plane = new CPlane(GenPlane_ModelIndex, RANDOM_VEHICLE);
            plane->SetPosn(pos);
            plane->m_planeCreationHeading = heading;
            SetVehicleHeading(plane, heading);
            CCarCtrl::JoinCarWithRoadSystem(plane);
            plane->SetStatus(STATUS_PHYSICS);
            CWorld::Add(plane);

            plane->m_planeHeading            = heading;
            plane->m_minAltitude             = flightHeight;
            plane->m_autoPilot.m_nCarMission = MISSION_PLANE_FLYINDIRECTION;
            plane->m_planeHeadingPrev        = heading;
            plane->m_maxAltitude             = flightHeight;
            plane->SetVelocity(CVector{ std::cos(heading) * (2.0f / 3.0f), std::sin(heading) * (2.0f / 3.0f), 0.0f });
            plane->vehicleFlags.bNeverUseSmallerRemovalRange = true;

            if (isBigPlane || GenPlane_ModelIndex == MODEL_HYDRA) {
                plane->m_nExtendedRemovalRange = 360;
                if (isBigPlane) {
                    plane->m_bUsesCollision = false;
                }
            } else {
                plane->m_nExtendedRemovalRange = 150;
            }

            if (GenPlane_ModelIndex == MODEL_HYDRA) {
                plane->m_autoPilot.m_nCarMission = MISSION_PLANE_ATTACK_PLAYER_POLICE;
                plane->m_wMiscComponentAngle     = 0;
            }

            plane->SetGearUp();
            SetVehicleEngineOn(plane, true);
            CStreaming::SetModelIsDeletable(GenPlane_ModelIndex);
            CStreaming::SetModelTxdIsDeletable(GenPlane_ModelIndex);
        }

        GenPlane_LastTimeGenerated = timeMs();
        GenPlane_Status            = 0;
        return;
    }

    if (GenPlane_Status != 0) {
        return;
    }

    // 0x6CD82A - Decide whether (and what) to generate
    if (CGame::currArea != AREA_CODE_NORMAL_WORLD || numPlanesAndHelis >= 5 || AreWeInNoPlaneZone() || CCutsceneMgr::ms_cutsceneProcessing) {
        GenPlane_LastTimeGenerated = timeMs();
        return;
    }

    const auto IsPlayerInAircraft = [] {
        auto* const veh = FindPlayerVehicle();
        return veh && (veh->GetVehicleAppearance() == VEHICLE_APPEARANCE_HELI || veh->GetVehicleAppearance() == VEHICLE_APPEARANCE_PLANE);
    };

    // 0x6CD874 - Hydras going after the player
    if ((int32)FindPlayerPed()->GetWantedLevel() > 3) {
        if (auto* const veh = FindPlayerVehicle()) {
            if ((veh->m_nVehicleSubType == VEHICLE_TYPE_PLANE || veh->m_nVehicleSubType == VEHICLE_TYPE_HELI)
                && veh->m_nModelIndex != MODEL_AT400
                && veh->m_nModelIndex != MODEL_ANDROM
                && timeMs() > GenPlane_LastTimeGenerated + 60'000
            ) {
                auto numAttackers = 0;
                for (auto& v : GetVehiclePool()->GetAllValid()) {
                    if (v.m_autoPilot.m_nCarMission == MISSION_PLANE_ATTACK_PLAYER_POLICE) {
                        numAttackers++;
                    }
                }
                if (numAttackers < 1) {
                    RequestGeneration(MODEL_HYDRA);
                } else if (numAttackers < 2 && (int32)FindPlayerPed()->GetWantedLevel() > 4) {
                    RequestGeneration(MODEL_HYDRA);
                }
            }
        }
    }

    if (numPlanesAndHelis >= 1 || GenPlane_Status != 0) {
        return;
    }

    const auto isSmallPlaneRegion = CWeather::WeatherRegion == WEATHER_REGION_DEFAULT || CWeather::WeatherRegion == WEATHER_REGION_DESERT;
    const auto isSandstorm        = CWeather::OldWeatherType == WEATHER_SANDSTORM_DESERT || CWeather::NewWeatherType == WEATHER_SANDSTORM_DESERT;

    // 0x6CD9A9 - Small planes in the countryside/desert
    if (isSmallPlaneRegion && !isSandstorm) {
        if (!IsPlayerInAircraft() && timeMs() > GenPlane_LastTimeGenerated + 120'000) {
            switch (rand() & 7) {
            case 0:
            case 1:
            case 2:
            case 7:
                GenPlane_ModelIndex = MODEL_RUSTLER;
                break;
            case 3:
            case 4:
                GenPlane_ModelIndex = MODEL_CROPDUST;
                break;
            case 5:
            case 6:
                GenPlane_ModelIndex = MODEL_BEAGLE;
                break;
            }
            GenPlane_Status = 1;
            return;
        }
    }

    // 0x6CDA84 - Police helis in gang areas
    if (CPopCycle::m_bCurrentZoneIsGangArea) {
        if (!CTheScripts::IsPlayerOnAMission() && timeMs() > GenPlane_LastTimeGenerated + 350'000) {
            RequestGeneration(MODEL_POLMAV);
            return;
        }
    }

    // 0x6CDADB - Police helis during the riots
    if (CGameLogic::LaRiotsActiveHere()) {
        RequestGeneration(MODEL_POLMAV);
        return;
    }

    // 0x6CDAEE - Big planes in the cities
    if (!isSmallPlaneRegion
        && !CPopCycle::m_bCurrentZoneIsGangArea
        && !isSandstorm
        && timeMs() > GenPlane_LastTimeGenerated + 200'000
        && !AreWeInNoBigPlaneZone()
        && !IsPlayerInAircraft()
    ) {
        RequestGeneration((rand() & 1) ? MODEL_ANDROM : MODEL_AT400);
    }
}

// 0x6C9140
bool CPlane::SetUpWheelColModel(CColModel* wheelCol) {
    return false;
}

// 0x6C9150
bool CPlane::BurstTyre(uint8 tyreComponentId, bool bPhysicalEffect) {
    return false;
}

// 0x6C94A0
void CPlane::PreRender() {
    CMatrix mat;

    auto* const mi = GetVehicleModelInfo();
    auto* const cm = GetColModel();

    CVehicle::PreRender();

    if (m_nModelIndex == MODEL_VORTEX) {
        DoHoverSuspensionRatios();
    }

    // 0x6C9505 - Update wheel positions
    if (vehicleFlags.bVehicleColProcessed) {
        DoBurstAndSoftGroundRatios();

        for (auto i = 0; i < 4; i++) {
            const auto springRatio = 1.0f - m_aSuspensionSpringLength[i] / m_aSuspensionLineLength[i];
            const auto compression = (m_fWheelsSuspensionCompression[i] - springRatio) / (1.0f - springRatio);

            CVector wheelPosn;
            mi->GetWheelPosn(i, wheelPosn, true);

            auto wheelZ = wheelPosn.z + m_pHandlingData->m_fSuspensionLowerLimit;
            if (compression > 0.0f) {
                wheelZ -= compression * m_aSuspensionSpringLength[i];
            }
            if (wheelZ <= m_wheelPosition[i] && !(physicalFlags.bAddMovingCollisionSpeed && handlingFlags.bHydraulicInst)) {
                wheelZ = (wheelZ - m_wheelPosition[i]) * 0.75f + m_wheelPosition[i];
            }
            m_wheelPosition[i] = wheelZ;

            if (m_nModelIndex == MODEL_ANDROM) {
                if (m_fWheelsSuspensionCompression[i] >= 1.0f) {
                    m_wheelRotation[i] *= 0.95f;
                } else {
                    const auto dot = DotProduct(m_matrix->GetForward(), m_wheelColPoint[i].m_vecNormal);
                    m_wheelRotation[i] = -std::asin(std::clamp(dot, -1.0f, 1.0f));
                }
            }
        }
    }

    UpdateWheelMatrix(PLANE_WHEEL_RB, 1);
    UpdateWheelMatrix(PLANE_WHEEL_LB, 1);
    UpdateWheelMatrix(PLANE_WHEEL_RF, 1);
    UpdateWheelMatrix(PLANE_WHEEL_LF, m_nModelIndex == MODEL_HYDRA ? 3 : 1);

    // 0x6C96B9 - `field_9C8` is the propeller's angle
    field_9C8 += CTimer::GetTimeStep() * m_fPropSpeed;
    while (field_9C8 > 2.0f * PI) {
        field_9C8 -= 2.0f * PI;
    }

    // Calculates the hinge axis of a control surface from the model's dummy position
    const auto CalcHingeAxis = [&](eVehicleDummy dummy, bool defaultIsZAxis) {
        CVector axis = mi->m_pVehicleStruct->m_avDummyPos[dummy];
        if (axis.x == 0.0f && axis.y == 0.0f && axis.z == 0.0f) {
            if (defaultIsZAxis) {
                axis.z = 1.0f;
            } else {
                axis.x = 1.0f;
            }
        } else {
            axis -= mat.GetPosition();
            axis.Normalise();
        }
        return axis;
    };

    // 0x6C9706 - Control surfaces
    if (GetStatus() <= STATUS_PHYSICS) {
        CQuaternion quat;

        if (auto* const rudder = m_aCarNodes[PLANE_RUDDER]) {
            mat.Attach(RwFrameGetMatrix(rudder), false);
            mat.UpdateRW();

            CVector axis = CalcHingeAxis(DUMMY_EXHAUST_SECONDARY, true);
            quat.Set(&axis, 40.0f * 0.0174532924f * m_fLeftRightSkid);
            quat.Get(mat.m_pAttachMatrix);

            if (m_nModelIndex == MODEL_VORTEX) {
                if (auto* const rudder2 = m_aCarNodes[PLANE_MISC_B]) {
                    mat.Attach(RwFrameGetMatrix(rudder2), false);
                    mat.UpdateRW();
                    quat.Get(mat.m_pAttachMatrix);
                }
            }
        }

        // 0x6C984E
        if (auto* const aileronR = m_aCarNodes[PLANE_AILERON_R]) {
            auto flapScale = 1.0f;
            auto flapAngle = 0.0f;
            if (m_pFlyingHandlingData->m_fGearDownL > 1.0f) {
                if (std::fabs(m_LandingGearAngle) < 1.0f && (m_aCarNodes[PLANE_GEAR_L] || m_aCarNodes[PLANE_GEAR_R])) {
                    flapScale = (1.0f - std::fabs(m_LandingGearAngle)) * 0.5f + 1.0f;
                    flapAngle = (1.0f - std::fabs(m_LandingGearAngle)) * 0.3f;
                }
            }

            mat.Attach(RwFrameGetMatrix(aileronR), false);
            mat.UpdateRW();

            CVector axis = CalcHingeAxis(DUMMY_TRAILER_ATTACH, false);
            quat.Set(&axis, -30.0f * 0.0174532924f * m_fSteeringLeftRight + flapAngle);
            quat.Get(mat.m_pAttachMatrix);

            if (flapScale > 1.0f) {
                mat.Update();
                if (axis.x == 1.0f) {
                    mat.GetForward().y *= flapScale;
                } else {
                    CVector compPos;
                    if (mi->GetOriginalCompPosition(compPos, PLANE_AILERON_R)) {
                        compPos.y += (1.0f - flapScale) * cm->m_boundBox.m_vecMax.y * 0.03f;
                        mat.SetTranslateOnly(compPos);
                    }
                }
                mat.UpdateRW();
            }

            // 0x6C9A15
            if (auto* const aileronL = m_aCarNodes[PLANE_AILERON_L]) {
                mat.Attach(RwFrameGetMatrix(aileronL), false);
                mat.UpdateRW();

                axis.x *= -1.0f;
                quat.Set(&axis, -30.0f * 0.0174532924f * m_fSteeringLeftRight - flapAngle);
                quat.Get(mat.m_pAttachMatrix);

                if (flapScale > 1.0f) {
                    mat.Update();
                    if (axis.x == -1.0f) {
                        mat.GetForward().y *= flapScale;
                    } else {
                        CVector compPos;
                        if (mi->GetOriginalCompPosition(compPos, PLANE_AILERON_L)) {
                            compPos.y += (1.0f - flapScale) * cm->m_boundBox.m_vecMax.y * 0.03f;
                            mat.SetTranslateOnly(compPos);
                        }
                    }
                    mat.UpdateRW();
                }
            }
        }

        // 0x6C9B4E
        if (auto* const elevatorR = m_aCarNodes[PLANE_ELEVATOR_R]) {
            mat.Attach(RwFrameGetMatrix(elevatorR), false);
            mat.UpdateRW();

            CVector axis = CalcHingeAxis(DUMMY_HAND_REST, false);
            quat.Set(&axis, -(25.0f * 0.0174532924f * m_fSteeringUpDown));
            quat.Get(mat.m_pAttachMatrix);

            if (auto* const elevatorL = m_aCarNodes[PLANE_ELEVATOR_L]) {
                mat.Attach(RwFrameGetMatrix(elevatorL), false);
                mat.UpdateRW();

                axis.x *= -1.0f;
                quat.Set(&axis, 25.0f * 0.0174532924f * m_fSteeringUpDown);
                quat.Get(mat.m_pAttachMatrix);
            }
        }
    }

    // 0x6C9C6E - Propellers
    const auto SetNodeAlpha = [this](ePlaneNodes node, int32 alpha) {
        RwObject* atomic = nullptr;
        RwFrameForAllObjects(m_aCarNodes[node], GetCurrentAtomicObjectCB, &atomic);
        if (atomic) {
            CVehicle::SetComponentAtomicAlpha(reinterpret_cast<RpAtomic*>(atomic), alpha);
        }
    };
    const auto UpdateProps = [&](ePlaneNodes staticProp, ePlaneNodes movingProp, float dir) {
        if (auto* const frame = m_aCarNodes[staticProp]) {
            const auto angle = dir * field_9C8;
            SetComponentRotation(frame, AXIS_Y, angle + angle, true);
            SetNodeAlpha(staticProp, 255);
        }
        if (auto* const frame = m_aCarNodes[movingProp]) {
            SetComponentRotation(frame, AXIS_Y, -(dir * field_9C8), true);
            SetNodeAlpha(movingProp, 0);
        }
    };
    UpdateProps(PLANE_STATIC_PROP, PLANE_MOVING_PROP, 1.0f);
    UpdateProps(PLANE_STATIC_PROP2, PLANE_MOVING_PROP2, -1.0f);

    // 0x6C9D6E - Landing gear
    {
        auto gearAxis   = AXIS_X;
        auto miscAxis   = (int32)-1;
        auto gearAngleL = 0.0f;
        auto gearAngleR = 0.0f;
        auto miscAngleA = 0.0f;
        auto miscAngleB = 0.0f;
        auto hasGear    = true;
        switch (m_nModelIndex) {
        case MODEL_RUSTLER:
            gearAxis   = AXIS_Y;
            gearAngleL = -1.4835299f;
            gearAngleR = 1.4835299f;
            break;
        case MODEL_SHAMAL:
        case MODEL_AT400:
            gearAxis   = AXIS_Y;
            gearAngleL = -1.4835299f;
            gearAngleR = 1.4835299f;
            miscAxis   = AXIS_X;
            miscAngleA = 2.268928f;
            break;
        case MODEL_HYDRA:
            gearAxis   = AXIS_X;
            gearAngleL = -HALF_PI;
            gearAngleR = -HALF_PI;
            miscAxis   = AXIS_X;
            miscAngleA = -1.3962634f;
            miscAngleB = 2.268928f;
            break;
        case MODEL_NEVADA:
            gearAxis   = AXIS_X;
            gearAngleL = 1.3089969f;
            gearAngleR = 1.3089969f;
            break;
        case MODEL_ANDROM:
            gearAxis   = AXIS_X;
            gearAngleL = 2.268928f;
            gearAngleR = 2.268928f;
            miscAxis   = AXIS_X;
            miscAngleA = -2.268928f;
            break;
        default:
            hasGear = false;
            break;
        }
        if (hasGear) {
            SetComponentRotation(m_aCarNodes[PLANE_GEAR_L], gearAxis, std::fabs(m_LandingGearAngle) * gearAngleL, true);
            SetComponentRotation(m_aCarNodes[PLANE_GEAR_R], gearAxis, std::fabs(m_LandingGearAngle) * gearAngleR, true);
            if (miscAxis > -1) {
                SetComponentRotation(m_aCarNodes[PLANE_MISC_A], (eRotationAxis)miscAxis, std::fabs(m_LandingGearAngle) * miscAngleA, true);
                if (miscAngleB > 0.0f) {
                    SetComponentRotation(m_aCarNodes[PLANE_MISC_B], (eRotationAxis)miscAxis, std::fabs(m_LandingGearAngle) * miscAngleB, true);
                }
            }
        }
    }

    // 0x6C9EDD - Model specific stuff
    if (m_nModelIndex == MODEL_ANDROM) {
        SetComponentRotation(m_aCarNodes[PLANE_MISC_B], AXIS_X, (float)m_wMiscComponentAngle * ANDROM_COL_ANGLE_MULT, true);
    } else if (m_nModelIndex == MODEL_HYDRA) {
        const auto nozzleAngle = (float)m_wMiscComponentAngle * HALF_PI / (float)(int16)HARRIER_NOZZLE_ROTATE_LIMIT;
        SetComponentRotation(m_aCarNodes[PLANE_WHEEL_LM], AXIS_X, nozzleAngle, true);
        SetComponentRotation(m_aCarNodes[PLANE_WHEEL_RM], AXIS_X, nozzleAngle, true);

        if (vehicleFlags.bEngineOn) {
            const auto power = (m_fAccelerationBreakStatus + 1.0f) * 0.5f;

            if ((int32)m_wMiscComponentAngle < (int32)(int16)HARRIER_NOZZLE_SWITCH_LIMIT) {
                if (m_pDustParticle) {
                    m_pDustParticle->Kill();
                    m_pDustParticle       = nullptr;
                    m_heliDustFxTimeConst = 0.0f;
                }
            } else {
                DoHeliDustEffect(power, 2.0f);
            }

            // NOTE: The game multiplies `CMatrix` with the node's `RwMatrix` directly (They share the same layout)
            const auto* const nozzleMatL = reinterpret_cast<const CMatrix*>(RwFrameGetMatrix(m_aCarNodes[PLANE_WHEEL_LM]));
            const auto* const nozzleMatR = reinterpret_cast<const CMatrix*>(RwFrameGetMatrix(m_aCarNodes[PLANE_WHEEL_RM]));

            for (auto& fx : m_apJettrusParticles) {
                if (auto* const modellingMat = GetModellingMatrix(); modellingMat && !fx) {
                    fx = g_fxMan.CreateFxSystem("jetthrust", CVector{ 0.0f, 0.0f, 0.0f }, modellingMat, false);
                    if (fx) {
                        fx->Play();
                        fx->SetLocalParticles(true);
                        fx->CopyParentMatrix();
                    }
                }
            }

            const auto UpdateNozzleFx = [&](FxSystem_c* fx, const CMatrix* nozzleMat, CVector offset) {
                if (!fx) {
                    return;
                }
                CMatrix fxMat = *m_matrix * *nozzleMat;
                fx->SetMatrix(reinterpret_cast<RwMatrix*>(&fxMat));
                fx->SetOffsetPos(offset);
                fx->SetConstTime(true, power);
            };
            UpdateNozzleFx(m_apJettrusParticles[0], nozzleMatL, { 0.7f, -0.45f, 0.05f });
            UpdateNozzleFx(m_apJettrusParticles[1], nozzleMatL, { -0.82f, -0.45f, 0.05f });
            UpdateNozzleFx(m_apJettrusParticles[2], nozzleMatR, { 0.63f, -0.45f, 0.07f });
            UpdateNozzleFx(m_apJettrusParticles[3], nozzleMatR, { -0.75f, -0.45f, 0.07f });
        } else {
            for (auto& fx : m_apJettrusParticles) {
                if (fx) {
                    fx->Kill();
                    fx = nullptr;
                }
            }
            if (m_pDustParticle) {
                m_pDustParticle->Kill();
                m_pDustParticle       = nullptr;
                m_heliDustFxTimeConst = 0.0f;
            }
        }
    } else if (m_nModelIndex == MODEL_VORTEX && m_aCarNodes[PLANE_MISC_A]) {
        // 0x6CA301 - Make the skirt lean according to the speed and the suspension
        const auto* const cd = GetColModel()->m_pColData;

        CVector wheelPosn;
        mi->GetWheelPosn(0, wheelPosn, false);

        mat.Attach(RwFrameGetMatrix(m_aCarNodes[PLANE_MISC_A]), false);

        mat.GetUp().y = std::clamp(DotProduct(m_vecMoveSpeed, m_matrix->GetForward()) * VORTEX_SKIRT_SPEED_MULT, -VORTEX_SKIRT_SPEED_LIMIT, VORTEX_SKIRT_SPEED_LIMIT);
        mat.GetUp().x = std::clamp(DotProduct(m_vecMoveSpeed, m_matrix->GetRight()) * VORTEX_SKIRT_SPEED_MULT, -VORTEX_SKIRT_SPEED_LIMIT, VORTEX_SKIRT_SPEED_LIMIT);

        const auto f    = std::pow(VORTEX_SKIRT_SMOOTHING, CTimer::GetTimeStep());
        const auto invF = 1.0f - f;

        const auto pitch = std::clamp(
            ((m_wheelPosition[2] - m_wheelPosition[3]) + (m_wheelPosition[0] - m_wheelPosition[1])) * 0.5f / (cd->m_pLines[0].m_vecStart.y - cd->m_pLines[1].m_vecStart.y),
            -VORTEX_SKIRT_PITCH_LIMIT,
            +VORTEX_SKIRT_PITCH_LIMIT
        );
        mat.GetForward().z = mat.GetForward().z * f + invF * pitch;

        const auto roll = std::clamp(
            ((m_wheelPosition[3] - m_wheelPosition[1]) + (m_wheelPosition[2] - m_wheelPosition[0])) * 0.5f / (cd->m_pLines[3].m_vecStart.x - cd->m_pLines[1].m_vecStart.x),
            -VORTEX_SKIRT_ROLL_LIMIT,
            +VORTEX_SKIRT_ROLL_LIMIT
        );
        mat.GetRight().z = mat.GetRight().z * f + invF * roll;

        const auto height = std::min(
            1.0f - ((m_wheelPosition[3] + m_wheelPosition[1] + m_wheelPosition[2] + m_wheelPosition[0]) * 0.25f - wheelPosn.z) / (mi->m_fWheelSizeFront * 0.5f),
            VORTEX_SKIRT_MAX_SCALE
        );
        mat.GetUp().z = mat.GetUp().z * f + invF * height;

        mat.UpdateRW();
    }

    // 0x6CA638 - Flapping components
    for (auto& panel : m_panels) {
        if ((int16)panel.m_nFrameId > -1) {
            panel.ProcessPanel(this, m_aCarNodes[(int16)panel.m_nFrameId], m_moveForce, m_turnForce, PLANE_PANEL_SPRING, PLANE_PANEL_DAMPING);
        }
    }
    m_moveForce = m_vecMoveSpeed + m_vecFrictionMoveSpeed;
    m_turnForce = m_vecTurnSpeed + m_vecFrictionTurnSpeed;

    // 0x6CA716
    CShadows::StoreShadowForVehicle(
        this,
        m_nModelIndex == MODEL_VORTEX
            ? VEH_SHD_CAR
            : m_nModelIndex == MODEL_AT400 || m_nModelIndex == MODEL_ANDROM
                ? VEH_SHD_BIG_PLANE
                : VEH_SHD_PLANE
    );

    // 0x6CA73F - Wing tip air trails
    if (!vehicleFlags.bIsDrowning && m_nModelIndex != MODEL_VORTEX && m_nModelIndex != MODEL_RCBARON) {
        const auto alpha = (uint8)(int32)std::max(
            std::fabs(DotProduct(m_vecMoveSpeed, m_matrix->GetUp()) * m_pFlyingHandlingData->m_fAttackLift * 6400.0f) - 32.0f,
            0.0f
        );

        CVector tipOuter = mi->m_pVehicleStruct->m_avDummyPos[DUMMY_WING_AIR_TRAIL];
        CVector tipInner = tipOuter;
        tipInner.x -= 0.1f;
        CMotionBlurStreaks::RegisterStreak((uint32)(uintptr_t)this, 255, 255, 255, alpha, m_matrix->TransformPoint(tipOuter), m_matrix->TransformPoint(tipInner));

        tipOuter.x = -tipOuter.x;
        tipInner.x = -tipInner.x;
        CMotionBlurStreaks::RegisterStreak((uint32)(uintptr_t)this + 1, 255, 255, 255, alpha, m_matrix->TransformPoint(tipOuter), m_matrix->TransformPoint(tipInner));
    }

    // 0x6CA937 - Smoke ejector
    if (m_bSmokeEjectorEnabled) {
        if (m_nModelIndex == MODEL_CROPDUST) {
            const auto    prtMult = FxPrtMult_c(1.0f, 1.0f, 1.0f, 0.4f, 1.0f, 1.0f, 0.2f);
            const CVector pos     = m_matrix->TransformPoint(CVector{ 0.0f, -0.5f, -0.5f });
            const CVector vel{ m_vecMoveSpeed.x * 10.0f, m_vecMoveSpeed.y * 10.0f, m_vecMoveSpeed.z * 10.0f - 3.0f };
            g_fx.m_SmokeHuge->AddParticle(pos, vel, 0.0f, prtMult, -1.0f, 1.2f, 0.6f, false);
        } else if (m_nModelIndex == MODEL_STUNT) {
            const auto    prtMult = FxPrtMult_c(1.0f, 0.0f, 0.0f, 0.4f, 1.0f, 1.0f, 0.3f);
            const CVector pos     = m_matrix->TransformPoint(CVector{ 0.0f, -5.0f, 0.0f });
            const CVector vel{ m_vecMoveSpeed.x * 10.0f, m_vecMoveSpeed.y * 10.0f, m_vecMoveSpeed.z * 10.0f };
            g_fx.m_SmokeHuge->AddParticle(pos, vel, 0.0f, prtMult, -1.0f, 1.2f, 0.6f, false);
        }
    }

    if (m_nModelIndex == MODEL_SKIMMER && physicalFlags.bSubmergedInWater) {
        DoBoatSplashes(m_fDoomHorizontalRotation); // 0x950 - Water damping for the Skimmer
    }
}

// 0x6CAB70
void CPlane::Render() {
    m_nTimeTillWeNeedThisCar = CTimer::GetTimeInMS() + 3000;
    CVehicle::Render();
}

// 0x6C9260
void CPlane::ProcessControl() {
    if (GetStatus() == STATUS_PLAYER) {
        if (m_nModelIndex == MODEL_CROPDUST || m_nModelIndex == MODEL_STUNT) {
            const auto pad = CPad::GetPad(m_pDriver->GetPadNumber());
            if (pad->IsRightShockPressed()) {
                m_bSmokeEjectorEnabled = !m_bSmokeEjectorEnabled;
            }
        }
    }

    if (m_bSmokeEjectorEnabled) {
        if (!vehicleFlags.bEngineOn || vehicleFlags.bIsDrowning || !m_pDriver) {
            m_bSmokeEjectorEnabled = false;
        }
    }

    if (m_nModelIndex == MODEL_SKIMMER) {
        m_damageManager.SetAllWheelsState(WHEEL_STATUS_MISSING);
    }

    CAutomobile::ProcessControl();

    m_vehicleAudio.m_DoCountStalls = static_cast<int16>(m_StallCounter);
    if (m_StallCounter) {
        m_StallCounter = 0;
    }

    CVehicle::ProcessWeapons();
    if (m_nModelIndex == MODEL_VORTEX) {
        m_WheelStates[0] = WHEEL_STATE_NORMAL;
        m_WheelStates[1] = WHEEL_STATE_NORMAL;
        m_WheelStates[2] = WHEEL_STATE_NORMAL;
        m_WheelStates[3] = WHEEL_STATE_NORMAL;
    }

    if (m_pSmokeParticle) {
        RwMatrix out;
        m_nSmokeTimer += static_cast<uint32>(-CTimer::GetTimeStepInMS());
        m_pSmokeParticle->GetCompositeMatrix(&out);
        const CVector velocity = -m_vecMoveSpeed * 5.0f;
        const auto particleData = FxPrtMult_c(0.0f, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0.1f);
        g_fx.m_SmokeHuge->AddParticle(out.pos, velocity, 0.0f, particleData);
        g_fx.m_SmokeHuge->AddParticle(out.pos, velocity, 0.05f, particleData);
        if (m_nSmokeTimer <= 0 || vehicleFlags.bIsDrowning) {
            m_pSmokeParticle->Kill();
            m_pSmokeParticle = nullptr;
        }
    }
}

// 0x6CADD0
void CPlane::ProcessControlInputs(uint8 playerNum) {
    const auto fwdSpeed = DotProduct(m_vecMoveSpeed, m_matrix->GetForward());

    // Steering using the pad (Smoothed)
    const auto SetSteeringFromPad = [&] {
        m_nLastControlInput = eControllerType::KEYBOARD;
        m_fSteeringLeftRight += ((float)CPad::GetPad(playerNum)->GetSteeringLeftRight() * (1.0f / 128.0f) - m_fSteeringLeftRight) * CTimer::GetTimeStep() * 0.2f;
        m_fSteeringUpDown    += ((float)CPad::GetPad(playerNum)->GetSteeringUpDown() * (1.0f / 128.0f) - m_fSteeringUpDown) * CTimer::GetTimeStep() * 0.2f;
    };

    if (!CCamera::m_bUseMouse3rdPerson || !m_bEnableMouseFlying) { // 0x6CB02B
        SetSteeringFromPad();
    } else {
        const auto& mouseMoved = CPad::NewMouseControllerState.m_AmountMoved;

        // 0x6CAE14
        const auto useMouse = [&] {
            if (mouseMoved.x != 0.0f || mouseMoved.y != 0.0f) {
                return true;
            }
            if (!(std::fabs(m_fSteeringLeftRight) > 0.0f) && !(std::fabs(m_fSteeringUpDown) > 0.0f)) {
                return false;
            }
            if (m_nLastControlInput != eControllerType::MOUSE) {
                return false;
            }
            return !CPad::GetPad(playerNum)->GetSteeringLeftRight() && !CPad::GetPad(playerNum)->GetSteeringUpDown();
        }();

        if (useMouse) { // 0x6CAF6B
            m_nLastControlInput = eControllerType::MOUSE;
            if (!CPad::GetPad(playerNum)->NewState.m_bVehicleMouseLook) {
                m_fSteeringLeftRight += mouseMoved.x * 0.0015f;
                m_fSteeringUpDown    += mouseMoved.y * 0.0015f;
            }
            // Slowly center the controls
            if (std::fabs(m_fSteeringLeftRight) < 0.15f) {
                m_fSteeringLeftRight *= std::pow(0.99f, CTimer::GetTimeStep());
            }
            if (std::fabs(m_fSteeringUpDown) < 0.15f) {
                m_fSteeringUpDown *= std::pow(0.99f, CTimer::GetTimeStep());
            }
        } else if (CPad::GetPad(playerNum)->GetSteeringLeftRight() || CPad::GetPad(playerNum)->GetSteeringUpDown() || m_nLastControlInput != eControllerType::MOUSE) { // 0x6CAEBA
            SetSteeringFromPad();
        }
    }

    // 0x6CB0C7
    m_fSteeringLeftRight = std::clamp(m_fSteeringLeftRight, -1.0f, 1.0f);
    m_fSteeringUpDown    = std::clamp(m_fSteeringUpDown, -1.0f, 1.0f);

    // 0x6CB16B - Rudder
    if (m_nModelIndex == MODEL_VORTEX) {
        m_fLeftRightSkid += ((float)CPad::GetPad(playerNum)->GetSteeringLeftRight() * (1.0f / 128.0f) - m_fLeftRightSkid) * CTimer::GetTimeStep() * 0.2f;
    } else {
        const auto lookRight = (float)(uint8)CPad::GetPad(playerNum)->GetLookRight();
        if (CPad::GetPad(playerNum)->GetLookLeft()) {
            m_fLeftRightSkid += (-1.0f - m_fLeftRightSkid) * CTimer::GetTimeStep() * 0.2f;
        } else {
            m_fLeftRightSkid += (lookRight - m_fLeftRightSkid) * CTimer::GetTimeStep() * 0.2f;
        }
    }
    m_fLeftRightSkid = std::clamp(m_fLeftRightSkid, -1.0f, 1.0f);

    // 0x6CB260
    m_fSteerAngle = -(m_pHandlingData->m_fSteeringLock * 0.0174532924f * m_fLeftRightSkid * std::max(1.0f - fwdSpeed, 0.1f));
    if (m_nModelIndex == MODEL_VORTEX && CPad::GetPad(playerNum)->GetHandBrake()) {
        m_fSteerAngle    *= 1.5f;
        m_fLeftRightSkid *= 1.5f;
    }

    // 0x6CB2E4
    m_fAccelerationBreakStatus = (float)(CPad::GetPad(playerNum)->GetAccelerate() - CPad::GetPad(playerNum)->GetBrake()) * (1.0f / 255.0f);

    // 0x6CB321 - Landing gear
    if (CPad::GetPad(playerNum)->IsRightShockPressed()
        && m_fWheelsSuspensionCompression[0] == 1.0f
        && m_fWheelsSuspensionCompression[1] == 1.0f
        && m_fWheelsSuspensionCompression[2] == 1.0f
        && m_fWheelsSuspensionCompression[3] == 1.0f
        && m_pFlyingHandlingData->m_fGearUpR < 1.0f
        && m_nModelIndex != MODEL_RCBARON
    ) {
        if (m_LandingGearAngle == 0.0f) { // Start retracting
            m_damageManager.SetAllWheelsState(WHEEL_STATUS_MISSING);
            m_LandingGearAngle += CTimer::GetTimeStep() * 0.02f;
        } else if (m_LandingGearAngle == 1.0f) { // Start lowering
            m_LandingGearAngle = CTimer::GetTimeStep() * 0.02f - 1.0f;
        } else { // Change direction
            m_LandingGearAngle *= -1.0f;
        }
    } else if (m_LandingGearAngle < 0.0f) { // 0x6CB435 - Lowering
        m_LandingGearAngle += CTimer::GetTimeStep() * 0.02f;
        if (!(m_LandingGearAngle < 0.0f)) {
            SetGearDown();
        }
    } else if (m_LandingGearAngle > 0.0f && m_LandingGearAngle < 1.0f) { // 0x6CB4C7 - Retracting
        m_LandingGearAngle += CTimer::GetTimeStep() * 0.02f;
        if (!(m_LandingGearAngle < 1.0f)) {
            SetGearUp();
        }
    }

    // 0x6CB4F3 - Thrust vectoring
    if (m_nModelIndex == MODEL_HYDRA) {
        if (std::fabs((float)CPad::GetPad(playerNum)->GetCarGunUpDown()) > 10.0f) {
            m_wMiscComponentAnglePrev = m_wMiscComponentAngle;

            const auto delta    = (int32)(int16)(int32)((float)CPad::GetPad(playerNum)->GetCarGunUpDown() * CTimer::GetTimeStep() * HARRIER_NOZZLE_ROTATERATE * (1.0f / 128.0f));
            const auto newAngle = std::max(delta + (int32)m_wMiscComponentAngle, 0);
            m_wMiscComponentAngle = (uint16)newAngle;
            if ((int32)m_wMiscComponentAngle > (int32)(int16)HARRIER_NOZZLE_ROTATE_LIMIT) {
                m_wMiscComponentAngle = HARRIER_NOZZLE_ROTATE_LIMIT;
            }
        }
    }

    // 0x6CB5EA
    vehicleFlags.bIsHandbrakeOn = false;
    m_GasPedal                  = 0.0f;

    if (fwdSpeed > 0.0f && CPad::GetPad(playerNum)->GetBrake()) {
        if (fwdSpeed > 0.35f) {
            m_BrakePedal = (float)CPad::GetPad(playerNum)->GetBrake() * (0.5f / 255.0f);
        } else {
            m_BrakePedal = (float)CPad::GetPad(playerNum)->GetBrake() * (1.0f / 255.0f);
        }
    } else if (m_vecMoveSpeed.Magnitude() < 0.1f && CPad::GetPad(playerNum)->GetBrake() < 10 && CPad::GetPad(playerNum)->GetAccelerate() < 10) {
        m_BrakePedal = 0.5f;
    } else {
        m_BrakePedal = 0.0f;
    }

    // 0x6CB6FC
    if (CPad::GetPad(playerNum)->DisablePlayerControls) {
        vehicleFlags.bIsHandbrakeOn = true;
        m_BrakePedal                = 1.0f;
        m_GasPedal                  = 0.0f;

        FindPlayerPed()->KeepAreaAroundPlayerClear();

        // Limit speed
        if (const auto speed = m_vecMoveSpeed.Magnitude(); speed > 0.28f) {
            m_vecMoveSpeed *= 0.28f / speed;
        }
    }
}

// 0x6CB7C0
void CPlane::ProcessFlyingCarStuff() {
    const auto timeStep = CTimer::GetTimeStep();
    if (timeStep <= 0.0f) {
        return;
    }

    // 0x6CB7E0
    {
        const auto rnd     = CGeneral::GetRandomNumberInRange(1.0f - PLANE_DAMAGE_WAVE_COUNTER_VAR, PLANE_DAMAGE_WAVE_COUNTER_VAR + 1.0f);
        const auto elapsed = (float)(uint32)(timeStep * 0.02f * 1000.0f);
        m_planeDamageWave += (int32)(rnd * elapsed);
    }

    const auto speedFactor = std::min(m_vecMoveSpeed.Magnitude() * 3.0f, 1.0f);

    auto yaw   = m_fLeftRightSkid;
    auto pitch = m_fSteeringUpDown;
    auto roll  = m_fSteeringLeftRight;

    // 0x6CB8CA - The player's plane is about to explode, so make it uncontrollable
    if (m_fHealth < 250.0f && GetStatus() == STATUS_PLAYER) {
        m_damageManager.SetAeroplaneCompStatus(PLANE_STATIC_PROP, DAMSTATE_DAMAGED);
        m_damageManager.SetAeroplaneCompStatus(PLANE_MOVING_PROP, DAMSTATE_DAMAGED);
        yaw += 0.5f;
        if (std::fabs(GetRoll()) < 2.3561945f) {
            roll += 0.75f;
        }
        if (std::fabs(GetRoll()) > HALF_PI) {
            pitch += 0.5f;
        }
    }

    // 0x6CB975 - Damaged components
    if (m_nModelIndex != MODEL_RCBARON) {
        for (auto node = (int32)PLANE_STATIC_PROP; node < (int32)PLANE_NUM_NODES; node++) {
            const auto status = (int32)m_damageManager.GetAeroplaneCompStatus((uint8)node);
            if (!m_aCarNodes[node] || status <= 0) {
                continue;
            }

            // NOTE: The game iterates 4 panels here, but `CAutomobile` has only 3 (So it reads into `m_swingingChassis`) - Not doing that.
            CBouncingPanel* panel = nullptr;
            for (auto& p : m_panels) {
                if ((int32)(int16)p.m_nFrameId == node) {
                    panel = &p;
                    break;
                }
            }

            const auto fStatus  = (float)status;
            const auto statusSq = (float)(status * status);

            // Shakes the given control, and the panel of the component too (if there's one)
            const auto ProcessDamagedControl = [&](float& control, float& outControl, float controlShake, float panelShake, float panelFeedback) {
                control *= 1.0f - fStatus * 0.2f;
                control += CGeneral::GetRandomNumberInRange(-controlShake, controlShake) * statusSq * timeStep * speedFactor;
                outControl = control;
                if (panel) {
                    panel->m_vecPos.y += CGeneral::GetRandomNumberInRange(-panelShake, panelShake) * statusSq * timeStep * speedFactor;
                    const auto feedback = panelFeedback * panel->m_vecRotation.y;
                    if (node == PLANE_AILERON_L) {
                        outControl += feedback;
                    } else {
                        outControl -= feedback;
                    }
                }
            };

            float smoke;
            switch (node) {
            case PLANE_STATIC_PROP:
            case PLANE_MOVING_PROP:
            case PLANE_STATIC_PROP2:
            case PLANE_MOVING_PROP2: { // 0x6CB9F1
                if (m_fAccelerationBreakStatus > 0.0f) {
                    const auto rnd  = CGeneral::GetRandomNumberInRange(1.0f - PLANE_DAMAGE_PROP_THRUST_VAR, PLANE_DAMAGE_PROP_THRUST_VAR + 1.0f);
                    const auto wave = std::sin((float)(uint32)m_planeDamageWave * (2.0f * PI) / (float)PLANE_DAMAGE_WAVE_PERIOD) - 1.0f;
                    m_fAccelerationBreakStatus += wave * rnd * fStatus * fStatus * timeStep * PLANE_DAMAGE_PROP_THRUST_MULT;
                }
                smoke = fStatus * 0.5f;
                break;
            }
            case PLANE_RUDDER: { // 0x6CBABA
                ProcessDamagedControl(m_fLeftRightSkid, yaw, PLANE_DAMAGE_RUDDER_CONTROL_SHAKE, PLANE_DAMAGE_RUDDER_PANEL_SHAKE, PLANE_DAMAGE_RUDDER_PANEL_FEEDBACK);
                smoke = fStatus * 0.2f;
                break;
            }
            case PLANE_ELEVATOR_L:
            case PLANE_ELEVATOR_R: { // 0x6CBBD7
                ProcessDamagedControl(m_fSteeringUpDown, pitch, PLANE_DAMAGE_ELEVATOR_CONTROL_SHAKE, PLANE_DAMAGE_ELEVATOR_PANEL_SHAKE, PLANE_DAMAGE_ELEVATOR_PANEL_FEEDBACK);
                smoke = fStatus * 0.15f;
                break;
            }
            case PLANE_AILERON_L:
            case PLANE_AILERON_R: { // 0x6CBCFC
                ProcessDamagedControl(m_fSteeringLeftRight, roll, PLANE_DAMAGE_AILERON_CONTROL_SHAKE, PLANE_DAMAGE_AILERON_PANEL_SHAKE, PLANE_DAMAGE_AILERON_PANEL_FEEDBACK);
                smoke = fStatus * 0.25f;
                break;
            }
            default:
                continue;
            }

            // 0x6CBE0B - Smoke from the damaged component
            if (smoke <= 0.0f || vehicleFlags.bIsDrowning) {
                continue;
            }
            if (!(speedFactor > 0.3f) && (rand() & 7) != 0) {
                continue;
            }
            if (!((GetPosition() - TheCamera.GetPosition()).SquaredMagnitude() < 6400.0f) && GetStatus() != STATUS_PLAYER) {
                continue;
            }
            const auto frame = m_aCarNodes[node];
            if (!frame) {
                continue;
            }

            CVector pos = m_matrix->TransformPoint(CVector{ RwFrameGetMatrix(frame)->pos });

            auto prtMult = FxPrtMult_c(0.0f, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, smoke);

            CVector vel{
                m_vecMoveSpeed.x * 0.25f * 50.0f,
                m_vecMoveSpeed.y * 0.25f * 50.0f,
                m_vecMoveSpeed.z * 0.25f * 50.0f
            };
            vel.x *= CGeneral::GetRandomNumberInRange(0.9f, 1.1f);
            vel.y *= CGeneral::GetRandomNumberInRange(0.9f, 1.1f);
            vel.z *= CGeneral::GetRandomNumberInRange(0.9f, 1.1f);

            prtMult.m_fLife = CGeneral::GetRandomNumberInRange(0.0f, 1.0f);

            pos.x += CGeneral::GetRandomNumberInRange(-smoke, smoke);
            pos.y += CGeneral::GetRandomNumberInRange(-smoke, smoke);
            pos.z += CGeneral::GetRandomNumberInRange(-smoke, smoke);

            g_fx.m_SmokeHuge->AddParticle(pos, vel, 0.0f, prtMult, -1.0f, 1.2f, 0.6f, false);
        }
    }

    // 0x6CC0EA - Propeller speed
    const auto status = GetStatus();
    if (status != STATUS_PLAYER && status != STATUS_REMOTE_CONTROLLED && status != STATUS_PHYSICS) {
        if (m_fPropSpeed <= timeStep * 0.001f) {
            m_fPropSpeed           = 0.0f;
            vehicleFlags.bEngineOn = false;
        } else if (m_fPropSpeed > PLANE_STD_PROP_SPEED) {
            m_nFakePhysics = 0;
            m_fPropSpeed -= timeStep * 0.003f;
        } else {
            m_nFakePhysics = 0;
            m_fPropSpeed -= timeStep * 0.001f;
        }
        return;
    }

    // 0x6CC1A6
    float targetPropSpeed = PLANE_STD_PROP_SPEED;
    if (m_fAccelerationBreakStatus > 0.0f) {
        targetPropSpeed = (PLANE_MAX_PROP_SPEED - PLANE_STD_PROP_SPEED) * m_fAccelerationBreakStatus + PLANE_STD_PROP_SPEED;
    } else if (m_fAccelerationBreakStatus < 0.0f) {
        targetPropSpeed = (PLANE_STD_PROP_SPEED - PLANE_MIN_PROP_SPEED) * m_fAccelerationBreakStatus + PLANE_STD_PROP_SPEED;
    }

    if (status == STATUS_PLAYER || status == STATUS_REMOTE_CONTROLLED) {
        const auto flightModel = m_nModelIndex != MODEL_RCBARON ? FLIGHT_MODEL_PLANE : FLIGHT_MODEL_RCPLANE;
        if (HeightAboveCeiling(GetPosition().z, flightModel) > 0.0f) {
            targetPropSpeed *= std::max(HeightAboveCeiling(GetPosition().z, flightModel) * 0.02f, 0.0f);
            m_fAccelerationBreakStatus = std::max(m_fAccelerationBreakStatus - HeightAboveCeiling(GetPosition().z, flightModel) * 0.04f, -1.0f);
        }
    }

    m_fPropSpeed += (targetPropSpeed - m_fPropSpeed) * timeStep * PLANE_ROC_PROP_SPEED;

    const bool isDrowning = vehicleFlags.bIsDrowning;
    if (isDrowning) {
        m_fPropSpeed               = 0.0f;
        m_fAccelerationBreakStatus = 0.0f;
        vehicleFlags.bEngineOn     = false;
    }

    // 0x6CC318
    if (m_nModelIndex == MODEL_RCBARON) {
        if (!isDrowning && vehicleFlags.bEngineOn) {
            FlyingControl(FLIGHT_MODEL_RCPLANE, yaw, pitch, roll, m_fAccelerationBreakStatus);
        }
        return;
    }

    if (!vehicleFlags.bEngineOn) {
        return;
    }
    if (!(m_fPropSpeed > PLANE_MIN_PROP_SPEED) && (double)m_vecMoveSpeed.SquaredMagnitude() <= 0.05) {
        return;
    }

    // 0x6CC3B1 - Hydra in hover mode uses the Hunter's flying handling
    if (m_nModelIndex == MODEL_HYDRA && status == STATUS_PLAYER && (int32)m_wMiscComponentAngle >= (int32)(int16)HARRIER_NOZZLE_SWITCH_LIMIT) {
        const auto savedFlyingHandling = m_pFlyingHandlingData;
        m_pFlyingHandlingData = gHandlingDataMgr.GetFlyingPointer((uint8)CModelInfo::GetVehicleModelInfo(MODEL_HUNTER)->m_nHandlingId);
        if (m_fAccelerationBreakStatus > 0.0f || (m_nNumContactWheels < 4 && !physicalFlags.bTouchingWater)) {
            FlyingControl(FLIGHT_MODEL_HELI, yaw, pitch, -roll, HARRIER_HOVER_THRUST_MULT * m_fAccelerationBreakStatus);
        }
        m_pFlyingHandlingData = savedFlyingHandling;
        return;
    }

    FlyingControl(FLIGHT_MODEL_PLANE, yaw, pitch, roll, m_fAccelerationBreakStatus);
}
