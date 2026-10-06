/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "WeaponEffects.h"

// Statics of this file (addresses are from the original executable)
static inline auto& s_afCrossHairScale  = StaticRef<std::array<float, MAX_NUM_WEAPON_CROSSHAIRS>>(0xC8A8A0);      // Per crosshair animated size offset
static inline auto& s_abPulseOutwards   = StaticRef<std::array<bool, MAX_NUM_WEAPON_CROSSHAIRS>>(0x8D6144);       // { true, true } - Whether the target's radius grows or shrinks
static inline auto& s_fFlightOffsetX    = StaticRef<float>(0xC8A89C);                                            // Flight (lock-on) crosshair offset from it's position
static inline auto& s_fFlightOffsetY    = StaticRef<float>(0xC8A898);
static inline auto& s_nTargetChangeTime = StaticRef<int32>(0xC8A890);                                            // Time (in ms) the lock-on target was last changed
static inline auto& s_pLastTarget       = StaticRef<CEntity*>(0xC8A894);                                         // The last lock-on target

void CWeaponEffects::InjectHooks() {
    RH_ScopedClass(CWeaponEffects);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x742AB0);
    RH_ScopedInstall(Shutdown, 0x742B80);
    RH_ScopedInstall(IsLockedOn, 0x742BD0);
    RH_ScopedInstall(MarkTarget, 0x742BF0);
    RH_ScopedInstall(ClearCrossHair, 0x742C60);
    RH_ScopedInstall(ClearCrossHairs, 0x742C80);
    RH_ScopedInstall(ClearCrossHairImmediately, 0x742CA0);
    RH_ScopedInstall(ClearCrossHairsImmediately, 0x742CC0);
    RH_ScopedInstall(Render, 0x742CF0);
}

void CPlayerCrossHair::InjectHooks() {
    RH_ScopedClass(CPlayerCrossHair);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Render, 0x56EF90);
    RH_ScopedInstall(Update, 0x56EC80);
}

// 0x56EC80
void CPlayerCrossHair::Update(int32 playerId, CPad* pad) {
    constexpr auto MAX_TARGET_OFFSET = 0.9f;   // 0x858C20, 0x858CAC
    constexpr auto MOVE_SPEED        = 1.0f / 3000.0f; // 0x865030
    constexpr auto TARGET_DIST       = 200.0f; // 0x858A48

    if (!m_bActivated) {
        return;
    }

    m_vecTarget.x += (float)pad->GetSteeringLeftRight() * CTimer::GetTimeStep() * MOVE_SPEED;
    if (CPad::bInvertLook4Pad) {
        m_vecTarget.y = (float)pad->GetSteeringUpDown() * CTimer::GetTimeStep() * MOVE_SPEED + m_vecTarget.y;
    } else {
        m_vecTarget.y = m_vecTarget.y - (float)pad->GetSteeringUpDown() * CTimer::GetTimeStep() * MOVE_SPEED;
    }

    // 0x56ED1F
    if (m_vecTarget.x > MAX_TARGET_OFFSET) {
        m_vecTarget.x = MAX_TARGET_OFFSET;
    }
    if (m_vecTarget.x < -MAX_TARGET_OFFSET) {
        m_vecTarget.x = -MAX_TARGET_OFFSET;
    }
    if (m_vecTarget.y > MAX_TARGET_OFFSET) {
        m_vecTarget.y = MAX_TARGET_OFFSET;
    }
    if (m_vecTarget.y < -MAX_TARGET_OFFSET) {
        m_vecTarget.y = -MAX_TARGET_OFFSET;
    }

    // 0x56ED75
    if (!pad->GetCarGunFired()) {
        return;
    }

    CamShakeNoPos(&TheCamera, 0.2f);

    // 0x56ED97 - Calculate the world space direction of the crosshair
    constexpr auto HALF_DEG_TO_RAD = 0.00872664619f; // 0x8631D4 - PI / 360
    const auto& camMat = TheCamera.m_mCameraMatrix;
    const auto  up     = camMat.GetUp() * (std::tan(TheCamera.FindCamFOV() * HALF_DEG_TO_RAD) / CDraw::ms_fAspectRatio * m_vecTarget.y);
    const auto  right  = camMat.GetRight() * m_vecTarget.x * std::tan(TheCamera.FindCamFOV() * HALF_DEG_TO_RAD);
    const auto  dir    = camMat.GetForward() - right - up;

    // 0x56EE8B
    CVector target = camMat.GetPosition() + dir * TARGET_DIST;

    CWeapon weapon{ WEAPON_M4, 5000 };
    CVector origin = TheCamera.GetPosition();

    // 0x56EF1D - Fire with doom-aim temporarily disabled
    const bool bDoomAim                         = CWorld::Players[playerId].m_pPed->bDoomAim;
    CWorld::Players[playerId].m_pPed->bDoomAim = false;
    weapon.FireInstantHit(CWorld::Players[playerId].m_pPed, &origin, &origin, nullptr, &target, nullptr, true, true);
    CWorld::Players[playerId].m_pPed->bDoomAim = bDoomAim;
}

