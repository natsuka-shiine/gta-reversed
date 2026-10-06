#include "StdInc.h"

#include "FxEmitter.h"
#include "EmissionInfo.h"
#include "FxEmitterPrt.h"
#include "FxPrimBP.h"
#include "FxEmitterBP.h"
#include "FxEmitterPrt.h"

void FxEmitter_c::InjectHooks() {
    RH_ScopedVirtualClass(FxEmitter_c, 0x85A7A4, 6);
    RH_ScopedCategory("Fx");

    // RH_ScopedInstall(Init_Reversed, 0x4A2550);
    // RH_ScopedInstall(Update_Reversed, 0x4A4460); // bad
    // RH_ScopedInstall(Reset_Reversed, 0x4A2570);
    RH_ScopedVMTOverloadedInstall(AddParticle, "pos", 0x4A3EA0, void(FxEmitter_c::*)(const CVector&, const CVector&, float, const FxPrtMult_c&, float, float, bool));
    RH_ScopedVMTOverloadedInstall(AddParticle, "matrix", 0x4A4050, void(FxEmitter_c::*)(const RwMatrix&, const CVector&, float, const FxPrtMult_c&, float, float, bool));
    RH_ScopedInstall(CreateParticles, 0x4A41E0);
    RH_ScopedInstall(CreateParticle, 0x4A2580);
}


// 0x4A2550
bool FxEmitter_c::Init(FxPrimBP_c* primBP, FxSystem_c* system) {
    m_PrimBP             = primBP;
    m_System             = system;
    m_fEmissionIntensity = 0.0f;
    return true;
}

// 0x4A4460
void FxEmitter_c::Update(float currentTime, float deltaTime) {
    if (m_bEnabled && !m_System->m_stopParticleCreation) {
        CreateParticles(currentTime, deltaTime);
    }
}

// 0x4A2570
void FxEmitter_c::Reset() {
    m_fEmissionIntensity = 0.0f;
}

// 0x4A3EA0
void FxEmitter_c::AddParticle(const CVector& pos, const CVector& vel, float timeSince, const FxPrtMult_c& fxMults, float rotZ, float brightness, bool createLocal) {
    EmissionInfo_t emission;
    m_PrimBP->m_FxInfoManager.ProcessEmissionInfo(0.0f, 0.0f, m_System->m_SystemBP->m_fLength, m_System->m_UseConstTime, &emission);

    auto* const wldMat    = g_fxMan.FxRwMatrixCreate();
    auto* const parentMat = g_fxMan.FxRwMatrixCreate();
    auto* const localMat  = g_fxMan.FxRwMatrixCreate();

    RwMatrixSetIdentity(localMat);
    RwV3dAssign(RwMatrixGetPos(localMat), &pos);
    RwMatrixUpdate(localMat);

    if (m_System->m_ParentMatrix) {
        RwMatrixMultiply(parentMat, localMat, m_System->m_ParentMatrix);
    } else {
        *parentMat = *localMat;
    }

    auto* const primMat = g_fxMan.FxRwMatrixCreate();
    m_PrimBP->GetRWMatrix(*primMat);
    RwMatrixMultiply(wldMat, primMat, parentMat);
    g_fxMan.FxRwMatrixDestroy(primMat);

    if (auto* const particle = CreateParticle(emission, *wldMat, &vel, timeSince, fxMults, brightness, createLocal)) {
        if (rotZ >= 0.0f) {
            particle->m_RotZ = (int8)(int32)(rotZ * 0.5f);
        }
    }

    g_fxMan.FxRwMatrixDestroy(localMat);
    g_fxMan.FxRwMatrixDestroy(parentMat);
    g_fxMan.FxRwMatrixDestroy(wldMat);
}

// 0x4A4050
void FxEmitter_c::AddParticle(const RwMatrix& mat, const CVector& vel, float timeSince, const FxPrtMult_c& fxMults, float rotZ, float brightness, bool createLocal) {
    EmissionInfo_t emission;
    m_PrimBP->m_FxInfoManager.ProcessEmissionInfo(0.0f, 0.0f, m_System->m_SystemBP->m_fLength, m_System->m_UseConstTime, &emission);

    auto* const wldMat    = g_fxMan.FxRwMatrixCreate();
    auto* const parentMat = g_fxMan.FxRwMatrixCreate();
    auto* const localMat  = g_fxMan.FxRwMatrixCreate();

    RwMatrixSetIdentity(localMat); // Pointless, it's overwritten right after
    *localMat = mat;
    RwMatrixUpdate(localMat);

    if (m_System->m_ParentMatrix) {
        RwMatrixMultiply(parentMat, localMat, m_System->m_ParentMatrix);
    } else {
        *parentMat = *localMat;
    }

    auto* const primMat = g_fxMan.FxRwMatrixCreate();
    m_PrimBP->GetRWMatrix(*primMat);
    RwMatrixMultiply(wldMat, primMat, parentMat);
    g_fxMan.FxRwMatrixDestroy(primMat);

    if (auto* const particle = CreateParticle(emission, *wldMat, &vel, timeSince, fxMults, brightness, createLocal)) {
        if (rotZ >= 0.0f) {
            particle->m_RotZ = (int8)(int32)(rotZ * 0.5f);
        }
    }

    g_fxMan.FxRwMatrixDestroy(localMat);
    g_fxMan.FxRwMatrixDestroy(parentMat);
    g_fxMan.FxRwMatrixDestroy(wldMat);
}

