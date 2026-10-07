#include "StdInc.h"

#include "CollisionEventScanner.h"

#include "PedDamageResponseCalculator.h"
#include "PedGeometryAnalyser.h"
#include "PedClothesDesc.h"
#include "Tasks/TaskTypes/TaskSimpleGoTo.h"
#include "Events/EventDamage.h"
#include "Events/EventVehicleCollision.h"
#include "Events/EventPedCollisionWithPed.h"
#include "Events/EventPedCollisionWithPlayer.h"
#include "Events/EventPlayerCollisionWithPed.h"
#include "Events/EventObjectCollision.h"
#include "Events/EventBuildingCollision.h"
#include "Events/EventAcquaintancePedHate.h"
#include "Events/EventSoundQuiet.h"
#include "Events/EventGlobalGroup.h"

// Tunables (All in `.data`, so they're referenced instead of copied)
static inline auto& s_BumpSoundLoudIntensity          = StaticRef<float, 0x8D239C>(); // 3.0
static inline auto& s_BumpSoundMinIntensity           = StaticRef<float, 0x8D23A0>(); // 1.0
static inline auto& s_DoorSoundMinTurnSpeed           = StaticRef<float, 0x8D23A4>(); // 0.04
static inline auto& s_ObjectCollisionDamageMult       = StaticRef<float, 0x8D23A8>(); // 10.0
static inline auto& s_ObjectCollisionDamageThreshPlyr = StaticRef<float, 0x8D23AC>(); // 2.0
static inline auto& s_ObjectCollisionDamageThresh     = StaticRef<float, 0x8D23B0>(); // 1.0

void CCollisionEventScanner::InjectHooks() {
    RH_ScopedClass(CCollisionEventScanner);
    RH_ScopedCategory("Collision");

    RH_ScopedInstall(ScanForCollisionEvents, 0x604500);
}

