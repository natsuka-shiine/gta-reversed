/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "StdInc.h"

#include "Fx.h"
#include "Shadows.h"

static auto& TempVertexBuffer = StaticRef<std::array<RxObjSpace3DVertex, 4>, 0xC4D958>();

auto& g_fx = StaticRef<Fx_c, 0xA9AE00>();

void Fx_c::InjectHooks() {
    RH_ScopedClass(Fx_c);
    RH_ScopedCategory("Fx");

    // + RH_ScopedInstall(Constructor, 0x49E620);
    // + RH_ScopedInstall(Destructor, 0x49E630);
    // + RH_ScopedInstall(InitStaticSystems, 0x49E660);
    // + RH_ScopedInstall(ExitStaticSystems, 0x49E850);
    // + RH_ScopedInstall(InitEntitySystems, 0x49EA60);
    // + RH_ScopedInstall(ExitEntitySystems, 0x4A12D0);
    // + RH_ScopedInstall(Init, 0x49EA90);
    // + RH_ScopedInstall(Exit, 0x4A1320);
    // + RH_ScopedInstall(Reset, 0x49EAE0);
    // + RH_ScopedInstall(CreateEntityFx, 0x4A11E0);
    // + RH_ScopedInstall(DestroyEntityFx, 0x4A1280);
    // + RH_ScopedInstall(Update, 0x49E640);
    // + RH_ScopedInstall(Render, 0x49E650);
    RH_ScopedInstall(CreateMatFromVec, 0x49E950);
    // + RH_ScopedInstall(SetFxQuality, 0x49EA40);
    // + RH_ScopedInstall(GetFxQuality, 0x49EA50);
    RH_ScopedInstall(AddBlood, 0x49EB00);
    RH_ScopedInstall(AddWood, 0x49EE10);
    RH_ScopedInstall(AddSparks, 0x49F040);
    RH_ScopedInstall(AddTyreBurst, 0x49F300);
    RH_ScopedInstall(AddBulletImpact, 0x49F3D0);
    RH_ScopedInstall(AddPunchImpact, 0x49F670);
    RH_ScopedInstall(AddDebris, 0x49F750);
    RH_ScopedInstall(AddGlass, 0x49F970);
    RH_ScopedInstall(AddWheelSpray, 0x49FB30);
    RH_ScopedInstall(AddWheelGrass, 0x49FF20);
    RH_ScopedInstall(AddWheelGravel, 0x4A0170);
    RH_ScopedInstall(AddWheelMud, 0x4A03C0);
    RH_ScopedInstall(AddWheelSand, 0x4A0610);
    RH_ScopedInstall(AddWheelDust, 0x4A09C0);
    RH_ScopedInstall(TriggerWaterHydrant, 0x4A0D70);
    RH_ScopedInstall(TriggerGunshot, 0x4A0DE0);
    RH_ScopedInstall(TriggerTankFire, 0x4A0FA0);
    RH_ScopedInstall(TriggerWaterSplash, 0x4A1070);
    RH_ScopedInstall(TriggerBulletSplash, 0x4A10E0);
    RH_ScopedInstall(TriggerFootSplash, 0x4A1150);

    RH_ScopedGlobalInstall(RenderAddTri_, 0x4A1410);
    RH_ScopedGlobalInstall(RenderEnd, 0x4A1600);
    RH_ScopedGlobalInstall(RenderBegin, 0x4A13B0);
    RH_ScopedGlobalInstall(RotateVecIntoVec, 0x4A1660);
    RH_ScopedGlobalInstall(RotateVecAboutVec, 0x4A1780);
}

Fx_c* Fx_c::Constructor() { this->Fx_c::Fx_c(); return this; }
Fx_c* Fx_c::Destructor() { this->Fx_c::~Fx_c(); return this; }

// 0x49E660
void Fx_c::InitStaticSystems() {
    m_Blood          = g_fxMan.CreateFxSystem("prt_blood",            CVector{}, nullptr, true);
    m_BoatSplash     = g_fxMan.CreateFxSystem("prt_boatsplash",       CVector{}, nullptr, true);
    m_Bubble         = g_fxMan.CreateFxSystem("prt_bubble",           CVector{}, nullptr, true);
    m_Cardebris      = g_fxMan.CreateFxSystem("prt_cardebris",        CVector{}, nullptr, true);
    m_CollisionSmoke = g_fxMan.CreateFxSystem("prt_collisionsmoke",   CVector{}, nullptr, true);
    m_GunShell       = g_fxMan.CreateFxSystem("prt_gunshell",         CVector{}, nullptr, true);
    m_Sand           = g_fxMan.CreateFxSystem("prt_sand",             CVector{}, nullptr, true);
    m_Sand2          = g_fxMan.CreateFxSystem("prt_sand2",            CVector{}, nullptr, true);
    m_SmokeHuge      = g_fxMan.CreateFxSystem("prt_smoke_huge",       CVector{}, nullptr, true);
    m_SmokeII3expand = g_fxMan.CreateFxSystem("prt_smokeII_3_expand", CVector{}, nullptr, true);
    m_Spark          = g_fxMan.CreateFxSystem("prt_spark",            CVector{}, nullptr, true);
    m_Spark2         = g_fxMan.CreateFxSystem("prt_spark_2",          CVector{}, nullptr, true);
    m_Splash         = g_fxMan.CreateFxSystem("prt_splash",           CVector{}, nullptr, true);
    m_Wake           = g_fxMan.CreateFxSystem("prt_wake",             CVector{}, nullptr, true);
    m_WaterSplash    = g_fxMan.CreateFxSystem("prt_watersplash",      CVector{}, nullptr, true);
    m_WheelDirt      = g_fxMan.CreateFxSystem("prt_wheeldirt",        CVector{}, nullptr, true);
    m_Glass          = g_fxMan.CreateFxSystem("prt_glass",            CVector{}, nullptr, true);
}

