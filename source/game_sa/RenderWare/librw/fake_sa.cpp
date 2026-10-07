/*
* RenderWare API on top of librw (https://github.com/aap/librw).
* The part of the RenderWare 3.6 API San Andreas uses that the `fakerw` layer of re3/reVC (`fake.cpp`) doesn't have.
* Only used when building with `NOTSA_LIBRW`.
*
* Functions that aren't (fully) implemented yet report themselves once with `LIBRW_TODO`, search for that.
*/
#ifdef NOTSA_LIBRW

#include "StdInc.h"

#include <unordered_map>

#include "rwcore.h"
#include "rpworld.h"
#include "rpmatfx.h"
#include "rphanim.h"
#include "rpskin.h"
#include "rpuvanim.h"
#include "rtanim.h"
#include "rtquat.h"
#include "rtslerp.h"
#include "rtdict.h"
#include <rw/rwtexdict.h>

#include <imgui.h>
#include <bindings/imgui_impl_dx9.h>

namespace rw {
namespace d3d {
extern IDirect3DDevice9* d3ddevice; // (Only declared by librw if `d3d9.h` is included before it)
void setD3dMaterial(D3DMATERIAL9* mat9);

// Local additions to librw (see `thirdparty/librw/CMakeLists.txt`): called before/after it resets the device, and if that failed
extern void (*deviceReleaseCB)(void);
extern void (*deviceRestoreCB)(void);
extern void (*deviceResetFailedCB)(long);
} // namespace d3d
} // namespace rw

#define LIBRW_TODO(what)                                                                  \
    do {                                                                                  \
        static bool s_Reported = false;                                                   \
        if (!s_Reported) {                                                                \
            s_Reported = true;                                                            \
            NOTSA_LOG_ERR("librw adapter: {} is not implemented ({})", __FUNCTION__, what); \
        }                                                                                 \
    } while (false)

/*
 ***********************************************
 *
 * Vectors and matrices
 *
 ***********************************************
 */

RwReal RwV2dLength(const RwV2d* in) { return rw::length(*in); }
void   RwV2dSub(RwV2d* out, const RwV2d* ina, const RwV2d* inb) { out->x = ina->x - inb->x; out->y = ina->y - inb->y; }
void   RwV3dAssign(RwV3d* out, const RwV3d* ina) { *out = *ina; }
void   RwV3dCrossProduct(RwV3d* out, const RwV3d* ina, const RwV3d* inb) { *out = rw::cross(*ina, *inb); }

RwReal RwV3dNormalize(RwV3d* out, const RwV3d* in) {
    const RwReal len = rw::length(*in);
    if (len > 0.0f) {
        *out = rw::scale(*in, 1.0f / len);
    } else {
        out->x = out->y = out->z = 0.0f;
    }
    return len;
}

RwV3d* RwV3dTransformVectors(RwV3d* vectorsOut, const RwV3d* vectorsIn, RwInt32 numPoints, const RwMatrix* matrix) {
    rw::V3d::transformVectors(vectorsOut, vectorsIn, numPoints, matrix);
    return vectorsOut;
}

RwMatrix* RwMatrixMultiply(RwMatrix* matrixOut, const RwMatrix* matrixIn1, const RwMatrix* matrixIn2) {
    return rw::Matrix::mult(matrixOut, matrixIn1, matrixIn2);
}

/*
 ***********************************************
 *
 * Frames, rasters, images, textures
 *
 ***********************************************
 */

RwInt32  RwFrameCount(RwFrame* frame) { return frame->count(); }
RwFrame* _rwFrameCloneAndLinkClones(RwFrame* root) { return root->cloneAndLink(); }

RwInt32   RwRasterGetFormat(const RwRaster* raster) { return raster->format; }
RwInt32   RwRasterGetStride(const RwRaster* raster) { return raster->stride; }
RwRaster* RwRasterUnlockPalette(RwRaster* raster) { raster->unlockPalette(); return raster; }

