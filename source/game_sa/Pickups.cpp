/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "PostEffects.h"
#include "Pickups.h"
#include "Garages.h"
#include "tPickupMessage.h"
#include "Radar.h"
#include "Shadows.h"
#include "Coronas.h"
#include "TaskSimpleJetPack.h"

using namespace ModelIndices;
static int32 GenerateNewOne_Hook(CVector coors, uint32 modelId, ePickupType pickupType, uint32 ammo, uint32 moneyPerDay, bool isEmpty, char* message);

void CPickups::InjectHooks() {
    RH_ScopedClass(CPickups);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x454A70);
    RH_ScopedInstall(ReInit, 0x456E60);
    RH_ScopedInstall(AddToCollectedPickupsArray, 0x455240);
    RH_ScopedOverloadedInstall(CreatePickupCoorsCloseToCoors, "", 0x458A80, void(*)(float, float, float, float&, float&, float&));
    RH_ScopedInstall(CreateSomeMoney, 0x458970);
    RH_ScopedInstall(DetonateMinesHitByGunShot, 0x4590C0);
    RH_ScopedInstall(DoCollectableEffects, 0x455E20);
    RH_ScopedInstall(DoMineEffects, 0x4560E0);
    RH_ScopedInstall(DoMoneyEffects, 0x454E80);
    RH_ScopedInstall(DoPickUpEffects, 0x455720);
    RH_ScopedInstall(FindPickUpForThisObject, 0x4551C0);

    // NOTE: A wrapper is used, because the game's function returns a 32 bit value in `eax`, while `tPickupReference` would be returned through a hidden pointer
    RH_ScopedNamedGlobalInstall(GenerateNewOne_Hook, "GenerateNewOne", 0x456F20);

    RH_ScopedInstall(GenerateNewOne_WeaponType, 0x457380);
    RH_ScopedInstall(GetActualPickupIndex, 0x4552A0);
    RH_ScopedInstall(GetNewUniquePickupIndex, 0x456A30);
    RH_ScopedInstall(GetUniquePickupIndex, 0x455280);
    RH_ScopedInstall(GivePlayerGoodiesWithPickUpMI, 0x4564F0);
    RH_ScopedInstall(IsPickUpPickedUp, 0x454B40);
    RH_ScopedInstall(ModelForWeapon, 0x454AC0);
    RH_ScopedInstall(PassTime, 0x455200);
    RH_ScopedInstall(PickedUpHorseShoe, 0x455390);
    RH_ScopedInstall(PickedUpOyster, 0x4552D0);
    RH_ScopedInstall(PictureTaken, 0x456A70);
    RH_ScopedInstall(PlayerCanPickUpThisWeaponTypeAtThisMoment, 0x4554C0);
    RH_ScopedInstall(RemoveMissionPickUps, 0x456DE0);
    RH_ScopedInstall(RemovePickUp, 0x4573D0);
    RH_ScopedInstall(RemovePickUpsInArea, 0x456D30);
    RH_ScopedInstall(RemovePickupObjects, 0x455470);
    RH_ScopedInstall(RemoveUnnecessaryPickups, 0x4563A0);
    RH_ScopedInstall(RenderPickUpText, 0x455000);
    RH_ScopedInstall(TestForPickupsInBubble, 0x456450);
    RH_ScopedInstall(TryToMerge_WeaponType, 0x4555A0);
    RH_ScopedInstall(Update, 0x458DE0);
    RH_ScopedInstall(UpdateMoneyPerDay, 0x455680);
    RH_ScopedInstall(WeaponForModel, 0x454AE0);
    RH_ScopedInstall(Load, 0x5D35A0);
    RH_ScopedInstall(Save, 0x5D3540);

    RH_ScopedGlobalInstall(ModifyStringLabelForControlSetting, 0x454B70);
}

// 0x454A70
void CPickups::Init() {
    ZoneScoped;

    NumMessages = 0;
    for (auto& pickup : aPickUps) {
        pickup.m_nPickupType = PICKUP_NONE;
        pickup.m_nReferenceIndex = 1;
        pickup.m_pObject = nullptr;
    }
    aPickUpsCollected.fill(0);
    CollectedPickUpIndex = 0u;
    DisplayHelpMessage = 10u;
}

// 0x456E60
void CPickups::ReInit() {
    rng::for_each(GetAllActivePickups(), [](auto& pickup) { pickup.Remove(); });
    Init();
}

// 0x455240
void CPickups::AddToCollectedPickupsArray(int32 pickupIndex) {
    aPickUpsCollected[CollectedPickUpIndex++] = GetUniquePickupIndex(pickupIndex).num;
    CollectedPickUpIndex %= std::size(aPickUpsCollected);
}

/*!
 * @addr 0x458A80
 * @brief Created a pickup close to pos (inX, inY, inZ)
 *
 * @param [out] outX, outY, outZ Created pickup's position
 */
