/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include "rw/rwcore.h"
#include "rw/rphanim.h"
#include "rw/rpuvanim.h"
#include "rw/rpskin.h"
#include "rw/rpmatfx.h"
#include "rw/skeleton.h"
#include "rw/rwplcore.h"
#include <type_traits>

#ifdef _DX9_SDK_INSTALLED
#include "d3d9.h"
#endif

static inline auto& RwInitialized = StaticRef<bool, 0xC920E8>();
static inline auto& RsGlobal =  StaticRef<RsGlobalType, 0xC17040>();
#ifndef NOTSA_LIBRW
static inline auto& RwEngineInstance =  StaticRef<RwGlobals*, 0xC97B24>();
static inline auto& geometryTKList =  StaticRef<RwPluginRegistry, 0x8D628C>();
static inline auto& RpUVAnimDictSchema =  StaticRef<RtDictSchema, 0x8DED50>();

// Accessors for fields that are named differently in librw (the adapter has its own versions of these)
#define RpHAnimHierarchyGetNodeFlags(_hier, _index) ((_hier)->pNodeInfo[_index].flags)
#define RpHAnimHierarchyGetNodeID(_hier, _index)    ((_hier)->pNodeInfo[_index].nodeID)
#define RpHAnimHierarchyGetInterpolator(_hier)      ((_hier)->currentAnim)
#define RpGeometryGetMeshHeader(_geometry)          ((_geometry)->mesh)
#define RxPipelineSetPluginId(_pipe, _id)           ((_pipe)->pluginId = (_id))
#define RxPipelineGetPluginId(_pipe)                ((_pipe)->pluginId)
#endif
static inline auto& AmbientSaturated = StaticRef<RwRGBAReal, 0x8E2418>();

#ifdef NOTSA_LIBRW
// (The ones below are globals of the original's RenderWare, which isn't running)
inline IDirect3DDevice9 *GetD3D9Device() {
    return static_cast<IDirect3DDevice9 *>(RwD3D9GetCurrentD3DDevice());
}

//! Of the camera being rendered with
inline _D3DMATRIX *GetD3DViewTransform() {
    return reinterpret_cast<_D3DMATRIX *>(&RwCameraGetCurrentCamera()->devView);
}

inline _D3DMATRIX *GetD3DProjTransform() {
    return reinterpret_cast<_D3DMATRIX *>(&RwCameraGetCurrentCamera()->devProj);
}
#else
inline IDirect3DDevice9 *GetD3D9Device() {
    return *reinterpret_cast<IDirect3DDevice9 **>(0xC97C28);
}

#ifndef D3DMATRIX_DEFINED
struct _D3DMATRIX;
#endif
inline _D3DMATRIX *GetD3DViewTransform() {
    return reinterpret_cast<_D3DMATRIX *>(0xC9BC80);
}

inline _D3DMATRIX *GetD3DProjTransform() {
    return reinterpret_cast<_D3DMATRIX *>(0x8E2458);
}
#endif

inline void _rpMaterialSetDefaultSurfaceProperties(RwSurfaceProperties *surfProps) {
    ((void(__cdecl *)(RwSurfaceProperties*))0x74D870)(surfProps);
}

#define RWRSTATE(a) (reinterpret_cast<void *>(a))
#define PSGLOBAL(var) (((psGlobalType *)(RsGlobal.ps))->var)

/*!
* @brief Generic PipeID construction.
*/
#define rpPDS_MAKEPIPEID(vendorID, pipeID) ((((vendorID) & 0xFFFF) << 16) | ((pipeID) & 0xFFFF))

struct RwResEntrySA : RwResEntry {
    RxD3D9ResEntryHeader header;
    RxD3D9InstanceData meshData;
};


void RwCoreInjectHooks();

template<typename T>
inline T RwStreamRead(RwStream* stream, size_t size = sizeof(T)) {
    T data;
    RwStreamRead(stream, &data, size);
    return data;
}

namespace RtAnim {
    void InjectHooks();
}
