/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include "Vector.h"
#include "RGBA.h"

class CPad;

typedef int32 CrossHairId;

enum eWeaponEffectsLockTexture {
    WEAPONEFFECTS_LOCK_ON = 0,
    WEAPONEFFECTS_LOCK_ON_FIRE = 1
};

class CWeaponEffects {
public:
    bool    m_bActive;
    int32   m_nTimeWhenToDeactivate; // -1 default
    CVector m_vecPosn;
    CRGBA   m_color;
    float   m_fSize;
    float   m_fTargetRotation;   // Rotation of the (triangle) target marker [in radians]
    float   m_fFlightOffsetDist; // Distance of the 2nd flight (lock-on) crosshair from the 1st one, the target is locked on once this reaches 0
    float   m_fRotation;         // TODO: Bad name, it's really `1.f` if locked on, `0.f` otherwise - See `IsLockedOn`
    bool    m_bClearImmediately; // TODO: Bad name, if set the flight (lock-on) crosshair is rendered instead of the regular target marker

public:
    static void InjectHooks();

    CWeaponEffects() = default;  // 0x742A90
    ~CWeaponEffects() = default; // 0x742AA0

    static void Init();
    static void Shutdown();
    static bool IsLockedOn(CrossHairId id);
    static void MarkTarget(CrossHairId id, CVector posn, uint8 red, uint8 green, uint8 blue, uint8 alpha, float size, bool bClearImmediately);
    static void ClearCrossHair(CrossHairId id);
    static void ClearCrossHairs();
    static void ClearCrossHairImmediately(CrossHairId id);
    static void ClearCrossHairsImmediately();
    static void Render();
};

VALIDATE_SIZE(CWeaponEffects, 0x2C);

/*!
 * @brief The player's crosshair in screen space (Used for the heli/plane guns, etc)
 * @brief Embedded in `CPlayerInfo` (As `m_nCrosshairActivated` + `m_vecCrosshairTarget`)
 */
struct CPlayerCrossHair {
    bool      m_bActivated;
    CVector2D m_vecTarget; // -1 ... 1 on screen

public:
    static void InjectHooks();

    void Update(int32 playerId, CPad* pad);
    void Render(int32 playerId);
};
VALIDATE_SIZE(CPlayerCrossHair, 0xC);

constexpr auto MAX_NUM_WEAPON_CROSSHAIRS{ 2u };
static inline auto& gCrossHair = StaticRef<std::array<CWeaponEffects, MAX_NUM_WEAPON_CROSSHAIRS>>(0xC8A838);
static inline auto& gpCrossHairTex = StaticRef<RwTexture*>(0xC8A818);
static inline auto& gpCrossHairTexFlight = StaticRef<RwTexture*[2]>(0xC8A810);