// 0x56EF90
void CPlayerCrossHair::Render(int32 playerId) {
    constexpr auto NUM_HISTORY = 5;

    // Function-local statics in the original
    struct History {
        uint32 time[2][NUM_HISTORY]; // 0xB9B8F8
        float  y[2][NUM_HISTORY];    // 0xB9B920
        float  x[2][NUM_HISTORY];    // 0xB9B948
    };
    static auto& s_History             = StaticRef<History>(0xB9B8F8);
    static auto& s_nCrossHairCoronaTex = StaticRef<int32>(0x8CDF1C); // 8

    if (!m_bActivated) {
        return;
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,       RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(rwBLENDINVDESTALPHA));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,     RWRSTATE(RwTextureGetRaster(gpCoronaTexture[s_nCrossHairCoronaTex])));

    // 0x56F003
    uint32 r, g, b;
    if (playerId != 0) {
        r = 50, g = 0, b = 255;
    } else {
        r = 0, g = 255, b = 50;
    }

    // 0x56F027 - Shift the history
    auto& times = s_History.time[playerId];
    auto& xs    = s_History.x[playerId];
    auto& ys    = s_History.y[playerId];
    for (auto i = NUM_HISTORY - 1; i > 0; i--) {
        xs[i] = xs[i - 1];
        ys[i] = ys[i - 1];
        // NOTE/BUG: The original reads `time[2][i - 1]` here, which is out-of-bounds, and is the same memory as `y[0][i - 1]`.
        //           It doesn't matter, as only `time[playerId][0]` is ever used.
        times[i] = std::bit_cast<uint32>(s_History.y[0][i - 1]);
    }
    xs[0]    = m_vecTarget.x;
    ys[0]    = m_vecTarget.y;
    times[0] = CTimer::GetTimeInMS();

    // 0x56F08D - Render the current position, and the trail (Each older one being half as bright as the previous)
    for (auto h = 0; h < NUM_HISTORY; h++) {
        const auto pulse = std::sin((float)(times[0] % 1024) * 0.0061359233f /* 2pi / 1024 */) * 0.2f + 1.f;
        for (auto ring = 0; ring < 3; ring++) {
            const auto radius = (float)ring * 10.f + 20.f;
            for (auto i = 0; i < 4; i++) {
                const auto angle = (float)i * 1.5707964f + 0.7853982f; // i * (pi / 2) + (pi / 4)
                CSprite::RenderOneXLUSprite_Rotate_Aspect(
                    CVector{
                        std::sin(angle) * radius * pulse + (xs[h] + 1.f) * (float)RsGlobal.maximumWidth * 0.5f,
                        std::cos(angle) * radius * pulse + (ys[h] + 1.f) * (float)RsGlobal.maximumHeight * 0.5f,
                        100.f
                    },
                    CVector2D{ 15.f, 15.f },
                    (uint8)r, (uint8)g, (uint8)b,
                    255,
                    0.01f,
                    0.f,
                    255
                );
            }
        }
        r >>= 1;
        g >>= 1;
        b >>= 1;
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,  RWRSTATE(TRUE));
}

