#include "StdInc.h"

#include "FxEmitterBP.h"
#include "FxEmitter.h"
#include "FxEmitterPrt.h"
#include "FxPrimBP.h"
#include "FxInfo.h"
#include "FxInfoManager.h"
#include "MovementInfo.h"

#include "Particle.h"
#include "FxTools.h"
#include "RenderInfo.h"
#include "Fx.h"
#include "FxManager.h"
#include "FxSystem.h"
#include "FxSystemBP.h"
#include "FxPrtMult.h"
#include "Camera.h"

void FxEmitterBP_c::InjectHooks() {
    RH_ScopedVirtualClass(FxEmitterBP_c, 0x85A788, 7);
    RH_ScopedCategory("Fx");

    RH_ScopedInstall(Constructor, 0x4A18D0);
    RH_ScopedInstall(RenderHeatHaze, 0x4A1940);
    RH_ScopedInstall(UpdateParticle, 0x4A21D0);
    RH_ScopedVMTInstall(CreateInstance, 0x4A2B40);
    RH_ScopedVMTInstall(Update, 0x4A2BC0);
    RH_ScopedVMTInstall(Load, 0x5C25F0);
    RH_ScopedVMTInstall(LoadTextures, 0x5C0A30, {.Reversed = true});
    RH_ScopedVMTInstall(Render, 0x4A2C40);
    RH_ScopedVMTInstall(FreePrtFromPrim, 0x4A2510);
}

// 0x4A18D0
FxEmitterBP_c::FxEmitterBP_c() : FxPrimBP_c() {
    m_Type = 0;
}

// NOTSA - Code shared by `Render` and `RenderHeatHaze` (it's all inlined in both of them)

// 0x4A2DB0 / 0x4A19D0 - Position of the particle in world space
static CVector GetParticleRenderPos(FxEmitterPrt_c* prt) {
    if (!prt->m_bLocalToSystem) {
        return prt->m_Pos;
    }
    CVector out{};
    auto* const mat = g_fxMan.FxRwMatrixCreate();
    prt->m_System->GetCompositeMatrix(mat);
    RwV3dTransformPoints(&out, &prt->m_Pos, 1, mat);
    g_fxMan.FxRwMatrixDestroy(mat);
    return out;
}

// 0x4A3338 / 0x4A1B26 - Build the matrix of a particle that is aligned along `up` (and faces the camera)
static void SetParticleMatrixFromUp(RwMatrix* mat, const CVector& up, const CVector& toCam, CVector& outAt) {
    const CVector right{ // up x toCam
        toCam.z * up.y - toCam.y * up.z,
        toCam.x * up.z - toCam.z * up.x,
        toCam.y * up.x - toCam.x * up.y
    };
    outAt = CVector{ // up x right
        right.z * up.y - right.y * up.z,
        right.x * up.z - right.z * up.x,
        right.y * up.x - right.x * up.y
    };
    mat->right = right;
    mat->up    = up;
    mat->at    = outAt;
}

// 0x4A3604 / 0x4A1C4E - Wrap the rotation of the particle into [0, 360) and calculate the (rotated) right/up vectors
static void ApplyParticleRotation(FxEmitterPrt_c* prt, RwMatrix* mat, const CVector& at, CVector& outRight, CVector& outUp) {
    if (prt->m_RotZ != -1) {
        prt->m_CurrentRotation = static_cast<float>(static_cast<uint8>(prt->m_RotZ)) * 2.0f;
    }

    if (prt->m_CurrentRotation < 0.0f) {
        auto rot = prt->m_CurrentRotation;
        do {
            rot += 360.0f;
        } while (rot < 0.0f);
        prt->m_CurrentRotation = rot;
    }
    if (!(prt->m_CurrentRotation < 360.0f)) {
        auto rot = prt->m_CurrentRotation;
        do {
            rot -= 360.0f;
        } while (!(rot < 360.0f));
        prt->m_CurrentRotation = rot;
    }

    if (prt->m_CurrentRotation > 0.0f) {
        RotateVecAboutVec(outRight, mat->right, at, prt->m_CurrentRotation * 0.0174533f); // 0x85A7BC - degrees to radians
        outUp = CVector{ // at x right
            at.y * outRight.z - at.z * outRight.y,
            at.z * outRight.x - at.x * outRight.z,
            at.x * outRight.y - at.y * outRight.x
        };
    } else {
        outRight = mat->right;
        outUp    = mat->up;
    }
}

