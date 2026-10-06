#include "StdInc.h"
#include "StencilShadowObject.h"

// TODO: Statically allocate after reversing RenderForVehicle&RenderForObject.
static inline auto& s_ShadowTrianglePointsUnk         = StaticRef<RxVertexIndex*>(0xC6A170);
static inline auto& s_ShadowTrianglePoints            = StaticRef<CVector*>(0xC6A174);
static inline auto& s_TransformedShadowTrianglePoints = StaticRef<CVector*>(0xC6A178);
static inline auto& s_SunPosNrm                       = StaticRef<CVector>(0x8D5244); // CVector(1.0, 1.0, -2.0)

// 0x70FA70
// Maintains the silhouette edge list used for shadow volume extrusion.
// Custom calling convention (matches the original binary exactly):
//   ECX   - uint16 edge pairs array (two uint16s per edge)
//   EDX   - uint16* edge count (in/out)
//   EDI   - first vertex index (passed in the register, not saved/restored)
//   stack - second vertex index (caller cleans up)
// Adds the edge, unless it's already present (in either direction),
// in which case it's removed again (shared/interior edges cancel out).
static void __declspec(naked) AddShadowSilhouetteEdge() {
    _asm push ebx
    _asm movzx ebx, word ptr [edx]
    _asm push ebp
    _asm mov bp, word ptr [esp + 0x0C]
    _asm xor eax, eax
    _asm test ebx, ebx
    _asm push esi
    _asm jle EDGE_APPEND
EDGE_LOOP:
    _asm mov si, word ptr [ecx + eax*4]
    _asm cmp si, di
    _asm jnz EDGE_CHECK_SWAPPED
    _asm cmp word ptr [ecx + eax*4 + 2], bp
    _asm jz EDGE_FOUND
EDGE_CHECK_SWAPPED:
    _asm cmp si, bp
    _asm jnz EDGE_NEXT
    _asm cmp word ptr [ecx + eax*4 + 2], di
    _asm jz EDGE_FOUND
EDGE_NEXT:
    _asm movzx esi, word ptr [edx]
    _asm inc eax
    _asm cmp eax, esi
    _asm jl EDGE_LOOP
EDGE_APPEND:
    _asm mov word ptr [ecx + ebx*4], di
    _asm movzx eax, word ptr [edx]
    _asm pop esi
    _asm mov word ptr [ecx + eax*4 + 2], bp
    _asm inc word ptr [edx]
    _asm pop ebp
    _asm pop ebx
    _asm ret
EDGE_FOUND:
    _asm cmp word ptr [edx], 1
    _asm jbe EDGE_DECR
    _asm mov si, word ptr [ecx + ebx*4 - 4]
    _asm mov word ptr [ecx + eax*4], si
    _asm movzx esi, word ptr [edx]
    _asm mov si, word ptr [ecx + esi*4 - 2]
    _asm mov word ptr [ecx + eax*4 + 2], si
EDGE_DECR:
    _asm dec word ptr [edx]
    _asm pop esi
    _asm pop ebp
    _asm pop ebx
    _asm ret
}

// C++-callable wrapper for the custom-convention 0x70FA70 helper above.
// Original callers pass (ECX = edges, EDX = &count, EDI = first index, stack = second index).
static void CallAddShadowSilhouetteEdge(uint16 a, uint16 b, RxVertexIndex* edges, uint16* count) {
    uint16 aa = a, bb = b;
    _asm mov ecx, edges
    _asm mov edx, count
    _asm mov di, aa
    _asm push bb
    _asm call AddShadowSilhouetteEdge
    _asm add esp, 2
}

void CStencilShadows::InjectHooks() {
    RH_ScopedClass(CStencilShadows);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x70F9E0);
    RH_ScopedInstall(Shutdown, 0x711390);
    RH_ScopedInstall(Process, 0x711D90);
    RH_ScopedInstall(GraphicsHighQuality, 0x70F9B0);
    RH_ScopedInstall(UpdateHierarchy, 0x710BC0);
    RH_ScopedInstall(RegisterStencilShadows, 0x711760);
    RH_ScopedInstall(RenderStencilShadows, 0x7113B0);
    RH_ScopedInstall(RenderForVehicle, 0x70FAE0);
    RH_ScopedInstall(RenderForObject, 0x710310);
    RH_ScopedInstall(Render, 0x710D50);
    RH_ScopedInstall(RenderBuffer, 0x710B50);
    RH_ScopedInstall(sub_710CC0, 0x710CC0);
}