// 0x49E850
void Fx_c::ExitStaticSystems() {
    g_fxMan.DestroyFxSystem(m_Blood);
    g_fxMan.DestroyFxSystem(m_BoatSplash);
    g_fxMan.DestroyFxSystem(m_Bubble);
    g_fxMan.DestroyFxSystem(m_Cardebris);
    g_fxMan.DestroyFxSystem(m_CollisionSmoke);
    g_fxMan.DestroyFxSystem(m_GunShell);
    g_fxMan.DestroyFxSystem(m_Sand);
    g_fxMan.DestroyFxSystem(m_Sand2);
    g_fxMan.DestroyFxSystem(m_SmokeHuge);
    g_fxMan.DestroyFxSystem(m_SmokeII3expand);
    g_fxMan.DestroyFxSystem(m_Spark);
    g_fxMan.DestroyFxSystem(m_Spark2);
    g_fxMan.DestroyFxSystem(m_Splash);
    g_fxMan.DestroyFxSystem(m_Wake);
    g_fxMan.DestroyFxSystem(m_WaterSplash);
    g_fxMan.DestroyFxSystem(m_WheelDirt);
    g_fxMan.DestroyFxSystem(m_Glass);
}

// 0x49EA60
void Fx_c::InitEntitySystems() {
    // NOP
}

// NOTSA
static void CreateFxWithinCameraRange(const char* name, const CVector& pos, float range) {
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos) <= range) {
        if (auto* fxSystem = g_fxMan.CreateFxSystem(name, pos, nullptr, false)) {
            fxSystem->PlayAndKill();
        }
    }
}

// NOTSA - `rand() * (1 / RAND_MAX)` => [0, 1]
static float RandomUnit() {
    return static_cast<float>(CGeneral::GetRandomNumber()) * RAND_MAX_FLOAT_RECIPROCAL;
}

// NOTSA - `(rand() % 10000) * 0.0001` => [0, 1)
static float RandomUnitMod() {
    return static_cast<float>(CGeneral::GetRandomNumber() % 10'000) * 0.0001f;
}

// NOTSA
static float GetDistSqToCamera(const CVector& pos) {
    return DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos);
}

// NOTSA - Common culling code of AddWheelSpray/Grass/Gravel/Mud
static bool ShouldAddWheelFxThisFrame(const CVehicle* vehicle, const CVector& pos) {
    const auto playerVeh = FindPlayerVehicle();
    const auto distSq    = GetDistSqToCamera(pos);
    const auto frame     = CTimer::m_FrameCounter + vehicle->m_nModelIndex;
    if (distSq > sq(25.0f)) {
        return false;
    }
    if (distSq > sq(20.0f)) {
        return (frame & 3) == 0;
    }
    if (distSq <= sq(8.0f) && playerVeh) {
        return true;
    }
    return (frame & 1) == 0;
}

// 0x4A12D0
void Fx_c::ExitEntitySystems() {
    for (auto it = m_FxEntities.GetHead(); it;) {
        const auto next = m_FxEntities.GetNext(it); // `it` gets deleted
        m_FxEntities.RemoveItem(it);
        g_fxMan.DestroyFxSystem(it->m_System);
        delete it;
        it = next;
    }
}

// 0x49EA90
void Fx_c::Init() {
    ZoneScoped;

    g_fxMan.Init();
    g_fxMan.LoadFxProject("models\\effects.fxp");
    g_fxMan.SetWindData(&CWeather::WindDir, &CWeather::Wind);
    InitStaticSystems();
    InitEntitySystems();
    m_Randomizer = 0;
}

// 0x4A1320
void Fx_c::Exit() {
    ExitEntitySystems();
    ExitStaticSystems();
    g_fxMan.Exit();
}

// 0x49EAE0
void Fx_c::Reset() {
    g_fxMan.DestroyAllFxSystems();
    InitStaticSystems();
    InitEntitySystems(); // NOTSA
}