void CPickups::CreatePickupCoorsCloseToCoors(float inX, float inY, float inZ, float& outX, float& outY, float& outZ) {
    // Binary verb (S_0x458A80.txt): up to 32 random candidates on a 1.5-unit ring around
    // the input; each is lifted to ground + 0.5. Past 16 failures any clear-LOS candidate
    // wins, otherwise (matches TestSphereAgainstWorld(_, 1.2, nullptr, false, false, true,
    // false, false, false) == nullptr + no other pickup within 1.3 + 2.0 clear of the
    // player) the candidate must also win. Fallback: input x/y, input z + 0.4.
    for (auto tries = 0; tries < 32; tries++) {
        const auto angle = static_cast<float>(CGeneral::GetRandomNumber() & 0xFF) * (2.0f * 3.14159265f / 256.0f);
        const CVector candidate{ inX + 1.5f * std::sinf(angle), inY + 1.5f * std::cosf(angle), inZ };
        bool foundGround{};
        const CVector ground{
            candidate.x,
            candidate.y,
            CWorld::FindGroundZFor3DCoord(candidate, &foundGround, nullptr) + 0.5f,
        };
        if (!foundGround)
            continue;
        const auto relaxed = tries > 0x10;
        // Binary: dist(candidate.xz -> player.xz) must EXCEED 2.0; when it does and we are
        // past 16 failures any clear-LOS candidate wins, otherwise the candidate must also
        // have no other pickup within 1.3 and a clear 1.2 test-sphere.
        if ((candidate - FindPlayerCoors()).Magnitude2D() <= 2.0f) {
            if (!relaxed)
                continue;
        } else if (!relaxed && TestForPickupsInBubble(ground, 1.3f)) {
            continue;
        }
        if (!CWorld::GetIsLineOfSightClear({ inX, inY, inZ + 0.3f }, ground - CVector{ 0.0f, 0.0f, 0.4f }, true, relaxed, false, relaxed, false, false, false))
            continue;
        if (!relaxed && CWorld::TestSphereAgainstWorld(ground, 1.2f, nullptr, false, false, true, false, false, false))
            continue;
        outX = candidate.x;
        outY = candidate.y;
        outZ = ground.z;
        return;
    }
    outX = inX;
    outY = inY;
    outZ = inZ + 0.4f;
}

/*!
 * @addr 0x458970
 * @brief Creates wads of money that is worth amount of money at the position coors.
 */
void CPickups::CreateSomeMoney(CVector coors, int32 amount) {
    const auto wads = std::min(amount / 20 + 1, 7);
    const auto perWad = amount / wads;

    for (auto i = 0; i < wads; i++) {
        bool result;
        coors.x += std::sinf(CGeneral::GetRandomNumberInRange(0.f, TWO_PI)) * 1.5f;
        coors.y += std::cosf(CGeneral::GetRandomNumberInRange(0.f, TWO_PI)) * 1.5f;
        coors.z = CWorld::FindGroundZFor3DCoord(coors, &result, nullptr) + 0.5f;

        if (result) {
            GenerateNewOne(coors, MI_MONEY, PICKUP_MONEY, perWad + (CGeneral::GetRandomNumber() % 4));
        }
    }
}

// 0x4590C0
void CPickups::DetonateMinesHitByGunShot(const CVector& shotOrigin, const CVector& shotTarget) {
    for (auto& pickup : aPickUps) {
        if (pickup.m_nPickupType == PICKUP_NAUTICAL_MINE_ARMED) {
            pickup.ProcessGunShot(shotOrigin, shotTarget);
        }
    }
}

// 0x455E20
void CPickups::DoCollectableEffects(CEntity* entity) {
    const auto& entityPos = entity->GetPosition();

    if (const auto d = DistanceBetweenPoints(TheCamera.GetPosition(), entityPos); d < 14.0f) {
        // shade of gray
        const auto t = (uint8)((std::sinf((float)(((uint16)std::bit_cast<uintptr_t>(entity) + (uint16)CTimer::GetTimeInMS()) % 2048) * 0.0030664064f) + 1.0f) / 2.0f * ((14.0f - d) / 14.0f) * 255.0f);

        CShadows::StoreStaticShadow(
            (uint32)entity,
            SHADOW_ADDITIVE,
            gpShadowExplosionTex,
            entityPos,
            2.0f,
            0.0f,
            0.0f,
            -2.0f,
            0,
            t,
            t,
            t,
            4.0f,
            1.0f,
            40.0f,
            false,
            0.0f
        );
        CCoronas::RegisterCorona(
            (uint32)entity,
            nullptr,
            t,
            t,
            t,
            255,
            entityPos,
            0.6f,
            40.0f,
            CORONATYPE_TORUS,
            FLARETYPE_NONE,
            eCoronaReflType::CORREFL_NONE,
            eCoronaLOSCheck::LOSCHECK_OFF,
            eCoronaTrail::TRAIL_OFF,
            0.0f,
            false,
            1.5f,
            0,
            15.0f,
            false,
            false
        );
    }

    entity->GetMatrix().SetRotateZOnly(static_cast<float>(CTimer::GetTimeInMS() % 4096) * 0.0015283204f);
}

