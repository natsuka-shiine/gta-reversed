#include "StdInc.h"

#include "TaskSimpleDead.h"
#include "CarEnterExit.h"
#include "EventDeadPed.h"
#include "AccidentManager.h"
#include "Localisation.h"
#include "WaterLevel.h"
#include "Shadows.h"

void CTaskSimpleDead::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleDead, 0x86DEA4, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedVMTInstall(ProcessPed, 0x630600);
}

// NOTSA: *deathTime* originally int32
// 0x630590
CTaskSimpleDead::CTaskSimpleDead(uint32 deathTime, bool hasDrowned) :
    m_nDeathTimeMS{deathTime},
    m_bHasDrowned{hasDrowned}
{
}

// 0x636100
CTaskSimpleDead::CTaskSimpleDead(const CTaskSimpleDead& o) :
    CTaskSimpleDead{o.m_nDeathTimeMS, o.m_bHasDrowned}
{
}

// 0x630600
bool CTaskSimpleDead::ProcessPed(CPed* ped) {
    const auto IsTargetedBy = [ped](int32 playerId) {
        const auto player = FindPlayerPed(playerId);
        return player && player->m_pTargetedObject == ped;
    };

    auto isTargetedByPlayer = false;
    auto wasStanding        = false;

    if (m_bFirstTime) {
        if (ped->bInVehicle) {
            const auto door = CCarEnterExit::ComputeTargetDoorToExit(ped->m_pVehicle, ped);
            CAnimManager::BlendAnimation(
                ped->GetRpClump(),
                ANIM_GROUP_DEFAULT,
                (door == TARGET_DOOR_DRIVER || door == TARGET_DOOR_REAR_LEFT) ? ANIM_ID_CAR_DEAD_LHS : ANIM_ID_CAR_DEAD_RHS,
                4.0f
            );
        }

        ped->SetPedState(PEDSTATE_DEAD);
        m_bFirstTime = false;

        wasStanding        = ped->bIsStanding || ped->bWasStanding;
        isTargetedByPlayer = FindPlayerPed(0)->m_pTargetedObject == ped || IsTargetedBy(1);

        if (!m_bHasDrowned && !ped->m_standingOnEntity && !isTargetedByPlayer) {
            ped->m_bUsesCollision = false;
        }

        ped->m_fHealth = 0.0f;
        ped->RemoveWeaponModel(CWeaponInfo::GetWeaponInfo(ped->GetActiveWeapon().m_Type, eWeaponSkill::STD)->m_nModelId1);
        ped->m_nActiveWeaponSlot = 0;

        if (!ped->IsPlayer()) {
            ped->RemoveWeaponAnims(0, -1000.0f);
            ped->CreateDeadPedWeaponPickups();
            ped->CreateDeadPedMoney();
        }

        CEventDeadPed event{ ped, m_bHasDrowned, m_nDeathTimeMS };
        GetEventGlobalGroup()->Add(&event, false);
        CAccidentManager::GetInstance()->ReportAccident(ped);
    }

    // Figure out whenever the ped has to be aligned to the ground
    auto alignToGround = false;
    if (m_bFirstTime || !m_bHasDrowned || !ped->bIsStanding) {
        alignToGround = wasStanding;
        if (ped->m_bUsesCollision && !m_bHasDrowned && ped->bIsStanding && !ped->m_standingOnEntity) {
            if (FindPlayerPed(0)->m_pTargetedObject != ped) {
                const auto player1 = FindPlayerPed(1);
                if (player1 ? (player1->m_pTargetedObject != ped && !isTargetedByPlayer) : !isTargetedByPlayer) {
                    ped->m_bUsesCollision = false;
                    alignToGround         = true;
                }
            }
        }
    } else { // Drowned ped that reached the bottom
        ped->m_bUsesCollision = false;
        m_bHasDrowned         = false;
        const auto assoc = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, ANIM_ID_FLOOR_HIT_F, 8.0f);
        assoc->m_Flags &= ~ANIMATION_IS_FINISH_AUTO_REMOVE;
        alignToGround = true;
    }

    if (alignToGround) {
        const auto& mat          = ped->GetMatrix();
        const auto& groundNormal = ped->field_578;
        ped->m_pedIK.m_fSlopeRoll  = std::asin(std::clamp(DotProduct(groundNormal, mat.GetRight()), -1.0f, 1.0f));
        ped->m_pedIK.m_fSlopePitch = std::asin(std::clamp(DotProduct(groundNormal, mat.GetForward()), -1.0f, 1.0f));
    }

    ped->DeadPedMakesTyresBloody();

    if (CLocalisation::Blood() && !m_bHasDrowned) {
        constexpr auto BLOOD_POOL_DELAY_MS    = 2000u;
        constexpr auto BLOOD_POOL_GROW_TIME   = 5000u;
        constexpr auto BLOOD_POOL_GROW_RATE   = 0.00015f;    // 0x86DEC8
        constexpr auto BLOOD_POOL_MAX_RADIUS  = 0.75000006f; // = BLOOD_POOL_GROW_TIME * BLOOD_POOL_GROW_RATE

        const auto timeSinceDeath = CTimer::GetTimeInMS() - m_nDeathTimeMS;

        auto poolRadius = 0.0f;
        if (timeSinceDeath >= BLOOD_POOL_DELAY_MS) {
            poolRadius = timeSinceDeath <= BLOOD_POOL_DELAY_MS + BLOOD_POOL_GROW_TIME
                ? (float)(timeSinceDeath - BLOOD_POOL_DELAY_MS) * BLOOD_POOL_GROW_RATE
                : BLOOD_POOL_MAX_RADIUS;
        }

        // Peds walking thru the blood pool will leave bloody footprints
        const auto entities = ped->GetIntelligence()->GetPedEntities();
        for (auto i = 0; i < 16; i++) {
            const auto other = static_cast<CPed*>(entities[i]);
            if (!other) {
                continue;
            }
            if (sq(poolRadius) > (other->GetPosition() - ped->GetPosition()).SquaredMagnitude()) {
                other->m_nDeathTimeMS      = 200; // Reused as the bloody footprint counter
                other->bDoBloodyFootprints = true;
            }
        }

        if (timeSinceDeath > BLOOD_POOL_DELAY_MS && !m_bBloodPuddleCreated) {
            const auto poolTime = timeSinceDeath - BLOOD_POOL_DELAY_MS;
            auto       pos      = ped->GetPosition();

            // First frame of the puddle - don't create it if the ped is under water
            if (poolTime <= CTimer::GetTimeInMS() - CTimer::GetPreviousTimeInMS()) {
                float waterLevel;
                if (CWaterLevel::GetWaterLevelNoWaves(ped->GetPosition(), &waterLevel, nullptr, nullptr)) {
                    if (ped->GetPosition().z <= waterLevel) {
                        m_bBloodPuddleCreated = true;
                    }
                }
            }

            if (!m_bBloodPuddleCreated && CLocalisation::Blood()) {
                if (poolTime >= BLOOD_POOL_GROW_TIME) {
                    CShadows::AddPermanentShadow(
                        SHADOW_DEFAULT,
                        gpBloodPoolTex,
                        &pos,
                        BLOOD_POOL_MAX_RADIUS, 0.0f,
                        0.0f, -BLOOD_POOL_MAX_RADIUS,
                        255,
                        200, 0, 0,
                        4.0f,
                        40'000,
                        1.0f
                    );
                    m_bBloodPuddleCreated = true;
                } else {
                    const auto radius = (float)poolTime * BLOOD_POOL_GROW_RATE;
                    CShadows::StoreStaticShadow(
                        (uint32)this + 17,
                        SHADOW_DEFAULT,
                        gpBloodPoolTex,
                        pos,
                        radius, 0.0f,
                        0.0f, -radius,
                        255,
                        200, 0, 0,
                        4.0f,
                        1.0f,
                        40.0f,
                        false,
                        0.0f
                    );
                }
            }
        }
    }

    if (!m_bHasDrowned) {
        ped->m_pedIK.bSlopePitch = true;
        ped->m_vecMoveSpeed.Set(0.0f, 0.0f, 0.0f);
    } else {
        ped->bIsStanding  = false;
        ped->bWasStanding = false;
    }

    return false;
}