// 0x4A41E0
void FxEmitter_c::CreateParticles(float currentTime, float deltaTime) {
    EmissionInfo_t emission;
    m_PrimBP->m_FxInfoManager.ProcessEmissionInfo(currentTime, deltaTime, m_System->m_SystemBP->m_fLength, m_System->m_UseConstTime, &emission);

    const auto lodStart = (float)m_PrimBP->m_FxInfoManager.m_nLodStart / 64.0f;
    const auto lodEnd   = (float)m_PrimBP->m_FxInfoManager.m_nLodEnd / 64.0f;

    float visibility;
    if (m_System->m_fCameraDistance < lodStart) {
        visibility = 1.0f;
    } else if (m_System->m_fCameraDistance <= lodEnd) {
        visibility = 1.0f - (m_System->m_fCameraDistance - lodStart) / (lodEnd - lodStart);
    } else {
        visibility = 0.0f;
    }

    m_fEmissionIntensity += emission.m_fCount * visibility * (float)m_System->m_nRateMult * 0.001f;

    if (CWeather::Wind < emission.m_fMinWind || CWeather::Wind > emission.m_fMaxWind) {
        return;
    }
    if (CWeather::Rain < emission.m_fMinRain || CWeather::Rain > emission.m_fMaxRain) {
        return;
    }
    if (m_fEmissionIntensity < 1.0f) {
        return;
    }

    auto* const wldMat    = g_fxMan.FxRwMatrixCreate();
    auto* const parentMat = g_fxMan.FxRwMatrixCreate();

    RwMatrixUpdate(&m_System->m_LocalMatrix);
    if (m_System->m_ParentMatrix) {
        RwMatrixMultiply(parentMat, &m_System->m_LocalMatrix, m_System->m_ParentMatrix);
    } else {
        *parentMat = m_System->m_LocalMatrix;
    }

    auto* const primMat = g_fxMan.FxRwMatrixCreate();
    m_PrimBP->GetRWMatrix(*primMat);
    RwMatrixMultiply(wldMat, primMat, parentMat);
    g_fxMan.FxRwMatrixDestroy(primMat);

    for (int32 i = 0; i < (int32)m_fEmissionIntensity; i++) {
        FxPrtMult_c   prtMult;
        const auto    timeSince = (float)i / m_fEmissionIntensity * deltaTime;
        CreateParticle(emission, *wldMat, nullptr, timeSince, prtMult, 1.2f, m_System->m_createLocal);
    }
    m_fEmissionIntensity -= (float)(int32)m_fEmissionIntensity; // Keep the fractional part only

    g_fxMan.FxRwMatrixDestroy(parentMat);
    g_fxMan.FxRwMatrixDestroy(wldMat);
}