RwUInt32 RwRGBAToPixel(RwRGBA* rgbIn, RwInt32 rasterFormat) {
    const RwUInt32 r = rgbIn->red, g = rgbIn->green, b = rgbIn->blue, a = rgbIn->alpha;
    switch (rasterFormat & rwRASTERFORMATPIXELFORMATMASK) {
    case rwRASTERFORMAT1555: return ((a >> 7) << 15) | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
    case rwRASTERFORMAT555:  return ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
    case rwRASTERFORMAT565:  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    case rwRASTERFORMAT4444: return ((a >> 4) << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4);
    case rwRASTERFORMATLUM8: return (r * 77 + g * 150 + b * 29) >> 8;
    case rwRASTERFORMAT888:  return 0xFF000000 | (r << 16) | (g << 8) | b;
    default:                 return (a << 24) | (r << 16) | (g << 8) | b; // 8888, and the frame buffer
    }
}

RwImage* RwImageSetFromRaster(RwImage* image, RwRaster* raster) {
    rw::Image* const src = raster->toImage();
    if (!src) {
        return nullptr;
    }
    if (src->depth != image->depth && image->depth == 32) {
        src->convertTo32();
    }
    if (src->depth != image->depth || !src->pixels || !image->pixels) {
        LIBRW_TODO("images of different depths");
        src->destroy();
        return nullptr;
    }
    const auto rows     = std::min(src->height, image->height);
    const auto rowBytes = (size_t)std::min(src->width, image->width) * (image->depth / 8);
    for (auto y = 0; y < rows; y++) {
        memcpy(image->pixels + y * image->stride, src->pixels + y * src->stride, rowBytes);
    }
    src->destroy();
    return image;
}

RwBool                RwTextureSetReadCallBack(RwTextureCallBackRead fpCallBack) { rw::Texture::readCB = fpCallBack; return TRUE; }
RwTextureCallBackFind RwTextureGetFindCallBack(void) { return rw::Texture::findCB; }
RwBool                RwTextureSetFindCallBack(RwTextureCallBackFind callBack) { rw::Texture::findCB = callBack; return TRUE; }

RwTexture* RwTexDictionaryRemoveTexture(RwTexture* texture) {
    if (texture->dict) {
        texture->dict->remove(texture);
    }
    return texture;
}

RwInt32 RwTexDictionaryRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return rw::TexDictionary::registerPlugin(size, pluginID, constructCB, destructCB, (rw::CopyConstructor)copyCB);
}

// 0x734940
RwTexture* GetFirstTexture(RwTexDictionary* txd) {
    rw::LLLink* const first = txd->textures.link.next;
    return first != &txd->textures.link
        ? rw::Texture::fromDict(first)
        : nullptr;
}

/*
 ***********************************************
 *
 * Streams
 *
 ***********************************************
 */

RwBool RwStreamReadChunkHeaderInfo(RwStream* stream, RwChunkHeaderInfo* chunkHeaderInfo) {
    rw::ChunkHeaderInfo header;
    if (!rw::readChunkHeaderInfo(stream, &header)) {
        return FALSE;
    }
    chunkHeaderInfo->type      = header.type;
    chunkHeaderInfo->length    = header.length;
    chunkHeaderInfo->version   = header.version;
    chunkHeaderInfo->buildNum  = header.build;
    chunkHeaderInfo->isComplex = FALSE;
    return TRUE;
}

// Streams that live in storage of the caller (`_rwStreamInitialize`), `RwStreamClose` must not free these
static std::vector<RwStream*> s_CallerOwnedStreams;

bool RwLibrwIsCallerOwnedStream(RwStream* stream) {
    return std::find(s_CallerOwnedStreams.begin(), s_CallerOwnedStreams.end(), stream) != s_CallerOwnedStreams.end();
}

