/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "CarCtrl.h"
#include "Shadows.h"
#include "WindModifiers.h"
#include "CustomBuildingDNPipeline.h"
#include "Ropes.h"
#include "InterestingEvents.h"
#include "Tasks/TaskTypes/TaskComplexUseSwatRope.h"
#include "Tasks/TaskTypes/TaskComplexWanderCop.h"

auto& HELI_MAIN_ROTOR_SPIN_MULT = StaticRef<float>(0x8D33A0); // 1.66f

void CHeli::InjectHooks() {
    RH_ScopedVirtualClass(CHeli, 0x871680, 71);
    RH_ScopedCategory("Vehicle");

    RH_ScopedInstall(InitHelis, 0x6C4560);
    RH_ScopedInstall(AddHeliSearchLight, 0x6C45B0);
    RH_ScopedInstall(Pre_SearchLightCone, 0x6C4650);
    RH_ScopedInstall(Post_SearchLightCone, 0x6C46E0);
    RH_ScopedInstall(FindSwatPositionRelativeToHeli, 0x6C4760);
    RH_ScopedInstall(SwitchPoliceHelis, 0x6C4800);
    RH_ScopedInstall(SearchLightCone, 0x6C58E0);
    RH_ScopedInstall(RenderAllHeliSearchLights, 0x6C7C50);
    RH_ScopedInstall(GenerateHeli, 0x6C6520);
    RH_ScopedInstall(TestSniperCollision, 0x6C6890);
    RH_ScopedInstall(SendDownSwat, 0x6C69C0);
    RH_ScopedInstall(UpdateHelis, 0x6C79A0);
    RH_ScopedVMTInstall(ProcessControl, 0x6C7050);
    RH_ScopedVMTInstall(ProcessControlInputs, 0x6C4830);
    RH_ScopedVMTInstall(Render, 0x6C4400);
    RH_ScopedVMTInstall(Fix, 0x6C4530);
    RH_ScopedVMTInstall(BurstTyre, 0x6C4330);
    RH_ScopedVMTInstall(SetUpWheelColModel, 0x6C4320);
    RH_ScopedVMTInstall(BlowUpCar, 0x6C6D30);
    RH_ScopedVMTInstall(ProcessFlyingCarStuff, 0x6C4E60);
    RH_ScopedVMTInstall(PreRender, 0x6C5420);
}

// 0x6C4190
CHeli::CHeli(int32 modelIndex, eVehicleCreatedBy createdBy) : CAutomobile(modelIndex, createdBy, true) {
    m_nVehicleSubType = VEHICLE_TYPE_HELI;

    m_fLeftRightSkid           = 0.0f;
    m_fSteeringUpDown          = 0.0f;
    m_fSteeringLeftRight       = 0.0f;
    m_fAccelerationBreakStatus = 0.0f;

    field_99C = 0;
    m_fRotorZ = 0;
    m_fSecondRotorZ = 0;

    m_fMinAltitude = 10.0f;
    m_fMaxAltitude = 10.0f;

    field_9AC = 10.0f;
    field_9B4 = 0;

    m_nHeliFlags = m_nHeliFlags & 0xFC;
    m_fSearchLightIntensity = 0.0f;
    physicalFlags.bDontCollideWithFlyers = true;

    if (modelIndex == MODEL_HUNTER) {
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_OK);
        m_doors[DOOR_LEFT_FRONT].Init((3.0f * PI) / 10.0f, 0.0f, DOOR_AXIS_NEG_X, DOOR_AXIS_Y, DOOR_EXTRA_BASED);
    }

    m_nNumSwatOccupants = 4;
    m_aSwatState.fill(0);

    m_nSearchLightTimer = CTimer::GetTimeInMS();

    m_aSearchLightHistoryX.fill(0.0f);
    m_aSearchLightHistoryY.fill(0.0f);

    m_nShootTimer = 0;
    m_nPoliceShoutTimer = CTimer::GetTimeInMS();

    vehicleFlags.bNeverUseSmallerRemovalRange = true; // 0x6C42BD
    m_autoPilot.m_ucHeliTargetDist2 = 10;

    m_ppGunflashFx = nullptr;
    m_nFiringMultiplier = 16;

    field_9B8 = 0;
    m_bSearchLightEnabled = false;
    field_A14 = CGeneral::GetRandomNumberInRange(2.f, 8.f);
}

// 0x6C4340
CHeli::~CHeli() {
    if (m_ppGunflashFx) {
        for (auto i = 0; i < CVehicle::GetPlaneNumGuns(); i++) {
            if (auto& fx = m_ppGunflashFx[i]) {
                fx->Kill();
                g_fxMan.DestroyFxSystem(fx);
            }
        }
        delete[] m_ppGunflashFx;
        m_ppGunflashFx = nullptr;
    }

    m_vehicleAudio.Terminate();
}

// 0x6C4560
void CHeli::InitHelis() {
    std::ranges::fill(pHelis, nullptr);
    for (auto& light : HeliSearchLights) {
        light.Init();
    }
    NumberOfSearchLights = 0;
    bPoliceHelisAllowed = true;
}

// 0x6C45B0
void CHeli::AddHeliSearchLight(const CVector& origin, const CVector& target, float targetRadius, float power, uint32 coronaIndex, uint8 unknownFlag, uint8 drawShadow) {
    auto& light = HeliSearchLights[NumberOfSearchLights];

    light.m_vecOrigin     = origin;
    light.m_vecTarget     = target;
    light.m_fTargetRadius = targetRadius;
    light.m_fPower        = power;
    light.m_nCoronaIndex  = coronaIndex;
    light.field_24        = unknownFlag;
    light.m_bDrawShadow   = drawShadow;

    NumberOfSearchLights += 1;
}

// 0x6C4640
void CHeli::PreRenderAlways() {
    // NOP
}

// 0x6C4650
void CHeli::Pre_SearchLightCone() {
    ZoneScoped;

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,         RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,          RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,             RWRSTATE(rwBLENDONE));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,            RWRSTATE(rwBLENDONE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,    RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,        RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,            RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATESHADEMODE,            RWRSTATE(rwSHADEMODEGOURAUD));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION,    RWRSTATE(rwALPHATESTFUNCTIONGREATEREQUAL));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(0));
}

// 0x6C46E0
void CHeli::Post_SearchLightCone() {
    ZoneScoped;

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,         RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,          RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,             RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,            RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,    RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATECULLMODE,             RWRSTATE(rwCULLMODECULLBACK));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION,    RWRSTATE(rwALPHATESTFUNCTIONGREATER));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(2u));
}

// 0x6C4750
void CHeli::SpecialHeliPreRender() {
    // NOP
}