// 0x4560E0
void CPickups::DoMineEffects(CEntity* entity) {
    const auto& entityPos = entity->GetPosition();

    if (const auto d = DistanceBetweenPoints(TheCamera.GetPosition(), entityPos); d < 20.0f) {
        // shade of red
        const auto t = (uint8)((std::sinf((float)(((uint16)std::bit_cast<uintptr_t>(entity) + (uint16)CTimer::GetTimeInMS()) % 512) * 0.012265625f) + 1.0f) / 2.0f * ((20.0f - d) / 20.0f) * 64.0f);

        CShadows::StoreStaticShadow(
            (uint32)entity,
            SHADOW_ADDITIVE,
            gpShadowExplosionTex,
            entityPos,
            2.0f,
            0.0f,
            0.0f,
            -2.0f,
            0,
            t,
            0,
            0,
            4.0f,
            1.0f,
            40.0f,
            false,
            0.0f
        );
        CCoronas::RegisterCorona(
            (uint32)entity,
            nullptr,
            t,
            0,
            0,
            255,
            entityPos,
            0.6f,
            40.0f,
            CORONATYPE_TORUS,
            FLARETYPE_NONE,
            eCoronaReflType::CORREFL_NONE,
            eCoronaLOSCheck::LOSCHECK_OFF,
            eCoronaTrail::TRAIL_OFF,
            0.0f,
            false,
            1.5f,
            0,
            15.0f,
            false,
            false
        );
    }

    entity->GetMatrix().SetRotateZOnly(static_cast<float>(CTimer::GetTimeInMS() % 1024) * 0.0061132815f);
}

// 0x454E80
void CPickups::DoMoneyEffects(CEntity* entity) {
    entity->GetMatrix().SetRotateZOnly(static_cast<float>(CTimer::GetTimeInMS() % 2048) * 0.0030566407f);
}

// 0x455720
void CPickups::DoPickUpEffects(CEntity* entity) {
    constexpr uint32 OBJ_FLAG_INVISIBLE = 0x2000000; // TODO: Name this flag in `CObject` (bit 25 of `m_nObjectFlags`)

    auto* const obj    = entity->AsObject();
    auto* const pickup = FindPickUpForThisObject(obj);

    if (entity->m_nModelIndex != ModelIndices::MI_PICKUP_CAMERA) {
        if (pickup->PickUpShouldBeInvisible()) {
            obj->m_nObjectFlags |= OBJ_FLAG_INVISIBLE;
        } else {
            obj->m_nObjectFlags &= ~OBJ_FLAG_INVISIBLE;
        }
    } else if (TheCamera.m_aCams[TheCamera.m_nActiveCam].m_nMode == MODE_CAMERA) {
        obj->m_nObjectFlags &= ~OBJ_FLAG_INVISIBLE;
    } else {
        obj->m_nObjectFlags |= OBJ_FLAG_INVISIBLE;
        if (CClock::GetGameClockHours() < 5 || CPostEffects::IsVisionFXActive()) {
            const auto red     = 100 - (int32)((float)(CGeneral::GetRandomNumber() & 0xFFFF) * (1.0f / 32768.0f) * -50.0f);
            const auto size    = (std::sin((float)(CTimer::GetTimeInMS() & 0x1FFF) * 0.000766601588f) + 1.7f) * 3.7f;
            const auto greenBlue = (int32)((float)red * 0.7f);
            CCoronas::RegisterCorona(
                reinterpret_cast<uint32>(&entity) - 3, // NOTE: The original code uses an address on the stack too (not the entity's address)
                nullptr,
                (uint8)red, (uint8)greenBlue, (uint8)greenBlue, 255,
                entity->GetPosition(),
                size,
                100.0f,
                CORONATYPE_HEADLIGHT,
                static_cast<eCoronaFlareType>(0),
                static_cast<eCoronaReflType>(0),
                static_cast<eCoronaLOSCheck>(0),
                static_cast<eCoronaTrail>(0),
                0.0f,
                false,
                1.5f,
                false,
                15.0f,
                false,
                false
            );
        }
    }

    if (obj->m_nObjectFlags & OBJ_FLAG_INVISIBLE) {
        return;
    }

    // Index into the color table below
    // NOTE: Uninitialized in the original code for the bribe/info/killfrenzy/property/savegame pickups
    int32 colorIdx = 0;
    {
        using namespace ModelIndices;
        const auto model = entity->m_nModelIndex;
        if (model == MI_PICKUP_ADRENALINE) {
            colorIdx = 47;
        } else if (model == MI_PICKUP_BODYARMOUR) {
            colorIdx = 48;
        } else if (model == MI_PICKUP_BRIBE || model == MI_PICKUP_INFO || model == MI_PICKUP_KILLFRENZY) {
            /* nop */
        } else if (model == MI_PICKUP_HEALTH || model == MI_PICKUP_BONUS) {
            colorIdx = 47;
        } else if (model == MI_PICKUP_PROPERTY) {
            /* nop */
        } else if (model == MI_PICKUP_PROPERTY_FORSALE) {
            colorIdx = 47;
        } else if (model == MI_PICKUP_REVENUE) {
            colorIdx = 53;
        } else if (model == MI_PICKUP_SAVEGAME) {
            /* nop */
        } else if (model == MI_PICKUP_CLOTHES) {
            colorIdx = 47;
        } else {
            colorIdx = (int32)WeaponForModel(model);
        }
    }

    // Pickup message (price, etc)
    if ((obj->m_nObjectFlags & 0xC) || obj->m_nBonusValue || obj->m_wCostValue) {
        const auto dist2D = (TheCamera.GetPosition() - entity->GetPosition()).Magnitude2D();
        if (dist2D < 14.0f && NumMessages < MAX_PICKUP_MESSAGES) {
            struct tPickupColor { // TODO: Proper name + move it to the header
                uint8 r, g, b;
                uint8 pad[5];
            };
            static const auto& s_PickupColors = StaticRef<tPickupColor[54]>(0x8A5FB0);

            const CVector textPos = entity->GetPosition() + CVector{ 0.0f, 0.0f, 0.7f };
            RwV3d         screenPos;
            float         width, height;
            if (CSprite::CalcScreenCoors(textPos, &screenPos, &width, &height, true, true)) {
                auto& msg = aMessages[NumMessages];
                msg.pos.x = screenPos.x;
                msg.pos.y = screenPos.y;
                *reinterpret_cast<int32*>(&msg.pos.z) = (int32)WeaponForModel(entity->m_nModelIndex); // The `z` component is actually used as the weapon type
                msg.width  = width;
                msg.height = height;
                msg.color  = CRGBA{
                    s_PickupColors[colorIdx].r,
                    s_PickupColors[colorIdx].g,
                    s_PickupColors[colorIdx].b,
                    (uint8)(int32)((1.0f - dist2D * (1.0f / 14.0f)) * 255.0f)
                };
                if (obj->m_nObjectFlags & 8) {
                    msg.flags |= 1;
                } else {
                    msg.flags &= ~1;
                }
                msg.field_19 = (char)obj->m_nBonusValue;
                msg.price    = (uint32)obj->m_wCostValue * 5;

                const auto GetPropertyText = [&] {
                    return const_cast<GxtChar*>(TheText.Get(CPickup::FindStringForTextIndex(
                        static_cast<ePickupPropertyText>(FindPickUpForThisObject(obj)->m_nFlags.nPropertyTextIndex)
                    )));
                };
                if (entity->m_nModelIndex == ModelIndices::MI_PICKUP_PROPERTY) {
                    msg.text   = GetPropertyText();
                    msg.flags &= ~2;
                } else if (entity->m_nModelIndex == ModelIndices::MI_PICKUP_PROPERTY_FORSALE) {
                    msg.text   = GetPropertyText();
                    msg.flags |= 2;
                } else {
                    msg.text   = nullptr;
                    msg.flags &= ~2;
                }
                NumMessages++;
            }
        }
    }

    // Scale (so that all pickups are roughly the same size) + rotate
    const auto& bb     = CModelInfo::GetModelInfo(entity->m_nModelIndex)->GetColModel()->m_boundBox;
    const auto  extent = std::max({
        bb.m_vecMax.x - bb.m_vecMin.x,
        bb.m_vecMax.y - bb.m_vecMin.y,
        bb.m_vecMax.z - bb.m_vecMin.z,
    });
    auto scale = (std::max(1.2f / extent, 1.0f) - 1.0f) * 0.6f + 1.0f;
    if (entity->m_nModelIndex == MODEL_MINIGUN) {
        scale = 1.2f;
    }
    const auto angle = (float)(CTimer::GetTimeInMS() & 0x7FF) * 0.00305664074f;
    const auto c     = std::cos(angle) * scale;
    const auto s     = std::sin(angle) * scale;

    auto& mat = entity->GetMatrix();
    mat.GetRight()   = CVector{ c, s, 0.0f };
    mat.GetForward() = CVector{ -s, c, 0.0f };
    mat.GetUp()      = CVector{ 0.0f, 0.0f, scale };
}

