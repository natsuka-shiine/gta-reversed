#include "StdInc.h"

#include "ProjectileInfo.h"

#include "Explosion.h"

#include "Entity/Object/Projectile.h"
#include "Explosion.h"
#include "AudioEngine.h"
#include "Radar.h"
#include "World.h"
#include "Pools/Pools.h"
#include "Collision/Box.h"

void CProjectileInfo::InjectHooks() {
    RH_ScopedClass(CProjectileInfo);
    RH_ScopedCategoryGlobal();

    // Install("CProjectileInfo", "", , &CProjectileInfo::);
    RH_ScopedInstall(Initialise, 0x737B40);
    RH_ScopedInstall(Shutdown, 0x737BC0);
    RH_ScopedInstall(GetProjectileInfo, 0x737BF0);
    RH_ScopedInstall(RemoveNotAdd, 0x737C00);
    RH_ScopedInstall(AddProjectile, 0x737C80);
    RH_ScopedInstall(RemoveDetonatorProjectiles, 0x738860);
    RH_ScopedInstall(RemoveProjectile, 0x7388F0);
    RH_ScopedInstall(Update, 0x738B20);
    RH_ScopedInstall(IsProjectileInRange, 0x739860);
    RH_ScopedInstall(RemoveAllProjectiles, 0x7399B0);
    RH_ScopedInstall(RemoveIfThisIsAProjectile, 0x739A40);
    RH_ScopedInstall(RemoveFXSystem, 0x737B80);
}

// 0x737B40
void CProjectileInfo::Initialise() {
    for (auto& projectile : ms_apProjectile) {
        projectile = nullptr;
    }

    for (auto& info : ms_aProjectileInfo) {
        info.m_nWeaponType  = WEAPON_GRENADE;
        info.m_pCreator     = nullptr;
        info.m_nDestroyTime = 0;
        info.m_bActive      = false;
        info.m_pFxSystem    = nullptr;
    }
}

// 0x737BC0
void CProjectileInfo::Shutdown() {
    for (auto& info : gaProjectileInfo) {
        if (info.m_pFxSystem) {
            g_fxMan.DestroyFxSystem(info.m_pFxSystem);
            info.m_pFxSystem = nullptr;
        }
    }
}

// 0x737BF0
CProjectileInfo* CProjectileInfo::GetProjectileInfo(int32 infoId) {
    return &gaProjectileInfo[infoId];
}

// 0x737C00
// Explodes a projectile that was never fired (e.g.: ped holding it died/weapon switched)
void CProjectileInfo::RemoveNotAdd(CEntity* creator, eWeaponType weaponType, CVector pos) {
    eExplosionType explosionType;
    switch (weaponType) {
    case WEAPON_GRENADE:
    case WEAPON_REMOTE_SATCHEL_CHARGE:
        explosionType = EXPLOSION_GRENADE;
        break;
    case WEAPON_MOLOTOV:
        explosionType = EXPLOSION_MOLOTOV;
        break;
    case WEAPON_ROCKET:
    case WEAPON_ROCKET_HS:
        explosionType = EXPLOSION_ROCKET;
        break;
    default:
        return;
    }
    CExplosion::AddExplosion(nullptr, creator, explosionType, pos, 0u, 1u, -1.f, 0u);
}