// 0x4A11E0
void Fx_c::CreateEntityFx(CEntity* entity, const char* fxName, const CVector& pos, RwMatrix* transform) {
    // ((void(__thiscall*)(Fx_c*, CEntity*, char*, RwV3d*, RwMatrix*))0x4A11E0)(this, entity, fxName, pos transform);

    auto* particle = g_fxMan.CreateFxSystem(fxName, pos, transform, true);
    if (particle) {
        auto it = new FxEntitySystem();
        it->m_System = particle;
        it->m_Entity = entity;
        m_FxEntities.AddItem(it);
        it->m_System->Play();
    }
}

// 0x4A1280
void Fx_c::DestroyEntityFx(CEntity* entity) {
    // ((void(__thiscall*)(Fx_c*, CEntity*))0x4A1280)(this, entity);

    for (auto it = m_FxEntities.GetHead(); it;) {
        const auto next = m_FxEntities.GetNext(it); // `it` might get deleted
        if (it->m_Entity == entity) {
            m_FxEntities.RemoveItem(it);
            it->m_System->Kill();
            operator delete(it);
        }
        it = next;
    }
}

// 0x49E640
void Fx_c::Update(RwCamera* camera, float timeDelta) {
    ZoneScoped;

    g_fxMan.Update(camera, timeDelta);
}

// 0x49E650
void Fx_c::Render(RwCamera* camera, bool heatHaze) {
    ZoneScoped;

    g_fxMan.Render(camera, heatHaze);
}

// 0x49E950
void Fx_c::CreateMatFromVec(RwMatrix* out, const CVector* origin, const CVector* direction) {
    RwMatrixSetIdentity(out);
    *RwMatrixGetPos(out) = *origin;
    *RwMatrixGetUp(out)  = *direction;
    RwV3dNormalize(RwMatrixGetUp(out), RwMatrixGetUp(out));

    // NOTE: Android special-cases `direction == (0, 0, -1)` (and uses the Y axis instead), the PC version doesn't
    const CVector up    = *RwMatrixGetUp(out);
    const CVector right = CVector{ 0.0f, 0.0f, -1.0f }.Cross(up);
    const CVector at    = right.Cross(up);

    *RwMatrixGetRight(out) = right;
    *RwMatrixGetAt(out)    = at;

    RwMatrixUpdate(out);
}

// 0x49EA40
void Fx_c::SetFxQuality(FxQuality_e quality) {
    m_FxQuality = quality;
}

// 0x49EA50
FxQuality_e Fx_c::GetFxQuality() const {
    return m_FxQuality;
}

// 0x49EB00
void Fx_c::AddBlood(const CVector& pos, const CVector& direction, int32 amount, float lightMult) {
    if (!CLocalisation::Blood()) {
        return;
    }
    if (GetDistSqToCamera(pos) > sq(25.0f)) {
        return;
    }

    FxPrtMult_c fxMults{ 0.5f, 0.0f, 0.0f, 1.0f, 0.8f, 0.0f, 0.8f };
    for (auto i = amount; i > 0; i--) {
        fxMults.m_fSize = RandomUnitMod() * 0.3f + 0.7f;

        CVector vel = direction * 1.5f;
        vel.x += RandomUnitMod() * 2.0f - 1.0f;
        vel.y += RandomUnitMod() * 2.0f - 1.0f;
        vel.z += RandomUnitMod() * 2.0f - 1.0f;

        m_Blood->AddParticle(pos, vel, 0.0f, fxMults, -1.0f, lightMult, 0.6f, false);
    }

    // Blood pool on the ground
    CVector shadowPos = pos + direction * 0.5f;
    shadowPos.x += RandomUnitMod() * 0.2f - 0.1f;
    shadowPos.y += RandomUnitMod() * 0.2f - 0.1f;
    shadowPos.z += 1.0f;

    m_Randomizer++;
    switch (m_Randomizer & 7) {
    case 5: {
        const auto time = (CGeneral::GetRandomNumber() & 0xFFF) + 2000;
        CShadows::AddPermanentShadow(SHADOW_DEFAULT, gpBloodPoolTex, &shadowPos, 0.1f, 0.0f, 0.0f, -0.1f, 255, 200, 0, 0, 4.0f, time, 1.0f);
        break;
    }
    case 2: {
        const auto time = (CGeneral::GetRandomNumber() & 0xFFF) + 8000;
        CShadows::AddPermanentShadow(SHADOW_DEFAULT, gpBloodPoolTex, &shadowPos, 0.2f, 0.0f, 0.0f, -0.2f, 255, 200, 0, 0, 4.0f, time, 1.0f);
        break;
    }
    }
}