/*
* Initializes a stream in the caller's storage. The game does this with one global stream object for everything it streams in,
* and is relaxed about it: closes it more than once, or not at all before initializing it again. That's fine with RenderWare's.
* TODO(librw): Only memory streams (All the game uses), the storage has to be big enough for a `rw::StreamMemory` (It is for an `RwStream`)
*/
RwStream* _rwStreamInitialize(RwStream* stream, RwBool rwOwned, RwStreamType type, RwStreamAccessType accessType, const void* pData) {
    if (rwOwned || type != rwSTREAMMEMORY || accessType != rwSTREAMREAD) {
        LIBRW_TODO("only reading from memory, in the caller's storage");
        return RwStreamOpen(type, accessType, pData);
    }
    const auto* const memory = static_cast<const RwMemory*>(pData);
    auto* const       mem    = new (static_cast<void*>(stream)) rw::StreamMemory;
    mem->open(memory->start, memory->length);
    if (!RwLibrwIsCallerOwnedStream(mem)) {
        s_CallerOwnedStreams.push_back(mem);
    }
    return mem;
}

// The game's own readers for texture dictionaries and clumps, which read a file in two halves as the streaming delivers it.
// TODO(librw): these read everything in one go, with librw's readers (No support for the game's native texture format changes either)

// 0x730FC0
RwTexDictionary* RwTexDictionaryGtaStreamRead(RwStream* stream) {
    return rw::TexDictionary::streamRead(stream);
}

// 0x731070
RwTexDictionary* RwTexDictionaryGtaStreamRead1(RwStream* stream) {
    LIBRW_TODO("reads the whole dictionary, not the first half");
    return rw::TexDictionary::streamRead(stream);
}

// 0x731150
RwTexDictionary* RwTexDictionaryGtaStreamRead2(RwStream* stream, RwTexDictionary* txd) {
    (void)stream;
    return txd; // Already read completely by `RwTexDictionaryGtaStreamRead1`
}

// 0x72E570
bool RpClumpGtaStreamRead1(RwStream* stream) {
    (void)stream;
    LIBRW_TODO("the clump is read by RpClumpGtaStreamRead2");
    return true;
}

// 0x72E620
RpClump* RpClumpGtaStreamRead2(RwStream* stream) {
    return rw::Clump::streamRead(stream);
}

// 0x72E700
void RpClumpGtaCancelStream() {
    // Nothing is kept between the two halves
}

/*
 ***********************************************
 *
 * Materials, geometries, lights, atomics
 *
 ***********************************************
 */

RpMaterial* RpMaterialSetSurfaceProperties(RpMaterial* material, const RwSurfaceProperties* surfaceProperties) {
    material->surfaceProps = *surfaceProperties;
    return material;
}

RwInt32 RpMaterialRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return rw::Material::registerPlugin(size, pluginID, constructCB, destructCB, (rw::CopyConstructor)copyCB);
}

RwInt32 RpMaterialRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return rw::Material::registerPluginStream(pluginID, readCB, (rw::StreamWrite)writeCB, (rw::StreamGetSize)getSizeCB);
}

RwInt32 RpGeometryRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return rw::Geometry::registerPlugin(size, pluginID, constructCB, destructCB, (rw::CopyConstructor)copyCB);
}

RwInt32 RpGeometryRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return rw::Geometry::registerPluginStream(pluginID, readCB, (rw::StreamWrite)writeCB, (rw::StreamGetSize)getSizeCB);
}

RwInt32 RpAtomicRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return rw::Atomic::registerPluginStream(pluginID, readCB, (rw::StreamWrite)writeCB, (rw::StreamGetSize)getSizeCB);
}

RwInt32 RpGeometryGetNumMaterials(const RpGeometry* geometry) { return geometry->matList.numMaterials; }

RpMaterial* RpGeometryTriangleGetMaterial(const RpGeometry* geometry, const RpTriangle* triangle) {
    return geometry->matList.materials[triangle->matId];
}

const RpGeometry* RpGeometryTriangleGetVertexIndices(const RpGeometry* geometry, const RpTriangle* triangle, RwUInt16* vert1, RwUInt16* vert2, RwUInt16* vert3) {
    *vert1 = triangle->v[0];
    *vert2 = triangle->v[1];
    *vert3 = triangle->v[2];
    return geometry;
}

RpMaterialList* _rpMaterialListDeinitialize(RpMaterialList* matList) { matList->deinit(); return matList; }
RwInt32         _rpMaterialListAppendMaterial(RpMaterialList* matList, RpMaterial* material) { return matList->appendMaterial(material); }
RwInt32         _rpMaterialListFindMaterialIndex(const RpMaterialList* matList, const RpMaterial* material) {
    return const_cast<RpMaterialList*>(matList)->findIndex(const_cast<RpMaterial*>(material));
}