// 0x737C80
bool CProjectileInfo::AddProjectile(CEntity* creator, eWeaponType projectileType, CVector origin, float force, const CVector* dir, CEntity* target) {
    CMatrix mat;
    CVector velocity{}; // Uninitialized in the original (only matters for weapon types that aren't projectiles)
    uint32  destroyTime{};
    float   elasticity          = 0.75f;
    bool    applyGravity        = true;
    bool    useThrownObjectInfo = false;

    // Thrown projectiles: Launched in the direction the creator is facing
    const auto GetThrowSpeed = [&] {
        return force == 0.0f
            ? 0.0f
            : force * 0.22f + 0.15f;
    };
    const auto SetupThrow = [&](float heading, float speed, float upSpeedMult) {
        mat.SetTranslate(CVector{ 0.0f, 0.0f, 0.0f });
        mat.RotateZ(heading);
        mat.GetPosition() += origin;

        velocity.x = std::sin(heading) * speed * -1.0f;
        velocity.y = std::cos(heading) * speed;
        velocity.z = upSpeedMult * speed;
    };
    // Dropped projectiles: Inherit the matrix and the speed of the creator
    const auto SetupDrop = [&] {
        switch (creator->GetType()) {
        case ENTITY_TYPE_VEHICLE:
        case ENTITY_TYPE_PED:
        case ENTITY_TYPE_OBJECT:
            velocity = creator->AsPhysical()->m_vecMoveSpeed;
            break;
        default:
            velocity = CVector{ 0.0f, 0.0f, 0.0f };
            break;
        }
        mat = creator->GetMatrix();
        mat.GetPosition() = origin;
    };

    switch (projectileType) {
    case WEAPON_GRENADE:
    case WEAPON_REMOTE_SATCHEL_CHARGE: { // 0x737E02
        destroyTime = CTimer::GetTimeInMS() + 2'000;

        auto speed = GetThrowSpeed();
        if (projectileType == WEAPON_REMOTE_SATCHEL_CHARGE) {
            speed *= 0.5f;
        }

        auto heading = creator->GetHeading();
        if (creator->GetIsTypeVehicle()) {
            heading = CGeneral::LimitRadianAngle(heading + 3.14159274f); // 0x858CB8
        }

        SetupThrow(heading, speed, (force + 1.0f) * 0.4f);
        if (creator->m_nModelIndex == MODEL_SENTINEL) { // 0x737F41
            velocity += creator->AsPhysical()->m_vecMoveSpeed;
        }

        useThrownObjectInfo = true;
        elasticity          = projectileType == WEAPON_REMOTE_SATCHEL_CHARGE ? 0.03f : 0.5f;
        break;
    }
    case WEAPON_TEARGAS: { // 0x737CED
        destroyTime = CTimer::GetTimeInMS() + 20'000;
        SetupThrow(creator->GetHeading(), GetThrowSpeed(), (force + 1.0f) * 0.4f);
        useThrownObjectInfo = true;
        elasticity          = 0.5f;
        break;
    }
    case WEAPON_MOLOTOV: { // 0x737F87
        destroyTime = CTimer::GetTimeInMS() + 2'000;
        SetupThrow(creator->GetHeading(), std::max(force * 0.22f + 0.15f, 0.2f), force * 0.2f + 0.4f);
        break;
    }
    case WEAPON_ROCKET:
    case WEAPON_ROCKET_HS: { // 0x738086
        float speed;
        if (projectileType == WEAPON_ROCKET) {
            destroyTime = CTimer::GetTimeInMS() + 3'000;
            speed       = 0.4f;
        } else {
            destroyTime = CTimer::GetTimeInMS() + 10'000;
            speed       = 0.2f;
        }

        if (creator->GetIsTypeVehicle()) { // 0x7380C4
            mat = creator->GetMatrix();
            mat.GetPosition() = origin;
            speed += creator->AsPhysical()->m_vecMoveSpeed.Magnitude();
        } else if (creator->GetIsTypePed() && creator->AsPed()->IsPlayer()) { // 0x738122 - Fired where the camera is looking
            const auto& cam = TheCamera.m_aCams[TheCamera.m_nActiveCam];
            mat.GetForward()  = cam.m_vecFront;
            mat.GetUp()       = cam.m_vecUp;
            mat.GetRight()    = CrossProduct(cam.m_vecUp, cam.m_vecFront);
            mat.GetPosition() = origin;
        } else if (dir) { // 0x7381E2
            mat.GetForward()  = *dir;
            mat.GetRight()    = creator->GetMatrix().GetRight();
            mat.GetUp()       = CrossProduct(mat.GetRight(), mat.GetForward());
            mat.GetPosition() = origin;
        } else { // 0x738262 - NOTE: Position isn't set to `origin` in this case
            mat = creator->GetMatrix();
        }

        // 0x738273
        velocity     = mat.TransformVector(CVector{ 0.0f, speed, 0.0f });
        applyGravity = false;
        break;
    }
    case WEAPON_FREEFALL_BOMB: { // 0x7382CF
        destroyTime = CTimer::GetTimeInMS() + 2'000'000;
        SetupDrop();
        break;
    }
    case WEAPON_FLARE: { // 0x738303
        CStreaming::RequestModel(ModelIndices::MI_FLARE, 0);
        destroyTime = CTimer::GetTimeInMS() + 10'000;
        SetupDrop();
        break;
    }
    default:
        break;
    }

    // 0x73836A - Find a free slot
    auto slot = 0u;
    while (slot < MAX_PROJECTILES && gaProjectileInfo[slot].m_bActive) {
        slot++;
    }
    if (slot == MAX_PROJECTILES) {
        return false;
    }
    auto& info = gaProjectileInfo[slot];
    auto& proj = ms_apProjectile[slot];

    // 0x73839B - Create the object
    switch (projectileType) {
    case WEAPON_GRENADE:
    case WEAPON_TEARGAS:
    case WEAPON_MOLOTOV:
    case WEAPON_REMOTE_SATCHEL_CHARGE: { // 0x7383B5
        proj = new CProjectile(CWeaponInfo::GetWeaponInfo(projectileType, eWeaponSkill::STD)->m_nModelId1);
        if (!proj) {
            break;
        }

        // 0x738405 - Make sure the model has a collision sphere (So it can bounce)
        auto* const cm = CModelInfo::GetModelInfo(proj->m_nModelIndex)->GetColModel();
        if (auto* const cd = cm->m_pColData; !cd) {
            cm->AllocateData(1, 0, 0, 0, 0, false);
            cm->m_pColData->m_pSpheres->Set(cm->GetBoundRadius() * 0.75f, cm->GetBoundCenter(), SURFACE_GIRDER, 0, tColLighting{ 0xFF });
        } else if (cd->m_nNumSpheres == 0 && !cd->m_pSpheres) {
            cd->m_nNumSpheres = 1;
            cd->m_pSpheres    = static_cast<CColSphere*>(CMemoryMgr::Malloc(sizeof(CColSphere), 0));
            cd->m_pSpheres->Set(cm->GetBoundRadius() * 0.75f, cm->GetBoundCenter(), SURFACE_GIRDER, 0, tColLighting{ 0xFF });
        }
        break;
    }
    case WEAPON_ROCKET:
    case WEAPON_ROCKET_HS:
    case WEAPON_FREEFALL_BOMB: { // 0x7384A0, 0x7384E6
        proj = new CProjectile(CWeaponInfo::GetWeaponInfo(projectileType, eWeaponSkill::STD)->m_nModelId1);
        break;
    }
    case WEAPON_FLARE: { // 0x738515
        proj = new CProjectile(ModelIndices::MI_FLARE);
        if (proj) {
            proj->m_fAirResistance = 0.9f;
        }
        break;
    }
    default: // NOTE: Whatever was left in this slot is used
        break;
    }

    // 0x738562
    if (!proj) {
        return false;
    }

    // 0x73858F
    info.m_nWeaponType = projectileType;
    info.m_pCreator    = creator;
    creator->RegisterReference(&info.m_pCreator);

    proj->SetMatrix(mat);
    proj->m_vecMoveSpeed              = velocity;
    proj->physicalFlags.bApplyGravity = applyGravity;
    info.m_nDestroyTime               = (int32)destroyTime;
    proj->m_fElasticity               = elasticity;
    if (useThrownObjectInfo) {
        proj->m_pObjectInfo = &CObjectData::ms_aObjectInfo[4]; // 0xBB4BD0
    }

    // 0x738616
    info.m_pVictim = target;
    if (target) {
        target->RegisterReference(&info.m_pVictim);
    }

    info.m_bActive = true;
    CWorld::Add(proj);
    proj->RegisterReference(reinterpret_cast<CEntity**>(&proj));
    info.m_vecLastPosn = proj->GetPosition();

    // 0x738681
    if (projectileType == WEAPON_TEARGAS) {
        if (auto* const modellingMat = proj->GetModellingMatrix()) {
            info.m_pFxSystem = g_fxMan.CreateFxSystem("teargasAD", CVector{ 0.0f, 0.0f, 0.0f }, modellingMat, false);
            if (info.m_pFxSystem) {
                info.m_pFxSystem->Play();
            }
        }
    }

    // 0x7386DB
    proj->m_pEntityIgnoredCollision        = creator;
    proj->physicalFlags.bCanBeCollidedWith = true;
    if (creator) {
        switch (creator->GetType()) {
        case ENTITY_TYPE_VEHICLE:
        case ENTITY_TYPE_PED:
        case ENTITY_TYPE_OBJECT: {
            auto* const physical = creator->AsPhysical();
            if (!physical->m_pEntityIgnoredCollision) {
                physical->m_pEntityIgnoredCollision = creator; // Yes, itself
            }
            break;
        }
        default:
            break;
        }
    }

    // 0x738716
    if (projectileType == WEAPON_ROCKET_HS) {
        const auto blip = CRadar::SetEntityBlip(BLIP_OBJECT, GetObjectPool()->GetRef(proj), 0xFF0000FF, BLIP_DISPLAY_BLIPONLY);
        CRadar::ChangeBlipScale(blip, 1);
        CRadar::ChangeBlipColour(
            blip,
            creator == FindPlayerPed() || creator == FindPlayerVehicle()
                ? static_cast<eBlipColour>(0xFFFFFFFF)
                : static_cast<eBlipColour>(0xFF0000FF)
        );
    }

    // 0x738792
    AudioEngine.ReportWeaponEvent(AE_PROJECTILE_FIRE, projectileType, proj);

    return true;
}

