#include "StdInc.h"

#include "Lines.h"

void CLines::InjectHooks() {
    RH_ScopedClass(CLines);
    RH_ScopedCategoryGlobal();

    RH_ScopedOverloadedInstall(RenderLineNoClipping, "", 0x6FF460, void(*)(float, float, float, float, float, float, uint32, uint32));
    RH_ScopedOverloadedInstall(RenderLineWithClipping, "", 0x6FF4F0, void(*)(float, float, float, float, float, float, uint32, uint32));
    RH_ScopedOverloadedInstall(ImmediateLine2D, "", 0x6FF790, void(*)(int32, int32, int32, int32, uint8, uint8, uint8, uint8, uint8, uint8, uint8, uint8));
}

// 0x6FF460
void CLines::RenderLineNoClipping(float startX, float startY, float startZ, float endX, float endY, float endZ, uint32 startColor, uint32 endColor) {
    RxObjSpace3DVertex vertices[] = {
        { .objVertex = { startX, startY, startZ }, .color = startColor >> 8 | startColor << 24 }, // Convert color to ARGB
        { .objVertex = { endX,   endY,   endZ   }, .color =   endColor >> 8 | endColor   << 24 }, // Convert color to ARGB
    };

    LittleTest();
    if (RwIm3DTransform(vertices, 2u, nullptr, 0)) {
        RwIm3DRenderLine(0, 1);
        RwIm3DEnd();
    }
}

// 0x6FF4F0
void CLines::RenderLineWithClipping(float startX, float startY, float startZ, float endX, float endY, float endZ, uint32 startColor, uint32 endColor) {
    const CVector start = { startX, startY, startZ };
    const CVector end   = { endX,   endY,   endZ };

    // The line is split into segments (At most 7), each of them has 2 vertices
    const auto numSegments = (int16)std::min(DistanceBetweenPoints(start, end) * 0.4f + 1.0f, 7.0f);
    if (numSegments > 0) {
        // Colors are RGBA (R being the most significant byte)
        const auto GetChannel = [](uint32 color, uint32 shift) { return (float)((color >> shift) & 0xFF); };
        const float startR = GetChannel(startColor, 24), deltaR = GetChannel(endColor, 24) - startR;
        const float startG = GetChannel(startColor, 16), deltaG = GetChannel(endColor, 16) - startG;
        const float startB = GetChannel(startColor, 8),  deltaB = GetChannel(endColor, 8)  - startB;
        const float startA = GetChannel(startColor, 0),  deltaA = GetChannel(endColor, 0)  - startA;
        const auto  delta  = end - start;

        const auto SetVertex = [&](RxObjSpace3DVertex& vertex, float t) {
            const auto a = (uint32)(uint8)(int32)(deltaA * t + startA);
            const auto r = (uint32)(uint8)(int32)(deltaR * t + startR);
            const auto g = (uint32)(uint8)(int32)(deltaG * t + startG);
            const auto b = (uint32)(uint8)(int32)(deltaB * t + startB);
            vertex.color     = (a << 24) | (r << 16) | (g << 8) | b; // ARGB
            vertex.objVertex = { delta.x * t + start.x, delta.y * t + start.y, delta.z * t + start.z };
        };
        for (int32 i = 0; i < numSegments; i++) {
            SetVertex(TempBufferVertices.m_3d[2 * i + 0], (float)(i) / (float)(numSegments));
            SetVertex(TempBufferVertices.m_3d[2 * i + 1], (float)(i + 1) / (float)(numSegments));
        }
    }

    static RwImVertexIndex indices[] = { // 0x8D503C
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
        12, 13, 14, 15, 16, 17, 18, 19, 20, 0
    };
    LittleTest();
    if (RwIm3DTransform(TempBufferVertices.m_3d, 2 * numSegments, nullptr, 0)) {
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPELINELIST, indices, 2 * numSegments);
        RwIm3DEnd();
    }
}

// 0x6FF790
void CLines::ImmediateLine2D(int32 startX, int32 startY, int32 endX, int32 endY, uint8 startR, uint8 startG, uint8 startB, uint8 startA, uint8 endR, uint8 endG, uint8 endB, uint8 endA) {
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,       RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER,     RWRSTATE(rwFILTERLINEAR));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,     RWRSTATE(NULL));

    RwIm2DVertex vertices[] = {
        { .x = float(startX), .y = float(startY), .emissiveColor = CRGBA(startR, startG, startB, startA).ToIntARGB() },
        { .x = float(endX),   .y = float(endY),   .emissiveColor = CRGBA(endR, endG, endB, endA).ToIntARGB() }
    };
    RwIm2DRenderLine(vertices, 2, 0, 1);

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,       RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(TRUE));
}