RwUInt32 RpLightGetFlags(const RpLight* light) { return light->object.object.flags; }

RwBool     RpMatFXAtomicQueryEffects(RpAtomic* atomic) { return rw::MatFX::getEffects(atomic); }
RwTexture* RpMatFXMaterialGetEnvMapTexture(const RpMaterial* material) {
    rw::MatFX* const fx = rw::MatFX::get(material);
    return fx ? fx->getEnvTexture() : nullptr;
}

/*
 ***********************************************
 *
 * Skin and hierarchical animation
 *
 ***********************************************
 */

RpSkin* RpSkinCreate(RwUInt32 numVertices, RwUInt32 numBones, RwMatrixWeights* vertexWeights, RwUInt32* vertexIndices, RwMatrix* inverseMatrices) {
    rw::Skin* const skin = rwNewT(rw::Skin, 1, rw::MEMDUR_EVENT | rw::ID_SKIN);
    skin->init(numBones, numBones, numVertices); // (Room for all bones to be used, `findUsedBones` below sets how many are)
    skin->legacyType   = 0;
    skin->platformData = nullptr;
    memcpy(skin->weights, vertexWeights, numVertices * 4 * sizeof(float));
    memcpy(skin->indices, vertexIndices, numVertices * 4); // One byte for each of the 4 bones of a vertex
    memcpy(skin->inverseMatrices, inverseMatrices, numBones * 64);
    for (RwUInt32 i = 0; i < numBones; i++) { // The matrices are plain 4x4 in librw, ours have flags and padding in the last column
        float* const m = &skin->inverseMatrices[i * 16];
        m[3] = m[7] = m[11] = 0.0f;
        m[15] = 1.0f;
    }
    skin->findNumWeights(numVertices);
    skin->findUsedBones(numVertices);
    return skin;
}

RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin) {
    rw::Skin::set(geometry, skin);
    return geometry;
}

RpAtomic* RpSkinAtomicSetType(RpAtomic* atomic, RpSkinType type) {
    rw::Skin::setPipeline(atomic, type);
    return atomic;
}

RpHAnimHierarchy* RpHAnimHierarchyCreateFromHierarchy(RpHAnimHierarchy* hierarchy, RpHAnimHierarchyFlag flags, RwInt32 maxInterpKeyFrameSize) {
    std::vector<rw::int32> nodeFlags(hierarchy->numNodes), nodeIDs(hierarchy->numNodes);
    for (auto i = 0; i < hierarchy->numNodes; i++) {
        nodeFlags[i] = hierarchy->nodeInfo[i].flags;
        nodeIDs[i]   = hierarchy->nodeInfo[i].id;
    }
    return rw::HAnimHierarchy::create(hierarchy->numNodes, nodeFlags.data(), nodeIDs.data(), flags, maxInterpKeyFrameSize);
}

RwMatrix* RpHAnimHierarchyGetNodeMatrix(RpHAnimHierarchy* hierarchy, RwInt32 nodeID) {
    const auto index = hierarchy->getIndex(nodeID);
    return index >= 0
        ? &hierarchy->matrices[index]
        : nullptr;
}

RwBool RtAnimInitialize(void) { return TRUE; } // (The interpolation schemes are registered by the plugins in librw)

RwBool RtAnimRegisterInterpolationScheme(RtAnimInterpolatorInfo* interpolatorInfo) {
    rw::AnimInterpolatorInfo::registerInterp(interpolatorInfo); // NOTE: Keeps the pointer
    return TRUE;
}

RtAnimAnimation* RtAnimAnimationCreate(RwInt32 typeID, RwInt32 numFrames, RwInt32 flags, RwReal duration) {
    rw::AnimInterpolatorInfo* const info = rw::AnimInterpolatorInfo::find(typeID);
    return info
        ? rw::Animation::create(info, numFrames, flags, duration)
        : nullptr;
}