// 0x6C4760
CVector CHeli::FindSwatPositionRelativeToHeli(int32 swatNumber) {
    switch (swatNumber) {
    case 0:
        return { -1.2f, -1.0f, -0.5f };
    case 1:
        return { 1.2f,  -1.0f, -0.5f };
    case 2:
        return { -1.2f, 1.0f,  -0.5f };
    case 3:
        return { 1.2f,  1.0f,  -0.5f };
    default:
        return { 0.0f,  0.0f,  0.0f  };
    }
}

// 0x6C4800
void CHeli::SwitchPoliceHelis(bool enable) {
    bPoliceHelisAllowed = enable;
}

// 0x6C58E0
void CHeli::SearchLightCone(int32 coronaIndex,
                            CVector origin,
                            CVector target,
                            float targetRadius,
                            float power,
                            uint8 unknownFlag,
                            uint8 drawShadow,
                            CVector& useless0,
                            CVector& useless1,
                            CVector& useless2,
                            bool a11,
                            float baseRadius,
                            float a13,
                            float a14,
                            float a15
) {
    constexpr auto NUM_SEGMENTS = 40;
    constexpr auto NUM_VERTS    = (NUM_SEGMENTS + 1) * 2;

    // NOTE: `unknownFlag` (clipIfColliding), `a11`, `a13`, `a14` and `a15` are not used by the original code

    auto dir = target - origin;
    dir.Normalise();

    // 0x6C593E - Find where the light hits the ground
    target += dir * 3.0f;
    CColPoint colPoint;
    CEntity*  hitEntity;
    if (CWorld::ProcessLineOfSight(origin, target, colPoint, hitEntity, true, false, false, false, false, false, false, false)) {
        target = colPoint.m_vecPoint;
    }

    const auto farCenter = origin + dir * 100.0f;

    auto dirToCam = TheCamera.GetPosition() - origin;
    dirToCam.Normalise();

    // 0x6C5A8E - The more the light points towards the camera the brighter the corona is
    {
        const auto facing  = std::max(DotProduct(dirToCam, dir), 0.0f);
        const auto facing6 = facing * facing * facing * facing * facing * facing;
        CCoronas::RegisterCorona(
            coronaIndex,
            nullptr,
            200, 200, 255, static_cast<uint8>(facing6 * 255.0f),
            origin,
            20.0f * facing6,
            100.0f,
            CORONATYPE_SHINYSTAR,
            FLARETYPE_NONE,
            CORREFL_SIMPLE,
            LOSCHECK_OFF,
            TRAIL_OFF,
            0.0f,
            false,
            1.5f,
            false,
            15.0f,
            false,
            false
        );
    }

    // 0x6C5B27 - Build the cone
    std::array<float, NUM_VERTS> vertBrightness{}; // Per-vertex base brightness
    std::array<float, NUM_VERTS> vertFacing{};     // Per-vertex (squared) camera facing factor
    float   maxFacing = 0.0f;
    CVector groundPt20{}, groundPt30{};

    int32 numVerts = 0, numIndices = 0;
    uiTempBufferIndicesStored  = 0;
    uiTempBufferVerticesStored = 0;

    for (int32 i = 0; i <= NUM_SEGMENTS; i++) {
        auto right = CrossProduct(dir, CVector{ 0.0f, 0.0f, 1.0f });
        right.Normalise();
        auto up = CrossProduct(right, dir);
        up.Normalise();

        const auto angle = static_cast<float>(i) * (2.0f * 3.14159274f / static_cast<float>(NUM_SEGMENTS)); // = 0.15707964f
        const auto offset = right * std::sin(angle) + up * std::cos(angle);

        const auto nearPt  = offset * baseRadius + origin;
        const auto farEdge = offset * targetRadius + farCenter;

        // Intersect this edge of the cone with the ground plane (Height of `target`)
        const auto t        = (nearPt.z - target.z) / (nearPt.z - farEdge.z);
        const auto groundPt = (farEdge - nearPt) * t + nearPt;

        switch (i) {
        case 20: groundPt20 = groundPt; break;
        case 30: groundPt30 = groundPt; break;
        }

        // 0x6C5E7A - Limit the length of the edge
        auto endPt = groundPt;
        if (auto edge = groundPt - nearPt; edge.Magnitude() > 100.0f) {
            edge.Normalise();
            endPt = edge * 100.0f + nearPt;
        }

        RwIm3DVertexSetPos(&TempBufferVertices.m_3d[numVerts + 0], nearPt.x, nearPt.y, nearPt.z);
        RwIm3DVertexSetPos(&TempBufferVertices.m_3d[numVerts + 1], endPt.x, endPt.y, endPt.z);

        auto originToNear = nearPt - origin;
        originToNear.Normalise();
        const auto facing = sq(std::abs(DotProduct(originToNear, dirToCam)));
        maxFacing = std::max(maxFacing, facing);

        vertBrightness[numVerts + 0] = CCustomBuildingDNPipeline::m_fDNBalanceParam * 0.15f + 0.1f;
        vertBrightness[numVerts + 1] = 0.0f;
        vertFacing[numVerts + 0]     = facing;
        vertFacing[numVerts + 1]     = facing;

        if (i != NUM_SEGMENTS) {
            aTempBufferIndices[numIndices++] = numVerts;
            aTempBufferIndices[numIndices++] = numVerts + 3;
            aTempBufferIndices[numIndices++] = numVerts + 1;
            if (baseRadius > 0.0f) {
                aTempBufferIndices[numIndices++] = numVerts;
                aTempBufferIndices[numIndices++] = numVerts + 2;
                aTempBufferIndices[numIndices++] = numVerts + 3;
            }
            uiTempBufferIndicesStored = numIndices;
        }

        numVerts += 2;
        uiTempBufferVerticesStored = numVerts;
    }

    // 0x6C60B7 - Calculate vertex colors
    for (int32 i = 0; i < numVerts; i++) {
        const auto f = vertFacing[i] * vertBrightness[i] * (1.0f / maxFacing);
        RwIm3DVertexSetRGBA(
            &TempBufferVertices.m_3d[i],
            static_cast<uint8>(static_cast<uint32>(200.0f * f)),
            static_cast<uint8>(static_cast<uint32>(200.0f * f)),
            static_cast<uint8>(static_cast<uint32>(f * 255.0f)),
            0
        );
    }

    if (numIndices > 0 && RwIm3DTransform(TempBufferVertices.m_3d, numVerts, nullptr, rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA)) {
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, aTempBufferIndices, uiTempBufferIndicesStored);
        RwIm3DEnd();
    }

    // 0x6C6340
    useless0 = target;
    useless1 = groundPt20 - target;
    useless2 = groundPt30 - target;

    if (!drawShadow) {
        return;
    }

    // 0x6C63B6 - Light on the ground
    const auto shadowPos   = CVector{ target.x, target.y, target.z + 5.0f };
    const auto shadowFront = CVector2D{ useless1.x, useless1.y } * 1.2f;
    const auto shadowSide  = CVector2D{ useless2.x, useless2.y } * 1.2f;
    if (shadowFront.Magnitude() >= 100.0f || shadowSide.Magnitude() >= 100.0f) {
        return;
    }
    const auto camDist2D = (target - TheCamera.GetPosition()).Magnitude2D();
    if (camDist2D > 25.0f) {
        return;
    }
    const auto intensity = (1.0f - camDist2D * 0.04f) * power * 0.5f;
    CShadows::StoreShadowToBeRendered(
        SHADOW_ADDITIVE,
        gpShadowExplosionTex,
        shadowPos,
        shadowFront.x, shadowFront.y,
        shadowSide.x, shadowSide.y,
        static_cast<int16>(intensity * 128.0f),
        static_cast<uint8>(200.0f * intensity),
        static_cast<uint8>(200.0f * intensity),
        static_cast<uint8>(255.0f * intensity),
        15.0f,
        true,
        1.0f,
        nullptr,
        false
    );
}

