#include "StdInc.h"

#include "PedStuckChecker.h"
#include "Events/EventInWater.h"
#include "Events/EventStuckInAir.h"

void CPedStuckChecker::InjectHooks() {
    RH_ScopedClass(CPedStuckChecker);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(TestPedStuck, 0x602C00);
}

// 0x602C00
bool CPedStuckChecker::TestPedStuck(CPed* ped, CEventGroup* eventGroup) {
    const auto Reset = [this] {
        m_radius = 0;
        m_state  = PED_STUCK_STATE_NONE;
    };

    if (!ped->m_bUsesCollision || ped->m_pAttachedTo) {
        Reset();
        return false;
    }
    if (ped->m_nPedState == PEDSTATE_DIE || ped->m_nPedState == PEDSTATE_DEAD) {
        Reset();
        return false;
    }

    // 0x602F76 - Common tail
    const auto ProcessStuckState = [&]() -> bool {
        if (m_state == PED_STUCK_STATE_NONE) {
            return false;
        }
        if (const auto inWaterEvent = static_cast<CEventInWater*>(eventGroup->GetEventOfType(EVENT_IN_WATER))) {
            if (inWaterEvent->m_acceleration > 1.f) {
                ped->bIsStanding = false;
                return false;
            }
        }
        CEventStuckInAir event{ ped };
        eventGroup->Add(&event, false);
        return true;
    };

    const auto bCanBeStuck = [&] {
        if (ped->bIsStanding || ped->bWasStanding) {
            return false;
        }
        if (!(ped->m_pDamageEntity && ped->m_fDamageIntensity > 0.f)) {
            if (!ped->IsPlayer() || !ped->m_bIsStuck) {
                return false;
            }
        }
        const auto intel = ped->GetIntelligence();
        return !intel->GetTaskSwim() && !intel->GetTaskJetPack() && !intel->GetTaskClimb();
    }();
    if (!bCanBeStuck) {
        Reset();
        return ProcessStuckState();
    }

    // 0x602CE8
    CVector  moved{ 0.f, 0.f, 0.f };
    CEntity* hitEntity{};
    if (m_radius > 10'000 || m_radius == 0) {
        m_radius            = 1;
        m_state             = PED_STUCK_STATE_NONE;
        m_lastNonStuckPoint = ped->GetPosition();
    } else {
        moved = ped->GetPosition() - m_lastNonStuckPoint;
        m_radius++;
    }

    const auto timeStep = std::max(CTimer::GetTimeStep(), 0.01f);
    const auto counter  = (float)m_radius;

    // Push away from below by an object?
    const auto IsBeingPushedUpByObject = [&] {
        return ped->m_fDamageIntensity > 0.f
            && ped->m_pDamageEntity
            && ped->m_pDamageEntity->GetIsTypeObject()
            && ped->m_vecLastCollisionImpactVelocity.z > 0.3f;
    };

    // 0x602E40
    const auto ProcessStuckInAir = [&] {
        if (((uint8)ped->m_nRandomSeed + CTimer::m_FrameCounter + 3) & 7) {
            m_state = PED_STUCK_STATE_STUCK;
            return;
        }

        CVector origin = ped->GetPosition();
        origin.z += 1.f;

        CColPoint cp;
        if (!CWorld::ProcessVerticalLine(origin, ped->GetPosition().z - 1.f, cp, hitEntity, true, true, false, true, false, false, nullptr)) {
            return;
        }

        if (!ped->bHeadStuckInCollision || cp.m_vecPoint.z + 1.f < ped->GetPosition().z) {
            if (!IsBeingPushedUpByObject()) {
                origin.z = cp.m_vecPoint.z + 1.f;
                ped->SetPosn(origin);
                if (ped->bHeadStuckInCollision) {
                    ped->bHeadStuckInCollision = false;
                }
            }
        }

        ped->bIsStanding = true;
        Reset();
    };

    // 0x602FD8 - Stuck against a building, try to step sideways out of it
    const auto ProcessStuckOnBuilding = [&] {
        CVector side{ -moved.y, moved.x, 1.f };
        side.Normalise();

        CColPoint cpA, cpB;

        float groundZA = 5001.f;
        if (CWorld::ProcessVerticalLine(ped->GetPosition() + side, -20.f, cpA, hitEntity, true, false, false, false, false, false, nullptr)) {
            if (CWorld::GetIsLineOfSightClear(ped->GetPosition(), ped->GetPosition() + side, true, true, false, true, false, false, false)) {
                groundZA = cpA.m_vecPoint.z;
            }
        }

        float groundZB = 5002.f;
        if (CWorld::ProcessVerticalLine(ped->GetPosition() - side, -20.f, cpB, hitEntity, true, false, false, false, false, false, nullptr)) {
            if (CWorld::GetIsLineOfSightClear(ped->GetPosition(), ped->GetPosition() - side, true, true, false, true, false, false, false)) {
                groundZB = cpB.m_vecPoint.z;
            }
        }

        int16 chosen = 0;
        if (groundZA <= 5000.f || groundZB <= 5000.f) {
            const auto minZ = ped->GetPosition().z - 1.f;
            if (minZ < groundZA && groundZA < 5000.f && (groundZB < 5001.f || groundZA < groundZB)) {
                chosen = 1;
            } else if (minZ < groundZB && groundZB < 5001.f) {
                chosen = 2;
            }
            if (chosen != 0 && !ped->bHeadStuckInCollision && !IsBeingPushedUpByObject()) {
                ped->SetPosn(chosen == 2 ? cpB.m_vecPoint : cpA.m_vecPoint);
                ped->GetPosition().z += 1.f;
            }
        } else {
            chosen = -1;
        }

        if (groundZA < groundZB) {
            side *= -1.f;
        }
        side.z = 1.f;
        ped->ApplyMoveForce(CVector{ side.x * 4.f, side.y * 4.f, 4.f });

        if (chosen >= 0) {
            ped->SetPosn(ped->GetPosition() + side * 0.25f);

            const auto heading = CGeneral::GetRadianAngleBetweenPoints(side.x, side.y, 0.f, 0.f);
            ped->m_fCurrentRotation = heading;
            ped->m_fCurrentRotation = ped->m_fAimingRotation = CGeneral::LimitRadianAngle(heading);
            ped->SetOrientation(0.f, 0.f, ped->m_fCurrentRotation);
        }

        m_state = PED_STUCK_STATE_WAS_STUCK;
    };

    if (counter > 50.f / (4.f * timeStep) && counter * 0.01f > moved.SquaredMagnitude()) { // 0x602D9D
        ProcessStuckInAir();
    } else if (ped->m_nNumEntitiesCollided > 1) { // 0x602DD6
        if (counter > 50.f / (timeStep + timeStep) && counter * 0.004f > std::fabs(moved.z)) {
            ProcessStuckInAir();
        }
    } else if (ped->m_nNumEntitiesCollided == 1) { // 0x602FD8
        const auto collided = ped->m_apCollidedEntities[0];
        if (collided && collided->GetIsTypeBuilding()) {
            if (counter > 50.f / (timeStep + timeStep) && counter * 0.004f > std::fabs(moved.z)) {
                ProcessStuckOnBuilding();
            }
        }
    }

    return ProcessStuckState();
}