RwBool RtAnimAnimationDestroy(RtAnimAnimation* animation) { animation->destroy(); return TRUE; }
RwBool RtAnimInterpolatorSetCurrentAnim(RtAnimInterpolator* animI, RtAnimAnimation* anim) { return animI->setCurrentAnim(anim); }

/*
 ***********************************************
 *
 * UV animation
 *
 ***********************************************
 */

// RenderWare has generic dictionaries described by a schema, librw only the one kind the game uses: of UV animations
struct RtDictSchema {
    const RwChar* name;
};
static RtDictSchema s_UVAnimDictSchema{ "UVAnimDict" };
RtDictSchema&       RpUVAnimDictSchema = s_UVAnimDictSchema;

RtDict* RtDictSchemaStreamReadDict(RtDictSchema* schema, RwStream* stream) {
    (void)schema;
    return reinterpret_cast<RtDict*>(rw::UVAnimDictionary::streamRead(stream));
}

RtDict* RtDictSchemaSetCurrentDict(RtDictSchema* schema, RtDict* dict) {
    (void)schema;
    rw::currentUVAnimDictionary = reinterpret_cast<rw::UVAnimDictionary*>(dict);
    return dict;
}

RtDict* RtDictSchemaGetCurrentDict(RtDictSchema* schema) {
    (void)schema;
    return reinterpret_cast<RtDict*>(rw::currentUVAnimDictionary);
}

RwBool RtDictDestroy(RtDict* dictionary) {
    auto* const dict = reinterpret_cast<rw::UVAnimDictionary*>(dictionary);
    if (rw::currentUVAnimDictionary == dict) {
        rw::currentUVAnimDictionary = nullptr;
    }
    dict->destroy();
    return TRUE;
}

RwBool      RpUVAnimPluginAttach(void) { rw::registerUVAnimPlugin(); return TRUE; }
RwBool      RpMaterialUVAnimExists(const RpMaterial* material) { return rw::UVAnim::exists(const_cast<RpMaterial*>(material)); }
RpMaterial* RpMaterialUVAnimAddAnimTime(RpMaterial* material, RwReal deltaTime) { rw::UVAnim::addTime(material, deltaTime); return material; }
RpMaterial* RpMaterialUVAnimApplyUpdate(RpMaterial* material) { rw::UVAnim::applyUpdate(material); return material; }

/*
 ***********************************************
 *
 * Quaternions
 *
 ***********************************************
 */

RwBool RtQuatConvertFromMatrix(RtQuat* const qpQuat, const RwMatrix* const mpMatrix) {
    const rw::Quat q = const_cast<RwMatrix*>(mpMatrix)->getRotation();
    qpQuat->imag.x   = q.x;
    qpQuat->imag.y   = q.y;
    qpQuat->imag.z   = q.z;
    qpQuat->real     = q.w;
    return TRUE;
}

void RtQuatUnitConvertToMatrix(const RtQuat* const qpQuat, RwMatrix* const mpMatrix) {
    RtQuatConvertToMatrix(qpQuat, mpMatrix);
}

RwV3d* RtQuatTransformVectors(RwV3d* vectorsOut, const RwV3d* vectorsIn, const RwInt32 numPoints, const RtQuat* quat) {
    const rw::Quat q = rw::makeQuat(quat->real, quat->imag);
    for (auto i = 0; i < numPoints; i++) {
        vectorsOut[i] = rw::rotate(vectorsIn[i], q);
    }
    return vectorsOut;
}