// 0x70F9E0
void CStencilShadows::Init() {
    ZoneScoped;

    RwD3D9SetStencilClear(0);
    pFirstAvailableStencilShadowObject = m_StencilShadowObjects.data();
    pFirstActiveStencilShadowObject = nullptr;

    for (auto&& [i, obj] : rngv::enumerate(m_StencilShadowObjects)) {
        obj.m_pOwner                = nullptr;
        obj.m_NumShadowFaces        = 0;
        obj.m_Type                  = eStencilShadowObjType::NONE;
        obj.m_SizeOfShadowFacesData = 0;
        obj.m_FaceID                = 0;
        obj.m_ShadowFacesData       = 0;

        obj.m_pPrev = i ? &m_StencilShadowObjects[i - 1] : nullptr;
        obj.m_pNext = (i != m_StencilShadowObjects.size() - 1) ? &m_StencilShadowObjects[i + 1] : nullptr;
    }
}

// 0x711390
void CStencilShadows::Shutdown() {
    for (auto* obj = pFirstActiveStencilShadowObject; obj;) {
        auto* next = obj->m_pNext;
        obj->Destroy();
        obj = next;
    }
}

// 0x710D50
void CStencilShadows::Render(const CRGBA& color) {
    uiTempBufferIndicesStored  = 0;
    uiTempBufferVerticesStored = 0;

    for (auto* shadow = pFirstActiveStencilShadowObject; shadow; shadow = shadow->m_pNext) {
        const auto  numFaces = shadow->m_SizeOfShadowFacesData / 6; // 6 CVector components per face // 0x2AAAAAAB
        const auto* facePts  = shadow->m_ShadowFacesData;

        // Each shadow face holds 6 extruded verts (2 triangles) stored as 6 consecutive CVectors.
        for (auto face = 0u; face < numFaces; face++, facePts += 6) {
            // CRGBA is RGBA in memory, but the Im3D vertex color is ABGR - swizzle R and B
            // (original does this with shifts/ors rather than calling ToIntABGR).
            const auto intColor = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
            const auto nextVert = std::exchange(uiTempBufferVerticesStored, static_cast<uint16>(uiTempBufferVerticesStored + 6));

            const auto nextIdx  = std::exchange(uiTempBufferIndicesStored, static_cast<uint16>(uiTempBufferIndicesStored + 6));
            aTempBufferIndices[nextIdx + 0] = nextVert + 0;
            aTempBufferIndices[nextIdx + 1] = nextVert + 1;
            aTempBufferIndices[nextIdx + 2] = nextVert + 2;
            aTempBufferIndices[nextIdx + 3] = nextVert + 3;
            aTempBufferIndices[nextIdx + 4] = nextVert + 4;
            aTempBufferIndices[nextIdx + 5] = nextVert + 5;
            for (auto i = 0; i < 6; i++) {
                auto& vert     = TempBufferVertices.m_3d[nextVert + i];
                vert.objVertex = facePts[i];
                vert.color     = intColor;
            }
        }

        // Odd trailing triangle? - if (m_SizeOfShadowFacesData / 3) is odd, append the 3 leftover verts.
        // (original computes `(size / 3) & 1` via `0x55555556ull * size >> 32` magic division)
        if ((shadow->m_SizeOfShadowFacesData / 3) & 1) {
            const auto* triPts = shadow->m_ShadowFacesData + numFaces * 6;

            sub_710CC0(3, 3);

            const auto intColor = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
            const auto nextVert = std::exchange(uiTempBufferVerticesStored, static_cast<uint16>(uiTempBufferVerticesStored + 3));
            const auto nextIdx  = std::exchange(uiTempBufferIndicesStored, static_cast<uint16>(uiTempBufferIndicesStored + 3));
            aTempBufferIndices[nextIdx + 0] = nextVert + 0;
            aTempBufferIndices[nextIdx + 1] = nextVert + 1;
            aTempBufferIndices[nextIdx + 2] = nextVert + 2;

            for (auto i = 0; i < 3; i++) {
                auto& vert     = TempBufferVertices.m_3d[nextVert + i];
                vert.objVertex = triPts[i];
                vert.color     = intColor;
            }
        }
    }

    // Flush whatever's left in the temp buffer (same as sub_710CC0's flush, but unconditional on overflow).
    if (uiTempBufferIndicesStored && uiTempBufferVerticesStored) {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
        LittleTest();
        if (RwIm3DTransform(TempBufferVertices.m_3d, uiTempBufferVerticesStored, nullptr, rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA)) {
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, aTempBufferIndices, uiTempBufferIndicesStored);
            RwIm3DEnd();
        }
        uiTempBufferVerticesStored = uiTempBufferIndicesStored = 0;
    }
}