// 0x49EE10
void Fx_c::AddWood(const CVector& pos, const CVector& direction, int32 amount, float lightMult) {
    if (GetDistSqToCamera(pos) > sq(25.0f)) {
        return;
    }

    FxPrtMult_c fxMults{ 0.5f, 0.25f, 0.0f, 1.0f, 0.3f, 0.0f, 1.0f };
    for (auto i = amount; i > 0; i--) {
        fxMults.m_Color.red   = RandomUnitMod() * 0.12f + 0.13f;
        fxMults.m_Color.green = RandomUnitMod() * 0.03f + 0.12f;
        fxMults.m_Color.blue  = RandomUnitMod() * 0.03f + 0.04f;
        fxMults.m_fSize       = RandomUnitMod() * 0.3f + 0.7f;

        CVector vel = direction * 4.0f;
        vel.x += RandomUnitMod() * 4.0f - 2.0f;
        vel.y += RandomUnitMod() * 4.0f - 2.0f;
        vel.z += RandomUnitMod() * 4.0f - 2.0f;

        // NOTE: Yes, the blood system is used here
        m_Blood->AddParticle(pos, vel, 0.0f, fxMults, -1.0f, lightMult, 0.6f, false);
    }
}

// 0x49F040
void Fx_c::AddSparks(const CVector& origin, const CVector& direction, float force, int32 amount, CVector across, eSparkType sparksType, float spread, float life) {
    const auto distSq = GetDistSqToCamera(origin);
    if (distSq > sq(150.0f)) {
        return;
    }
    if (distSq > sq(15.0f) && (CTimer::m_FrameCounter & 1) != 0) {
        return;
    }

    const FxPrtMult_c fxMults{ 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, life * 0.8f };
    const CVector     acrossStep = across * CTimer::ms_fTimeStep;

    for (auto i = 0; i < amount; i++) {
        const auto t = 1.0f - static_cast<float>(i) / static_cast<float>(amount);

        CVector vel = direction;
        vel.x += RandomUnit() * (spread - -spread) + -spread;
        vel.y += RandomUnit() * (spread - -spread) + -spread;
        vel.z += RandomUnit() * (spread - -spread) + -spread;
        vel *= force;

        const CVector pos = origin - acrossStep * t;

        const auto system = sparksType != SPARK_PARTICLE_SPARK2 ? m_Spark : m_Spark2;
        system->AddParticle(pos, vel, t * 0.05f, fxMults, -1.0f, 1.2f, 0.6f, false);
    }
}

// 0x49F300
void Fx_c::AddTyreBurst(const CVector& posn, const CVector& velocity) {
    if (GetDistSqToCamera(posn) > sq(25.0f)) {
        return;
    }

    const FxPrtMult_c fxMults{ 1.0f, 1.0f, 1.0f, 0.4f, 0.12f, 0.0f, 0.1f };
    for (auto i = 0; i < 4; i++) {
        m_SmokeII3expand->AddParticle(posn, velocity, static_cast<float>(i) * 0.05f, fxMults, -1.0f, 1.2f, 0.6f, false);
    }
}

// 0x49F3D0
void Fx_c::AddBulletImpact(const CVector& posn, const CVector& direction, int32 bulletFxType, int32 amount, float arg4) {
    // NOTE: `bulletFxType` is actually a surface id, and `arg4` is the light multiplier
    const auto fxType    = g_surfaceInfos.GetBulletFx(static_cast<SurfaceId>(bulletFxType));
    const auto lightMult = arg4;

    if (GetDistSqToCamera(posn) > sq(150.0f)) {
        return;
    }

    switch (fxType) {
    case BULLET_FX_SPARKS: {
        AddSparks(posn, direction, 3.0f, amount, CVector{ 0.0f, 0.0f, 0.0f }, SPARK_PARTICLE_SPARK, 0.4f, 1.0f);

        FxPrtMult_c fxMults{ 1.0f, 1.0f, 1.0f, 0.15f, 0.4f, 0.0f, 0.075f };
        auto        count = 2;
        if (amount >= 8) {
            count = 1;
            fxMults.m_Color.alpha *= 2.0f;
        }
        for (auto i = 0; i < count; i++) {
            m_SmokeII3expand->AddParticle(posn, direction, static_cast<float>(i) * 0.05f, fxMults, -1.0f, lightMult, 0.6f, false);
        }
        break;
    }
    case BULLET_FX_SAND:
    case BULLET_FX_DUST: {
        FxPrtMult_c fxMults{ 0.81f, 0.67f, 0.57f, 0.15f, 0.4f, 0.0f, 0.3f };
        if (fxType == BULLET_FX_DUST) {
            fxMults.m_Color.red   = 0.6f;
            fxMults.m_Color.green = 0.6f;
            fxMults.m_Color.blue  = 0.6f;
        }
        auto count = 4;
        if (amount >= 8) {
            count = 2;
            fxMults.m_Color.alpha *= 2.0f;
        }
        for (auto i = 0; i < count; i++) {
            const CVector vel = direction * 0.3f;
            m_Sand->AddParticle(posn, vel, static_cast<float>(i) * 0.05f, fxMults, -1.0f, lightMult, 0.6f, false);
        }
        break;
    }
    case BULLET_FX_WOOD: {
        AddWood(posn, direction, static_cast<int32>(static_cast<float>(amount) * 0.5f), 1.0f);
        break;
    }
    }
}

