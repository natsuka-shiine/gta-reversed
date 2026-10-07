#include "StdInc.h"

#include "RwHelper.h"
#include "Plugins/TwoDEffectPlugin/2dEffect.h"
#include "Plugins/TwoDEffectPlugin/2dEffectStream.h"

void RwHelperInjectHooks() {
    RH_ScopedNamespaceName("RwHelper");
    RH_ScopedCategoryGlobal();

    RH_ScopedGlobalInstall(GetEventGlobalGroup, 0x4ABA50);
    RH_ScopedGlobalInstall(GetNameAndDamage, 0x5370A0);
    RH_ScopedGlobalInstall(GetFirstAtomicCallback, 0x734810);
    RH_ScopedGlobalInstall(GetFirstAtomic, 0x734820);
    RH_ScopedGlobalInstall(Get2DEffectAtomicCallback, 0x734850);
    RH_ScopedGlobalInstall(Get2DEffectAtomic, 0x734880);
    RH_ScopedGlobalInstall(GetFirstObjectCallback, 0x7348B0);
    RH_ScopedGlobalInstall(GetFirstObject, 0x7348C0);
    RH_ScopedGlobalInstall(GetFirstFrameCallback, 0x7348F0);
    RH_ScopedGlobalInstall(GetFirstChild, 0x734900);
    RH_ScopedGlobalInstall(SkinAtomicGetHAnimHierarchCB, 0x734A20);
    RH_ScopedGlobalInstall(GetAnimHierarchyFromSkinClump, 0x734A40);
    RH_ScopedGlobalInstall(GetAnimHierarchyFromFrame, 0x734AB0);
    RH_ScopedGlobalInstall(GetAnimHierarchyFromClump, 0x734B10);
    RH_ScopedGlobalInstall(AtomicRemoveAnimFromSkinCB, 0x734B90);
    RH_ScopedGlobalInstall(RpAtomicConvertGeometryToTL, 0x734BE0);
    RH_ScopedGlobalInstall(RpAtomicConvertGeometryToTS, 0x734C20);
    RH_ScopedGlobalInstall(atomicConvertGeometryToTL, 0x734C60);
    RH_ScopedGlobalInstall(RpClumpConvertGeometryToTL, 0x734CB0);
    RH_ScopedGlobalInstall(atomicConvertGeometryToTS, 0x734CE0);
    RH_ScopedGlobalInstall(RpClumpConvertGeometryToTS, 0x734D30);
    RH_ScopedGlobalInstall(forceLinearFilteringAtomicsCB, 0x734DA0);
    RH_ScopedGlobalInstall(SetFilterModeOnClumpsTextures, 0x734DC0);
    RH_ScopedGlobalInstall(forceLinearFilteringMatTexturesCB, 0x734D60);
    RH_ScopedGlobalInstall(SetFilterModeOnAtomicsTextures, 0x734D80);
    RH_ScopedGlobalInstall(RpGeometryReplaceOldMaterialWithNewMaterial, 0x734DE0);
    RH_ScopedGlobalInstall(RwTexDictionaryFindHashNamedTexture, 0x734E50);
    RH_ScopedGlobalInstall(RpClumpGetBoundingSphere, 0x734FC0);
    RH_ScopedGlobalInstall(SkinGetBonePositions, 0x735140);
    RH_ScopedGlobalInstall(SkinSetBonePositions, 0x7352D0);
    RH_ScopedGlobalInstall(SkinGetBonePositionsToTable, 0x735360);
    RH_ScopedGlobalInstall(RemoveRefsCB, 0x7226D0);
}

// 0x4ABA50
CEventGlobalGroup* GetEventGlobalGroup() {
    static auto& globalEvents = StaticRef<CEventGlobalGroup*, 0xA9AF6C>();

    if (globalEvents)
        return globalEvents;

    globalEvents = new CEventGlobalGroup(nullptr);
    return globalEvents;
}