// 0x738860
void CProjectileInfo::RemoveDetonatorProjectiles() {
    for (auto i = 0u; i < MAX_PROJECTILES; i++) {
        auto& info   = gaProjectileInfo[i];
        auto* object = ms_apProjectile[i];
        if (!info.m_bActive || info.m_nWeaponType != WEAPON_REMOTE_SATCHEL_CHARGE) {
            continue;
        }
        CExplosion::AddExplosion(nullptr, info.m_pCreator, EXPLOSION_GRENADE, object->GetPosition(), 0, true, -1.0f, false);
        info.m_bActive = false;
        if (info.m_pFxSystem) {
            info.m_pFxSystem->Kill();
            info.m_pFxSystem = nullptr;
        }
        object->m_bRemoveFromWorld = true;
    }
}

// 0x7388F0
void CProjectileInfo::RemoveProjectile(CProjectileInfo* info, CProjectile* object) {
    const CVector pos = object->GetPosition();
    switch (static_cast<eWeaponType>(info->m_nWeaponType)) {
    case WEAPON_GRENADE:
    case WEAPON_FREEFALL_BOMB:
        CExplosion::AddExplosion(nullptr, info->m_pCreator, EXPLOSION_GRENADE, pos, 0, true, -1.0f, false);
        break;
    case WEAPON_MOLOTOV:
        CExplosion::AddExplosion(nullptr, info->m_pCreator, EXPLOSION_MOLOTOV, pos, 0, true, -1.0f, false);
        AudioEngine.ReportObjectDestruction(object);
        break;
    case WEAPON_ROCKET: {
        CEntity* creator = info->m_pCreator;
        if (creator && creator->GetType() == ENTITY_TYPE_VEHICLE) {
            creator = creator->AsVehicle()->GetDriver();
        }
        CExplosion::AddExplosion(nullptr, creator, EXPLOSION_ROCKET, pos, 0, true, -1.0f, false);
        break;
    }
    case WEAPON_ROCKET_HS:
        if (info->m_pCreator == FindPlayerPed()) {
            CExplosion::AddExplosion(nullptr, info->m_pCreator, EXPLOSION_ROCKET, pos, 0, true, -1.0f, false);
        } else {
            CExplosion::AddExplosion(nullptr, info->m_pCreator, EXPLOSION_WEAK_ROCKET, pos, 0, true, -1.0f, false);
        }
        break;
    default:
        break;
    }
    info->m_bActive = false;
    if (info->m_pFxSystem) {
        info->m_pFxSystem->Kill();
        info->m_pFxSystem = nullptr;
    }
    CRadar::ClearBlipForEntity(BLIP_OBJECT, GetObjectPool()->GetRef(object));
    CWorld::Remove(object);
    delete object;
}