// 0x742AB0
void CWeaponEffects::Init() {
    for (auto& crossHair : gCrossHair) {
        crossHair.m_vecPosn = CVector();
        crossHair.m_bActive = false;
        crossHair.m_color = CRGBA(255, 0, 0, 127);
        crossHair.m_fSize = 1.0f;
        crossHair.m_fTargetRotation = 0.0f;
        crossHair.m_nTimeWhenToDeactivate = 0;
        crossHair.m_bClearImmediately = false;
        crossHair.m_fRotation = 0.0f;
    }

    CTxdStore::PushCurrentTxd();
    CTxdStore::SetCurrentTxd(CTxdStore::FindTxdSlot("particle"));
    gpCrossHairTex          = RwTextureRead("target256",  "target256m");
    gpCrossHairTexFlight[0] = RwTextureRead("lockon",     "lockonA");
    gpCrossHairTexFlight[1] = RwTextureRead("lockonFire", "lockonFireA");
    CTxdStore::PopCurrentTxd();
}

// 0x742B80
void CWeaponEffects::Shutdown() {
    for (auto& crossHair : gpCrossHairTexFlight) {
        RwTextureDestroy(crossHair);
        crossHair = nullptr;
    }

    RwTextureDestroy(gpCrossHairTex);
    gpCrossHairTex = nullptr;
}

// 0x742BD0
bool CWeaponEffects::IsLockedOn(CrossHairId id) {
    return gCrossHair[id].m_fRotation;
}

// 0x742BF0
void CWeaponEffects::MarkTarget(CrossHairId id, CVector posn, uint8 red, uint8 green, uint8 blue, uint8 alpha, float size, bool bClearImmediately) {
    auto& crossHair = gCrossHair[id];
    crossHair.m_vecPosn               = posn;
    crossHair.m_color.r               = red;
    crossHair.m_color.g               = green;
    crossHair.m_color.b               = blue;
    crossHair.m_color.a               = alpha;
    crossHair.m_bActive               = true;
    crossHair.m_fSize                 = size;
    crossHair.m_nTimeWhenToDeactivate = -1;
    crossHair.m_bClearImmediately     = bClearImmediately;
}

// 0x742C60
void CWeaponEffects::ClearCrossHair(CrossHairId id) {
    gCrossHair[id].m_nTimeWhenToDeactivate = static_cast<int32>(CTimer::GetTimeInMS() + 400);
}

// 0x742C80
void CWeaponEffects::ClearCrossHairs() {
    for (auto& crossHair : gCrossHair) {
        crossHair.m_bActive = false;
    }
}

// 0x742CA0
void CWeaponEffects::ClearCrossHairImmediately(CrossHairId id) {
    gCrossHair[id].m_nTimeWhenToDeactivate = static_cast<int32>(CTimer::GetTimeInMS() - 100);
    gCrossHair[id].m_bActive = false;
}

// 0x742CC0
void CWeaponEffects::ClearCrossHairsImmediately() {
    for (auto i = 0u; i < gCrossHair.size(); i++) {
        ClearCrossHair(i);
    }
}