// unused
// 0x710AF0
void CStencilShadows::SunSetPositionFromEntity(const CEntity* entity) {
    if (!entity) {
        return;
    }
    s_SunPosNrm = entity->GetPosition().Normalized();
}

// 0x710B50
void CStencilShadows::RenderBuffer(const CVector& pos) {
    s_SunPosNrm = pos.Normalized(); 
}

// 0x710CC0
void CStencilShadows::sub_710CC0(int32 indices, int32 vertices) {
    if (uiTempBufferIndicesStored + indices < TOTAL_TEMP_BUFFER_INDICES
        && uiTempBufferVerticesStored + vertices < TOTAL_TEMP_BUFFER_3DVERTICES) {
        return;
    }

    if (!uiTempBufferIndicesStored || !uiTempBufferVerticesStored) {
        return;
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
    LittleTest();
    if (RwIm3DTransform(TempBufferVertices.m_3d, uiTempBufferVerticesStored, nullptr, rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA)) {
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, aTempBufferIndices, uiTempBufferIndicesStored);
        RwIm3DEnd();
    }
    uiTempBufferVerticesStored = uiTempBufferIndicesStored = 0;
}

// 0x7113B0
void CStencilShadows::RenderStencilShadows() {
    ZoneScoped;

    if (!GraphicsHighQuality()) {
        return;
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,             RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATESTENCILENABLE,            RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESHADEMODE,                RWRSTATE(rwSHADEMODEFLAT));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,                RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,            RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,        RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,                 RWRSTATE(rwBLENDZERO));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,                RWRSTATE(rwBLENDONE));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTIONMASK,      RWRSTATE(uint32(-1)));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTIONWRITEMASK, RWRSTATE(uint32(-1)));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTION,          RWRSTATE(rwSTENCILFUNCTIONALWAYS));
    RwRenderStateSet(rwRENDERSTATESTENCILFAIL,              RWRSTATE(rwSTENCILOPERATIONKEEP));
    RwRenderStateSet(rwRENDERSTATESTENCILZFAIL,             RWRSTATE(rwSTENCILOPERATIONKEEP));
    RwRenderStateSet(rwRENDERSTATESTENCILPASS,              RWRSTATE(rwSTENCILOPERATIONKEEP));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTIONREF,       RWRSTATE(0));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,              RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESTENCILZFAIL,             RWRSTATE(rwSTENCILOPERATIONINCR));
    RwRenderStateSet(rwRENDERSTATECULLMODE,                 RWRSTATE(rwCULLMODECULLFRONT));

    Render(CRGBA{ 0, 0, 0, 255 });

    RwRenderStateSet(rwRENDERSTATESTENCILZFAIL,             RWRSTATE(rwSTENCILOPERATIONDECR));
    RwRenderStateSet(rwRENDERSTATECULLMODE,                 RWRSTATE(rwCULLMODECULLBACK));

    Render(CRGBA{ 0, 0, 0, 255 });

    // WTF is up with these states?
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,        RWRSTATE(FALSE)); // same state
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,             RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,              RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESTENCILENABLE,            RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATESHADEMODE,                RWRSTATE(rwSHADEMODEGOURAUD));
    RwRenderStateSet(rwRENDERSTATECULLMODE,                 RWRSTATE(rwCULLMODECULLBACK));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,             RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,              RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATESTENCILENABLE,            RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,                RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,        RWRSTATE(TRUE)); // same state
    RwRenderStateSet(rwRENDERSTATESRCBLEND,                 RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,                RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATESHADEMODE,                RWRSTATE(rwSHADEMODEFLAT));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,            RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATECULLMODE,                 RWRSTATE(rwCULLMODECULLNONE));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTIONREF,       RWRSTATE(1u));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTION,          RWRSTATE(rwSTENCILFUNCTIONLESSEQUAL));
    RwRenderStateSet(rwRENDERSTATESTENCILFAIL,              RWRSTATE(rwSTENCILOPERATIONKEEP));
    RwRenderStateSet(rwRENDERSTATESTENCILZFAIL,             RWRSTATE(rwSTENCILOPERATIONKEEP));
    RwRenderStateSet(rwRENDERSTATESTENCILPASS,              RWRSTATE(rwSTENCILOPERATIONKEEP));

    CSprite2d::InitPerFrame();
    CSprite2d::DrawRect(
        CRect{ 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT },
        CRGBA{ 0, 0, 0, (uint8)(50u * CTimeCycle::m_CurrentColours.m_nShadowStrength / 256) }
    );

    RwRenderStateSet(rwRENDERSTATESTENCILENABLE,            RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,             RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,              RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,        RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATESHADEMODE,                RWRSTATE(rwSHADEMODEGOURAUD));
    RwRenderStateSet(rwRENDERSTATECULLMODE,                 RWRSTATE(rwCULLMODECULLBACK));
}