// 0x4A3720 / 0x4A1D70 - Apply the per-particle random size bias and size multiplier
static void ApplyParticleSize(FxEmitterPrt_c* prt, RenderInfo_t& renderInfo) {
    renderInfo.m_fSizeX += (static_cast<float>(prt->m_RandR) / 255.0f - 0.5f) * renderInfo.m_fSizeXBias;
    renderInfo.m_fSizeY += (static_cast<float>(prt->m_RandG) / 255.0f - 0.5f) * renderInfo.m_fSizeYBias;

    const float multSize = prt->m_MultSize;
    if (multSize < 1.0f) { // SA: raw value < 255
        renderInfo.m_fSizeX *= multSize;
        renderInfo.m_fSizeY *= multSize;
    }
}

// 0x4A3A9E / 0x4A1E10 - Raster of the (possibly animated) texture of the particle
static RwRaster* GetParticleRaster(const std::array<RwTexture*, 4>& textures, const RenderInfo_t& renderInfo, RwRaster* prevRaster) {
    if (!renderInfo.m_bHasAnimTextures) {
        return RwTextureGetRaster(textures[0]);
    }
    switch (renderInfo.m_nCurrentTexId) {
    case 1:
        return RwTextureGetRaster(textures[0]);
    case 2:
    case 3:
    case 4: {
        auto* const tex = textures[renderInfo.m_nCurrentTexId - 1];
        return RwTextureGetRaster(tex ? tex : textures[0]);
    }
    default:
        return prevRaster; // SA: Whatever was used for the previous particle
    }
}

// 0x4A3B7D / 0x4A1EB4 - Add the 2 triangles of the particle to the render buffer
static void AddParticleQuad(const CVector& pos, const CVector& right, const CVector& up, const RenderInfo_t& renderInfo) {
    const auto top    = up * (renderInfo.m_fSpriteTop * renderInfo.m_fSizeY);
    const auto bottom = up * (renderInfo.m_fSpriteBottom * renderInfo.m_fSizeY);
    const auto left   = right * (renderInfo.m_fSpriteLeft * renderInfo.m_fSizeX);
    const auto rght   = right * (renderInfo.m_fSpriteRight * renderInfo.m_fSizeX);

    const CVector bottomRight{ rght.x + bottom.x, rght.y + bottom.y, rght.z + bottom.z };
    const CVector topLeft{ left.x + top.x, left.y + top.y, left.z + top.z };

    const int32 r = renderInfo.m_Color1.red;
    const int32 g = renderInfo.m_Color1.green;
    const int32 b = renderInfo.m_Color1.blue;
    const int32 a = renderInfo.m_Color1.alpha;

    RenderAddTri_(
        topLeft.x + pos.x,           topLeft.y + pos.y,           topLeft.z + pos.z,
        bottomRight.x + pos.x,       bottomRight.y + pos.y,       bottomRight.z + pos.z,
        rght.x + top.x + pos.x,      rght.y + top.y + pos.y,      rght.z + top.z + pos.z,
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        r, g, b, a,
        r, g, b, a,
        r, g, b, a
    );
    RenderAddTri_(
        bottomRight.x + pos.x,       bottomRight.y + pos.y,       bottomRight.z + pos.z,
        topLeft.x + pos.x,           topLeft.y + pos.y,           topLeft.z + pos.z,
        left.x + bottom.x + pos.x,   left.y + bottom.y + pos.y,   left.z + bottom.z + pos.z,
        1.0f, 1.0f,
        0.0f, 0.0f,
        0.0f, 1.0f,
        r, g, b, a,
        r, g, b, a,
        r, g, b, a
    );
}