// 0x4551C0
CPickup* CPickups::FindPickUpForThisObject(CObject* object) {
    for (auto& pickup : GetAllActivePickups()) {
        if (pickup.m_pObject == object) {
            return &pickup;
        }
    }
    return aPickUps.data();
}

// returns pickup handle
// g
// 0x456F20
tPickupReference CPickups::GenerateNewOne(CVector coors, uint32 modelId, ePickupType pickupType, uint32 ammo, uint32 moneyPerDay, bool isEmpty, char* message) {
    const auto FindFirstOfType = [](std::initializer_list<ePickupType> types) -> int32 {
        for (auto&& [i, pickup] : rngv::enumerate(aPickUps)) {
            if (rng::find(types, pickup.m_nPickupType) != types.end()) {
                return (int32)i;
            }
        }
        return -1;
    };

    int32 idx = -1;

    // These kind of pickups are allocated from the end of the array
    if (pickupType == PICKUP_FLOATINGPACKAGE || pickupType == PICKUP_NAUTICAL_MINE_INACTIVE || isEmpty) {
        for (auto i = (int32)aPickUps.size() - 1; i >= 0; i--) {
            if (aPickUps[i].m_nPickupType == PICKUP_NONE) {
                idx = i;
                break;
            }
        }
    }

    if (idx == -1) {
        idx = FindFirstOfType({ PICKUP_NONE });
        if (idx == -1) { // No free slot, try reusing one of the less important ones
            idx = FindFirstOfType({ PICKUP_MONEY });
            if (idx == -1) {
                idx = FindFirstOfType({ PICKUP_ONCE_TIMEOUT, PICKUP_ONCE_TIMEOUT_SLOW });
                if (idx == -1) {
                    return tPickupReference{ -1 };
                }
            }
            if (auto*& obj = aPickUps[idx].m_pObject) {
                CWorld::Remove(obj);
                delete obj;
                obj = nullptr;
            }
        }
    }

    auto&      pickup = aPickUps[idx];
    const auto timeMs = CTimer::GetTimeInMS();

    pickup.m_nAmmo                        = ammo;
    pickup.m_nMoneyPerDay                 = (uint16)moneyPerDay;
    pickup.m_nPickupType                  = pickupType;
    pickup.m_fRevenueValue                = 0.0f;
    pickup.m_nRegenerationTime            = timeMs;
    pickup.m_nFlags.bDisabled             = false;
    pickup.m_nFlags.bEmpty                = isEmpty;
    pickup.m_nFlags.bHelpMessageDisplayed = false;

    switch (pickupType) {
    case PICKUP_ONCE_TIMEOUT:
        pickup.m_nRegenerationTime = timeMs + 20'000;
        break;
    case PICKUP_ONCE_TIMEOUT_SLOW:
        pickup.m_nRegenerationTime = timeMs + 120'000;
        break;
    case PICKUP_MONEY:
        pickup.m_nRegenerationTime = timeMs + 30'000;
        break;
    case PICKUP_MINE_INACTIVE:
    case PICKUP_MINE_ARMED:
        pickup.m_nPickupType       = PICKUP_MINE_INACTIVE;
        pickup.m_nRegenerationTime = timeMs + 1'500;
        break;
    case PICKUP_NAUTICAL_MINE_INACTIVE:
    case PICKUP_NAUTICAL_MINE_ARMED:
        pickup.m_nPickupType       = PICKUP_NAUTICAL_MINE_INACTIVE;
        pickup.m_nRegenerationTime = timeMs + 1'500;
        break;
    default:
        break;
    }

    pickup.m_nModelIndex               = (int16)modelId;
    pickup.m_nFlags.nPropertyTextIndex = CPickup::FindTextIndexForString(message);
    pickup.SetPosn(coors);
    pickup.m_nFlags.bVisible           = pickup.IsVisible();
    pickup.m_pObject                   = nullptr;
    if (pickup.m_nFlags.bVisible) {
        pickup.GiveUsAPickUpObject(pickup.m_pObject, -1);
        if (pickup.m_pObject) {
            CWorld::Add(pickup.m_pObject);
        }
    }

    if ((uint16)pickup.m_nReferenceIndex >= 0xFFFE) {
        pickup.m_nReferenceIndex = 1;
    } else {
        pickup.m_nReferenceIndex++;
    }
    return tPickupReference{ (int32)(((uint32)(uint16)pickup.m_nReferenceIndex << 16) | (uint32)idx) };
}