/*!
 * @notsa
 * @brief The shadow volume generation shared by `RenderForVehicle` (0x70FB8D onwards) and `RenderForObject` (0x71037C onwards).
 *        Both originals contain a copy of this code, the only difference is the length of the shadow.
 * @param object       The shadow object to (re)build the shadow volume of
 * @param colData      The collision data of the owner (Contains the shadow mesh)
 * @param matrix       The matrix used for transforming the shadow mesh to world space
 * @param shadowLength How far the shadow volume is extruded
 */
static void BuildStencilShadowVolume(CStencilShadowObject* object, CCollisionData* colData, CMatrix& matrix, float shadowLength) {
    // Sun direction in the object's space
    CMatrix invMatrix;
    Invert(matrix, invMatrix);
    const CVector sunPos    = s_SunPosNrm;
    const CVector originLcl = invMatrix * CVector{ 0.0f, 0.0f, 0.0f };
    const CVector sunPosLcl = invMatrix * sunPos;
    const CVector sunDir    = sunPosLcl - originLcl;

    auto* const edges       = s_ShadowTrianglePointsUnk; // Silhouette edges (2 vertex indices per edge)
    const auto  numTris     = (int32)object->m_NumShadowFaces;
    const auto  numVerts    = (int32)colData->m_nNumShadowVertices;
    auto* const points      = s_ShadowTrianglePoints;
    auto* const pointsWorld = s_TransformedShadowTrianglePoints;

    object->m_SizeOfShadowFacesData = 0;

    for (auto i = 0; i < numVerts; i++) {
        colData->GetShadTrianglePoint(points[i], i);
    }
    TransformPoints(reinterpret_cast<RwV3d*>(pointsWorld), numVerts, matrix, reinterpret_cast<RwV3d*>(points));

    // NB: `m_SizeOfShadowFacesData` is the number of vertices in the buffer, `m_FaceID` is it's capacity
    const auto GetNumVerts = [object] { return (int32)object->m_SizeOfShadowFacesData; };
    const auto GetCapacity = [object] { return (int32)object->m_FaceID; };

    // Caps
    uint16 numEdges = 0;
    for (auto i = 0; i < numTris; i++) {
        const auto& tri = colData->m_pShadowTriangles[i];
        const auto  a = tri.vA, b = tri.vB, c = tri.vC;

        const CVector ab = points[b] - points[a];
        const CVector ac = points[c] - points[a];

        const CVector wa = pointsWorld[a];
        const CVector wb = pointsWorld[b];
        const CVector wc = pointsWorld[c];

        // dot(sunDir, cross(ab, ac))
        const auto dot = sunDir.z * (ac.y * ab.x - ac.x * ab.y)
                       + (ac.x * ab.z - ac.z * ab.x) * sunDir.y
                       + (ac.z * ab.y - ac.y * ab.z) * sunDir.x;

        CVector out[3];
        if (dot < 0.0f) {
            // Facing away from the sun: Becomes the far cap
            const CVector extrusion{ sunPos.x * shadowLength, sunPos.y * shadowLength, sunPos.z * shadowLength };
            out[0] = CVector{ extrusion.x + wa.x, wa.y + extrusion.y, wa.z + extrusion.z };
            out[1] = CVector{ wc.x + extrusion.x, wc.y + extrusion.y, wc.z + extrusion.z };
            out[2] = CVector{ wb.x + extrusion.x, wb.y + extrusion.y, extrusion.z + wb.z };
            if (GetNumVerts() + 3 >= GetCapacity()) {
                break;
            }
        } else {
            // Facing the sun: Becomes the near cap, and it's edges are (possibly) part of the silhouette
            CallAddShadowSilhouetteEdge(a, b, edges, &numEdges);
            CallAddShadowSilhouetteEdge(b, c, edges, &numEdges);
            CallAddShadowSilhouetteEdge(c, a, edges, &numEdges);
            if (GetNumVerts() + 3 >= GetCapacity()) {
                break;
            }
            out[0] = wa;
            out[1] = wc;
            out[2] = wb;
        }
        auto* const dst = &object->m_ShadowFacesData[object->m_SizeOfShadowFacesData];
        dst[0] = out[0];
        dst[1] = out[1];
        dst[2] = out[2];
        object->m_SizeOfShadowFacesData += 3;
    }

    // Sides: Extrude the silhouette edges into quads (2 triangles each)
    if (numEdges > 0) {
        const CVector extrusion{ sunPos.x * shadowLength, sunPos.y * shadowLength, sunPos.z * shadowLength };
        for (auto i = 0; i < (int32)numEdges; i++) {
            const CVector p0 = pointsWorld[edges[i * 2 + 0]];
            const CVector p1 = pointsWorld[edges[i * 2 + 1]];
            const CVector e0{ extrusion.x + p0.x, p0.y + extrusion.y, extrusion.z + p0.z };
            const CVector e1{ p1.x + extrusion.x, p1.y + extrusion.y, p1.z + extrusion.z };
            if (GetNumVerts() + 6 >= GetCapacity()) {
                break;
            }
            auto* const dst = &object->m_ShadowFacesData[object->m_SizeOfShadowFacesData];
            dst[0] = p0;
            dst[1] = p1;
            dst[2] = e0;
            dst[3] = p1;
            dst[4] = e1;
            dst[5] = e0;
            object->m_SizeOfShadowFacesData += 6;
        }
    }
}