// 0x6C6520
CHeli* CHeli::GenerateHeli(CPed* target, bool newsHeli) {
    auto* const heli = new CHeli(newsHeli ? MODEL_VCNMAV : MODEL_POLMAV, PERMANENT_VEHICLE);

    // 0x6C65AD - Pick a random position 250 units away from the target
    auto angle = static_cast<float>(CGeneral::GetRandomNumber() & 0xFF) * (2.0f * 3.14159274f / 256.0f); // = 0.02453125f
    auto pos   = target->GetPosition();
    pos.x += std::cos(angle) * 250.0f;
    pos.y += std::sin(angle) * 250.0f;
    if (pos.x < -3000.0f || pos.x > 3000.0f || pos.y < -3000.0f || pos.y > 3000.0f) { // Outside of the map, so use the opposite direction
        angle += 3.14159274f;
        pos    = target->GetPosition();
        pos.x += std::cos(angle) * 250.0f;
        pos.y += std::sin(angle) * 250.0f;
    }
    pos.z += 50.0f;

    // 0x6C669C
    if (std::hypot(pos.x - -2322.0f, pos.y - -1653.0f) < 350.0f) {
        pos   = target->GetPosition();
        pos.z = std::max(pos.z + 200.0f, 560.0f);
    }
    pos.z = std::max(pos.z, CWorld::FindGroundZForCoord(pos.x, pos.y) + 20.0f);

    auto& mat = heli->GetMatrix();
    mat.SetTranslate(pos);

    heli->SetStatus(STATUS_PHYSICS);
    heli->vehicleFlags.bIsLocked = true;

    // 0x6C6758
    auto& ap = heli->m_autoPilot;
    if (newsHeli) {
        ap.m_nCarMission   = MISSION_HELI_NEWS_BEHAVIOUR;
        ap.m_nCruiseSpeed  = 35;
        heli->m_fMaxAltitude = 30.0f;
        heli->m_fMinAltitude = 27.0f;
    } else {
        ap.m_nCarMission   = MISSION_HELI_POLICE_BEHAVIOUR;
        ap.m_nCruiseSpeed  = 70;
        heli->m_fMaxAltitude = 20.0f;
        heli->m_fMinAltitude = 12.0f;
    }
    ap.m_TargetEntity = reinterpret_cast<CVehicle*>(target); // NOTE: No reference is registered by the original code

    heli->m_fHeliRotorSpeed = 0.165f;

    // 0x6C67D5 - Face towards the target
    const auto heading = angle + 3.14159274f;
    const auto s = std::sin(heading), c = std::cos(heading);
    mat.GetRight()   = CVector{ s, -c, 0.0f };
    mat.GetForward() = CVector{ c, s, 0.0f };
    mat.GetUp()      = CVector{ 0.0f, 0.0f, 1.0f };

    CWorld::Add(heli);
    heli->SetUpDriver(-1, false, false);

    return heli;
}

// 0x6C6890
void CHeli::TestSniperCollision(CVector* origin, CVector* target) {
    CVector point = *target - *origin;

    if (point.z >= point.Magnitude() / 2.0f)
        return;

    for (auto& heli : pHelis) {
        if (!heli || heli->physicalFlags.bBulletProof)
            continue;

        const auto mat = (CMatrix*)heli->m_matrix;
        if (CCollision::DistToLine(*origin, *target, mat->TransformPoint({ -0.43f, 1.49f, 1.5f })) < 0.8f) {
            heli->m_fRotationBalance = (float)(CGeneral::GetRandomNumber() < pow(2, 14) - 1) * 0.1f - 0.05f; // 2^14 - 1 = 16383 [-0.05, 0.05]
            heli->BlowUpCar(FindPlayerPed(), false);
            heli->m_nNumSwatOccupants = 0;
        };
    }
}

// 0x6C69C0
bool CHeli::SendDownSwat() {
    const auto targetPos = m_autoPilot.m_TargetEntity->GetPosition();

    if (!m_nNumSwatOccupants) {
        return false;
    }
    if (physicalFlags.bSubmergedInWater || !CStreaming::IsModelLoaded(MODEL_SWAT)) {
        return false;
    }
    if (CGeneral::GetRandomNumber() & 0x7F) {
        return false;
    }
    if ((GetPosition() - targetPos).Magnitude() > 50.0f) {
        return false;
    }
    if (m_vecMoveSpeed.Magnitude() > 0.1f) {
        return false;
    }

    // 0x6C6ABB
    const CMatrix mat{ GetMatrix() };
    const auto swatPos = mat.TransformVector(FindSwatPositionRelativeToHeli(m_nNumSwatOccupants - 1)) + GetPosition();

    // Only if the target is on the ground below us
    const auto groundZ = CWorld::FindGroundZFor3DCoord(swatPos, nullptr, nullptr);
    if (std::abs(targetPos.z - groundZ) >= 2.5f) {
        return false;
    }

    // And not if there's water below us
    if (float waterZ; CWaterLevel::GetWaterLevelNoWaves(swatPos, &waterZ, nullptr, nullptr) && waterZ >= groundZ) {
        return false;
    }

    // 0x6C6B96
    const auto ropeId = GetRopeId();
    if (!CRopes::RegisterRope(ropeId, static_cast<uint32>(eRopeType::SWAT), swatPos, false, 0, 0, nullptr, 20'000)) { // NOTE: Original checks for `< 0` (int return value)
        return false;
    }

    auto* const ped = CPopulation::AddPed(PED_TYPE_COP, static_cast<eModelID>(COP_TYPE_SWAT2), swatPos, true);

    auto* const seq = new CTaskComplexSequence{};
    seq->AddTask(new CTaskComplexUseSwatRope{ ropeId, this });
    seq->AddTask(new CTaskComplexWanderCop{ PEDMOVE_WALK, static_cast<uint8>(CGeneral::GetRandomNumberInRange(0, 8)) });
    ped->GetTaskManager().SetTask(seq, TASK_PRIMARY_PRIMARY);

    ped->m_bUsesCollision = false;

    m_nNumSwatOccupants--;
    m_aSwatState[m_nNumSwatOccupants] = 170;

    CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, ANIM_ID_ABSEIL, 4.0f);

    return true;
}