void RtQuatSetupSlerpCache(RtQuat* qpFrom, RtQuat* qpTo, RtQuatSlerpCache* sCache) {
    RwReal cosOmega = qpFrom->imag.x * qpTo->imag.x + qpFrom->imag.y * qpTo->imag.y + qpFrom->imag.z * qpTo->imag.z + qpFrom->real * qpTo->real;

    // Take the short way around
    sCache->raFrom = *qpFrom;
    sCache->raTo   = *qpTo;
    if (cosOmega < 0.0f) {
        cosOmega          = -cosOmega;
        sCache->raTo.imag = rw::scale(qpTo->imag, -1.0f);
        sCache->raTo.real = -qpTo->real;
    }

    sCache->nearlyZeroOm = (1.0f - cosOmega) < 0.00001f;
    sCache->omega        = 0.0f;
    if (!sCache->nearlyZeroOm) {
        sCache->omega = std::acos(std::min(cosOmega, 1.0f));

        // So the interpolation only has to multiply with `sin(t * omega)`
        const RwReal recipSinOmega = 1.0f / std::sin(sCache->omega);
        sCache->raFrom.imag        = rw::scale(sCache->raFrom.imag, recipSinOmega);
        sCache->raFrom.real       *= recipSinOmega;
        sCache->raTo.imag          = rw::scale(sCache->raTo.imag, recipSinOmega);
        sCache->raTo.real         *= recipSinOmega;
    }
}

void RtQuatSlerp(RtQuat* qpResult, RtQuat* qpFrom, RtQuat* qpTo, RwReal rT, RtQuatSlerpCache* sCache) {
    if (rT <= 0.0f) {
        *qpResult = *qpFrom;
    } else if (rT >= 1.0f) {
        *qpResult = *qpTo;
    } else {
        RwReal scaleFrom = 1.0f - rT;
        RwReal scaleTo   = rT;
        if (!sCache->nearlyZeroOm) {
            scaleFrom = std::sin(scaleFrom * sCache->omega);
            scaleTo   = std::sin(scaleTo * sCache->omega);
        }
        qpResult->imag = rw::add(rw::scale(sCache->raFrom.imag, scaleFrom), rw::scale(sCache->raTo.imag, scaleTo));
        qpResult->real = sCache->raFrom.real * scaleFrom + sCache->raTo.real * scaleTo;
    }
}

/*
 ***********************************************
 *
 * Immediate mode
 *
 ***********************************************
 */

RwBool RwIm3DRenderPrimitive(RwPrimitiveType primType) {
    rw::im3d::RenderPrimitive((rw::PrimitiveType)primType);
    return TRUE;
}

/*
 ***********************************************
 *
 * Direct3D 9
 *
 ***********************************************
 */

static rwD3D9DeviceRestoreCallBack s_DeviceRestoreCallback{};
static RwUInt32                    s_StencilClear{};

void _rwD3D9SetStreams(const RxD3D9VertexStream* streams, RwBool useOffsets) {
    for (auto i = 0; i < RWD3D9_MAX_VERTEX_STREAMS; i++) {
        rw::d3d::setStreamSource(i, streams[i].vertexBuffer, useOffsets ? streams[i].offset : 0, streams[i].vertexBuffer ? streams[i].stride : 0);
    }
}

void _rwD3D9DrawIndexedPrimitive(RwUInt32 primitiveType, RwInt32 baseVertexIndex, RwUInt32 minIndex, RwUInt32 numVertices, RwUInt32 startIndex, RwUInt32 primitiveCount) {
    rw::d3d::flushCache();
    rw::d3d::d3ddevice->DrawIndexedPrimitive((D3DPRIMITIVETYPE)primitiveType, baseVertexIndex, minIndex, numVertices, startIndex, primitiveCount);
}

void _rwD3D9DrawPrimitive(RwUInt32 primitiveType, RwUInt32 startVertex, RwUInt32 primitiveCount) {
    rw::d3d::flushCache();
    rw::d3d::d3ddevice->DrawPrimitive((D3DPRIMITIVETYPE)primitiveType, startVertex, primitiveCount);
}

void _rwD3D9EnableClippingIfNeeded(void* object, RwUInt32 type) {
    (void)object;
    (void)type;
    rw::d3d::setRenderState(D3DRS_CLIPPING, TRUE); // TODO(librw): Only if the object isn't completely inside the frustum
}

RwBool RwD3D9SetTransform(RwUInt32 state, const void* matrix) {
    D3DMATRIX identity{};
    identity._11 = identity._22 = identity._33 = identity._44 = 1.0f;
    return SUCCEEDED(rw::d3d::d3ddevice->SetTransform((D3DTRANSFORMSTATETYPE)state, matrix ? static_cast<const D3DMATRIX*>(matrix) : &identity));
}