// 0x70FAE0
void CStencilShadows::RenderForVehicle(CStencilShadowObject* object) {
    auto* const entity = object->m_pOwner;

    // Helis and planes have longer shadows (As they're usually high up in the air)
    auto shadowLength = 5.0f;
    if (entity->GetType() == ENTITY_TYPE_VEHICLE) {
        const auto subType = static_cast<const CVehicle*>(entity)->m_nVehicleSubType; // 0x594
        if (subType == VEHICLE_TYPE_PLANE || subType == VEHICLE_TYPE_HELI) {
            shadowLength = 40.0f;
        }
    }

    auto* const colData = entity->GetColModel()->m_pColData;
    if (!colData || (int32)colData->m_nNumShadowTriangles != (int32)object->m_NumShadowFaces) {
        return;
    }

    if (!entity->m_matrix) {
        entity->AllocateMatrix();
        entity->m_placement.UpdateMatrix(entity->m_matrix);
    }
    CMatrix* matrix = entity->m_matrix;

    // Bikes use their lean matrix
    if (entity->GetType() == ENTITY_TYPE_VEHICLE && static_cast<const CVehicle*>(entity)->m_nVehicleType == VEHICLE_TYPE_BIKE) { // 0x590
        auto* const bike = static_cast<CBike*>(entity);
        bike->CalculateLeanMatrix();
        matrix = &bike->m_mLeanMatrix;
    }

    BuildStencilShadowVolume(object, colData, *matrix, shadowLength);
}