// 0x6C79A0
void CHeli::UpdateHelis() {
    ZoneScoped;

    NumberOfSearchLights = 0;

    auto numHelisRequired = FindPlayerWanted()->NumOfHelisRequired();

    // Count the helis we currently have, and check if there's a healthy police heli among them
    int32 numHelis         = 0;
    bool  hasWorkingPolmav = false;
    for (const auto heli : pHelis) {
        if (!heli) {
            continue;
        }
        numHelis++;
        if (heli->m_nModelIndex == MODEL_POLMAV && !heli->physicalFlags.bRenderScorched && !heli->vehicleFlags.bIsDrowning) {
            hasWorkingPolmav = true;
        }
    }

    // 0x6C7A18 - No helis in interiors, when disabled, or in a sandstorm
    if (CCullZones::PlayerNoRain() || CGame::currArea != AREA_CODE_NORMAL_WORLD) {
        numHelisRequired = 0;
    }
    if (!bPoliceHelisAllowed) {
        numHelisRequired = 0;
    }
    if (CWeather::OldWeatherType == WEATHER_SANDSTORM_DESERT || CWeather::NewWeatherType == WEATHER_SANDSTORM_DESERT) {
        numHelisRequired = 0;
    }

    // 0x6C7A50 - A news heli is only generated if there's a police heli already, and there's no news heli yet
    bool generateNewsHeli = hasWorkingPolmav;
    for (const auto heli : pHelis) {
        if (heli && heli->m_nModelIndex == MODEL_VCNMAV) {
            generateNewsHeli = false;
        }
    }
    if (!CWanted::UseNewsHeliInAdditionToPolice) {
        generateNewsHeli = false;
    }

    // 0x6C7A96 - Generate a new heli if necessary
    if (CStreaming::IsModelLoaded(generateNewsHeli ? MODEL_VCNMAV : MODEL_POLMAV) && CTimer::GetTimeInMS() > TestForNewRandomHelisTimer) {
        TestForNewRandomHelisTimer = CTimer::GetTimeInMS() + 15'000;
        if (numHelis < numHelisRequired) {
            const auto newHeli = GenerateHeli(FindPlayerPed(), generateNewsHeli);
            if (!pHelis[0]) {
                pHelis[0] = newHeli;
                CEntity::RegisterReference(pHelis[0]);
            } else if (!pHelis[1]) {
                pHelis[1] = newHeli;
                CEntity::RegisterReference(pHelis[1]);
            }
        }
    }

    // 0x6C7B12 - Forget about wrecked helis, and remove the ones that have flown far enough away
    for (auto& heli : pHelis) {
        if (!heli) {
            continue;
        }
        if (heli->physicalFlags.bRenderScorched || heli->vehicleFlags.bIsDrowning) {
            heli->m_autoPilot.SetCarMission(MISSION_HELI_FLY_AWAY_FROM_PLAYER);
            heli = nullptr;
        } else if (heli->m_autoPilot.m_nCarMission == MISSION_HELI_FLY_AWAY_FROM_PLAYER) {
            if ((FindPlayerCoors() - heli->GetPosition()).Magnitude() > 170.0f) {
                CWorld::Remove(heli);
                delete heli;
                heli = nullptr;
            }
        }
    }

    // 0x6C7BCB - Send away the helis that aren't required anymore
    for (const auto heli : pHelis) {
        if (!heli || heli->m_autoPilot.m_nCarMission == MISSION_HELI_FLY_AWAY_FROM_PLAYER) {
            continue;
        }
        if (numHelisRequired > 0) {
            numHelisRequired--;
        } else {
            heli->m_autoPilot.SetCarMission(MISSION_HELI_FLY_AWAY_FROM_PLAYER);
            heli->m_fMinAltitude = 100.0f;
            heli->m_fMaxAltitude = 100.0f;
        }
    }
}

// 0x6C7C50
void CHeli::RenderAllHeliSearchLights() {
    ZoneScoped;

    for (auto& light : HeliSearchLights) {
        SearchLightCone(
            light.m_nCoronaIndex,
            light.m_vecOrigin,
            light.m_vecTarget,
            light.m_fTargetRadius,
            light.m_fPower,
            light.field_24,
            light.m_bDrawShadow,
            light.m_vecUseless[0],
            light.m_vecUseless[1],
            light.m_vecUseless[2],
            false,
            0.05f,
            0.0f,
            0.0f,
            1.0f
        );
    }
}