RwBool RwD3D9SetMaterial(const void* material) {
    rw::d3d::setD3dMaterial(const_cast<D3DMATERIAL9*>(static_cast<const D3DMATERIAL9*>(material)));
    return TRUE;
}

RwBool RwD3D9SetSurfaceProperties(const RwSurfaceProperties* surfaceProps, const RwRGBA* color, RwUInt32 flags) {
    (void)flags; // TODO(librw): `rxGEOMETRY_PRELIT`/`rxGEOMETRY_MODULATE` select where the diffuse and ambient colours come from
    rw::d3d::setMaterial(*color, *surfaceProps);
    return TRUE;
}

RwBool RwD3D9SetLight(RwInt32 index, const void* light) {
    return SUCCEEDED(rw::d3d::d3ddevice->SetLight(index, static_cast<const D3DLIGHT9*>(light)));
}

RwBool RwD3D9EnableLight(RwInt32 index, RwBool enable) {
    return SUCCEEDED(rw::d3d::d3ddevice->LightEnable(index, enable));
}

const void* RwD3D9GetCaps(void) {
    static D3DCAPS9 s_Caps{};
    rw::d3d::d3ddevice->GetDeviceCaps(&s_Caps);
    return &s_Caps;
}

void RwD3D9SetStencilClear(RwUInt32 stencilClear) {
    s_StencilClear = stencilClear;
    LIBRW_TODO("librw always clears the stencil buffer to 0");
}

RwBool RwD3D9ChangeVideoMode(RwInt32 modeIndex) {
    (void)modeIndex;
    LIBRW_TODO("changing the video mode while running");
    return FALSE;
}

RwBool RwD3D9ChangeMultiSamplingLevels(RwUInt32 numLevels) {
    (void)numLevels;
    LIBRW_TODO("changing the multi-sampling level while running");
    return FALSE;
}

void                        _rwD3D9DeviceSetRestoreCallback(rwD3D9DeviceRestoreCallBack callback) { s_DeviceRestoreCallback = callback; }
rwD3D9DeviceRestoreCallBack _rwD3D9DeviceGetRestoreCallback(void) { return s_DeviceRestoreCallback; }

// librw resets the device when the size of the window or the presentation interval changes, and after the device was lost.
// Everything created on the device directly (not through librw) has to be gone by then, or the reset fails.
static const struct LibrwDeviceResetCallbacks {
    LibrwDeviceResetCallbacks() {
        rw::d3d::deviceReleaseCB = [] {
            if (ImGui::GetCurrentContext() && ImGui::GetIO().BackendRendererUserData) {
                ImGui_ImplDX9_InvalidateDeviceObjects();
            }
        };
        rw::d3d::deviceRestoreCB = [] {
            if (ImGui::GetCurrentContext() && ImGui::GetIO().BackendRendererUserData) {
                ImGui_ImplDX9_CreateDeviceObjects();
            }
            if (s_DeviceRestoreCallback) {
                s_DeviceRestoreCallback();
            }
        };
        rw::d3d::deviceResetFailedCB = [](long result) {
            NOTSA_LOG_ERR("librw adapter: Resetting the Direct3D device failed ({:#010x}). Something created on the device directly wasn't released before", static_cast<uint32>(result));
        };
    }
} s_LibrwDeviceResetCallbacks;

void RpD3D9GeometrySetUsageFlags(RpGeometry* geometry, RpD3D9GeometryUsageFlag flags) {
    (void)geometry;
    (void)flags; // (A hint for how to create the vertex buffers, librw decides that from the geometry's lock flags)
}

/*
 ***********************************************
 *
 * Pipelines
 *
 * RenderWare's are graphs of nodes; for atomics on Direct3D 9 a single "all in one" node with 4 callbacks.
 * librw's object pipelines are the callbacks directly. A node here is the pipeline itself, and its callbacks are remembered,
 * but TODO(librw): nothing calls them yet. The game's pipelines (`game_sa/Pipelines`) have to be ported to `rw::d3d9::ObjPipeline`.
 *
 ***********************************************
 */