// TODO: Check `outName` size (to avoid buffer overflow)
// 0x5370A0
void GetNameAndDamage(const char* name, char* objName, bool& bIsDamageModel) {
    const size_t nodesz = strlen(name);

    const auto TerminatedCopy = [=](size_t offset) {
        strncpy_s(objName, nodesz - offset + 1, name, nodesz - offset);
        objName[nodesz - offset] = 0;
    };

    // EndsWith "_dam"
    if (name[nodesz - 4] == '_' && name[nodesz - 3] == 'd' && name[nodesz - 2] == 'a' && name[nodesz - 1] == 'm'
    ) {
        bIsDamageModel = true;
        TerminatedCopy(sizeof("_dam") - 1);
    }
    else {
        bIsDamageModel = false;
        // EndsWith "_l0" or "_L0"
        if (name[nodesz - 3] == '_' &&
            (name[nodesz - 2] == 'L' || name[nodesz - 2] == 'l') && name[nodesz - 1] == '0'
        ) {
            TerminatedCopy(sizeof("_l0") - 1);
        } else
            strcpy_s(objName, strlen(name) + 1, name);
    }
}

// 0x734810
RpAtomic* GetFirstAtomicCallback(RpAtomic* atomic, void* data) {
    *(RpAtomic**)(data) = atomic;
    return nullptr;
}

// 0x734820
RpAtomic* GetFirstAtomic(RpClump* clump) {
    RpAtomic* atomic{};
    RpClumpForAllAtomics(clump, GetFirstAtomicCallback, &atomic);
    return atomic;
}

// 0x734850
RpAtomic* Get2DEffectAtomicCallback(RpAtomic* atomic, void* data) {
    const auto* const effects = RWPLUGINOFFSET(t2dEffectPlugin, RpAtomicGetGeometry(atomic), C2dEffect::g2dEffectPluginOffset)->m_pEffectEntries;
    if (!effects || static_cast<int32>(effects->m_nObjCount) <= 0) {
        return atomic;
    }
    *static_cast<RpAtomic**>(data) = atomic;
    return nullptr;
}

// 0x734880
RpAtomic* Get2DEffectAtomic(RpClump* clump) {
    RpAtomic* atomic{};
    RpClumpForAllAtomics(clump, Get2DEffectAtomicCallback, &atomic);
    return atomic;
}

// 0x7348B0
RwObject* GetFirstObjectCallback(RwObject* object, void* data) {
    *(RwObject**)(data) = object;
    return nullptr;
}

// 0x7348C0
RwObject* GetFirstObject(RwFrame* frame) {
    RwObject* obj{};
    RwFrameForAllObjects(frame, GetFirstObjectCallback, &obj);
    return obj;
}

// 0x7348F0
RwFrame* GetFirstFrameCallback(RwFrame* frame, void* data) {
    *(RwFrame**)(data) = frame;
    return nullptr;
}

// 0x734900
RwFrame* GetFirstChild(RwFrame* frame) {
    RwFrame* child{};
    RwFrameForAllChildren(frame, GetFirstFrameCallback, &child);
    return child;
}

// 0x734A70 - Finds the first frame (depth-first) that has a hierarchy
static RwFrame* GetAnimHierarchyFromFrameCB(RwFrame* frame, void* data) {
    if (auto* const hier = RpHAnimFrameGetHierarchy(frame)) {
        *static_cast<RpHAnimHierarchy**>(data) = hier;
        return nullptr;
    }
    RwFrameForAllChildren(frame, GetAnimHierarchyFromFrameCB, data);
    return frame;
}

// 0x734AB0
RpHAnimHierarchy* GetAnimHierarchyFromFrame(RwFrame* frame) {
    assert(frame);
    if (auto* const hier = RpHAnimFrameGetHierarchy(frame)) {
        return hier;
    }
    RpHAnimHierarchy* hier{};
    RwFrameForAllChildren(frame, GetAnimHierarchyFromFrameCB, &hier);
    return hier;
}

// 0x734B10
RpHAnimHierarchy* GetAnimHierarchyFromClump(RpClump* clump) {
    assert(clump);
    return GetAnimHierarchyFromFrame(RpClumpGetFrame(clump));
}

// 0x734A40
RpHAnimHierarchy* GetAnimHierarchyFromSkinClump(RpClump* clump) {
    RpHAnimHierarchy* anim{};
    RpClumpForAllAtomics(clump, SkinAtomicGetHAnimHierarchCB, &anim);
    return anim;
}