// 0x49F670
void Fx_c::AddPunchImpact(const CVector& pos, const CVector& velocity, int32 num) {
    if (GetDistSqToCamera(pos) > sq(25.0f)) {
        return;
    }

    const FxPrtMult_c fxMults{ 1.0f, 1.0f, 1.0f, 0.4f, 0.1f, 0.0f, 0.1f };
    m_SmokeII3expand->AddParticle(pos, velocity, 0.0f, fxMults, -1.0f, 1.2f, 0.6f, false);
    m_SmokeII3expand->AddParticle(pos, velocity, 0.05f, fxMults, -1.0f, 1.2f, 0.6f, false);
}

// 0x49F750
void Fx_c::AddDebris(const CVector& pos, const RwRGBA& color, float scale, int32 amount) {
    static auto& s_DebrisPrimIdx = StaticRef<int32, 0xA9ADE4>();

    if (GetDistSqToCamera(pos) > sq(25.0f)) {
        return;
    }

    FxPrtMult_c fxMults{};
    fxMults.m_Color.red   = static_cast<float>(color.red) / 255.0f;
    fxMults.m_Color.green = static_cast<float>(color.green) / 255.0f;
    fxMults.m_Color.blue  = static_cast<float>(color.blue) / 255.0f;
    fxMults.m_Color.alpha = static_cast<float>(color.alpha) / 255.0f;
    fxMults.m_fSize       = scale;
    fxMults.m_fLife       = 0.2f;
    fxMults.m_Rot         = (RandomUnitMod() + 1.0f) * 0.5f;

    for (auto i = amount; i > 0; i--) {
        CVector vel;
        vel.x = RandomUnit() * 0.5f * 20.0f - 5.0f;
        vel.y = RandomUnit() * 0.5f * 20.0f - 5.0f;
        vel.z = RandomUnit() * 0.15f * 20.0f + 2.0f;

        // Only one of the 4 debris primitives is used for each particle
        m_Cardebris->EnablePrim(0, false);
        m_Cardebris->EnablePrim(1, false);
        m_Cardebris->EnablePrim(2, false);
        m_Cardebris->EnablePrim(3, false);
        m_Cardebris->EnablePrim(s_DebrisPrimIdx, true);

        m_Cardebris->AddParticle(pos, vel, 0.0f, fxMults, -1.0f, 1.2f, 0.6f, false);

        s_DebrisPrimIdx = (s_DebrisPrimIdx + 1) & 3;
    }
}

// 0x49F970
void Fx_c::AddGlass(const CVector& pos, const RwRGBA& color, float scale, int32 amount) {
    if (GetDistSqToCamera(pos) > sq(25.0f)) {
        return;
    }

    FxPrtMult_c fxMults{};
    fxMults.m_Color.red   = static_cast<float>(color.red) / 255.0f;
    fxMults.m_Color.green = static_cast<float>(color.green) / 255.0f;
    fxMults.m_Color.blue  = static_cast<float>(color.blue) / 255.0f;
    fxMults.m_Color.alpha = static_cast<float>(color.alpha) / 255.0f;
    fxMults.m_fSize       = scale;
    fxMults.m_fLife       = 0.2f;
    fxMults.m_Rot         = (RandomUnitMod() + 1.0f) * 0.5f;

    for (auto i = amount; i > 0; i--) {
        CVector vel;
        vel.x = RandomUnit() * 0.5f * 20.0f - 5.0f;
        vel.y = RandomUnit() * 0.5f * 20.0f - 5.0f;
        vel.z = RandomUnit() * 0.15f * 20.0f + 2.0f;

        m_Glass->AddParticle(pos, vel, 0.0f, fxMults, -1.0f, 1.2f, 0.6f, false);
    }
}

// 0x49FB30
void Fx_c::AddWheelSpray(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, bool bInWater, float lightMult) {
    if (!ShouldAddWheelFxThisFrame(vehicle, pos)) {
        return;
    }

    const auto& moveSpeed = vehicle->GetMoveSpeed();
    if (std::abs(moveSpeed.Magnitude()) <= 0.01f && !bWheelsSpinning) {
        return;
    }

    FxPrtMult_c fxMults{ 1.0f, 1.0f, 1.0f, 0.05f, 0.0f, 1.0f, 0.0f };

    const auto speedMult = bWheelsSpinning
        ? 1.0f
        : std::min(moveSpeed.Magnitude() * 2.0f, 1.0f);

    fxMults.m_Color.alpha = (speedMult + 1.0f) * (bInWater ? 0.2f : 0.15f);
    fxMults.m_fLife       = 0.08f;
    fxMults.m_fSize       = (speedMult + 1.0f) * 0.2f;

    const auto velRange = (speedMult + 1.0f) * 10.0f;
    const auto velMin   = 30.0f - velRange;
    const auto velMult  = RandomUnit() * (velRange + 30.0f - velMin) + velMin;
    const CVector baseVel = moveSpeed * velMult;

    const auto count = std::max(1, static_cast<int32>((moveSpeed * CTimer::ms_fTimeStep).Magnitude()));
    for (auto i = 0; i < count; i++) {
        const auto    step   = 1.0f / static_cast<float>(count);
        const CVector offset = moveSpeed * step * static_cast<float>(i) * CTimer::ms_fTimeStep;

        CVector prtPos = pos - offset;
        prtPos.z += 0.25f;

        CVector vel = baseVel;
        vel.z += (RandomUnit() + 1.0f) * speedMult;

        m_BoatSplash->AddParticle(prtPos, vel, 0.0f, fxMults, -1.0f, lightMult, 0.6f, false);
    }
}