// 0x604500
void CCollisionEventScanner::ScanForCollisionEvents(CPed* victim, CEventGroup* eventGroup) {
    static auto& s_LastBumpSoundEvent = StaticRef<uint32, 0xC0B1B0>();
    static auto& s_LastDoorSoundEvent = StaticRef<uint32, 0xC0B1B4>();

    auto* const ped = victim;

    if (ped->m_pDamageEntity && ped->m_fDamageIntensity != 0.0f) {
        const auto moveState = [&]() -> eMoveState {
            const auto task = ped->GetTaskManager().GetSimplestActiveTask();
            return task && CTask::IsGoToTask(task)
                ? static_cast<CTaskSimpleGoTo*>(task)->m_moveState
                : PEDMOVE_STILL;
        }();

        const auto ApplyRammedByCarDamage = [&](CVehicle* veh) {
            const auto hitSide = CPedGeometryAnalyser::ComputePedHitSide(*ped, *veh);
            CPedDamageResponseCalculator calculator{ veh, ped->m_fDamageIntensity, WEAPON_RAMMEDBYCAR, PED_PIECE_TORSO, false };
            CEventDamage                 eventDamage{ veh, CTimer::GetTimeInMS(), WEAPON_RAMMEDBYCAR, PED_PIECE_TORSO, (uint8)hitSide, false, ped->bInVehicle };
            if (eventDamage.AffectsPed(ped)) {
                calculator.ComputeDamageResponse(ped, eventDamage.m_damageResponse, true);
            }
        };

        // Damage intensity reduced by how much the ped is moving away from the thing it collided with
        const auto GetIntensityAdjustedForPedMovement = [&] {
            const auto dot = ped->m_vecLastCollisionImpactVelocity.x * ped->m_vecAnimMovingShiftLocal.x
                           + ped->m_vecLastCollisionImpactVelocity.y * ped->m_vecAnimMovingShiftLocal.y;
            return dot < 0.0f
                ? std::max(0.0f, dot * ped->m_fMass + ped->m_fDamageIntensity)
                : ped->m_fDamageIntensity;
        };

        auto* const damageEntity = ped->m_pDamageEntity;
        switch (damageEntity->GetType()) {
        case ENTITY_TYPE_BUILDING: {
            CEventBuildingCollision event{
                (int16)ped->m_nPieceType,
                ped->m_fDamageIntensity,
                static_cast<CBuilding*>(damageEntity),
                ped->m_vecLastCollisionImpactVelocity,
                ped->m_vecLastCollisionPosn,
                (int16)moveState
            };
            eventGroup->Add(&event);
            break;
        }
        case ENTITY_TYPE_VEHICLE: {
            if (m_bAlreadyHitByCar) {
                break;
            }

            auto* const veh     = static_cast<CVehicle*>(damageEntity);
            const auto  speedSq = veh->m_vecMoveSpeed.SquaredMagnitude();

            if (veh->m_nVehicleSubType == VEHICLE_TYPE_TRAIN
                && veh->physicalFlags.bDisableCollisionForce
                && ped->m_bIsStuck
                && ped->bIsStanding
                && !ped->m_standingOnEntity
                && speedSq > 0.0001f
            ) {
                ped->KillPedWithCar(veh, 15.0f, false);
            }

            const auto IsPedAttachedToVehicle = [&] {
                return ped->m_pAttachedTo && ped->m_pAttachedTo->GetIsTypeVehicle();
            };

            if (speedSq <= sq(0.05f)) { // Vehicle is (almost) stationary
                if (IsPedAttachedToVehicle()) {
                    ApplyRammedByCarDamage(veh);
                } else if (!ped->IsPlayer()) {
                    CEventVehicleCollision event{
                        (int16)ped->m_nPieceType,
                        ped->m_fDamageIntensity,
                        static_cast<CVehicle*>(ped->m_pDamageEntity),
                        ped->m_vecLastCollisionImpactVelocity,
                        ped->m_vecLastCollisionPosn,
                        (int8)ped->GetIntelligence()->GetMoveStateFromGoToTask(),
                        VEHICLE_EVADE_NONE
                    };
                    eventGroup->Add(&event);
                }
                break;
            }

            auto intensity = ped->bIsStanding
                ? GetIntensityAdjustedForPedMovement()
                : ped->m_fDamageIntensity;

            if (!ped->IsPlayer()) {
                if (IsPedAttachedToVehicle()) {
                    ApplyRammedByCarDamage(veh);
                } else {
                    ped->KillPedWithCar(veh, ped->m_fDamageIntensity, false); // NOTE: Not using the adjusted intensity
                }
                break;
            }

            intensity = std::min(intensity, 20.0f);

            const auto  vehHeading = veh->GetHeading();
            const auto  bbMin      = veh->GetColModel()->GetBoundingBox().m_vecMin;
            const auto  bbMax      = veh->GetColModel()->GetBoundingBox().m_vecMax;
            const auto  bbCenter   = veh->GetMatrix().TransformPoint((bbMin + bbMax) / 2.0f);
            const auto  vehToPed   = bbCenter - ped->GetPosition();
            const auto  relAngle   = CGeneral::LimitRadianAngle(vehHeading - std::atan2(-vehToPed.x, vehToPed.y));
            const float cornerAngle = std::atan2(bbMax.x - bbMin.x, bbMax.y - bbMin.y);

            auto pedDir = ped->GetPosition() - veh->GetPosition();
            pedDir.Normalise();

            float speedTowardsPed;
            if (std::abs(relAngle) < cornerAngle || std::abs(relAngle) > PI - cornerAngle) { // Ped is in front of/behind the vehicle
                speedTowardsPed = pedDir.Dot(veh->m_vecMoveSpeed);
            } else if (relAngle > 0.0f) {
                speedTowardsPed = (veh->GetRight() * -1.0f).Dot(veh->m_vecMoveSpeed);
            } else {
                // NOTE: The Windows build has a few more checks here (speedSq > 0.01, speed < 0.1, dot of the vehicle's right
                // and the ped's forward < 0), but their only effect is a write to a dead stack variable, so they're omitted.
                speedTowardsPed = veh->GetRight().Dot(veh->m_vecMoveSpeed);
            }

            if (speedTowardsPed > 0.1f) {
                ped->KillPedWithCar(veh, intensity, false);
            }
            break;
        }
        case ENTITY_TYPE_PED: {
            auto* const otherPed       = static_cast<CPed*>(damageEntity);
            const auto  otherMoveState = otherPed->GetIntelligence()->GetMoveStateFromGoToTask();

            if (ped->IsPlayer()) {
                CEventPlayerCollisionWithPed event{
                    (int16)ped->m_nPieceType,
                    ped->m_fDamageIntensity,
                    static_cast<CPed*>(ped->m_pDamageEntity),
                    ped->m_vecLastCollisionImpactVelocity,
                    ped->m_vecLastCollisionPosn,
                    moveState,
                    otherMoveState
                };
                eventGroup->Add(&event);
                otherPed->Say(CTX_GLOBAL_BUMP);
            } else if (otherPed->IsPlayer()) {
                CEventPedCollisionWithPlayer event{
                    (int16)ped->m_nPieceType,
                    ped->m_fDamageIntensity,
                    static_cast<CPed*>(ped->m_pDamageEntity),
                    ped->m_vecLastCollisionImpactVelocity,
                    ped->m_vecLastCollisionPosn,
                    moveState,
                    otherMoveState
                };
                eventGroup->Add(&event);
                ped->GetIntelligence()->IncrementAngerAtPlayer(1);
                ped->Say(CTX_GLOBAL_BUMP);
                if (ped->GetAcquaintance().GetAcquaintances(ACQUAINTANCE_HATE) & CPedType::GetPedFlag(otherPed->m_nPedType)) {
                    CEventAcquaintancePedHate hateEvent{ otherPed };
                    eventGroup->Add(&hateEvent);
                }
            } else {
                CEventPedCollisionWithPed event{
                    (int16)ped->m_nPieceType,
                    ped->m_fDamageIntensity,
                    static_cast<CPed*>(ped->m_pDamageEntity),
                    ped->m_vecLastCollisionImpactVelocity,
                    ped->m_vecLastCollisionPosn,
                    moveState,
                    otherMoveState
                };
                eventGroup->Add(&event);
            }

            // If the other ped didn't register the collision, also give it the (mirrored) event
            if (!otherPed->m_pDamageEntity) {
                const CVector impactVelocity = ped->m_vecLastCollisionImpactVelocity * -1.0f;
                auto&         otherGroup     = otherPed->GetIntelligence()->GetEventGroup();

                // NOTE: Move states aren't swapped in the original code either
                if (otherPed->IsPlayer()) {
                    CEventPlayerCollisionWithPed event{
                        (int16)ped->m_nPieceType,
                        ped->m_fDamageIntensity,
                        ped,
                        impactVelocity,
                        ped->m_vecLastCollisionPosn,
                        moveState,
                        otherMoveState
                    };
                    otherGroup.Add(&event);
                } else if (ped->IsPlayer()) {
                    CEventPedCollisionWithPlayer event{
                        (int16)ped->m_nPieceType,
                        ped->m_fDamageIntensity,
                        ped,
                        impactVelocity,
                        ped->m_vecLastCollisionPosn,
                        moveState,
                        otherMoveState
                    };
                    otherGroup.Add(&event);
                    otherPed->GetIntelligence()->IncrementAngerAtPlayer(1);
                    if (otherPed->GetAcquaintance().GetAcquaintances(ACQUAINTANCE_HATE) & CPedType::GetPedFlag(ped->m_nPedType)) {
                        CEventAcquaintancePedHate hateEvent{ ped };
                        otherGroup.Add(&hateEvent);
                    }
                } else {
                    CEventPedCollisionWithPed event{
                        (int16)ped->m_nPieceType,
                        ped->m_fDamageIntensity,
                        ped,
                        impactVelocity,
                        ped->m_vecLastCollisionPosn,
                        moveState,
                        otherMoveState
                    };
                    otherGroup.Add(&event);
                }
            }
            break;
        }
        case ENTITY_TYPE_OBJECT: {
            auto* const obj       = static_cast<CObject*>(damageEntity);
            const bool  bStanding = ped->bIsStanding;

            const auto intensity = bStanding && !obj->GetIsStatic()
                ? GetIntensityAdjustedForPedMovement()
                : ped->m_fDamageIntensity;

            const auto threshold = ped->GetPlayerData()
                ? s_ObjectCollisionDamageThreshPlyr
                : s_ObjectCollisionDamageThresh;

            if (intensity <= threshold
                || obj->GetIsStatic()
                || !bStanding
                || obj == ped->m_pContactEntity
                || (obj->m_pAttachedTo && obj == obj->m_pAttachedTo)
            ) {
                CEventObjectCollision event{
                    (int16)ped->m_nPieceType,
                    ped->m_fDamageIntensity,
                    obj,
                    ped->m_vecLastCollisionImpactVelocity,
                    ped->m_vecLastCollisionPosn,
                    (int16)moveState
                };
                eventGroup->Add(&event);
            } else {
                const auto     damage = intensity / threshold * s_ObjectCollisionDamageMult;
                const CVector2D dir{ -ped->m_vecLastCollisionImpactVelocity.x, -ped->m_vecLastCollisionImpactVelocity.y };
                CWeapon::GenerateDamageEvent(
                    ped,
                    ped->m_pDamageEntity,
                    WEAPON_FALL,
                    (int32)damage,
                    PED_PIECE_TORSO,
                    (uint8)ped->GetLocalDirection(dir)
                );
                ped->m_pEntityIgnoredCollision = ped->m_pDamageEntity;
            }
            break;
        }
        default:
            break;
        }

        // Noise made by the player bumping into stuff while burgling
        if (const auto* const playerData = ped->GetPlayerData()) {
            auto* const entity = ped->m_pDamageEntity;
            if ((entity->GetIsTypeBuilding() || entity->GetIsTypeObject()) && playerData->m_pPedClothesDesc->GetIsWearingBalaclava()) {
                const auto AddSoundEvent = [&](float loudness) {
                    CEventSoundQuiet event{ ped, loudness, (uint32)-1, CVector{ 0.0f, 0.0f, 0.0f } };
                    GetEventGlobalGroup()->Add(&event);
                };

                const auto now = CTimer::GetTimeInMS();
                if (entity->GetIsTypePhysical() && static_cast<CPhysical*>(entity)->physicalFlags.bDisableMoveForce) { // Doors
                    if (std::abs(static_cast<CPhysical*>(entity)->m_vecTurnSpeed.z) > s_DoorSoundMinTurnSpeed) {
                        if (now > s_LastDoorSoundEvent + 2000) {
                            s_LastDoorSoundEvent = now;
                            AddSoundEvent(40.0f);
                        }
                    }
                } else if (ped->m_fDamageIntensity > s_BumpSoundMinIntensity) {
                    if (now > s_LastBumpSoundEvent + 1000) {
                        const auto loudness = ped->m_fDamageIntensity > s_BumpSoundLoudIntensity ? 40.0f : 30.0f;
                        s_LastBumpSoundEvent = now;
                        if (loudness > 0.0f) {
                            AddSoundEvent(loudness);
                        }
                    }
                }
            }
        }
    }

    m_bAlreadyHitByCar = false;
}