// 0x4A1940
void FxEmitterBP_c::RenderHeatHaze(RwCamera* camera, uint32 txdHashKey, float brightness) {
    if (!m_Particles.GetNumItems()) {
        return;
    }

    auto* currRaster = RwTextureGetRaster(m_apTextures[0]);

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(rwBLENDONE));

    RenderBegin(currRaster, nullptr, rwIM3D_VERTEXUV);

    const auto* const camMat = RwFrameGetMatrix(RwCameraGetFrame(camera));

    RwRaster* prtRaster = currRaster; // SA: Uninitialized until a particle picks a texture
    for (auto* it = m_Particles.GetHead(); it; it = m_Particles.GetNext(it)) {
        auto* const prt = it->AsFxEmitterPrt();

        const auto pos = GetParticleRenderPos(prt);

        RenderInfo_t renderInfo;
        m_FxInfoManager.ProcessRenderInfo(
            prt->m_System->m_fCurrentTime,
            prt->m_fCurrentLife / prt->m_fTotalLife,
            0.0f,
            prt->m_System->m_SystemBP->m_fLength,
            false,
            &renderInfo
        );

        // 0x4A1A4E
        auto* const mat = g_fxMan.FxRwMatrixCreate();
        RwMatrixSetIdentity(mat);

        CVector right, up, at;
        if (renderInfo.m_bHasDir) {
            up = renderInfo.m_bUseVel ? prt->m_Velocity : renderInfo.m_Direction;
            RwV3dNormalize(&up, &up);

            CVector toCam = pos - CVector{ camMat->pos };
            RwV3dNormalize(&toCam, &toCam);

            SetParticleMatrixFromUp(mat, up, toCam, at);
        } else {
            *mat = renderInfo.m_bIsFlat ? renderInfo.m_FlatMatrix : *camMat;
            at   = mat->at;
        }

        // 0x4A1C4E
        ApplyParticleRotation(prt, mat, at, right, up);
        g_fxMan.FxRwMatrixDestroy(mat);

        // 0x4A1D70
        ApplyParticleSize(prt, renderInfo);

        renderInfo.m_Color1.red   = 0;
        renderInfo.m_Color1.green = 0;
        renderInfo.m_Color1.blue  = 0;

        // 0x4A1E10
        prtRaster = GetParticleRaster(m_apTextures, renderInfo, prtRaster);
        if (currRaster != prtRaster) {
            RenderEnd();
            currRaster = prtRaster;
            RenderBegin(prtRaster, nullptr, rwIM3D_VERTEXUV);
        }

        // 0x4A1EB4
        AddParticleQuad(pos, right, up, renderInfo);
    }

    RenderEnd();
}