// 0x738B20
void CProjectileInfo::Update() {
    if (CReplay::Mode == MODE_PLAYBACK) {
        return;
    }

    for (auto i = 0u; i < MAX_PROJECTILES; i++) {
        auto&       info = gaProjectileInfo[i];
        auto* const proj = ms_apProjectile[i];

        if (!info.m_bActive) {
            continue;
        }

        // 0x738B64
        if (proj->physicalFlags.bSubmergedInWater && info.m_pFxSystem) {
            info.m_pFxSystem->Kill();
            info.m_pFxSystem = nullptr;
        }

        // 0x738B7B
        if (info.m_pCreator && info.m_pCreator->GetIsTypePed() && !info.m_pCreator->AsPed()->IsPointerValid()) {
            info.m_pCreator = nullptr;
        }

        // NOTSA: The original code keeps on using the projectile after `RemoveProjectile` has deleted it:
        // It re-enables it's collision (0x739809) and reads it's position into `m_vecLastPosn` (0x739812).
        // Neither is done here if the projectile was removed. (The info is inactive by then, and `AddProjectile` sets the position)
        bool removed = false;
        const auto Remove = [&] {
            RemoveProjectile(&info, proj);
            removed = true;
        };

        // Checks if there's nothing between the last and the current position of the projectile
        const auto IsPathClear = [&] {
            return CWorld::GetIsLineOfSightClear(info.m_vecLastPosn, proj->GetPosition(), true, true, true, true, false, false, false);
        };

        // 0x738B97
        switch (info.m_nWeaponType) {
        case WEAPON_REMOTE_SATCHEL_CHARGE:
        case WEAPON_GRENADE:
        case WEAPON_TEARGAS: {
            // Stop it from bouncing forever
            if (   proj->m_fElasticity > 0.1f
                && std::abs(proj->m_vecMoveSpeed.x) < 0.05f
                && std::abs(proj->m_vecMoveSpeed.y) < 0.05f
                && std::abs(proj->m_vecMoveSpeed.z) < 0.05f
            ) {
                proj->m_fElasticity = 0.03f;
            }

            // 0x738BFF
            if (info.m_nWeaponType == WEAPON_TEARGAS && CTimer::GetTimeInMS() > (uint32)info.m_nDestroyTime - 17'500u) {
                if (CGeneral::GetRandomNumberInRange(0, 100) < 10) {
                    const auto& pos = proj->GetPosition();
                    CWorld::SetPedsChoking(pos.x, pos.y, pos.z, 6.0f, info.m_pCreator);
                }
            }
            break;
        }
        }

        // 0x738C63 - Smoke trail
        if (info.m_nWeaponType == WEAPON_ROCKET || info.m_nWeaponType == WEAPON_ROCKET_HS) {
            FxPrtMult_c prtMult{ 0.3f, 0.3f, 0.3f, 0.3f, 0.5f, 1.0f, 0.08f };

            const auto step         = proj->m_vecMoveSpeed * CTimer::GetTimeStep();
            const auto numParticles = std::max(1, (int32)step.Magnitude());
            for (auto n = 0; n < numParticles; n++) {
                const auto brightness = (float)CGeneral::GetRandomNumber() * RAND_MAX_FLOAT_RECIPROCAL * 0.25f + 0.25f;
                prtMult.m_Color.blue  = brightness;
                prtMult.m_Color.green = brightness;
                prtMult.m_Color.red   = brightness;
                prtMult.m_fLife       = (float)CGeneral::GetRandomNumber() * RAND_MAX_FLOAT_RECIPROCAL * 0.04f + 0.08f;

                const auto t   = 1.0f - (float)n / (float)numParticles;
                const auto pos = proj->GetPosition() - step * t;

                CVector randomDir;
                randomDir.x = (float)CGeneral::GetRandomNumber() * RAND_MAX_FLOAT_RECIPROCAL * 2.0f - 1.0f;
                randomDir.y = (float)CGeneral::GetRandomNumber() * RAND_MAX_FLOAT_RECIPROCAL * 2.0f - 1.0f;
                randomDir.z = (float)CGeneral::GetRandomNumber() * RAND_MAX_FLOAT_RECIPROCAL * 2.0f - 1.0f;
                randomDir.Normalise();

                auto moveDir = proj->m_vecMoveSpeed;
                moveDir.Normalise();

                const auto vel = CrossProduct(moveDir, randomDir) * 1.5f;
                g_fx.m_SmokeHuge->AddParticle(pos, vel, 0.0f, prtMult, -1.0f, 1.2f, 0.6f, false);
            }
        }

        // Limit the speed of rockets
        const auto LimitMoveSpeed = [&] {
            if (const auto speed = proj->m_vecMoveSpeed.Magnitude(); speed > 9.9f) {
                proj->m_vecMoveSpeed *= 9.9f / speed;
            }
        };

        // 0x7396BB
        const auto ProcessRocketCollision = [&] {
            if (!proj->physicalFlags.bOnSolidSurface) {
                CWorld::pIgnoreEntity = info.m_pCreator;
                proj->SetUsesCollision(false);
                const auto isClear = IsPathClear();
                CWorld::pIgnoreEntity = nullptr;
                proj->SetUsesCollision(true);
                proj->m_pEntityIgnoredCollision = info.m_pCreator;
                if (isClear) {
                    return;
                }
            }

            // 0x739090 - Explode, unless it has only collided with it's creator or with another missile
            if (proj->m_nNumEntitiesCollided) {
                if (const auto* const collided = proj->m_apCollidedEntities[0]) {
                    if (collided == info.m_pCreator || collided->m_nModelIndex == MODEL_MISSILE) {
                        return;
                    }
                }
            }
            Remove();
        };

        if (CTimer::GetTimeInMS() > (uint32)info.m_nDestroyTime && info.m_nDestroyTime != 0) { // 0x738F22 - Time is up
            if (info.m_nWeaponType != WEAPON_REMOTE_SATCHEL_CHARGE) {
                Remove();
            } else if (info.m_pCreator->GetIsTypePed() && info.m_pCreator->AsPed()->IsPlayer()) { // 0x738F3A
                // The satchels stay for as long as the player has the detonator
                auto* const ped = info.m_pCreator->AsPed();
                if (   ped->GetWeaponInSlot(ped->GetWeaponSlot(WEAPON_DETONATOR)).GetType() != WEAPON_DETONATOR
                    || ped->GetWeaponInSlot(ped->GetWeaponSlot(WEAPON_DETONATOR)).GetTotalAmmo() == 0
                ) {
                    info.m_nDestroyTime = 0;
                }
            }
        } else {
            switch (info.m_nWeaponType) {
            case WEAPON_ROCKET: { // 0x738FA4
                proj->m_vecMoveSpeed += proj->GetMatrix().GetForward() * (CTimer::GetTimeStep() * 0.008f);
                LimitMoveSpeed();
                ProcessRocketCollision();
                break;
            }
            case WEAPON_FLARE: { // 0x7390D1
                proj->SetUsesCollision(false);
                CWorld::pIgnoreEntity = info.m_pCreator;
                const auto isClear = IsPathClear();
                proj->SetUsesCollision(true);
                CWorld::pIgnoreEntity = nullptr;
                if (!isClear) {
                    proj->m_vecMoveSpeed = CVector{ 0.0f, 0.0f, 0.0f };
                    proj->SetPosn(info.m_vecLastPosn);
                }
                break;
            }
            case WEAPON_MOLOTOV:
            case WEAPON_FREEFALL_BOMB: { // 0x739745
                const auto pos = proj->GetPosition();
                CWorld::pIgnoreEntity = info.m_pCreator;
                proj->SetUsesCollision(false);
                if (!info.m_pCreator || (info.m_vecLastPosn - info.m_pCreator->GetPosition()).SquaredMagnitude() >= 2.0f) {
                    if (proj->physicalFlags.bOnSolidSurface || !CWorld::GetIsLineOfSightClear(info.m_vecLastPosn, pos, true, true, true, true, false, false, false)) {
                        Remove();
                    }
                }
                CWorld::pIgnoreEntity = nullptr;
                if (!removed) {
                    proj->SetUsesCollision(true);
                }
                break;
            }
            case WEAPON_ROCKET_HS: { // 0x7391B6
                if (info.m_pVictim) {
                    if (info.m_pVictim == FindPlayerVehicle()) {
                        AudioEngine.ReportFrontendAudioEvent(AE_MISSILE_LOCK, 0.0f, 1.0f);
                    }

                    // 0x7391E7 - Check if there's a flare that's a better target than the victim
                    const auto forward     = proj->GetMatrix().GetForward();
                    const auto evalOrigin  = proj->GetPosition() + forward;
                    const auto victimScore = CWeapon::EvaluateTargetForHeatSeekingMissile(info.m_pVictim, evalOrigin, forward, 1.2f, true, nullptr);

                    float    bestFlareScore = 0.0f;
                    CEntity* bestFlare      = nullptr;
                    for (auto k = 0u; k < MAX_PROJECTILES; k++) {
                        if (gaProjectileInfo[k].m_nWeaponType != WEAPON_FLARE || !gaProjectileInfo[k].m_bActive) {
                            continue;
                        }
                        const auto score = CWeapon::EvaluateTargetForHeatSeekingMissile(ms_apProjectile[k], evalOrigin, forward, 1.2f, true, nullptr);
                        if (!(score < bestFlareScore)) {
                            bestFlareScore = score;
                            bestFlare      = ms_apProjectile[k];
                        }
                    }

                    // 0x7392F3
                    auto* const target = (bestFlare && bestFlareScore > victimScore ? bestFlare : info.m_pVictim)->AsPhysical();

                    // 0x739311 - Missiles of the player are way better at following planes
                    bool isPlayerVsPlane = false;
                    if (target->GetIsTypeVehicle()) {
                        if (info.m_pCreator == FindPlayerPed() || info.m_pCreator == FindPlayerVehicle()) {
                            if (target->AsVehicle()->m_nVehicleSubType == VEHICLE_TYPE_PLANE) {
                                isPlayerVsPlane = true;
                            }
                        }
                    }

                    // 0x739355
                    const auto aimPos = isPlayerVsPlane
                        ? proj->GetPosition()
                        : proj->GetPosition() + proj->m_vecMoveSpeed * 100.0f;

                    const auto& targetPos    = target->GetPosition();
                    const auto  targetDist   = (proj->GetPosition() - targetPos).Magnitude();
                    const auto  predictSteps = isPlayerVsPlane
                        ? std::min(targetDist, 1.5f)
                        : std::min(targetDist, 50.0f);
                    const auto  predictedPos = targetPos + target->m_vecMoveSpeed * predictSteps;

                    // 0x7394A3
                    auto steerDir = predictedPos - aimPos;
                    auto moveDir  = proj->m_vecMoveSpeed;
                    moveDir.Normalise();
                    if (const auto dot = DotProduct(steerDir, moveDir); dot < 0.0f) {
                        steerDir -= moveDir * dot;
                    }
                    steerDir.Normalise();

                    // 0x739547
                    auto accel = info.m_pCreator == FindPlayerPed() || info.m_pCreator == FindPlayerVehicle()
                        ? 0.0117f
                        : 0.009f;
                    if (target->m_vecMoveSpeed.Magnitude() > 0.8f) {
                        accel *= 1.2f;
                    }
                    auto speedMult = 1.0f;
                    if (isPlayerVsPlane) { // 0x7395C7
                        speedMult = std::pow(0.95f, CTimer::GetTimeStep()); // 0x8D6104
                        accel     = 0.15f;                                  // 0x8D6108
                    }

                    // 0x7395EC
                    proj->m_vecMoveSpeed *= speedMult;
                    proj->m_vecMoveSpeed += steerDir * (accel * CTimer::GetTimeStep());
                    LimitMoveSpeed();

                    proj->GetMatrix().GetForward() = moveDir;
                }
                ProcessRocketCollision();
                break;
            }
            case WEAPON_REMOTE_SATCHEL_CHARGE: { // 0x7396FB - Stick to whatever it has hit
                if (proj->m_fDamageIntensity > 0.0f && proj->m_pDamageEntity && !proj->m_pAttachedTo) {
                    proj->AttachEntityToEntity(proj->m_pDamageEntity->AsPhysical(), static_cast<CVector*>(nullptr), static_cast<CQuaternion*>(nullptr));
                    proj->SetUsesCollision(false);
                }
                break;
            }
            }
        }

        // 0x739812
        if (!removed) {
            info.m_vecLastPosn = proj->GetPosition();
        }
    }
}