// name not from Android
// 0x734A20
RpAtomic* SkinAtomicGetHAnimHierarchCB(RpAtomic* atomic, void* data) {
    *(RpHAnimHierarchy**)(data) = RpSkinAtomicGetHAnimHierarchy(atomic);
    return nullptr;
}

// 0x734B90
RpAtomic* AtomicRemoveAnimFromSkinCB(RpAtomic* atomic, void* data) {
    if (RpSkinGeometryGetSkin(RpAtomicGetGeometry(atomic))) {
        if (RpHAnimHierarchy* hier = RpSkinAtomicGetHAnimHierarchy(atomic)) {
            RtAnimAnimation*& currAnim = RtAnimInterpolatorGetCurrentAnim(RpHAnimHierarchyGetInterpolator(hier));
            if (currAnim) {
                RtAnimAnimationDestroy(currAnim);
            }
            currAnim = nullptr;
        }
    }
    return atomic;
}

// 0x734BE0
bool RpAtomicConvertGeometryToTL(RpAtomic* atomic) {
    RpGeometry* geometry = RpAtomicGetGeometry(atomic);

    auto flags = RpGeometryGetFlags(geometry);
    if (flags & rpGEOMETRYNATIVE || !(flags & rpGEOMETRYTRISTRIP))
        return false;

    RpGeometryLock(geometry, rpGEOMETRYLOCKALL);
    RpGeometrySetFlags(geometry, flags & ~rpGEOMETRYTRISTRIP);
    RpGeometryUnlock(geometry);

    return true;
}

// 0x734C20
bool RpAtomicConvertGeometryToTS(RpAtomic* atomic) {
    RpGeometry* geometry = RpAtomicGetGeometry(atomic);

    auto flags = RpGeometryGetFlags(geometry);
    if (flags & rpGEOMETRYNATIVE || flags & rpGEOMETRYTRISTRIP)
        return false;

    RpGeometryLock(geometry, rpGEOMETRYLOCKALL);
    RpGeometrySetFlags(geometry, flags | rpGEOMETRYTRISTRIP);
    RpGeometryUnlock(geometry);

    return true;
}

// 0x734C60
RpAtomic* atomicConvertGeometryToTL(RpAtomic* atomic, void* data) {
    if (!RpAtomicConvertGeometryToTL(atomic)) {
        *(bool*)(data) = false;
    }
    return atomic;
}

// 0x734CB0
bool RpClumpConvertGeometryToTL(RpClump* clump) {
    bool success{ true };
    RpClumpForAllAtomics(clump, atomicConvertGeometryToTL, &success);
    return success;
}

// 0x734CE0
RpAtomic* atomicConvertGeometryToTS(RpAtomic* atomic, void* data) {
    if (!RpAtomicConvertGeometryToTS(atomic)) {
        *(bool*)(data) = false;
    }
    return atomic;
}

// 0x734D30
bool RpClumpConvertGeometryToTS(RpClump* clump) {
    bool success{ true };
    RpClumpForAllAtomics(clump, atomicConvertGeometryToTS, &success);
    return success;
}

// 0x734D60
RpMaterial* forceLinearFilteringMatTexturesCB(RpMaterial* material, void* data) {
    if (RwTexture* texture = RpMaterialGetTexture(material)) {
        RwTextureSetFilterMode(texture, (RwTextureFilterMode)((unsigned)data));
    }
    return material;
}

// 0x734D80
bool SetFilterModeOnAtomicsTextures(RpAtomic* atomic, RwTextureFilterMode filtering) {
    RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), forceLinearFilteringMatTexturesCB, (void*)(unsigned)filtering);
    return true;
}

// 0x734DA0
RpAtomic* forceLinearFilteringAtomicsCB(RpAtomic* atomic, void* data) {
    SetFilterModeOnAtomicsTextures(atomic, (RwTextureFilterMode)((unsigned)data));
    return atomic;
}

// 0x734DC0
bool SetFilterModeOnClumpsTextures(RpClump* clump, RwTextureFilterMode filtering) {
    RpClumpForAllAtomics(clump, forceLinearFilteringAtomicsCB, (void*)(unsigned)filtering);
    return true;
}