// 0x6C6D30
void CHeli::BlowUpCar(CEntity* damager, bool bHideExplosion) {
    if (!vehicleFlags.bCanBeDamaged) {
        return;
    }

    const auto isRC = notsa::contains({ MODEL_RCRAIDER, MODEL_RCGOBLIN }, GetModelId());

    // Helis not controlled by the player don't blow up right away, they crash and burn first
    if (GetStatus() != STATUS_PLAYER && m_autoPilot.m_nCarMission != MISSION_HELI_CRASH_AND_BURN && !isRC) {
        m_autoPilot.SetCarMission(MISSION_HELI_CRASH_AND_BURN);
        m_fHealth = 0.0f;
        return;
    }

    if (damager == FindPlayerPed() || damager == FindPlayerVehicle()) {
        auto& playerInfo = FindPlayerInfo();
        playerInfo.m_nHavocCaused += 20;
        playerInfo.m_fCurrentChaseValue += 10.0f;
        CStats::IncrementStat(STAT_COST_OF_PROPERTY_DAMAGED, (float)(CGeneral::GetRandomNumber() % 6000 + 4000));
    }

    if (m_nModelIndex == MODEL_VCNMAV) {
        CWanted::UseNewsHeliInAdditionToPolice = false;
    }

    if (GetStatus() == STATUS_PLAYER) { // 0x6C6DFA
        m_bUsesCollision = false;
        m_bIsVisible     = false;
        m_vecMoveSpeed.Set(0.0f, 0.0f, 0.0f);
        m_vecTurnSpeed.Set(0.0f, 0.0f, 0.0f);
    }

    SetStatus(STATUS_WRECKED);
    physicalFlags.bRenderScorched = true;
    m_nTimeWhenBlowedUp           = CTimer::GetTimeInMS();

    CVisibilityPlugins::SetClumpForAllAtomicsFlag(GetRpClump(), eAtomicComponentFlag::ATOMIC_PIPE_NO_EXTRA_PASSES);
    m_damageManager.FuckCarCompletely(false);

    if (!isRC) { // 0x6C6E69
        for (auto bumper : { FRONT_BUMPER, REAR_BUMPER }) {
            SetBumperDamage(bumper, false);
        }
        for (auto door : { DOOR_BONNET, DOOR_BOOT, DOOR_LEFT_FRONT, DOOR_RIGHT_FRONT, DOOR_LEFT_REAR, DOOR_RIGHT_REAR }) {
            SetDoorDamage(door, false);
        }
        SpawnFlyingComponent(CAR_WHEEL_LF, 1);

        RpAtomic* atomic = nullptr;
        RwFrameForAllObjects(m_aCarNodes[HELI_WHEEL_LF], GetCurrentAtomicObjectCB, &atomic);
        if (atomic) {
            RpAtomicSetFlags(atomic, 0);
        }
    }

    m_nBombOnBoard = 0;
    m_fHealth      = 0.0f;
    m_wBombTimer   = 0;

    TheCamera.CamShake(0.4f, GetPosition());
    KillPedsInVehicle();

    vehicleFlags.bLightsOn     = false;
    vehicleFlags.bEngineOn     = false;
    vehicleFlags.bSirenOrAlarm = false;
    m_nOverrideLights          = NO_CAR_LIGHT_OVERRIDE;
    autoFlags.bTaxiLight       = false;

    if (vehicleFlags.bIsAmbulanceOnDuty) {
        vehicleFlags.bIsAmbulanceOnDuty = false;
        CCarCtrl::NumAmbulancesOnDuty--;
    }

    if (vehicleFlags.bIsFireTruckOnDuty) {
        vehicleFlags.bIsFireTruckOnDuty = false;
        CCarCtrl::NumFireTrucksOnDuty--;
    }

    ChangeLawEnforcerState(false);

    gFireManager.StartFire(this, damager, 0.8f, true, 7'000, 0);
    CDarkel::RegisterCarBlownUpByPlayer(*this, 0);

    // NOTE: `bHideExplosion` is not used by the original code
    CExplosion::AddExplosion(
        this,
        damager,
        isRC ? EXPLOSION_RC_VEHICLE : EXPLOSION_AIRCRAFT,
        GetPosition(),
        0,
        true,
        -1.0f,
        false
    );
}

// 0x6C4530
void CHeli::Fix() {
    m_damageManager.ResetDamageStatus();
    SetupDamageAfterLoad();
}

// 0x6C4330
bool CHeli::BurstTyre(uint8 tyreComponentId, bool bPhysicalEffect) {
    return false;
}

// 0x6C4320
bool CHeli::SetUpWheelColModel(CColModel* wheelCol) {
    return false;
}

// 0x6C4830
void CHeli::ProcessControlInputs(uint8 playerNum) {
    auto* const pad = CPad::GetPad(playerNum);

    m_fAccelerationBreakStatus = static_cast<float>(pad->GetAccelerate() - pad->GetBrake()) * (1.0f / 255.0f);

    const auto SetSteeringFromPad = [&] {
        m_nLastControlInput  = eControllerType::KEYBOARD;
        m_fSteeringUpDown    = static_cast<float>(pad->GetSteeringUpDown()) * (1.0f / 128.0f);
        m_fSteeringLeftRight = static_cast<float>(-pad->GetSteeringLeftRight()) * (1.0f / 128.0f);
    };

    if (!CCamera::m_bUseMouse3rdPerson || !m_bEnableMouseFlying) {
        SetSteeringFromPad();
    } else {
        const auto& mouseMoved = CPad::NewMouseControllerState.m_AmountMoved;

        // 0x6C4893
        const auto useMouse = [&] {
            if (mouseMoved.x != 0.0f || mouseMoved.y != 0.0f) {
                return true;
            }
            if (std::fabs(m_fSteeringLeftRight) <= 0.0f && std::fabs(m_fSteeringUpDown) <= 0.0f) {
                return false;
            }
            if (m_nLastControlInput != eControllerType::MOUSE) {
                return false;
            }
            return !CPad::GetPad(playerNum)->GetSteeringLeftRight() && !CPad::GetPad(playerNum)->GetSteeringUpDown();
        }();

        if (useMouse) { // 0x6C4993
            m_nLastControlInput = eControllerType::MOUSE;
            if (!CPad::GetPad(playerNum)->NewState.m_bVehicleMouseLook) {
                m_fSteeringLeftRight -= mouseMoved.x * 0.0025f;
                m_fSteeringUpDown    += mouseMoved.y * 0.0025f;
            }
            // Slowly center the controls
            if (std::fabs(m_fSteeringLeftRight) < 0.5f) {
                m_fSteeringLeftRight *= std::pow(0.98f, CTimer::GetTimeStep());
            }
            if (std::fabs(m_fSteeringUpDown) < 0.5f) {
                m_fSteeringUpDown *= std::pow(0.98f, CTimer::GetTimeStep());
            }
        } else if (pad->GetSteeringLeftRight() || pad->GetSteeringUpDown() || m_nLastControlInput != eControllerType::MOUSE) { // 0x6C492E
            SetSteeringFromPad();
        }
    }

    // 0x6C4A96
    m_fSteeringUpDown    = std::clamp(m_fSteeringUpDown, -1.0f, 1.0f);
    m_fSteeringLeftRight = std::clamp(m_fSteeringLeftRight, -1.0f, 1.0f);

    m_fLeftRightSkid = pad->GetLookRight() ? 1.0f : 0.0f;
    if (pad->GetLookLeft()) {
        m_fLeftRightSkid = -1.0f;
    }

    // 0x6C4B4F - Auto-stabilize (Bring the heli to a halt)
    if (pad->GetHorn() && GetMatrix().GetUp().z > 0.0f) {
        m_fLeftRightSkid = 0.0f;

        auto right = CrossProduct(CVector{ 0.0f, 0.0f, 1.0f }, GetMatrix().GetRight());
        right.Normalise();
        m_fSteeringUpDown = std::clamp(DotProduct(right, m_vecMoveSpeed) * m_pFlyingHandlingData->m_fPitchStab, -2.0f, 2.0f);

        auto fwd = CrossProduct(GetMatrix().GetForward(), CVector{ 0.0f, 0.0f, 1.0f });
        fwd.Normalise();
        m_fSteeringLeftRight = std::clamp(DotProduct(fwd, m_vecMoveSpeed) * m_pFlyingHandlingData->m_fRollStab, -2.0f, 2.0f);
    }

    // 0x6C4D75
    vehicleFlags.bIsHandbrakeOn = false;
    m_fSteerAngle               = 0.0f;
    m_BrakePedal                = 1.0f;
    m_GasPedal                  = 0.0f;

    if (pad->DisablePlayerControls) {
        FindPlayerPed()->KeepAreaAroundPlayerClear();

        // Limit speed
        if (const auto speed = m_vecMoveSpeed.Magnitude(); speed > 0.28f) {
            m_vecMoveSpeed *= 0.28f / speed;
        }
    }

    // 0x6C4E1A - Heavily damaged helis are hard to control
    if (m_fHealth < 250.0f) {
        m_fAccelerationBreakStatus = -0.1f;
        m_fLeftRightSkid          += 0.5f;
    }
}

// 0x6C4400
void CHeli::Render() {
    auto* mi = GetVehicleModelInfo();
    m_nTimeTillWeNeedThisCar = CTimer::GetTimeInMS() + 3000;
    mi->SetVehicleColour(m_nPrimaryColor, m_nSecondaryColor, m_nTertiaryColor, m_nQuaternaryColor);

    auto staticRotor = m_aCarNodes[HELI_STATIC_ROTOR];
    RpAtomic* data = nullptr;
    if (staticRotor) {
        RwFrameForAllObjects(staticRotor, GetCurrentAtomicObjectCB, &data);
        if (data)
            CVehicle::SetComponentAtomicAlpha(data, 255);
    }

    auto staticRotor2 = m_aCarNodes[HELI_STATIC_ROTOR2];
    data = nullptr;
    if (staticRotor2) {
        RwFrameForAllObjects(staticRotor2, GetCurrentAtomicObjectCB, &data);
        if (data)
            CVehicle::SetComponentAtomicAlpha(data, 255);
    }

    auto movingRotor = m_aCarNodes[HELI_MOVING_ROTOR];
    data = nullptr;
    if (movingRotor) {
        RwFrameForAllObjects(movingRotor, GetCurrentAtomicObjectCB, &data);
        if (data)
            CVehicle::SetComponentAtomicAlpha(data, 0);
    }

    auto movingRotor2 = m_aCarNodes[HELI_MOVING_ROTOR2];
    data = nullptr;
    if (movingRotor2) {
        RwFrameForAllObjects(movingRotor2, GetCurrentAtomicObjectCB, &data);
        if (data)
            CVehicle::SetComponentAtomicAlpha(data, 0);
    }

    CEntity::Render(); // exactly CEntity
}

// 0x6C4550
void CHeli::SetupDamageAfterLoad() {
    vehicleFlags.bIsDamaged = false;
}

// 0x6C4E60
void CHeli::ProcessFlyingCarStuff() {
    const auto isRC   = notsa::contains({ MODEL_RCRAIDER, MODEL_RCGOBLIN }, GetModelId());
    const auto status = GetStatus();

    if (status == STATUS_PLAYER || status == STATUS_PHYSICS || status == STATUS_REMOTE_CONTROLLED) {
        // Spin up the rotor
        if (m_fHeliRotorSpeed < 0.22f && !physicalFlags.bSubmergedInWater) {
            m_fHeliRotorSpeed += isRC ? 0.003f : 0.001f;
        }

        // 0x6C4F43 - Flying
        if (m_fHeliRotorSpeed > 0.15f) {
            if (vehicleFlags.bIsRCVehicle) {
                FlyingControl(FLIGHT_MODEL_RCHELI, m_fLeftRightSkid, m_fSteeringUpDown, m_fSteeringLeftRight, m_fAccelerationBreakStatus);
            } else if ((m_nNumContactWheels < 4 && !(physicalFlags.bTouchingWater && IsAmphibiousHeli()))
                || m_fAccelerationBreakStatus > 0.0f
                || std::abs(m_vecMoveSpeed.x) > 0.02f
                || std::abs(m_vecMoveSpeed.y) > 0.02f
                || std::abs(m_vecMoveSpeed.z) > 0.02f
            ) {
                FlyingControl(FLIGHT_MODEL_HELI, m_fLeftRightSkid, m_fSteeringUpDown, m_fSteeringLeftRight, m_fAccelerationBreakStatus);
            }
        }

        // 0x6C501D - Rotor blade collision + wind
        if (m_fHeliRotorSpeed > 0.015f) {
            if (const auto rotorFrame = m_aCarNodes[HELI_STATIC_ROTOR]) {
                CMatrix rotorMat;
                rotorMat.Attach(RwFrameGetMatrix(rotorFrame), false);

                RpAtomic* atomic = nullptr;
                RwFrameForAllObjects(rotorFrame, GetCurrentAtomicObjectCB, &atomic);
                if (atomic) {
                    const auto radius = RpAtomicGetBoundingSphere(atomic)->radius;
                    if (radius > 0.1f) {
                        float damageMult = 1.0f;
                        switch (m_nModelIndex) {
                        case MODEL_RCRAIDER:
                        case MODEL_RCGOBLIN:
                            damageMult = 0.9f;
                            break;
                        case MODEL_SPARROW:
                        case MODEL_SEASPAR:
                            damageMult = 0.8f;
                            break;
                        case MODEL_HUNTER:
                            damageMult = 0.5f;
                            break;
                        }
                        if (status == STATUS_PLAYER || status == STATUS_REMOTE_CONTROLLED) {
                            DoBladeCollision(rotorMat.GetPosition(), GetMatrix(), -3, radius, damageMult); // -3 = top rotor
                        }
                    }
                }

                if (status == STATUS_PLAYER || status == STATUS_PHYSICS) {
                    if (m_fHeliRotorSpeed > 0.0075f) {
                        CWindModifiers::RegisterOne(GetPosition(), 1, std::min(m_fHeliRotorSpeed * 6.6666665f, 1.0f));
                    }
                } else if (status == STATUS_SIMPLE) { // Unreachable, but it is in the original code
                    CWindModifiers::RegisterOne(GetPosition(), 1, 1.0f);
                }
            }
        }
    } else {
        if (!IsRealHeli()) {
            return;
        }

        // Spin down the rotor
        vehicleFlags.bEngineOn = false;
        const auto slowDown = CTimer::GetTimeStep() * 0.00055f;
        if (slowDown < m_fHeliRotorSpeed) {
            m_nFakePhysics = 0;
            m_fHeliRotorSpeed -= slowDown;
        } else {
            m_fHeliRotorSpeed = 0.0f;
        }
    }

    // 0x6C5200 - Blade "swoosh" sound when the rotor is spinning slowly
    if (isRC || m_fHeliRotorSpeed >= 0.154f || m_fHeliRotorSpeed <= 0.0044f) {
        return;
    }

    const auto rotorFrame = m_aCarNodes[HELI_STATIC_ROTOR];
    if (!rotorFrame) {
        return;
    }

    const CVector heliToCam = TheCamera.GetPosition() - GetPosition();
    const auto    distSq    = heliToCam.SquaredMagnitude();
    if (distSq >= sq(20.0f)) {
        return;
    }

    // NOTE: `m_fPropRotate` is (re)used here to store the rotor rotation at which the sound was played the last time
    if (std::abs(m_fPropRotate - m_wheelRotation[1]) <= PI / 6.0f) {
        return;
    }

    CMatrix rotorMat;
    rotorMat.Attach(RwFrameGetMatrix(rotorFrame), false);

    const CVector bladeDir     = GetMatrix().TransformVector(rotorMat.GetRight());
    const CVector heliToCamDir = heliToCam * (1.0f / std::max(std::sqrt(distSq), 0.01f));
    if (std::abs(heliToCamDir.Dot(bladeDir)) > 0.95f) {
        m_vehicleAudio.AddAudioEvent(AE_HELI_BLADE, 0.0f);
        m_fPropRotate = m_wheelRotation[1];
    }
}

// 0x6C5420
void CHeli::PreRender() {
    CVehicle::PreRender();

    const auto mi = GetVehicleModelInfo();

    if (m_bSearchLightEnabled && m_fSearchLightIntensity > 0.0f && CClock::GetIsTimeInRange(19, 6)) {
        AddHeliSearchLight(
            GetMatrix().TransformPoint(CVector{ 0.0f, 3.5f, -0.3f }),
            m_vecSearchLightTarget,
            20.0f,
            m_fSearchLightIntensity,
            reinterpret_cast<uint32>(this) + 11,
            true,
            true
        );
    }

    // 0x6C5506 - Update wheel positions (suspension)
    if (vehicleFlags.bVehicleColProcessed) {
        DoBurstAndSoftGroundRatios();

        for (auto i = 0; i < 4; i++) {
            const auto springLength  = m_aSuspensionSpringLength[i];
            const auto springTension = 1.0f - springLength / m_aSuspensionLineLength[i];
            const auto compression   = (m_fWheelsSuspensionCompression[i] - springTension) / (1.0f - springTension);

            CVector wheelPos;
            mi->GetWheelPosn(i, wheelPos, true);

            auto wheelZ = wheelPos.z + m_pHandlingData->m_fSuspensionUpperLimit;
            if (compression > 0.0f) {
                wheelZ -= compression * springLength;
            }

            // Wheels going down are smoothed out
            if (wheelZ <= m_wheelPosition[i] && !(physicalFlags.bAddMovingCollisionSpeed && handlingFlags.bHydraulicInst)) {
                wheelZ = (wheelZ - m_wheelPosition[i]) * 0.75f + m_wheelPosition[i];
            }
            m_wheelPosition[i] = wheelZ;
        }
    }

    UpdateWheelMatrix(HELI_WHEEL_RB, 1);
    UpdateWheelMatrix(HELI_WHEEL_LB, 1);
    UpdateWheelMatrix(HELI_WHEEL_RF, 1);
    UpdateWheelMatrix(HELI_WHEEL_LF, 1);

    if (m_nModelIndex != MODEL_RCRAIDER && m_nModelIndex != MODEL_RCGOBLIN) {
        DoHeliDustEffect(1.0f, 1.0f);
    }

    // 0x6C5605 - Update rotor angles
    const auto rotorStep = CTimer::GetTimeStep() * m_fHeliRotorSpeed;

    m_fRotorZ -= notsa::contains({ MODEL_SPARROW, MODEL_SEASPAR, MODEL_MAVERICK, MODEL_VCNMAV, MODEL_POLMAV }, GetModelId())
        ? rotorStep * HELI_MAIN_ROTOR_SPIN_MULT
        : rotorStep;
    while (m_fRotorZ < -TWO_PI) {
        m_fRotorZ += TWO_PI;
    }

    m_fSecondRotorZ -= rotorStep * (m_nModelIndex == MODEL_LEVIATHN ? 2.0f : 2.3f);
    while (m_fSecondRotorZ > TWO_PI) {
        m_fSecondRotorZ -= TWO_PI;
    }

    // 0x6C56E9 - Apply rotor angles to the frames
    const auto SetRotorRotation = [this](eHeliNodes node, bool isMainRotor) {
        const auto frame = m_aCarNodes[node];
        if (!frame) {
            return;
        }

        CMatrix mat;
        mat.Attach(RwFrameGetMatrix(frame), false);
        const CVector pos = mat.GetPosition();
        if (isMainRotor) {
            mat.SetRotateZ(m_fRotorZ);
        } else {
            mat.SetRotateX(m_fSecondRotorZ);
        }
        mat.GetPosition() += pos;
        mat.UpdateRW();
    };
    SetRotorRotation(HELI_STATIC_ROTOR, true);
    SetRotorRotation(HELI_MOVING_ROTOR, true);
    SetRotorRotation(HELI_STATIC_ROTOR2, false);
    SetRotorRotation(HELI_MOVING_ROTOR2, false);

    CShadows::StoreShadowForVehicle(this, VEH_SHD_HELI);
}

// 0x6C7050
void CHeli::ProcessControl() {
    static auto& s_HeliGunBurstChance = StaticRef<float>(0x8D33A4);

    CAutomobile::ProcessControl();

    const auto now = CTimer::GetTimeInMS();

    // 0x6C705F
    if (!vehicleFlags.bEngineOn && m_pDustParticle) {
        m_pDustParticle->Kill();
        m_pDustParticle       = nullptr;
        m_heliDustFxTimeConst = 0.0f;
    }

    // 0x6C7088 - Toggle search light
    if (CPad::GetPad(m_pDriver && m_pDriver->m_nPedType == PED_TYPE_PLAYER2 ? 1 : 0)->HornJustDown()) {
        m_bSearchLightEnabled = !m_bSearchLightEnabled;
    }

    bool isChasingPlayer = false;

    if (physicalFlags.bRenderScorched || CCullZones::PlayerNoRain()) {
        m_fSearchLightIntensity = 0.0f;
    } else {
        bool     isLightOn   = false;
        CEntity* lightTarget = nullptr;

        // 0x6C70F1 - Figure out the target of the search light
        const auto IsPlayerFlying = [] {
            const auto* const plyrVeh = FindPlayerVehicle();
            return plyrVeh && (plyrVeh->m_nVehicleSubType == VEHICLE_TYPE_HELI || plyrVeh->m_nVehicleSubType == VEHICLE_TYPE_PLANE);
        };
        if (m_autoPilot.m_nCarMission == MISSION_HELI_POLICE_BEHAVIOUR && !IsPlayerFlying()) {
            isLightOn       = true;
            isChasingPlayer = true;
            lightTarget     = FindPlayerEntity();
        } else if (m_autoPilot.m_nCarMission == MISSION_HELI_FOLLOW_ENTITY && m_autoPilot.m_TargetEntity && (m_nHeliFlags & 2)) {
            isLightOn   = true;
            lightTarget = m_autoPilot.m_TargetEntity;
        } else if (GetStatus() == STATUS_PLAYER && m_nModelIndex == MODEL_POLMAV && m_bSearchLightEnabled) {
            isLightOn = true;
        }

        if (physicalFlags.bSubmergedInWater) {
            isLightOn       = false;
            isChasingPlayer = false;
        }

        m_bSearchLightEnabled = isLightOn;

        if (isLightOn) {
            // 0x6C71B9
            CVector targetPos, targetSpeed;
            if (lightTarget) {
                targetPos   = lightTarget->GetPosition();
                targetSpeed = static_cast<CPhysical*>(lightTarget)->m_vecMoveSpeed;
            } else { // Just point it forwards
                targetPos   = GetForwardVector() * 10.0f + GetPosition() + GetUpVector() * -30.0f;
                targetSpeed = m_vecMoveSpeed;
            }

            // 0x6C72A5 - Record target position every second
            auto& historyX = GetSearchLightHistoryX();
            auto& historyY = GetSearchLightHistoryY();

            auto timeSinceUpdate = static_cast<int32>(now - m_nSearchLightTimer);
            while (timeSinceUpdate > 1000) {
                for (auto i = 5; i > 0; i--) {
                    historyX[i] = historyX[i - 1];
                    historyY[i] = historyY[i - 1];
                }
                historyX[0] = targetSpeed.x * 100.0f + targetPos.x;
                historyY[0] = targetPos.y + targetSpeed.y * 100.0f;

                m_nSearchLightTimer += 1000;
                timeSinceUpdate     -= 1000;
            }

            // 0x6C7340 - The light follows the target with a delay
            const auto t = static_cast<float>(timeSinceUpdate) * 0.001f;
            m_vecSearchLightTarget.z = targetPos.z;
            m_vecSearchLightTarget.x = (1.0f - t) * historyX[2] + t * historyX[1];
            m_vecSearchLightTarget.y = (1.0f - t) * historyY[2] + t * historyY[1];

            const auto lightDist2D = (CVector2D{ m_vecSearchLightTarget } - CVector2D{ GetPosition() }).Magnitude();
            if (lightDist2D > 60.0f) {
                m_fSearchLightIntensity = 0.0f;
            } else if (lightDist2D < 40.0f) {
                m_fSearchLightIntensity = 1.0f;
            } else {
                m_fSearchLightIntensity = 1.0f - (lightDist2D - 40.0f) * 0.05f;
            }

            // 0x6C7437 - Is the target lit?
            const auto lightToTarget = CVector2D{ targetPos } - CVector2D{ m_vecSearchLightTarget };
            if (m_fSearchLightIntensity < 0.9f || lightToTarget.SquaredMagnitude() > 7.0f * 7.0f) {
                m_nShootTimer    = now;
                m_nGunFiringTime = now;
            } else if (now > m_nPoliceShoutTimer) {
                m_nPoliceShoutTimer = (CGeneral::GetRandomNumber() & 0xFFF) + now + 4500;
            }

            if (isChasingPlayer) { // 0x6C74B7
                // How long the target has to be lit before we start shooting
                int32 shootDelay = 999'999; // NOTE: Original leaves this uninitialized for wanted levels above 6 (which is not possible)
                switch (static_cast<int32>(FindPlayerWanted()->GetWantedLevel())) {
                case 0:
                case 1:
                case 2: shootDelay = 999'999; break;
                case 3: shootDelay = 10'000;  break;
                case 4: shootDelay = 5'000;   break;
                case 5: shootDelay = 3'500;   break;
                case 6: shootDelay = 2'000;   break;
                }

                if (FindPlayerWanted()->GetWantedLevel() != eWantedLevel::WANTED_CLEAN) {
                    AudioEngine.SayPedless(AE_SPEECH_PED, CTX_GLOBAL_POLICE_HELICOPTER, this, 0, 1.0f, false, false, false);
                }

                if (CCullZones::NoPolice()) {
                    shootDelay /= 2;
                }
                if (lightTarget != FindPlayerPed()) { // Player is in a vehicle
                    shootDelay = 5'000;
                }

                if (FindPlayerWanted()->PoliceBackOff()) { // 0x6C7576
                    m_nShootTimer    = now;
                    m_nGunFiringTime = now;
                } else {
                    const auto gunPos    = GetMatrix().TransformPoint(CVector{ 0.0f, 3.5f, -1.0f });
                    const auto shootTime = m_nShootTimer + static_cast<uint32>(shootDelay);

                    // 0x6C75C4 - Just about to start shooting, but the target isn't visible
                    if (now > shootTime && CTimer::GetPreviousTimeInMS() <= shootTime && !CWorld::GetIsLineOfSightClear(gunPos, targetPos, true, false, false, false, false, false, false)) {
                        m_nShootTimer    = now;
                        m_nGunFiringTime = now;
                    }

                    if (now > m_nShootTimer + static_cast<uint32>(shootDelay) && now > m_nGunFiringTime) { // 0x6C761E
                        CVector aimPos{ targetPos.x, targetPos.y, targetPos.z };
                        aimPos.x += static_cast<float>(static_cast<int32>(CGeneral::GetRandomNumber() & 0xFF) - 128) * 0.02f;
                        aimPos.y += static_cast<float>(static_cast<int32>(CGeneral::GetRandomNumber() & 0xFF) - 128) * 0.02f;

                        auto dir = targetPos - gunPos;
                        dir.Normalise();

                        FireOneInstantHitRound(gunPos + dir * 3.0f, aimPos + dir * 3.0f, 20);
                        AudioEngine.ReportWeaponEvent(AE_WEAPON_FIRE, WEAPON_M4, this);

                        m_nGunFiringTime = CTimer::GetTimeInMS() + (CGeneral::GetRandomNumberInRange(0.0f, 1.0f) < s_HeliGunBurstChance ? 400 : 150);
                    }
                }
            }
        }
    }

    // 0x6C77EC
    if (m_autoPilot.m_nCarMission == MISSION_HELI_POLICE_BEHAVIOUR && m_nNumSwatOccupants > 0) {
        SendDownSwat();
        g_InterestingEvents.Add(CInterestingEvents::ZELDICK_OCCUPATION, this);
    }

    // 0x6C7818 - Update the ropes of the SWAT guys abseiling
    for (int32 i = 0; i < 4; i++) {
        auto& state = m_aSwatState[i];
        if (!state) {
            continue;
        }
        state--;

        const auto ropeId = reinterpret_cast<uint32>(this) + i;
        CRopes::RegisterRope(ropeId, static_cast<uint32>(eRopeType::SWAT), GetMatrix().TransformPoint(FindSwatPositionRelativeToHeli(i)), false, 0, 0, nullptr, 20'000);
        if (!state) {
            auto speed = GetMatrix().TransformVector(FindSwatPositionRelativeToHeli(i) * 0.05f);
            speed.z    = 0.0f;
            CRopes::SetSpeedOfTopNode(ropeId, speed);
        }
    }

    UpdateWinch();
    ProcessWeapons();

    // 0x6C791C
    if (g_InterestingEvents.m_b1) {
        auto chance = CTimer::GetTimeStep() * 0.02f * 0.1f;
        if (isChasingPlayer) {
            chance *= 2.0f;
        }
        if (static_cast<float>(CGeneral::GetRandomNumber()) * (1.0f / 32767.0f) < chance) {
            g_InterestingEvents.Add(CInterestingEvents::INTERESTING_EVENT_21, this);
        }
    }
}