// 0x742CF0
void CWeaponEffects::Render() {
    static auto& s_fOffScreenArrowSize    = StaticRef<float>(0x8D6140); // = 15.f
    static auto& s_fWeaponRadiusToScreen  = StaticRef<float>(0x8D6148); // = 30.f

    constexpr auto TIME_TO_ANGLE = 6.2831855f / 1024.0f; // 0.0061359233f

    if (TheCamera.m_bWideScreenOn) {
        return;
    }

    const auto timeMs    = CTimer::GetTimeInMS();
    const auto timeAngle = static_cast<float>(timeMs & 0x3FF) * TIME_TO_ANGLE;

    for (auto&& [i, ch] : rngv::enumerate(gCrossHair)) {
        auto&       sizeOffset = s_afCrossHairScale[i];
        auto&       pulseOut   = s_abPulseOutwards[i];
        auto* const player     = CWorld::Players[i].m_pPed;

        if (ch.m_nTimeWhenToDeactivate != 0 && static_cast<uint32>(ch.m_nTimeWhenToDeactivate) < timeMs) {
            ch.m_bActive               = false;
            ch.m_nTimeWhenToDeactivate = 0;
            sizeOffset                 = 0.0f;
        }
        if (ch.m_nTimeWhenToDeactivate != -1) {
            sizeOffset = 0.0f;
        }
        if (!player) {
            ch.m_bActive = false;
        }
        if (!ch.m_bActive) {
            continue;
        }

        CVector scr;
        float   w, h;

        if (ch.m_bClearImmediately) { // 0x742D6A - Flight (heat-seeking missile lock-on) crosshair
            if (CSprite::CalcScreenCoors(ch.m_vecPosn, &scr, &w, &h, true, true)) {
                if (const auto f = 20.0f / h; f > 1.0f) {
                    w *= f;
                    h *= f;
                }

                RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(FALSE));
                RwRenderStateSet(rwRENDERSTATEZTESTENABLE,       RWRSTATE(FALSE));
                RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
                RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(rwBLENDSRCALPHA));
                RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(rwBLENDINVSRCALPHA));
                RwRenderStateSet(rwRENDERSTATETEXTURERASTER,     RWRSTATE(RwTextureGetRaster(gpCrossHairTexFlight[WEAPONEFFECTS_LOCK_ON])));

                constexpr float OUTLINE_SCALES[]{ 0.95f, 1.05f };

                const CVector2D size{ std::min(w, 28.0f), std::min(h, 20.0f) };

                // Outer (static) one
                for (const auto scale : OUTLINE_SCALES) {
                    CSprite::RenderOneXLUSprite_Rotate_Aspect(scr, size * scale * 1.8f, 0, 0, 0, 255, 0.01f, 0.0f, 255);
                }
                CSprite::RenderOneXLUSprite_Rotate_Aspect(scr, size * 1.8f, ch.m_color.r, ch.m_color.g, ch.m_color.b, 255, 0.01f, 0.0f, ch.m_color.a);

                // Inner (moving) one
                RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RWRSTATE(RwTextureGetRaster(gpCrossHairTexFlight[WEAPONEFFECTS_LOCK_ON_FIRE])));

                ch.m_fFlightOffsetDist -= 20.0f * timeAngle;
                if (ch.m_fFlightOffsetDist < 0.0f) {
                    ch.m_fFlightOffsetDist = 0.0f;
                }
                ch.m_fRotation = ch.m_fFlightOffsetDist == 0.0f ? 1.0f : 0.0f; // Locked on?

                s_fFlightOffsetX = std::sin(timeAngle) * ch.m_fFlightOffsetDist;
                s_fFlightOffsetY = std::cos(timeAngle) * ch.m_fFlightOffsetDist;

                const CVector innerPos{ scr.x - s_fFlightOffsetX, scr.y - s_fFlightOffsetY, scr.z };
                for (const auto scale : OUTLINE_SCALES) {
                    CSprite::RenderOneXLUSprite_Rotate_Aspect(innerPos, size * scale * 0.8f, 0, 0, 0, 255, 0.01f, timeAngle, 255);
                }
                CSprite::RenderOneXLUSprite_Rotate_Aspect(innerPos, size * 0.8f, ch.m_color.r, ch.m_color.g, ch.m_color.b, ch.m_color.a, 0.01f, timeAngle, 255);
            }

            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(TRUE));
            RwRenderStateSet(rwRENDERSTATEZTESTENABLE,  RWRSTATE(TRUE));
            continue;
        }

        // 0x7430F6 - Regular target marker (3 triangles pointing at the target)
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(FALSE));
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE,       RWRSTATE(FALSE));
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER,     RWRSTATE(NULL));

        CSprite::CalcScreenCoors(ch.m_vecPosn, &scr, &w, &h, false, false);

        if (scr.x > SCREEN_WIDTH || scr.x < 0.0f || scr.y > SCREEN_HEIGHT || scr.y < 0.0f) { // Off-screen
            if (CGameLogic::IsCoopGameGoingOn()) { // 0x7435C1 - Draw an arrow at the edge of the screen pointing towards the target
                auto& camMat = TheCamera.m_mCameraMatrix;

                const auto GetAngleTo = [&](const CVector& from) {
                    const auto dir = ch.m_vecPosn - from;
                    return std::atan2(camMat.GetUp().Dot(dir), camMat.GetRight().Dot(dir));
                };

                const auto camAngle = GetAngleTo(camMat.GetPosition());
                const auto camSin = std::sin(camAngle), camCos = std::cos(camAngle);

                const auto plyrAngle = GetAngleTo(player->GetPosition());
                const auto pSin = std::sin(plyrAngle), pCos = std::cos(plyrAngle);

                const auto size = s_fOffScreenArrowSize;

                // Outline
                CVector2D tip{
                    (1.0f - camCos) * SCREEN_WIDTH * 0.5f,
                    (1.0f - camSin) * SCREEN_HEIGHT * 0.5f
                };
                CSprite::RenderOneXLUSprite_Triangle(
                    tip,
                    { (pSin * 0.5f + pCos) * size + tip.x, size * pSin + tip.y - pCos * size * 0.5f },
                    { pCos * size + tip.x - size * pSin * 0.5f, (pCos * 0.5f + pSin) * size + tip.y },
                    2.5f,
                    0, 0, 0, 255,
                    1.0f,
                    255
                );

                // Colored inner part
                tip.x += pCos * size * 0.1f;
                tip.y += 0.1f * (size * pSin);
                CSprite::RenderOneXLUSprite_Triangle(
                    tip,
                    { (pSin * 0.5f + pCos) * size * 0.8f + tip.x, (size * pSin - 0.5f * (pCos * size)) * 0.8f + tip.y },
                    { (pCos * size - size * pSin * 0.5f) * 0.8f + tip.x, (pCos * 0.5f + pSin) * size * 0.8f + tip.y },
                    2.5f,
                    ch.m_color.r, ch.m_color.g, ch.m_color.b, 255,
                    1.0f,
                    255
                );
            }
        } else { // 0x7431E9
            ch.m_fSize = std::max(std::min(ch.m_fSize, 1.2f), 0.3f);

            auto innerRadius = 5.0f * ch.m_fSize;
            auto outerRadius = ch.m_fSize * 25.0f;

            if (scr.z < 2.5f) {
                scr.z = 2.5f;
            }

            if (const auto weaponRadius = player->GetWeaponRadiusOnScreen(); weaponRadius > 0.0f) {
                innerRadius = weaponRadius * s_fWeaponRadiusToScreen;
                outerRadius = ch.m_fSize * 20.0f + innerRadius;
            }

            RwRenderStateSet(rwRENDERSTATESRCBLEND,  RWRSTATE(rwBLENDZERO));
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(rwBLENDZERO));

            for (auto t = 0; t < 3; t++) {
                const auto baseAngleDeg = static_cast<float>(t) * 120.0f;

                const auto GetPoint = [&](float angleOffsetDeg, float radius) {
                    const auto a = DegreesToRadians(baseAngleDeg + angleOffsetDeg) + ch.m_fTargetRotation;
                    const auto r = radius + sizeOffset;
                    return CVector2D{ scr.x - std::sin(a) * r, scr.y - std::cos(a) * r };
                };

                auto p1 = GetPoint(0.0f, innerRadius);
                auto p2 = GetPoint(+15.0f, outerRadius);
                auto p3 = GetPoint(-15.0f, outerRadius);

                // Outline
                CSprite::RenderOneXLUSprite_Triangle(p1, p2, p3, scr.z, 0, 0, 0, 255, 1.0f, ch.m_color.a);

                // Colored inner part (Shrunk towards the centroid)
                const auto centroid = (p3 + p2 + p1) * (1.0f / 3.0f);
                p1 = (p1 - centroid) * 0.75f + centroid;
                p2 = (p2 - centroid) * 0.75f + centroid;
                p3 = (p3 - centroid) * 0.75f + centroid;

                RwRenderStateSet(rwRENDERSTATESRCBLEND,  RWRSTATE(rwBLENDSRCALPHA));
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(rwBLENDINVSRCALPHA));
                RwRenderStateSet(rwRENDERSTATECULLMODE,  RWRSTATE(rwCULLMODECULLNONE));

                CSprite::RenderOneXLUSprite_Triangle(p1, p2, p3, scr.z, ch.m_color.r, ch.m_color.g, ch.m_color.b, 255, 1.0f, ch.m_color.a);
            }

            // 0x7434E5 - Animate
            const auto deactivateTime = ch.m_nTimeWhenToDeactivate;
            if (deactivateTime <= 0) { // Includes `-1`
                ch.m_fTargetRotation += 0.05f;
            } else { // Fading out
                ch.m_fTargetRotation += 0.75f;
                sizeOffset           += 2.0f;
                ch.m_fSize           *= 0.9f;
            }
            if (ch.m_fTargetRotation > 6.2831855f) {
                ch.m_fTargetRotation = 0.0f;
            }

            if (deactivateTime == 0) { // Pulsate
                if (pulseOut) {
                    sizeOffset += ch.m_fSize + ch.m_fSize;
                    if (sizeOffset > ch.m_fSize * 20.0f) {
                        pulseOut = false;
                    }
                } else {
                    sizeOffset -= 2.0f;
                    if (sizeOffset < 0.0f) {
                        pulseOut = true;
                    }
                }
            }
        }

        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(FALSE));
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(TRUE));
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE,       RWRSTATE(TRUE));
    }

    // 0x7438DF - Crosshair of the shooting player in 2 player (in car) mode
    if (const auto& cam = TheCamera.GetActiveCam(); cam.m_nMode == MODE_TWOPLAYER_IN_CAR_AND_SHOOTING) {
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(FALSE));
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE,       RWRSTATE(FALSE));
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
        RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(rwBLENDSRCALPHA));
        RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(rwBLENDINVSRCALPHA));
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER,     RWRSTATE(RwTextureGetRaster(gpCrossHairTex)));

        CSprite::RenderOneXLUSprite_Rotate_Aspect(
            { (cam.m_fX_Targetting + 1.0f) * SCREEN_WIDTH * 0.5f, (cam.m_fY_Targetting + 1.0f) * SCREEN_HEIGHT * 0.5f, 100.0f },
            { 10.0f, 10.0f },
            255, 128, 0, 128,
            0.01f,
            timeAngle,
            255
        );

        // The player that's not driving is the shooter
        CPlayerPed* const player0 = CWorld::Players[0].m_pPed;
        CPlayerPed*       shooter = CWorld::Players[1].m_pPed;
        if (player0->m_pVehicle && player0->m_pVehicle->m_pDriver != player0) {
            shooter = player0;
        }

        CEntity* target = nullptr;
        if (shooter) {
            const auto* const wi = CWeaponInfo::GetWeaponInfo(shooter->GetActiveWeapon().m_Type, shooter->GetWeaponSkill());
            target = CWeapon::FindNearestTargetEntityWithScreenCoors(
                cam.m_fX_Targetting,
                cam.m_fY_Targetting,
                wi->m_fWeaponRange + wi->m_fWeaponRange,
                shooter->GetPosition(),
                nullptr,
                nullptr
            );
        }

        if (target != s_pLastTarget) {
            s_pLastTarget       = target;
            s_nTargetChangeTime = static_cast<int32>(timeMs);
        }

        if (target) {
            CVector scr;
            float   w, h;
            if (CSprite::CalcScreenCoors(target->GetPosition(), &scr, &w, &h, true, true)) {
                if (const auto f = 20.0f / h; f > 1.0f) {
                    w *= f;
                    h *= f;
                }

                // Shrinks and gets brighter the longer the target is the same
                const auto timeSinceChange = static_cast<int32>(timeMs) - s_nTargetChangeTime;
                const auto scale           = std::max(3.0f - static_cast<float>(timeSinceChange) * (1.0f / 512.0f), 1.0f);
                const auto intensity       = static_cast<uint8>(std::min(timeSinceChange / 4 + 70, 255));

                CSprite::RenderOneXLUSprite_Rotate_Aspect(
                    scr,
                    { scale * w, h * scale },
                    intensity, 0, 0, intensity,
                    0.01f,
                    timeAngle,
                    255
                );
            }
        }

        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(TRUE));
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE,  RWRSTATE(TRUE));
    }

    // 0x743AFD
    for (auto&& [i, playerInfo] : rngv::enumerate(CWorld::Players)) {
        // The `CPlayerCrossHair` struct is `m_nCrosshairActivated` + `m_vecCrosshairTarget`
        reinterpret_cast<CPlayerCrossHair*>(&playerInfo.m_nCrosshairActivated)->Render(static_cast<int32>(i));
    }
}