// The game's function returns a 32 bit value in `eax`, but `tPickupReference` isn't trivial, so it would be returned through a hidden pointer => A wrapper has to be used for the hook
static int32 GenerateNewOne_Hook(CVector coors, uint32 modelId, ePickupType pickupType, uint32 ammo, uint32 moneyPerDay, bool isEmpty, char* message) {
    return CPickups::GenerateNewOne(coors, modelId, pickupType, ammo, moneyPerDay, isEmpty, message).num;
}

/*!
 *
 * @param coors Position of new pickup
 * @param weaponType Weapon type
 * @param pickupType Pickup type
 * @param ammo
 * @param isEmpty
 * @param message
 * @return Pickup handle
 * @addr 0x457380
 */
tPickupReference CPickups::GenerateNewOne_WeaponType(CVector coors, eWeaponType weaponType, ePickupType pickupType, uint32 ammo, bool isEmpty, char* message) {
    return GenerateNewOne(coors, CWeaponInfo::GetWeaponInfo(weaponType)->m_nModelId1, pickupType, ammo, 0u, isEmpty, message);
}

/*!
 * @param pickupIndex Index of pickup
 * @return -1 if this index is not actual
 * @addr 0x4552A0
 */
int32 CPickups::GetActualPickupIndex(tPickupReference pickupRef) {
    if (pickupRef.num == -1)
        return -1;

    if (pickupRef.refIndex != aPickUps.at(pickupRef.index).m_nReferenceIndex)
        return -1;

    return pickupRef.index;
}

// 0x456A30
tPickupReference CPickups::GetNewUniquePickupIndex(int32 pickupIndex) {
    auto& refIdx = aPickUps[pickupIndex].m_nReferenceIndex;
    refIdx = (refIdx == -1) ? 1 : refIdx + 1;

    return GetUniquePickupIndex(pickupIndex);
}

// returns pickup handle
// 0x455280
tPickupReference CPickups::GetUniquePickupIndex(int32 pickupIndex) {
    return tPickupReference(pickupIndex, aPickUps.at(pickupIndex).m_nReferenceIndex);
}

// returns TRUE if player got goodies
// 0x4564F0
bool CPickups::GivePlayerGoodiesWithPickUpMI(uint16 modelId, int32 playerId) {
    auto* ped = FindPlayerPed(playerId);

    if (modelId == MI_PICKUP_ADRENALINE) {
        ped->GetPlayerData()->m_bAdrenaline = true;
        ped->GetPlayerData()->m_nAdrenalineEndTime = CTimer::GetTimeInMS() + 20'000;
        ped->ResetSprintEnergy();
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_ADRENALINE);
        return true;
    }

    if (modelId == MI_PICKUP_BODYARMOUR) {
        ped->m_fArmour = (float)FindPlayerInfo(playerId).m_nMaxArmour;
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_BODY_ARMOUR);
        return true;
    }

    if (modelId == MI_PICKUP_INFO) {
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_INFO);
        return true;
    }

    if (modelId == MI_PICKUP_HEALTH) {
        auto maxHealth = FindPlayerInfo(playerId).m_nMaxHealth;
        CStats::UpdateStatsAddToHealth((uint32)((float)maxHealth - ped->m_fHealth));
        ped->m_fHealth = (float)maxHealth;
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_HEALTH);
        return true;
    }

    if (modelId == MI_PICKUP_BONUS) {
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_INFO);
        return true;
    }

    if (modelId == MI_PICKUP_BRIBE) {
        auto wantedLevel = std::max(+eWantedLevel::WANTED_CLEAN, +FindPlayerPed()->GetWantedLevel() - +eWantedLevel::WANTED_LEVEL_1);
        FindPlayerPed(0)->SetWantedLevel((eWantedLevel)wantedLevel);
        CStats::IncrementStat(STAT_NUMBER_OF_POLICE_BRIBES, 1.0f);
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_INFO);
        return true;
    }

    if (modelId == MI_PICKUP_KILLFRENZY) {
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_INFO);
        return true;
    }

    if (modelId == MODEL_JETPACK) {
        auto* task = new CTaskSimpleJetPack(nullptr, 10.0f, 0, nullptr);
        CEventScriptCommand event(3, task, 0);
        ped->GetEventGroup().Add(&event, false);
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_INFO);
        return true;
    }

    if (modelId == MI_OYSTER) {
        PickedUpOyster();
        return true;
    }

    if (modelId == MI_HORSESHOE) {
        PickedUpHorseShoe();
        return true;
    }

    return false;
}