// NOTSA - Common code of AddWheelGrass/Gravel/Mud (They only differ in the color)
static void AddWheelDirt(FxSystem_c* system, CVehicle* vehicle, const CVector& pos, float lightMult, float red, float green, float blue) {
    // Only for vehicles driven by one of the players
    if (vehicle->m_pDriver != FindPlayerPed(0) && vehicle->m_pDriver != FindPlayerPed(1)) {
        return;
    }
    if (!ShouldAddWheelFxThisFrame(vehicle, pos)) {
        return;
    }

    FxPrtMult_c fxMults{ red, green, blue, 1.0f, 0.0f, 0.0f, 0.05f };
    for (auto i = 0; i < 3; i++) {
        fxMults.m_fSize = RandomUnit() * 0.03f + 0.03f;

        const auto& moveSpeed = vehicle->GetMoveSpeed();

        CVector vel;
        vel.x = RandomUnit() * (moveSpeed.x * -1.5f);
        vel.y = RandomUnit() * (moveSpeed.y * -1.5f);
        vel.z = RandomUnit() * 1.5f + 2.0f;

        CVector prtPos = pos;
        prtPos.x = RandomUnit() * 0.4f + prtPos.x - 0.2f;
        prtPos.y = RandomUnit() * 0.4f + prtPos.y - 0.2f;

        system->AddParticle(prtPos, vel, 0.0f, fxMults, -1.0f, lightMult, 0.6f, false);
    }
}

// 0x49FF20
void Fx_c::AddWheelGrass(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, float lightMult) {
    AddWheelDirt(m_WheelDirt, vehicle, pos, lightMult, 0.03f, 0.09f, 0.03f);
}

// 0x4A0170
void Fx_c::AddWheelGravel(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, float lightMult) {
    AddWheelDirt(m_WheelDirt, vehicle, pos, lightMult, 0.25f, 0.25f, 0.25f);
}

// 0x4A03C0
void Fx_c::AddWheelMud(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, float lightMult) {
    AddWheelDirt(m_WheelDirt, vehicle, pos, lightMult, 0.25f, 0.12f, 0.06f);
}

// NOTSA - Common code of AddWheelSand/Dust (They only differ in the color, and how the life is calculated)
static void AddWheelSandOrDust(Fx_c& fx, CVehicle* vehicle, const CVector& pos, bool bWheelsSpinning, float lightMult, bool isDust) {
    const auto playerVeh = FindPlayerVehicle();
    const auto distSq    = GetDistSqToCamera(pos);
    if (distSq > sq(25.0f)) {
        return;
    }

    const auto frame        = CTimer::m_FrameCounter + vehicle->m_nModelIndex;
    const auto isNearPlayer = distSq <= sq(8.0f) && playerVeh;
    if (fx.m_FxQuality >= FX_QUALITY_MEDIUM) {
        if ((frame & 1) != 0) {
            return;
        }
        if (!isNearPlayer && (frame & 3) != 0) {
            return;
        }
    } else if (fx.m_FxQuality == FX_QUALITY_LOW) {
        if ((frame & 3) != 0) {
            return;
        }
        if (!isNearPlayer && (frame & 7) != 0) {
            return;
        }
    }

    auto fxMults = isDust
        ? FxPrtMult_c{ 0.51f, 0.44f, 0.31f, 0.5f, 1.0f, 0.0f, 0.0f }
        : FxPrtMult_c{ 0.81f, 0.67f, 0.57f, 0.5f, 1.0f, 0.0f, 0.0f };

    const auto  gasPedal  = std::abs(vehicle->m_GasPedal);
    const auto& moveSpeed = vehicle->GetMoveSpeed();
    const auto  speedMult = bWheelsSpinning
        ? 1.0f
        : std::min(moveSpeed.Magnitude() * 2.0f, 1.0f);

    fxMults.m_fLife = isDust
        ? speedMult * 0.05f + 0.1f
        : (speedMult + 1.0f) * 0.1f;
    fxMults.m_fSize = speedMult * 0.9f + 0.1f;

    float countMult;
    switch (vehicle->m_nVehicleSubType) {
    case VEHICLE_TYPE_BMX:
        countMult = 2.0f;
        fxMults.m_fSize *= 0.25f;
        break;
    case VEHICLE_TYPE_BIKE:
    case VEHICLE_TYPE_QUAD:
        countMult = 2.0f;
        fxMults.m_fSize *= 0.5f;
        break;
    default:
        countMult = 1.5f;
        fxMults.m_fSize *= 0.7f;
        break;
    }

    const CVector step   = moveSpeed * CTimer::ms_fTimeStep;
    const auto    count  = std::max(1, static_cast<int32>(step.Magnitude() * countMult));
    const auto    velZ   = speedMult + 0.8f - 0.2f;
    for (auto i = 0; i < count; i++) {
        CVector vel;
        vel.x = RandomUnit() * (gasPedal * vehicle->GetMoveSpeed().x * -40.0f);
        vel.y = RandomUnit() * (gasPedal * vehicle->GetMoveSpeed().y * -40.0f);
        vel.z = RandomUnit() * velZ + 0.2f;

        const auto    t      = 1.0f - static_cast<float>(i) / static_cast<float>(count);
        const CVector prtPos = pos - step * t;

        fx.m_Sand->AddParticle(prtPos, vel, 0.0f, fxMults, -1.0f, lightMult, 0.7f, false);
    }
}