// 0x710310
void CStencilShadows::RenderForObject(CStencilShadowObject* object) {
    auto* const entity = object->m_pOwner;

    auto* const colData = entity->GetColModel()->m_pColData;
    if (!colData || (int32)colData->m_nNumShadowTriangles != (int32)object->m_NumShadowFaces) {
        return;
    }

    if (!entity->m_matrix) {
        entity->AllocateMatrix();
        entity->m_placement.UpdateMatrix(entity->m_matrix);
    }

    BuildStencilShadowVolume(object, colData, *entity->m_matrix, 60.0f);
}

// 0x711D90
void CStencilShadows::Process(CVector& cameraPos) {
    ZoneScoped;

    if (!GraphicsHighQuality()) {
        return;
    }

    static uint8 s_RegisterShadowCounter{}, s_RenderForObjCounter{};

    RegisterStencilShadows(cameraPos, ++s_RegisterShadowCounter % 8);

    // why do we even do this?
    s_ShadowTrianglePointsUnk         = (RxVertexIndex*)CMemoryMgr::Malloc(12'288 * sizeof(RxVertexIndex));
    s_ShadowTrianglePoints            = (CVector*)CMemoryMgr::Malloc(2'048 * sizeof(CVector));
    s_TransformedShadowTrianglePoints = (CVector*)CMemoryMgr::Malloc(2'048 * sizeof(CVector));

    auto i{ 0 };
    for (auto* obj = pFirstActiveStencilShadowObject; obj; obj = obj->m_pNext) {
        switch (obj->m_Type) {
        case eStencilShadowObjType::OBJECT:
            if ((i++ % 4) == s_RenderForObjCounter) {
                RenderForObject(obj);
            }
            break;
        case eStencilShadowObjType::VEHICLE:
            RenderForVehicle(obj);
            break;
        default:
            break;
        }
    }
    s_RenderForObjCounter = (s_RenderForObjCounter + 1) % 4;

    CMemoryMgr::Free(std::exchange(s_ShadowTrianglePointsUnk, nullptr));
    CMemoryMgr::Free(std::exchange(s_ShadowTrianglePoints, nullptr));
    CMemoryMgr::Free(std::exchange(s_TransformedShadowTrianglePoints, nullptr));
}

// 0x70F9B0
bool CStencilShadows::GraphicsHighQuality() {
    return ::GraphicsHighQuality();
}

// 0x710BC0
void CStencilShadows::UpdateHierarchy(CStencilShadowObject*& firstAvailable, CStencilShadowObject*& firstActive, CStencilShadowObject* newOne) {
    if (auto* prev = newOne->m_pPrev) {
        auto* next = newOne->m_pNext;
        if (next) {
            next->m_pPrev = prev;
            newOne->m_pPrev->m_pNext = newOne->m_pNext;
        } else {
            prev->m_pNext = nullptr;
        }
    } else {
        auto* next     = newOne->m_pNext;
        firstAvailable = next;
        if (next) {
            next->m_pPrev = nullptr;
        }
    }
    newOne->m_pNext = firstActive;
    newOne->m_pPrev = nullptr;
    firstActive     = newOne;

    if (newOne->m_pNext) {
        newOne->m_pNext->m_pPrev = newOne;
    }
}

// 0x70FA70-related helpers for RegisterStencilShadows below (originals at 0x710BA0/0x711160/0x7111F0/0x711280).
// Kept file-local: the originals are unreversed free functions with custom conventions/globals we can't name yet.
namespace {
// 0x710BA0 - find active shadow by owner.
CStencilShadowObject* FindActiveStencilShadow(const CEntity* owner) {
    for (auto* obj = CStencilShadows::pFirstActiveStencilShadowObject; obj; obj = obj->m_pNext) {
        if (obj->m_pOwner == owner) {
            return obj;
        }
    }
    return nullptr;
}

// 0x711160 / 0x7111F0 - (dist(camera, entity bound sphere center) - radius)^2 * k, k = -1 if inside the sphere, +1 otherwise.
// The two originals are instruction-for-instruction identical (entity in EBX, point in ESI, result in ST0).
float CalcStencilShadowDistSq(const CEntity* entity, const CVector& cameraPos) {
    const auto* colModel = entity->GetColModel();
    const auto center = entity->TransformFromObjectSpace(colModel->m_boundSphere.m_vecCenter);
    const auto radius = colModel->m_boundSphere.m_fRadius;
    const auto dist = (cameraPos - center).Magnitude();
    const auto k = dist < radius ? -1.0f /* 0x858C1C */ : 1.0f; /* 0x858624 */
    return k * (dist - radius) * (dist - radius);
}

// 0x711280 - set up the (first available) shadow object `obj` for entity (type: 1 = OBJECT, 2 = VEHICLE). Returns false when invalid.
bool CreateStencilShadowSlot(CStencilShadowObject* obj, CEntity* entity, eStencilShadowObjType type) {
    if (type != eStencilShadowObjType::OBJECT && type != eStencilShadowObjType::VEHICLE) {
        return false;
    }
    const auto* colData = entity->GetColModel()->m_pColData;
    if (!colData || !colData->bHasShadowInfo || (int32)colData->m_nNumShadowTriangles <= 0) {
        return false;
    }
    const auto numTris = (int32)colData->m_nNumShadowTriangles;
    obj->m_NumShadowFaces = static_cast<int16>(numTris);
    // m_FaceID is the shadow volume buffer capacity (num CVector slots, 12 bytes each).
    obj->m_FaceID = numTris * 15;
    obj->m_pOwner = entity;
    obj->m_Type = type;
    obj->m_SizeOfShadowFacesData = 0;
    obj->m_ShadowFacesData = static_cast<CVector*>(CMemoryMgr::Malloc(obj->m_FaceID * sizeof(CVector)));
    obj->m_pOwner->RegisterReference(&obj->m_pOwner);
    CStencilShadows::UpdateHierarchy(
        CStencilShadows::pFirstAvailableStencilShadowObject,
        CStencilShadows::pFirstActiveStencilShadowObject,
        obj
    );
    return true;
}

// Objects that are invisible, exploded or broken don't cast a shadow
bool IsObjectWithoutShadow(const CEntity* entity) {
    if (entity->GetType() != ENTITY_TYPE_OBJECT) {
        return false;
    }
    const auto* const obj = static_cast<const CObject*>(entity);
    return !obj->m_bIsVisible || obj->objectFlags.bIsExploded || obj->objectFlags.bIsBroken;
}

// Sector list entity processing (The original has a copy of this for both the sector's and the repeat sector's list).
// Returns `false` if there are no more shadow objects available (The original returns at that point)
bool RegisterShadowsFromList(CEntity* entity, const CVector& cameraPos) {
    if (entity->m_bIsProcObject) { // 0x4000000
        return true;
    }
    if (IsObjectWithoutShadow(entity)) {
        return true;
    }
    const auto* colModel = entity->GetColModel();
    if (!colModel || !colModel->m_pColData || !colModel->m_pColData->bHasShadowInfo) {
        return true;
    }
    if (entity->IsScanCodeCurrent()) {
        return true;
    }
    entity->SetCurrentScanCode();
    if (entity->GetAreaCode() != CGame::currArea && entity->GetAreaCode() != AREA_CODE_13) {
        return true;
    }
    if (CModelInfo::GetModelInfo(entity->m_nModelIndex)->GetModelType() != MODEL_INFO_ATOMIC) {
        return true;
    }
    if (FindActiveStencilShadow(entity)) {
        return true;
    }
    const auto distSq = CalcStencilShadowDistSq(entity, cameraPos);
    if (!(distSq <= 2500.0f /* 0x8598B0 */)) {
        return true;
    }
    auto* const available = CStencilShadows::pFirstAvailableStencilShadowObject;
    if (!available) {
        return false;
    }
    CreateStencilShadowSlot(available, entity, eStencilShadowObjType::OBJECT);
    return CStencilShadows::pFirstAvailableStencilShadowObject != nullptr;
}
} // namespace

// 0x711760
void CStencilShadows::RegisterStencilShadows(CVector& cameraPos, bool doNotCreateNew) {
    if (doNotCreateNew) {
        // 0x711D40 - Only remove the shadows of dead entities
        for (auto* obj = pFirstActiveStencilShadowObject; obj;) {
            auto* const next = obj->m_pNext;
            if (!obj->m_pOwner || IsObjectWithoutShadow(obj->m_pOwner)) {
                obj->Destroy();
            }
            obj = next;
        }
        return;
    }

    // 0x711779 - Remove the shadows of dead/out of range entities
    for (auto* obj = pFirstActiveStencilShadowObject; obj;) {
        auto* const next  = obj->m_pNext;
        auto* const owner = obj->m_pOwner;
        if (!owner || IsObjectWithoutShadow(owner)) {
            obj->Destroy();
        } else if (obj->m_Type == eStencilShadowObjType::OBJECT) {
            const auto distSq = CalcStencilShadowDistSq(owner, cameraPos);
            if (!(distSq <= 2500.0f /* 0x8598B0 */)) {
                obj->Destroy();
            } else if (owner->GetAreaCode() != CGame::currArea && owner->GetAreaCode() != AREA_CODE_13) {
                obj->Destroy();
            }
        } else if (obj->m_Type == eStencilShadowObjType::VEHICLE) {
            const auto distSq = CalcStencilShadowDistSq(owner, cameraPos);
            if (!(distSq <= 2500.0f /* 0x8598B0 */)) {
                obj->Destroy();
            }
        }
        obj = next;
    }

    if (!pFirstAvailableStencilShadowObject) {
        return;
    }

    // 0x711872 - Vehicles
    for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
        const auto* colModel = vehicle.GetColModel();
        if (!colModel || !colModel->m_pColData || !colModel->m_pColData->bHasShadowInfo) {
            continue;
        }
        if (vehicle.m_pCollisionList.IsEmpty()) { // CPhysical + 0xB0: Not in the world
            continue;
        }
        if (FindActiveStencilShadow(&vehicle)) {
            continue;
        }
        const auto distSq = CalcStencilShadowDistSq(&vehicle, cameraPos);
        if (!(distSq <= 2500.0f /* 0x8598B0 */)) {
            continue;
        }
        auto* const available = pFirstAvailableStencilShadowObject;
        if (available) {
            CreateStencilShadowSlot(available, &vehicle, eStencilShadowObjType::VEHICLE);
        }
        if (!available || !pFirstAvailableStencilShadowObject) {
            return;
        }
    }

    if (!pFirstAvailableStencilShadowObject) {
        return;
    }

    // 0x71192B - Sectors within 50 units (50.0 = 0x858B40, 0.02 = 0x858B38, 60.0 = 0x858B34)
    // NB: Only the lower bound of the mins and the upper bound of the maxes are clamped here
    const auto ToSector = [](float v) { return (int32)std::floor(v * 0.02f + 60.0f); };
    const auto minX     = std::max(ToSector(cameraPos.x - 50.0f), 0);
    const auto minY     = std::max(ToSector(cameraPos.y - 50.0f), 0);
    const auto maxX     = std::min(ToSector(cameraPos.x + 50.0f), 119);
    const auto maxY     = std::min(ToSector(cameraPos.y + 50.0f), 119);

    CWorld::AdvanceCurrentScanCode();

    for (auto y = minY; y <= maxY; y++) {
        for (auto x = minX; x <= maxX; x++) {
            // NB: The list iterators fetch the next node before the entity is processed (Just like the original code)
            for (auto* const entity : CWorld::GetSector(x, y).Buildings) { // `GetSector` clamps the coords just like the original code
                if (!RegisterShadowsFromList(entity, cameraPos)) {
                    return;
                }
            }
            for (auto* const entity : CWorld::GetRepeatSector(x, y).Objects) {
                if (!RegisterShadowsFromList(entity, cameraPos)) {
                    return;
                }
            }
        }
    }
}