/*!
 * @brief Check if pickup was picked up, and then mark it as not picked up.
 * @addr 0x454B40
 */
bool CPickups::IsPickUpPickedUp(tPickupReference pickupRef) {
    if (const auto it = rng::find(aPickUpsCollected, pickupRef.num); it != aPickUpsCollected.end()) {
        *it = 0; // Reset
        return true;
    }
    return false;
}

/*!
 * @addr 0x454AC0
 * @returns The `nModelId1` of the given weapon type.
 */
int32 CPickups::ModelForWeapon(eWeaponType weaponType) {
    return CWeaponInfo::GetWeaponInfo(weaponType)->m_nModelId1;
}

/*!
 * @brief Update each pickup's (except if of type NONE or ASSET_REVENUE) `nRegenerationTime` field.
 * @addr 0x455200
 */
void CPickups::PassTime(uint32 time) {
    for (auto& pickup : aPickUps) {
        switch (pickup.m_nPickupType) {
        case PICKUP_NONE:
        case PICKUP_ASSET_REVENUE:
            continue;
        }

        if (pickup.m_nRegenerationTime <= time) {
            pickup.m_nRegenerationTime = 0;
        } else {
            pickup.m_nRegenerationTime -= time;
        }
    }
}

// 0x455390
void CPickups::PickedUpHorseShoe() {
    AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_COLLECTABLE1);

    CStats::IncrementStat(STAT_HORSESHOES_COLLECTED);
    CStats::IncrementStat(STAT_LUCK, 1000.f / CStats::GetStatValue(STAT_TOTAL_HORSESHOES)); // TODO: Is this some inlined stuff? (The division part)

    FindPlayerInfo().m_nMoney += 100; // originally rewarded to the player 1.

    const auto collected = CStats::GetStatValue(STAT_HORSESHOES_COLLECTED);
    const auto total = CStats::GetStatValue(STAT_TOTAL_HORSESHOES);
    if (collected == total) {
        CGarages::TriggerMessage("HO_ALL");
        FindPlayerInfo().m_nMoney += 100'000; // originally rewarded to the player 1.
    } else {
        CGarages::TriggerMessage("HO_ONE", static_cast<int16>(collected), 5000u, static_cast<int16>(total));
    }
}

// 0x4552D0
void CPickups::PickedUpOyster() {
    AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_COLLECTABLE1);

    CStats::IncrementStat(STAT_OYSTERS_COLLECTED);
    FindPlayerInfo().m_nMoney += 100; // originally rewarded to the player 1.

    const auto collected = CStats::GetStatValue(STAT_OYSTERS_COLLECTED);
    const auto total = CStats::GetStatValue(STAT_TOTAL_OYSTERS);
    if (collected == total) {
        CGarages::TriggerMessage("OY_ALL");
        FindPlayerInfo().m_nMoney += 100'000; // originally rewarded to the player 1.
    } else {
        CGarages::TriggerMessage("OY_ONE", static_cast<int16>(collected), 5000u, static_cast<int16>(total));
    }
}

// 0x456A70
void CPickups::PictureTaken() {
    std::optional<size_t> capturedPickup{};
    auto lastFoundDist = 999'999.88f; // maybe FLT_MAX

    for (auto&& [i, pickup] : rngv::enumerate(aPickUps)) {
        if (pickup.m_nPickupType != PICKUP_SNAPSHOT)
            continue;

        const auto pupPos = pickup.GetPosn();
        const auto dist = DistanceBetweenPoints(TheCamera.GetPosition(), pupPos);

        if (90.0f / TheCamera.FindCamFOV() * 20.0f > dist && dist < lastFoundDist) {
            CVector origin = pupPos;
            if (TheCamera.IsSphereVisible(pupPos, 0.2f) || TheCamera.IsSphereVisibleInMirror(pupPos, 0.2f)) {
                capturedPickup = i;
                lastFoundDist = dist;
            }
        }
    }

    if (!capturedPickup.has_value())
        return;

    aPickUps[*capturedPickup].Remove();

    FindPlayerInfo().m_nMoney += 100'000; // originally rewarded to the player 1.

    AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_COLLECTABLE1);
    CStats::IncrementStat(STAT_SNAPSHOTS_TAKEN, 1.0f);

    const auto taken = CStats::GetStatValue(STAT_SNAPSHOTS_TAKEN);
    const auto total = CStats::GetStatValue(STAT_TOTAL_SNAPSHOTS);
    if (taken == total) {
        CGarages::TriggerMessage("SN_ALL");
        FindPlayerInfo().m_nMoney += 100'000;
    } else {
        CGarages::TriggerMessage("SN_ONE", static_cast<int16>(taken), 5000u, static_cast<int16>(total));
    }
}