// 0x4A21D0
bool FxEmitterBP_c::UpdateParticle(float deltaTime, FxEmitterPrt_c* prt) {
    auto& system = *prt->m_System;
    auto& systemBP = *system.m_SystemBP;

    const auto correctedDeltaTime = (float)system.m_nTimeMult / 1000.0f * deltaTime;
    prt->m_fCurrentLife += correctedDeltaTime;
    if (prt->m_fTotalLife <= prt->m_fCurrentLife) {
        return true;
    }

    prt->m_Pos += prt->m_Velocity * correctedDeltaTime;
    MovementInfo_t movement{};
    movement.m_Pos = prt->m_Pos;
    movement.m_Vel = prt->m_Velocity;
    m_FxInfoManager.ProcessMovementInfo(
        system.m_fCurrentTime,
        prt->m_fCurrentLife / prt->m_fTotalLife,
        correctedDeltaTime,
        systemBP.m_fLength,
        false,
        &movement
    );
    prt->m_Pos = movement.m_Pos;
    prt->m_Velocity = movement.m_Vel;

    if (movement.m_bHasFloatInfo || movement.m_bHasUnderwaterInfo) {
        float waterLevel = 0.0f;
        const auto hasWaterLevel = CWaterLevel::GetWaterLevel(prt->m_Pos.x, prt->m_Pos.y, prt->m_Pos.z, waterLevel, true, nullptr);
        if (movement.m_bHasFloatInfo && hasWaterLevel && prt->m_Pos.z < waterLevel) {
            prt->m_Pos.z = waterLevel;
        }
        if (movement.m_bHasUnderwaterInfo) {
            if (!hasWaterLevel || waterLevel < prt->m_Pos.z) {
                return true;
            }
        }
    }

    if ((movement.m_Rot[0] <= 0.0f && movement.m_Rot[1] <= 0.0f) || (movement.m_Rot[2] <= 0.0f && movement.m_Rot[3] <= 0.0f)) {
        if (movement.m_Rot[0] > 0.0f || movement.m_Rot[1] > 0.0f) {
            prt->m_CurrentRotation += ((movement.m_Rot[1] - movement.m_Rot[0]) * (float)prt->m_RandR / 255.0f + movement.m_Rot[0]) * (float)prt->m_MultRot * correctedDeltaTime / 255.0f;
            return false;
        }
        if (movement.m_Rot[2] <= 0.0f && movement.m_Rot[3] <= 0.0f) {
            return false;
        }
        const auto rotSpeed = (movement.m_Rot[3] - movement.m_Rot[2]) * (float)prt->m_RandR / 255.0f + movement.m_Rot[2];
        prt->m_CurrentRotation -= rotSpeed * (float)prt->m_MultRot * correctedDeltaTime / 255.0f;
    } else {
        if (prt->m_RandR < 0x80) {
            prt->m_CurrentRotation += ((movement.m_Rot[1] - movement.m_Rot[0]) * (float)prt->m_RandR * (1.0f / 128.0f) + movement.m_Rot[0]) * (float)prt->m_MultRot * correctedDeltaTime / 255.0f;
            return false;
        }
        prt->m_CurrentRotation -= ((movement.m_Rot[3] - movement.m_Rot[2]) * ((float)prt->m_RandR - 128.0f) * (1.0f / 128.0f) + movement.m_Rot[2]) * (float)prt->m_MultRot * correctedDeltaTime / 255.0f;
    }
    return false;
}

// 0x4A2B40


FxPrim_c* FxEmitterBP_c::CreateInstance() {
    return new FxEmitter_c();
}

// 0x4A2BC0


void FxEmitterBP_c::Update(float deltaTime) {
    for (auto* particle = m_Particles.GetHead(); particle;) {
        if (particle->m_System->m_nKillStatus == eFxSystemKillStatus::FX_3) {
            particle->m_System->m_nKillStatus = eFxSystemKillStatus::FX_KILLED;
        }
        auto* next = m_Particles.GetNext(particle); // NB: cache it, the particle may be removed below
        if (particle->m_System->m_nPlayStatus != eFxSystemPlayStatus::T2 && UpdateParticle(deltaTime, reinterpret_cast<FxEmitterPrt_c*>(particle))) {
            m_Particles.RemoveItem(particle);
            g_fxMan.ReturnParticle(reinterpret_cast<FxEmitterPrt_c*>(particle));
        }
        particle = next;
    }
}

// 0x5C25F0


bool FxEmitterBP_c::Load(FILESTREAM file, int32 version, FxName32_t* textureNames) {
    FxPrimBP_c::Load(file, version, textureNames);

    m_FxInfoManager.m_nLodStart = uint16(ReadField<float>(file, "LODSTART:") * 64.0f);
    m_FxInfoManager.m_nLodEnd   = uint16(ReadField<float>(file, "LODEND:") * 64.0f);

    return true;
}

// 0x5C0A30