// 0x734DE0
bool RpGeometryReplaceOldMaterialWithNewMaterial(RpGeometry* geometry, RpMaterial* oldMaterial, RpMaterial* newMaterial) {
    auto       replaced   = false;
    auto*      meshHeader = RpGeometryGetMeshHeader(geometry);
    auto*      meshes     = reinterpret_cast<RpMesh*>(meshHeader + 1);
    for (auto i = 0u; i < meshHeader->numMeshes; i++) {
        auto& mesh = meshes[i];
        if (mesh.material != oldMaterial) {
            continue;
        }
        const auto idx = _rpMaterialListFindMaterialIndex(&geometry->matList, oldMaterial);
        RpMaterialDestroy(oldMaterial);
        geometry->matList.materials[idx] = newMaterial;
        mesh.material                    = newMaterial;
        newMaterial->refCount++;
        replaced = true;
    }
    return replaced;
}

// 0x734E50
RwTexture* RwTexDictionaryFindHashNamedTexture(RwTexDictionary* txd, uint32 hash) {
    struct Context {
        uint32     hash;
        RwTexture* found;
    } ctx{ hash, nullptr };
    RwTexDictionaryForAllTextures(
        txd,
        [](RwTexture* texture, void* data) -> RwTexture* {
            auto* const ctx = static_cast<Context*>(data);
            if (CKeyGen::GetUppercaseKey(texture->name) == ctx->hash) {
                ctx->found = texture;
                return nullptr; // Stop
            }
            return texture;
        },
        &ctx
    );
    return ctx.found;
}

static auto& s_BoundingSphereUseLTM = StaticRef<bool, 0x8D60BC>();

// Centre of the atomic's bounding sphere, transformed by its frame (Either the LTM or the modelling matrix, see above)
static RwV3d GetAtomicBoundingSphereCentre(RpAtomic* atomic) {
    auto* const frame = RpAtomicGetFrame(atomic);
    RwV3d       centre;
    RwV3dTransformPoints(
        &centre,
        &RpAtomicGetBoundingSphere(atomic)->center,
        1,
        s_BoundingSphereUseLTM ? RwFrameGetLTM(frame) : RwFrameGetMatrix(frame)
    );
    return centre;
}

// 0x734970 - Sums up the centres
static RpAtomic* AtomicAddBoundingSphereCentreCB(RpAtomic* atomic, void* data) {
    auto* const sum    = static_cast<RwV3d*>(data);
    const auto  centre = GetAtomicBoundingSphereCentre(atomic);
    sum->x += centre.x;
    sum->y += centre.y;
    sum->z += centre.z;
    return atomic;
}

// 0x734ED0 - Grows the sphere to contain the atomic's one
static RpAtomic* AtomicGrowBoundingSphereCB(RpAtomic* atomic, void* data) {
    auto* const sphere = static_cast<RwSphere*>(data);
    const auto  centre = GetAtomicBoundingSphereCentre(atomic);
    const auto  dist   = std::sqrt(sq(centre.x - sphere->center.x) + sq(centre.y - sphere->center.y) + sq(centre.z - sphere->center.z))
                       + RpAtomicGetBoundingSphere(atomic)->radius;
    if (dist > sphere->radius) {
        sphere->radius = dist;
    }
    return atomic;
}

// 0x734FC0
RpClump* RpClumpGetBoundingSphere(RpClump* clump, RwSphere* sphere, bool bUseLTM) {
    s_BoundingSphereUseLTM = bUseLTM;

    if (!clump || !sphere) {
        return nullptr;
    }

    *sphere = RwSphere{};

    const auto numAtomics = RpClumpGetNumAtomics(clump);
    if (numAtomics < 1) {
        return nullptr;
    }

    // The centre is the average of the atomics' centres
    RwV3d sum{};
    RpClumpForAllAtomics(clump, AtomicAddBoundingSphereCentreCB, &sum);

    const auto invNum = 1.0f / static_cast<float>(numAtomics);
    RwSphere   result{ .center = { sum.x * invNum, sum.y * invNum, sum.z * invNum }, .radius = 0.0f };
    RpClumpForAllAtomics(clump, AtomicGrowBoundingSphereCB, &result);

    // Bring the centre back into the clump's space
    auto* const frame = RpClumpGetFrame(clump);
    RwMatrix    invMat;
    RwMatrixInvert(&invMat, s_BoundingSphereUseLTM ? RwFrameGetLTM(frame) : RwFrameGetMatrix(frame));
    RwV3dTransformPoints(&result.center, &result.center, 1, &invMat);

    *sphere = result;
    return clump;
}