// 0x4554C0
bool CPickups::PlayerCanPickUpThisWeaponTypeAtThisMoment(eWeaponType weaponType) {
    if (!CWeaponInfo::GetWeaponInfo(weaponType)->flags.bAimWithArm) {
        if (FindPlayerPed()->GetIntelligence()->GetTaskJetPack()) {
            return false;
        }
    }
    return true;
}

// 0x456DE0
void CPickups::RemoveMissionPickUps() {
    for (auto&& [i, pickup] : rngv::enumerate(aPickUps)) {
        switch (pickup.m_nPickupType) {
        case PICKUP_ONCE_FOR_MISSION: {
            CRadar::ClearBlipForEntity(BLIP_PICKUP, GetUniquePickupIndex(i).num);
            pickup.GetRidOfObjects();

            pickup.m_nFlags.bDisabled = true;
            pickup.m_nPickupType = PICKUP_NONE;
            break;
        }
        }
    }
}

// 0x4573D0
void CPickups::RemovePickUp(tPickupReference pickupRef) {
    if (const auto i = GetActualPickupIndex(pickupRef); i != -1) {
        aPickUps[i].Remove();
    }
}

// 0x456D30
void CPickups::RemovePickUpsInArea(float minX, float maxX, float minY, float maxY, float minZ, float maxZ) {
    CBoundingBox bb{ { minX, minY, minZ }, { maxX, maxY, maxZ } }; // They didn't use a bounding box, but it's nicer to do so.

    for (auto& pickup : GetAllActivePickups()) {
        if (bb.IsPointWithin(pickup.GetPosn())) {
            pickup.Remove();
        }
    }
}

// 0x455470
void CPickups::RemovePickupObjects() {
    for (auto& pickup : GetAllActivePickups()) {
        if (pickup.m_pObject) {
            pickup.GetRidOfObjects();
            pickup.m_nFlags.bVisible = false;
        }
    }
}

// remove pickups with types PICKUP_ONCE_TIMEOUT and PICKUP_MONEY in area
// 0x4563A0
void CPickups::RemoveUnnecessaryPickups(const CVector& posn, float radius) {
    for (auto& pickup : aPickUps) {
        switch (pickup.m_nPickupType) {
        case PICKUP_ONCE_TIMEOUT:
        case PICKUP_MONEY: {
            if (IsPointInSphere(pickup.GetPosn(), posn, radius)) {
                pickup.Remove();
            }
            break;
        }
        }
    }
}

// 0x455000
void CPickups::RenderPickUpText() {
    GxtChar msgText[352]{};
    for (const auto& message : std::span{ aMessages.data(), NumMessages }) {
        if (message.price == 0u) {
            if (!message.text)
                continue;

            if (message.flags & 2) {
                CMessages::InsertNumberInString(message.text, 0, 0, 0, 0, 0, 0, msgText);
            }
        } else {
            AsciiToGxtChar(std::format("${:d}", message.price).c_str(), msgText);
        }

        // TODO: scaled wrong in windowed mode, but it's fine in fullscreen.
        auto scaleX = std::min(SCREEN_STRETCH_X(1.0f), message.width / 30.0f);
        auto scaleY = std::min(SCREEN_STRETCH_X(1.0f), message.height / 30.0f);

        CFont::SetProportional(true);
        CFont::SetBackground(false, false);
        CFont::SetScale(scaleX, scaleY);
        CFont::SetOrientation(eFontAlignment::ALIGN_CENTER);
        CFont::SetCentreSize(SCREEN_WIDTH);
        CFont::SetColor(message.color);
        CFont::SetFontStyle(eFontStyle::FONT_PRICEDOWN);
        CFont::PrintString(message.pos.x, message.pos.y, msgText);
    }
    NumMessages = 0;
}

// check for pickups in area
// 0x456450
bool CPickups::TestForPickupsInBubble(const CVector posn, float radius) {
    // NOTE: (Possible bug) - They dont check if the pickup is active (eg: type != NONE)
    return rng::any_of(aPickUps, [sp = CSphere{ posn, radius }](const CPickup& p) {
        return sp.IsPointWithin(p.GetPosn()); // They obviously didn't use `CSphere` here, but it's nicer.
    });
}

// search for pickup in area (radius = 5.5 units) with this weapon model and pickup type and add ammo to this pickup; returns TRUE if merged
// 0x4555A0
bool CPickups::TryToMerge_WeaponType(CVector posn, eWeaponType weaponType, ePickupType pickupType, uint32 ammo, bool arg4) {
    const auto mi = CWeaponInfo::GetWeaponInfo(weaponType)->m_nModelId1;

    for (auto& pickup : aPickUps) {
        if (mi == pickup.m_nModelIndex && pickup.m_nPickupType == pickupType && IsPointInSphere(pickup.GetPosn(), posn, 5.0f)) {
            pickup.m_nAmmo += ammo;

            return true;
        }
    }
    return false;
}