// 0x4A0610
void Fx_c::AddWheelSand(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, float lightMult) {
    AddWheelSandOrDust(*this, vehicle, pos, bWheelsSpinning, lightMult, false);
}

// 0x4A09C0
void Fx_c::AddWheelDust(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, float lightMult) {
    AddWheelSandOrDust(*this, vehicle, pos, bWheelsSpinning, lightMult, true);
}

// 0x4A0D70
void Fx_c::TriggerWaterHydrant(const CVector& pos) {
    CreateFxWithinCameraRange("water_hydrant", pos, 625.0f);
}

// 0x4A0DE0
void Fx_c::TriggerGunshot(CEntity* entity, const CVector& origin, const CVector& target, bool doGunflash) {
    if (GetDistSqToCamera(origin) > sq(25.0f)) {
        return;
    }

    RwMatrix* createdMat = nullptr; // Matrix we have to destroy afterwards
    RwMatrix* parentMat;
    CVector   pos;
    if (entity) {
        // Transform the origin into the entity's space
        const CVector offset = origin - entity->GetPosition();
        pos = entity->GetMatrix().InverseTransformVector(offset);

        if (!entity->GetRwObject()) {
            return;
        }
        parentMat = entity->GetRwMatrix();
    } else {
        createdMat = g_fxMan.FxRwMatrixCreate();
        CreateMatFromVec(createdMat, &origin, &target);
        pos       = CVector{ 0.0f, 0.0f, 0.0f };
        parentMat = createdMat;
    }

    if (parentMat) {
        if (doGunflash) {
            if (const auto fx = g_fxMan.CreateFxSystem("gunflash", pos, parentMat, false)) {
                if (!entity) {
                    fx->CopyParentMatrix();
                }
                fx->PlayAndKill();
            }
        }

        if (const auto fx = g_fxMan.CreateFxSystem("gunsmoke", pos, parentMat, false)) {
            if (!entity) {
                fx->CopyParentMatrix();
            }
            fx->PlayAndKill();
        }
    }

    if (createdMat) {
        g_fxMan.FxRwMatrixDestroy(createdMat);
    }
}

// 0x4A0FA0
void Fx_c::TriggerTankFire(const CVector& pos, const CVector& dir) {
    if (GetDistSqToCamera(pos) > sq(25.0f)) {
        return;
    }

    const auto mat = g_fxMan.FxRwMatrixCreate();
    CreateMatFromVec(mat, &pos, &dir);

    if (const auto fx = g_fxMan.CreateFxSystem("tank_fire", CVector{ 0.0f, 0.0f, 0.0f }, mat, false)) {
        fx->CopyParentMatrix();
        fx->PlayAndKill();
    }

    g_fxMan.FxRwMatrixDestroy(mat);
}

// 0x4A1070
void Fx_c::TriggerWaterSplash(const CVector& pos) {
    CreateFxWithinCameraRange("water_splash_big", pos, 625.0f);
}

// 0x4A10E0
void Fx_c::TriggerBulletSplash(const CVector& pos) {
    CreateFxWithinCameraRange("water_splash", pos, 625.0f);
}

// 0x4A1150
void Fx_c::TriggerFootSplash(const CVector& pos) {
    CreateFxWithinCameraRange("water_splsh_sml", pos, 625.0f);
}

// see RwIm3DTransformFlags
// 0x4A13B0
void RenderBegin(RwRaster* newRaster, RwMatrix* transform, uint32 transformRenderFlags) {
    g_fx.m_pTransformLTM = transform;
    g_fx.m_nVerticesCount = 0;
    g_fx.m_nVerticesCount2 = 0;
    g_fx.m_pRasterToRender = newRaster;
    g_fx.m_nTransformRenderFlags = transformRenderFlags;
    g_fx.m_pVerts = TempBufferVertices.m_3d;

    // And maybe update raster on RW if the same isnt already set...
    RwRaster* currRaster{};
    RwRenderStateGet(rwRENDERSTATETEXTURERASTER, &currRaster);
    if (currRaster != newRaster)
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RWRSTATE(newRaster));
}