struct tSkinBonePosition {
    int32 parent; //!< Index of the parent bone
    RwV3d pos;    //!< Position relative to the parent bone
};
VALIDATE_SIZE(tSkinBonePosition, 0x10);

static auto& s_SkinBonePositions          = StaticRef<std::array<tSkinBonePosition, 64>, 0xC88258>();
static auto& s_SkinBonePositionsAreStored = StaticRef<bool, 0xC88658>();

/*!
* @brief Walk the bones of the clump's skin, and call `fn(boneIdx, parentIdx, posRelativeToParent)` for each one (apart from the root)
*/
template<typename Fn>
static void ForEachSkinBonePosition(RpClump* clump, Fn&& fn) {
    auto* const skin = RpSkinGeometryGetSkin(RpAtomicGetGeometry(GetFirstAtomic(clump)));
    auto* const hier = GetAnimHierarchyFromSkinClump(clump);

    const auto numBones = static_cast<int32>(RpSkinGetNumBones(skin));

    int32  stack[32];
    int32* sp     = stack;
    int32  parent = 0;
    for (auto i = 1; i < numBones; i++) {
        // The bone's position is the translation of its inverted skin-to-bone matrix...
        RwMatrix boneToSkin;
        auto     skinToBone = RpSkinGetSkinToBoneMatrices(skin)[i];
        RwMatrixInvert(&boneToSkin, &skinToBone);

        // ...which is then brought into the parent's space
        auto  parentSkinToBone = RpSkinGetSkinToBoneMatrices(skin)[parent];
        RwV3d pos;
        RwV3dTransformPoints(&pos, RwMatrixGetPos(&boneToSkin), 1, &parentSkinToBone);

        fn(i, parent, pos);

        const auto flags = RpHAnimHierarchyGetNodeFlags(hier, i);
        if (flags & rpHANIMPUSHPARENTMATRIX) {
            *++sp = parent;
        }
        if (flags & rpHANIMPOPPARENTMATRIX) {
            parent = *sp--;
        } else {
            parent = i;
        }
    }
}

// 0x735140
void SkinGetBonePositions(RpClump* clump) {
    if (s_SkinBonePositionsAreStored) {
        return;
    }
    s_SkinBonePositionsAreStored = true;

    s_SkinBonePositions[0] = { .parent = -1, .pos = {} };
    ForEachSkinBonePosition(clump, [](int32 bone, int32 parent, const RwV3d& pos) {
        s_SkinBonePositions[bone] = { .parent = parent, .pos = pos };
    });
}

// 0x7352D0
void SkinSetBonePositions(RpClump* clump) {
    auto* const skin     = RpSkinGeometryGetSkin(RpAtomicGetGeometry(GetFirstAtomic(clump)));
    auto* const matrices = RpHAnimHierarchyGetMatrixArray(GetAnimHierarchyFromSkinClump(clump));

    const auto numBones = static_cast<int32>(RpSkinGetNumBones(skin));
    for (auto i = 1; i < numBones; i++) {
        const auto& bone = s_SkinBonePositions[i];
        RwV3dTransformPoints(RwMatrixGetPos(&matrices[i]), &bone.pos, 1, &matrices[bone.parent]);
    }
}

// 0x735360
void SkinGetBonePositionsToTable(RpClump* clump, RwV3d* table) {
    if (!table) {
        return;
    }
    table[0] = {};
    ForEachSkinBonePosition(clump, [table](int32 bone, int32 parent, const RwV3d& pos) {
        table[bone] = pos;
    });
}

// 0x7226D0
RpAtomic* RemoveRefsCB(RpAtomic* atomic, void* data) {
    UNUSED(data);
    auto* modelInfo = CVisibilityPlugins::GetModelInfo(atomic);
    modelInfo->RemoveRef();
    return atomic;
}

// 0x7226F0
void RemoveRefsForAtomic(RpClump* clump) {
    RpClumpForAllAtomics(clump, RemoveRefsCB, nullptr);
}