// 0x739860
bool CProjectileInfo::IsProjectileInRange(float x1, float x2, float y1, float y2, float z1, float z2, bool bDestroy) {
    const CBox bb{
        CVector{ x1, y1, z1 },
        CVector{ x2, y2, z2 }
    };
    bool found = false;
    for (auto&& [info, proj] : rngv::zip(gaProjectileInfo, ms_apProjectile)) {
        if (!info.m_bActive) {
            continue;
        }

        if (!IsWeaponTypeProjectile(static_cast<eWeaponType>(info.m_nWeaponType))) {
            continue;
        }

        if (!bb.IsPointInside(proj->GetPosition())) {
            continue;
        }

        found = true;
        if (bDestroy) {
            info.m_bActive = false;
            info.RemoveFXSystem(false);
            CRadar::ClearBlipForEntity(BLIP_OBJECT, GetObjectPool()->GetRef(proj));
            CWorld::Remove(proj);
            delete proj;
        }
    }
    return found;
}

// 0x7399B0
void CProjectileInfo::RemoveAllProjectiles() {
    for (auto i = 0u; i < MAX_PROJECTILES; i++) {
        auto& info   = gaProjectileInfo[i];
        auto* object = ms_apProjectile[i];
        if (!info.m_bActive) {
            continue;
        }
        info.m_bActive = false;
        if (info.m_pFxSystem) {
            g_fxMan.DestroyFxSystem(info.m_pFxSystem);
            info.m_pFxSystem = nullptr;
        }
        CRadar::ClearBlipForEntity(BLIP_OBJECT, GetObjectPool()->GetRef(object));
        CWorld::Remove(object);
        delete object;
    }
}

// 0x739A40
bool CProjectileInfo::RemoveIfThisIsAProjectile(CObject* object) {
    for (auto&& [info, projectile] : rngv::zip(ms_aProjectileInfo, ms_apProjectile)) {
        if (projectile != object || !info.m_bActive) {
            continue;
        }

        info.m_bActive = false;

        if (info.m_pFxSystem) {
            info.m_pFxSystem->Kill();
            info.m_pFxSystem = nullptr;
        }

        CRadar::ClearBlipForEntity(BLIP_OBJECT, GetObjectPool()->GetRef(projectile));
        CWorld::Remove(projectile);
        delete projectile;
        projectile = nullptr;
        return true;
    }
    return false;
}

// 0x737B80
void CProjectileInfo::RemoveFXSystem(bool bInstantly) {
    if (!m_pFxSystem) {
        return;
    }
    if (bInstantly) {
        g_fxMan.DestroyFxSystem(m_pFxSystem);
    } else {
        m_pFxSystem->Kill();
    }
    m_pFxSystem = nullptr;
}