// Wrapper for original func
// 0x4A1410
void RenderAddTri_(
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    float x3, float y3, float z3,
    float u1, float v1,
    float u2, float v2,
    float u3, float v3,
    int32 r1, int32 g1, int32 b1, int32 a1,
    int32 r2, int32 g2, int32 b2, int32 a2,
    int32 r3, int32 g3, int32 b3, int32 a3
) {
    RenderAddTri(
        { x1, y1, z1 },
        { x2, y2, z2 },
        { x3, y3, z3 },
        { u1, v1 },
        { u2, v2 },
        { u3, v3 },
        CRGBA().FromInt32(r1, g1, b1, a1),
        CRGBA().FromInt32(r2, g2, b2, a2),
        CRGBA().FromInt32(r3, g3, b3, a3)
    );
}

// TODO: I honestly think this should be a class method...
// Although originally it wasnt.
// NOTE: Method signature changed to use CVector + RwTexCoords instead of raw values for convenience.
void RenderAddTri(CVector pos1, CVector pos2, CVector pos3, RwTexCoords coord1, RwTexCoords coord2, RwTexCoords coord3, const CRGBA& color1, const CRGBA& color2, const CRGBA& color3) {
    const auto GetVertex = [](unsigned i) {
        return &g_fx.m_pVerts[i];
    };

    const CVector pos[] = { pos1, pos2, pos3 };
    const RwRGBA color[] = {
        { color1.ToRwRGBA() },
        { color2.ToRwRGBA() },
        { color3.ToRwRGBA() },
    };
    for (unsigned i = 0; i < 3; i++) {
        RxObjSpace3DVertexSetPos(GetVertex(i), &pos[i]);
        RxObjSpace3DVertexSetPreLitColor(GetVertex(i), &color[i]);
    }

    if (g_fx.m_pRasterToRender) {
        const RwTexCoords uvs[] = { coord1, coord2, coord3 };
        for (unsigned i = 0; i < 3; i++) {
            RxObjSpace3DVertexSetU(GetVertex(i), uvs[i].u);
            RxObjSpace3DVertexSetV(GetVertex(i), uvs[i].v);
        }
    }

    g_fx.m_pVerts += 3;
    g_fx.m_nVerticesCount2 += 3;
    g_fx.m_nVerticesCount++;

    if (g_fx.m_nVerticesCount2 >= TOTAL_TEMP_BUFFER_3DVERTICES - 3 || g_fx.m_nVerticesCount >= TOTAL_TEMP_BUFFER_INDICES - 1) {
        RenderEnd(); // Render vertices to free up vertex buffer
    }
}

// 0x4A1600
void RenderEnd() {
    if (!g_fx.m_nVerticesCount)
        return;

    if (RwIm3DTransform(TempBufferVertices.m_3d, 3 * g_fx.m_nVerticesCount, g_fx.m_pTransformLTM, g_fx.m_nTransformRenderFlags)) {
        RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
        RwIm3DEnd();
    }

    g_fx.m_pVerts = TempBufferVertices.m_3d;
    g_fx.m_nVerticesCount2 = 0;
    g_fx.m_nVerticesCount = 0;
}

// 0x4A1660
void RotateVecIntoVec(RwV3d& vecRes, const RwV3d& vec, const RwV3d& vecAlign) {
    const CVector up = vecAlign;
    const auto ref = CVector{ 3.f, 4.f, 5.f }.Normalized();

    RwV3d right;
    RwV3dCrossProduct(&right, &up, &ref);
    RwV3dNormalize(&right, &right);

    RwV3d at;
    RwV3dCrossProduct(&at, &up, &right);

    auto* m = g_fxMan.FxRwMatrixCreate();
    m->right = right;
    m->up    = up;
    m->at    = at;
    m->pos   = { 0.0f, 0.0f, 0.0f };

    RwMatrixUpdate(m);
    RwV3dTransformVectors(&vecRes, &vec, 1, m);
    g_fxMan.FxRwMatrixDestroy(m);
}

// 0x4A1780
void RotateVecAboutVec(RwV3d& vecRes, const RwV3d& vec, const RwV3d& axis, float angle) {
    const float x = axis.x, y = axis.y, z = axis.z;

    const float s = CMaths::GetSinFast(angle);
    const float c = CMaths::GetCosFast(angle);
    const float t = 1.0f - c;

    vecRes.x = (t*x*x + c)   * vec.x + (t*x*y - s*z) * vec.y + (t*x*z + s*y) * vec.z;
    vecRes.y = (t*x*y + s*z) * vec.x + (t*y*y + c)   * vec.y + (t*y*z - s*x) * vec.z;
    vecRes.z = (t*x*z - s*y) * vec.x + (t*y*z + s*x) * vec.y + (t*z*z + c)   * vec.z;
}