bool FxEmitterBP_c::LoadTextures(FxName32_t* textureNames, int32 version) {
    assert(textureNames);

    const auto LoadTexture = [&](auto ind) -> RwTexture* {
        char mask[64];
        sprintf(mask, "%sm", textureNames[ind]);

        auto* texture = RwTextureRead(textureNames[ind], mask);
        return texture ? texture : RwTextureRead(textureNames[ind], nullptr);
    };

    const auto LoadTextureIfExists = [=](auto ind) -> RwTexture* {
        assert(&textureNames[ind]);
        return strncmp(textureNames[ind], "NULL", 5u) != 0 ? LoadTexture(ind) : nullptr;
    };

    m_apTextures[0] = LoadTexture(0);

    if (version > 101) {
        m_apTextures[1] = LoadTextureIfExists(1);
        m_apTextures[2] = LoadTextureIfExists(2);
        m_apTextures[3] = LoadTextureIfExists(3);
    }

    return true;
}

// 0x4A2C40
void FxEmitterBP_c::Render(RwCamera* camera, uint32 txdHashKey, float brightness, bool doHeatHaze) {
    // 0x8A6230
    static constexpr RwBlendFunction g_BlendFunctions[] = {
        rwBLENDZERO,      rwBLENDONE,          rwBLENDSRCCOLOR,  rwBLENDINVSRCCOLOR,  rwBLENDSRCALPHA, rwBLENDINVSRCALPHA,
        rwBLENDDESTALPHA, rwBLENDINVDESTALPHA, rwBLENDDESTCOLOR, rwBLENDINVDESTCOLOR, rwBLENDSRCALPHASAT
    };

    const bool hasHeatHazeInfo = IsFxInfoPresent(FX_INFO_HEATHAZE_DATA);

    if (doHeatHaze) {
        if (m_FxInfoManager.m_bHasHeatHazeParticleEmitter) {
            RenderHeatHaze(camera, txdHashKey, brightness);
        }
        return;
    }

    if (hasHeatHazeInfo) {
        if (m_Particles.GetNumItems() > 0) {
            g_fxMan.m_bHeatHazeEnabled = true;
        }
        return;
    }

    if (!m_Particles.GetNumItems()) {
        return;
    }

    auto* currRaster = RwTextureGetRaster(m_apTextures[0]);

    // 0x4A2D0B
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(m_bAlphaOn));
    if (m_bAlphaOn) {
        RwRenderStateSet(rwRENDERSTATESRCBLEND,  RWRSTATE(g_BlendFunctions[m_nSrcBlendId]));
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(g_BlendFunctions[m_nDstBlendId]));
    } else {
        RwRenderStateSet(rwRENDERSTATESRCBLEND,  RWRSTATE(g_BlendFunctions[1])); // rwBLENDONE
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(g_BlendFunctions[0])); // rwBLENDZERO
    }

    RenderBegin(currRaster, nullptr, rwIM3D_VERTEXUV);

    const auto* const camMat = RwFrameGetMatrix(RwCameraGetFrame(camera));
    const CVector     camPos = camMat->pos;

    RwRaster* prtRaster = currRaster; // SA: Uninitialized until a particle picks a texture
    for (auto* it = m_Particles.GetHead(); it; it = m_Particles.GetNext(it)) {
        auto* const prt = it->AsFxEmitterPrt();

        const auto pos = GetParticleRenderPos(prt);

        RenderInfo_t renderInfo;
        m_FxInfoManager.ProcessRenderInfo(
            prt->m_System->m_fCurrentTime,
            prt->m_fCurrentLife / prt->m_fTotalLife,
            0.0f,
            prt->m_System->m_SystemBP->m_fLength,
            false,
            &renderInfo
        );

        // 0x4A2E36 - Particles that leave a smoke trail
        if (renderInfo.m_SmokeType > -1) {
            const auto rnd        = static_cast<float>(CGeneral::GetRandomNumber()) * RAND_MAX_FLOAT_RECIPROCAL;
            const auto brightBias = ((renderInfo.m_SmokeBrightness - (-renderInfo.m_SmokeBrightness)) * rnd + (-renderInfo.m_SmokeBrightness)) / 255.0f;

            FxPrtMult_c prtMult;
            prtMult.m_Color.red   = std::clamp(renderInfo.m_SmokeColor.red / 255.0f + brightBias, 0.0f, 1.0f);
            prtMult.m_Color.green = std::clamp(renderInfo.m_SmokeColor.green / 255.0f + brightBias, 0.0f, 1.0f);
            prtMult.m_Color.blue  = std::clamp(renderInfo.m_SmokeColor.blue / 255.0f + brightBias, 0.0f, 1.0f);
            prtMult.m_Color.alpha = renderInfo.m_SmokeColor.alpha / 255.0f;
            prtMult.m_fSize       = renderInfo.m_SmokeSize;
            prtMult.m_Rot         = 1.0f;
            prtMult.m_fLife       = renderInfo.m_SmokeLife;

            const CVector smokePos = prt->m_Pos; // NB: Not the world space position
            const CVector smokeVel{ 0.0f, 0.0f, 0.0f };
            g_fx.m_SmokeHuge->AddParticle(smokePos, smokeVel, 0.0f, prtMult, -1.0f, 1.2f, 0.6f, false);
        }

        // 0x4A3049
        auto* const mat = g_fxMan.FxRwMatrixCreate();
        RwMatrixSetIdentity(mat);

        CVector right, up, at;
        CVector trailDir{};
        if (renderInfo.m_nTrailScreenMode > 0) {
            // 0x4A309B
            if (renderInfo.m_nTrailScreenMode == 2) {
                // Movement of the particle relative to the camera since the previous frame
                const auto prevPos = prt->m_Pos - prt->m_Velocity * (CTimer::GetTimeStep() * 0.02f);
                trailDir = ((prt->m_Pos - camPos) - (prevPos - TheCamera.m_mCameraMatrixOld.GetPosition())) * renderInfo.m_fTrailTime;
            } else {
                const CVector prevPos{
                    prt->m_Pos.x - renderInfo.m_fTrailTime * prt->m_Velocity.x,
                    prt->m_Pos.y - renderInfo.m_fTrailTime * prt->m_Velocity.y,
                    prt->m_Pos.z - renderInfo.m_fTrailTime * prt->m_Velocity.z
                };
                trailDir = prt->m_Pos - prevPos;
            }

            // 0x4A32A1
            up = trailDir;
            if (up.x == 0.0f && up.y == 0.0f && up.z == 0.0f) {
                up.z = 1.0f;
            } else {
                RwV3dNormalize(&up, &up);
            }

            CVector toCam = prt->m_Pos - camPos; // NB: Not the world space position
            RwV3dNormalize(&toCam, &toCam);

            SetParticleMatrixFromUp(mat, up, toCam, at);
        } else if (renderInfo.m_bHasDir) {
            // 0x4A3411
            if (renderInfo.m_bUseVel) {
                up = prt->m_Velocity;
                if (up.x == 0.0f && up.y == 0.0f && up.z == 0.0f) {
                    up.z = 1.0f;
                }
            } else {
                up = renderInfo.m_Direction;
            }
            RwV3dNormalize(&up, &up);

            CVector toCam = pos - camPos;
            RwV3dNormalize(&toCam, &toCam);

            SetParticleMatrixFromUp(mat, up, toCam, at);
        } else {
            // 0x4A35A4
            *mat = renderInfo.m_bIsFlat ? renderInfo.m_FlatMatrix : *camMat;
            at   = mat->at;
        }

        // 0x4A3604
        ApplyParticleRotation(prt, mat, at, right, up);
        g_fxMan.FxRwMatrixDestroy(mat);

        // 0x4A3720
        ApplyParticleSize(prt, renderInfo);

        // 0x4A37C0 - Colour
        float r = static_cast<float>(renderInfo.m_Color1.red);
        float g = static_cast<float>(renderInfo.m_Color1.green);
        float b = static_cast<float>(renderInfo.m_Color1.blue);
        float a = static_cast<float>(renderInfo.m_Color1.alpha);

        switch (renderInfo.m_nColorType) {
        case ERenderColorType::RANGE: { // 0x4A381C
            r += (static_cast<float>(renderInfo.m_Color2.red) - r) * (static_cast<float>(prt->m_RandR) / 255.0f);
            g += (static_cast<float>(renderInfo.m_Color2.green) - g) * (static_cast<float>(prt->m_RandG) / 255.0f);
            b += (static_cast<float>(renderInfo.m_Color2.blue) - b) * (static_cast<float>(prt->m_RandB) / 255.0f);
            break;
        }
        case ERenderColorType::BRIGHT: { // 0x4A38B2
            const auto bias = (static_cast<float>(prt->m_RandR) * (1.0f / 128.0f) - 1.0f) * static_cast<float>(renderInfo.m_Color2.alpha);
            r = std::clamp(r + bias, 0.0f, 255.0f);
            g = std::clamp(g + bias, 0.0f, 255.0f);
            b = std::clamp(b + bias, 0.0f, 255.0f);
            break;
        }
        default:
            break;
        }

        // 0x4A3982 - Colour multiplier (only applied if the raw value isn't 255)
        if (prt->m_MultColor.r != 255) {
            r *= static_cast<float>(prt->m_MultColor.r) / 255.0f;
        }
        if (prt->m_MultColor.g != 255) {
            g *= static_cast<float>(prt->m_MultColor.g) / 255.0f;
        }
        if (prt->m_MultColor.b != 255) {
            b *= static_cast<float>(prt->m_MultColor.b) / 255.0f;
        }
        if (prt->m_MultColor.a != 255) {
            a *= static_cast<float>(prt->m_MultColor.a) / 255.0f;
        }

        // 0x4A39FC - Lighting
        if (!renderInfo.m_bSelfLit) {
            // NB: The brightness of a particle "sticks" - it's used for all following particles that don't have their own
            if (const float prtBrightness = prt->m_Brightness; prtBrightness <= 1.0f) { // SA: raw value <= 100
                brightness = prtBrightness;
            }
            r *= brightness;
            g *= brightness;
            b *= brightness;
        }
        renderInfo.m_Color1.red   = static_cast<uint8>(static_cast<int32>(r));
        renderInfo.m_Color1.green = static_cast<uint8>(static_cast<int32>(g));
        renderInfo.m_Color1.blue  = static_cast<uint8>(static_cast<int32>(b));
        renderInfo.m_Color1.alpha = static_cast<uint8>(static_cast<int32>(a));

        // 0x4A3A9E - Texture
        prtRaster = GetParticleRaster(m_apTextures, renderInfo, prtRaster);
        if (currRaster != prtRaster) {
            RenderEnd();
            currRaster = prtRaster;
            RenderBegin(prtRaster, nullptr, rwIM3D_VERTEXUV);
        }

        // 0x4A3B53 - Trails are stretched along their direction
        if (renderInfo.m_nTrailScreenMode > 0) {
            renderInfo.m_fSpriteTop    = RwV3dLength(&trailDir);
            renderInfo.m_fSpriteBottom = 0.0f;
        }

        // 0x4A3B7D
        AddParticleQuad(pos, right, up, renderInfo);
    }

    RenderEnd();
}

// 0x4A2510
bool FxEmitterBP_c::FreePrtFromPrim(FxSystem_c* system) {
    for (auto* particle = m_Particles.GetHead(); particle; particle = m_Particles.GetNext(particle)) {
        if (particle->m_System == system) {
            m_Particles.RemoveItem(particle);
            g_fxMan.ReturnParticle(reinterpret_cast<FxEmitterPrt_c*>(particle));
            return true;
        }
    }
    return false;
}

// todo: eFxInfo
// 0x4A24D0
bool FxEmitterBP_c::IsFxInfoPresent(eFxInfoType type) const {
    if (m_FxInfoManager.m_nNumInfos <= 0)
        return false;

    for (auto& info : m_FxInfoManager.GetInfos()) {
        if (info->m_nType == type) {
            return true;
        }
    }
    return false;
}