// 0x4A2580
FxEmitterPrt_c* FxEmitter_c::CreateParticle(const EmissionInfo_t& emissionInfo, RwMatrix& wldMat, const CVector* velOverride, float timeSince, const FxPrtMult_c& fxMults, float brightness, bool createLocal) {
    // Random numbers as the original game generates them
    const auto Rand01  = [] { return (float)(CGeneral::GetRandomNumber() % 10'000) * 0.0001f; };        // [0, 1)
    const auto RandPM1 = [] { return (float)(CGeneral::GetRandomNumber() % 10'000) * 0.0002f - 1.0f; }; // [-1, 1)
    // The original truncates to an integer first and then takes the low byte
    const auto ToU8 = [](float v) { return (uint8)(int32)v; };

    auto* particle = static_cast<FxEmitterPrt_c*>(g_fxMan.GetParticle(0));
    if (!particle) {
        if (!m_System->m_MustCreateParticles) {
            return nullptr;
        }
        g_fxMan.FreeUpParticle();
        particle = static_cast<FxEmitterPrt_c*>(g_fxMan.GetParticle(0));
        if (!particle) {
            return nullptr;
        }
    }

    particle->m_fTotalLife   = (RandPM1() * emissionInfo.m_fLifeBias + emissionInfo.m_fLife) * fxMults.m_fLife;
    particle->m_fCurrentLife = 0.0f;
    particle->m_System       = m_System;

    particle->m_MultColor.r = ToU8(fxMults.m_Color.red * 255.0f);
    particle->m_MultColor.g = ToU8(fxMults.m_Color.green * 255.0f);
    particle->m_MultColor.b = ToU8(fxMults.m_Color.blue * 255.0f);
    particle->m_MultColor.a = ToU8(fxMults.m_Color.alpha * 255.0f);
    particle->m_MultSize    = FixedFloat<uint8, 255.0f>{ ToU8(fxMults.m_fSize * 255.0f) };
    particle->m_MultRot     = FixedFloat<uint8, 255.0f>{ ToU8(fxMults.m_Rot * 255.0f) };

    particle->m_bLocalToSystem = createLocal;

    particle->m_RandR      = ToU8(Rand01() * 255.0f);
    particle->m_RandG      = ToU8(Rand01() * 255.0f);
    particle->m_RandB      = ToU8(Rand01() * 255.0f);
    particle->m_Brightness = FixedFloat<uint8, 100.0f>{ ToU8(brightness * 100.0f) };

    const float rotationRand    = Rand01();
    particle->m_RotZ            = -1;
    particle->m_CurrentRotation = rotationRand * (emissionInfo.m_fRotationMaxAngle - emissionInfo.m_fRotationMinAngle) + emissionInfo.m_fRotationMinAngle;

    if (particle->m_bLocalToSystem) {
        m_PrimBP->GetRWMatrix(wldMat);
    }

    CVector vec;
    if (std::fabs(emissionInfo.m_fRadius) < 0.001f) { // Box
        vec.x = Rand01() * (emissionInfo.m_SizeMax.x - emissionInfo.m_SizeMin.x) + emissionInfo.m_SizeMin.x;
        vec.y = Rand01() * (emissionInfo.m_SizeMax.y - emissionInfo.m_SizeMin.y) + emissionInfo.m_SizeMin.y;
        vec.z = Rand01() * (emissionInfo.m_SizeMax.z - emissionInfo.m_SizeMin.z) + emissionInfo.m_SizeMin.z;
    } else { // Sphere
        vec.x = RandPM1();
        vec.y = RandPM1();
        vec.z = RandPM1();

        float radius = 1.0f / std::sqrt(vec.z * vec.z + vec.y * vec.y + vec.x * vec.x);
        if (!(emissionInfo.m_fRadius < 0.0f)) { // Negative radius => Only on the surface of the sphere
            radius *= Rand01();
        }
        radius *= emissionInfo.m_fRadius;
        vec *= radius;
    }
    vec += emissionInfo.m_Pos;

    particle->m_Pos = vec.z * wldMat.at + vec.y * wldMat.up + vec.x * wldMat.right + wldMat.pos;

    if (velOverride) {
        particle->m_Velocity = *velOverride;
    } else {
        constexpr float DEG_TO_RAD = 0.017453279f; // 0x85A7BC

        const float randomAngle = Rand01() * TWO_PI;
        const float minAngle    = emissionInfo.m_fAngleMin * DEG_TO_RAD;
        const float maxAngle    = emissionInfo.m_fAngleMax * DEG_TO_RAD;

        const float randomAngleBetweenMinMax = Rand01() * (maxAngle - minAngle) + minAngle;

        CVector randomizedAngleVec{
            CMaths::GetCosFast(randomAngle) * CMaths::GetSinFast(randomAngleBetweenMinMax),
            CMaths::GetCosFast(randomAngleBetweenMinMax),
            CMaths::GetSinFast(randomAngle) * CMaths::GetSinFast(randomAngleBetweenMinMax)
        };

        CVector vectorsIn;
        CVector vectorsOut;
        if (emissionInfo.m_Direction.x <= 10.0f) {
            vectorsIn = emissionInfo.m_Direction;
            vectorsIn.Normalise();
        } else {
            vectorsIn = particle->m_Pos;
        }
        RwV3dTransformVectors(&vectorsOut, &vectorsIn, 1, &wldMat);
        CVector v38;
        RotateVecIntoVec(v38, randomizedAngleVec, vectorsOut);
        particle->m_Velocity = v38 * (RandPM1() * emissionInfo.m_fSpeedBias + emissionInfo.m_Speed);
    }

    particle->m_Velocity += m_System->m_VelAdd;
    static_cast<FxEmitterBP_c*>(m_PrimBP)->UpdateParticle(timeSince, particle);
    m_PrimBP->m_Particles.AddItem(particle);

    return particle;
}