struct LibrwAllInOneCallBacks {
    RxD3D9AllInOneInstanceCallBack   instance{};
    RxD3D9AllInOneReinstanceCallBack reinstance{};
    RxD3D9AllInOneLightingCallBack   lighting{};
    RxD3D9AllInOneRenderCallBack     render{};
};

static auto& LibrwGetAllInOneCallBacks(RxPipelineNode* node) {
    static std::unordered_map<RxPipelineNode*, LibrwAllInOneCallBacks> s_CallBacks;
    return s_CallBacks[node];
}

static RxNodeDefinition s_D3D9AtomicAllInOneNode{ "nodeD3D9AtomicAllInOne.csl" };

RxNodeDefinition* RxNodeDefinitionGetD3D9AtomicAllInOne(void) { return &s_D3D9AtomicAllInOneNode; }

RxPipeline* RxPipelineCreate(void) {
    LIBRW_TODO("the game's custom pipelines render with librw's default pipeline");

    // Same as librw's default pipeline (A new one comes without any callbacks)
    rw::d3d9::ObjPipeline* const pipe = rw::d3d9::ObjPipeline::create();
    pipe->instanceCB   = rw::d3d9::defaultInstanceCB;
    pipe->uninstanceCB = rw::d3d9::defaultUninstanceCB;
    pipe->renderCB     = rw::d3d9::defaultRenderCB_Shader;
    return pipe;
}

void          _rxPipelineDestroy(RxPipeline* pipeline) { pipeline->destroy(); }
RxLockedPipe* RxPipelineLock(RxPipeline* pipeline) { return pipeline; }
RxPipeline*   RxLockedPipeUnlock(RxLockedPipe* pipeline) { return pipeline; }

RxLockedPipe* RxLockedPipeAddFragment(RxLockedPipe* pipeline, RwUInt32* firstIndex, RxNodeDefinition* nodeDef0, RxNodeDefinition* nodeDef1) {
    (void)nodeDef0;
    (void)nodeDef1;
    if (firstIndex) {
        *firstIndex = 0;
    }
    return pipeline;
}

RxPipelineNode* RxPipelineFindNodeByName(RxPipeline* pipeline, const RwChar* name, RxPipelineNode* start, RwInt32* nodeIndex) {
    (void)name;
    (void)start;
    if (nodeIndex) {
        *nodeIndex = 0;
    }
    return reinterpret_cast<RxPipelineNode*>(pipeline);
}

void                             RxD3D9AllInOneSetInstanceCallBack(RxPipelineNode* node, RxD3D9AllInOneInstanceCallBack callback) { LibrwGetAllInOneCallBacks(node).instance = callback; }
RxD3D9AllInOneInstanceCallBack   RxD3D9AllInOneGetInstanceCallBack(RxPipelineNode* node) { return LibrwGetAllInOneCallBacks(node).instance; }
void                             RxD3D9AllInOneSetReinstanceCallBack(RxPipelineNode* node, RxD3D9AllInOneReinstanceCallBack callback) { LibrwGetAllInOneCallBacks(node).reinstance = callback; }
RxD3D9AllInOneReinstanceCallBack RxD3D9AllInOneGetReinstanceCallBack(RxPipelineNode* node) { return LibrwGetAllInOneCallBacks(node).reinstance; }
void                             RxD3D9AllInOneSetLightingCallBack(RxPipelineNode* node, RxD3D9AllInOneLightingCallBack callback) { LibrwGetAllInOneCallBacks(node).lighting = callback; }
RxD3D9AllInOneLightingCallBack   RxD3D9AllInOneGetLightingCallBack(RxPipelineNode* node) { return LibrwGetAllInOneCallBacks(node).lighting; }
void                             RxD3D9AllInOneSetRenderCallBack(RxPipelineNode* node, RxD3D9AllInOneRenderCallBack callback) { LibrwGetAllInOneCallBacks(node).render = callback; }
RxD3D9AllInOneRenderCallBack     RxD3D9AllInOneGetRenderCallBack(RxPipelineNode* node) { return LibrwGetAllInOneCallBacks(node).render; }

#endif // NOTSA_LIBRW