// 0x458DE0
void CPickups::Update() {
    ZoneScoped;

    if (CReplay::Mode == MODE_PLAYBACK)
        return;

    auto start = 620 * (CTimer::GetFrameCounter() % 32) / 32;
    auto end   = 620 * (CTimer::GetFrameCounter() % 32 + 1) / 32;
    for (auto i = start; i < end; i++) {
        auto& pickup = aPickUps[i];
        if (pickup.m_nPickupType == PICKUP_NONE)
            continue;

        if (pickup.m_nFlags.bVisible = pickup.IsVisible()) {
            if (!pickup.m_nFlags.bDisabled && !pickup.m_pObject) {
                pickup.GiveUsAPickUpObject(pickup.m_pObject);

                if (auto& obj = pickup.m_pObject; obj) {
                    CWorld::Add(obj);
                }
            }
        } else {
            pickup.GetRidOfObjects();
        }
    }

    const auto pad = CPad::GetPad();
    if (pad->CollectPickupJustDown()) {
        CollectPickupBuffer = 6;
    } else if (CollectPickupBuffer) {
        CollectPickupBuffer--;
    }

    if (PlayerOnWeaponPickup) {
        PlayerOnWeaponPickup--;
    }

    if (pad->GetTarget()) {
        CollectPickupBuffer = 0;
    }

    const auto player1 = FindPlayerPed(PED_TYPE_PLAYER1);
    const auto p1Busy = player1->GetIntelligence()->FindTaskByType(TASK_COMPLEX_ENTER_CAR_AS_DRIVER) || player1->GetIntelligence()->FindTaskByType(TASK_COMPLEX_USE_MOBILE_PHONE);

    start = 620 * (CTimer::GetFrameCounter() % 6) / 6;
    end   = 620 * (CTimer::GetFrameCounter() % 6 + 1) / 6;
    for (auto i = start; i < end; i++) {
        auto& pickup = aPickUps[i];

        if (pickup.m_nPickupType == PICKUP_NONE || !pickup.m_nFlags.bVisible)
            continue;

        if (!p1Busy) {
            if (pickup.Update(FindPlayerPed(), FindPlayerVehicle(), CWorld::PlayerInFocus)) {
                AddToCollectedPickupsArray(i);
            }
        } else if (FindPlayerPed(PED_TYPE_PLAYER2)) {
            if (pickup.Update(FindPlayerPed(1), FindPlayerVehicle(1), PED_TYPE_PLAYER2)) {
                AddToCollectedPickupsArray(i);
            }
        }
    }
}

// 0x455680
void CPickups::UpdateMoneyPerDay(tPickupReference pickupRef, uint16 money) {
    if (auto idx = GetActualPickupIndex(pickupRef); idx != -1) {
        aPickUps[idx].m_nMoneyPerDay = money;
    }
}

// 0x454AE0
eWeaponType CPickups::WeaponForModel(int32 modelId) {
    if (modelId == MI_PICKUP_BODYARMOUR) {
        return WEAPON_ARMOUR;
    }
    if (modelId == MI_PICKUP_HEALTH) {
        return WEAPON_LAST_WEAPON;
    }
    if (modelId == MI_PICKUP_ADRENALINE) {
        return WEAPON_ARMOUR;
    }

    switch (modelId) {
    case MODEL_JETPACK:
        return WEAPON_LAST_WEAPON;

    case MODEL_INVALID:
        return WEAPON_UNARMED;
    }

    if (auto mi = CModelInfo::GetModelInfo(modelId); mi->GetModelType() == MODEL_INFO_WEAPON) {
        return mi->AsWeaponModelInfoPtr()->GetWeaponInfo();
    }

    return WEAPON_UNARMED;
}

// 0x5D35A0
void CPickups::Load() {
    for (auto& pickup : aPickUps) {
        CGenericGameStorage::LoadDataFromWorkBuffer(pickup);
        if (pickup.m_nPickupType != PICKUP_NONE && pickup.m_pObject) {
            pickup.m_pObject = nullptr;
            pickup.m_nFlags.bVisible = false;
        }
    }
    NumMessages = 0u;
    CGenericGameStorage::LoadDataFromWorkBuffer(CPickups::CollectedPickUpIndex);
    CGenericGameStorage::LoadDataFromWorkBuffer(CPickups::DisplayHelpMessage);

    for (auto& collected : aPickUpsCollected) {
        CGenericGameStorage::LoadDataFromWorkBuffer(collected);
    }
}
// 0x5D3540
void CPickups::Save() {
    for (auto& pickup : aPickUps) {
        CGenericGameStorage::SaveDataToWorkBuffer(pickup);
    }
    CGenericGameStorage::SaveDataToWorkBuffer(CPickups::CollectedPickUpIndex);
    CGenericGameStorage::SaveDataToWorkBuffer(CPickups::DisplayHelpMessage);

    for (auto& collected : aPickUpsCollected) {
        CGenericGameStorage::SaveDataToWorkBuffer(collected);
    }
}

// 0x454B70
void ModifyStringLabelForControlSetting(char* stringLabel) {
    const auto len = strlen(stringLabel);

    if (len < 2 || stringLabel[len - 2] != '_')
        return;

    switch (CPad::GetPad(0)->Mode) {
    case 0:
    case 1:
        stringLabel[len - 1] = 'L';
        break;
    case 2:
        stringLabel[len - 1] = 'T';
        break;
    case 3:
        stringLabel[len - 1] = 'C';
        break;
    }
}
