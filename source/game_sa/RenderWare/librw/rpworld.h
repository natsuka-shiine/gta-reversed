/*
* RenderWare API on top of librw (https://github.com/aap/librw).
* Started from the `fakerw` layer of re3/reVC (by aap), extended for the RenderWare 3.6 API that San Andreas uses.
* Only used when building with `NOTSA_LIBRW`.
*/
#ifndef __GTA_RPWORLD_H__
#define __GTA_RPWORLD_H__

#include "rwcore.h"

#define rpATOMIC rw::Atomic::ID
#define rpCLUMP rw::Clump::ID

/*
 ***********************************************
 *
 * RpMaterial
 *
 ***********************************************
 */

//struct RpMaterial;
typedef rw::Material RpMaterial;

typedef RpMaterial *(*RpMaterialCallBack)(RpMaterial *material, void *data);

RpMaterial *RpMaterialCreate(void);
RwBool RpMaterialDestroy(RpMaterial *material);
RpMaterial *RpMaterialClone(RpMaterial *material);
RpMaterial *RpMaterialSetTexture(RpMaterial *material, RwTexture *texture);
RpMaterial *RpMaterialAddRef(RpMaterial *material);
RwTexture *RpMaterialGetTexture(const RpMaterial *material);
RpMaterial *RpMaterialSetColor(RpMaterial *material, const RwRGBA *color);
const RwRGBA *RpMaterialGetColor(const RpMaterial *material);
inline RwRGBA *RpMaterialGetColor(RpMaterial *material) { return &material->color; }
RpMaterial *RpMaterialSetSurfaceProperties(RpMaterial *material, const RwSurfaceProperties *surfaceProperties);
const RwSurfaceProperties *RpMaterialGetSurfaceProperties(const RpMaterial *material);
inline RwSurfaceProperties *RpMaterialGetSurfaceProperties(RpMaterial *material) { return &material->surfaceProps; }
RwInt32 RpMaterialRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpMaterialRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB);
RwInt32 RpMaterialSetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB);
RwInt32 RpMaterialGetPluginOffset(RwUInt32 pluginID);
RwBool RpMaterialValidatePlugins(const RpMaterial *material);
RwUInt32 RpMaterialStreamGetSize(const RpMaterial *material);
RpMaterial *RpMaterialStreamRead(RwStream *stream);
const RpMaterial *RpMaterialStreamWrite(const RpMaterial *material, RwStream *stream);
//RpMaterialChunkInfo *_rpMaterialChunkInfoRead(RwStream *stream,  RpMaterialChunkInfo *materialChunkInfo, RwInt32 *bytesRead);


/*
 ***********************************************
 *
 * RpLight
 *
 ***********************************************
 */

//struct RpLight;
typedef rw::Light RpLight;

enum RpLightType
{
	rpNALIGHTTYPE = 0,
	rpLIGHTDIRECTIONAL,
	rpLIGHTAMBIENT,
	rpLIGHTPOINT = 0x80,
	rpLIGHTSPOT,
	rpLIGHTSPOTSOFT,
};

enum RpLightFlag
{
	rpLIGHTLIGHTATOMICS = 0x01,
	rpLIGHTLIGHTWORLD = 0x02,
};

typedef RpLight    *(*RpLightCallBack) (RpLight * light, void *data);

RwReal RpLightGetRadius(const RpLight *light);
const RwRGBAReal *RpLightGetColor(const RpLight *light);
RpLight *RpLightSetFrame(RpLight *light, RwFrame *frame);
RwFrame *RpLightGetFrame(const RpLight *light);
RpLightType RpLightGetType(const RpLight *light);
RpLight *RpLightSetFlags(RpLight *light, RwUInt32 flags);
RwUInt32 RpLightGetFlags(const RpLight *light);
RpLight *RpLightCreate(RwInt32 type);
RwBool RpLightDestroy(RpLight *light);
RpLight *RpLightSetRadius(RpLight *light, RwReal radius);
RpLight *RpLightSetColor(RpLight *light, const RwRGBAReal *color);
RwReal RpLightGetConeAngle(const RpLight *light);
RpLight *RpLightSetConeAngle(RpLight * ight, RwReal angle);
RwUInt32 RpLightStreamGetSize(const RpLight *light);
RpLight *RpLightStreamRead(RwStream *stream);
const RpLight *RpLightStreamWrite(const RpLight *light, RwStream *stream);
//RpLightChunkInfo *_rpLightChunkInfoRead(RwStream *stream, RpLightChunkInfo *lightChunkInfo, RwInt32 *bytesRead);
RwInt32 RpLightRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpLightRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB);
RwInt32 RpLightSetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB);
RwInt32 RpLightGetPluginOffset(RwUInt32 pluginID);
RwBool RpLightValidatePlugins(const RpLight * light);

/*
 ***********************************************
 *
 * RpGeometry
 *
 ***********************************************
 */

typedef rw::Triangle RpTriangle;

//struct RpGeometry;
typedef rw::Geometry RpGeometry;
//struct RpMorphTarget;
typedef rw::MorphTarget RpMorphTarget;

enum RpGeometryFlag
{
	rpGEOMETRYTRISTRIP  = rw::Geometry::TRISTRIP,
	rpGEOMETRYPOSITIONS = rw::Geometry::POSITIONS,
	rpGEOMETRYTEXTURED  = rw::Geometry::TEXTURED,
	rpGEOMETRYPRELIT    = rw::Geometry::PRELIT,
	rpGEOMETRYNORMALS   = rw::Geometry::NORMALS,
	rpGEOMETRYLIGHT     = rw::Geometry::LIGHT,
	rpGEOMETRYMODULATEMATERIALCOLOR = rw::Geometry::MODULATE,
	rpGEOMETRYTEXTURED2 = rw::Geometry::TEXTURED2,
	rpGEOMETRYNATIVE            = rw::Geometry::NATIVE,
	rpGEOMETRYNATIVEINSTANCE    = rw::Geometry::NATIVEINSTANCE,
	rpGEOMETRYFLAGSMASK         = 0x000000FF,
	rpGEOMETRYNATIVEFLAGSMASK   = 0x0F000000,
};

enum RpGeometryLockMode
{
	rpGEOMETRYLOCKPOLYGONS     = rw::Geometry::LOCKPOLYGONS,
	rpGEOMETRYLOCKVERTICES     = rw::Geometry::LOCKVERTICES,
	rpGEOMETRYLOCKNORMALS      = rw::Geometry::LOCKNORMALS,
	rpGEOMETRYLOCKPRELIGHT     = rw::Geometry::LOCKPRELIGHT,
	rpGEOMETRYLOCKTEXCOORDS    = rw::Geometry::LOCKTEXCOORDS,
	rpGEOMETRYLOCKTEXCOORDS1   = rw::Geometry::LOCKTEXCOORDS1,
	rpGEOMETRYLOCKTEXCOORDS2   = rw::Geometry::LOCKTEXCOORDS2,
	rpGEOMETRYLOCKTEXCOORDS3   = rw::Geometry::LOCKTEXCOORDS3,
	rpGEOMETRYLOCKTEXCOORDS4   = rw::Geometry::LOCKTEXCOORDS4,
	rpGEOMETRYLOCKTEXCOORDS5   = rw::Geometry::LOCKTEXCOORDS5,
	rpGEOMETRYLOCKTEXCOORDS6   = rw::Geometry::LOCKTEXCOORDS6,
	rpGEOMETRYLOCKTEXCOORDS7   = rw::Geometry::LOCKTEXCOORDS7,
	rpGEOMETRYLOCKTEXCOORDS8   = rw::Geometry::LOCKTEXCOORDS8,
	rpGEOMETRYLOCKTEXCOORDSALL = rw::Geometry::LOCKTEXCOORDSALL,
	rpGEOMETRYLOCKALL          = rw::Geometry::LOCKALL
};

RpGeometry  *RpGeometryCreate(RwInt32 numVert, RwInt32 numTriangles, RwUInt32 format);
RwBool RpGeometryDestroy(RpGeometry *geometry);
RpGeometry *_rpGeometryAddRef(RpGeometry *geometry);
RpGeometry  *RpGeometryLock(RpGeometry *geometry, RwInt32 lockMode);
RpGeometry  *RpGeometryUnlock(RpGeometry *geometry);
RpGeometry  *RpGeometryTransform(RpGeometry *geometry, const RwMatrix *matrix);
RpGeometry  *RpGeometryCreateSpace(RwReal radius);
RpMorphTarget  *RpMorphTargetSetBoundingSphere(RpMorphTarget *morphTarget, const RwSphere *boundingSphere);
RwSphere  *RpMorphTargetGetBoundingSphere(RpMorphTarget *morphTarget);
const RpMorphTarget  *RpMorphTargetCalcBoundingSphere(const RpMorphTarget *morphTarget, RwSphere *boundingSphere);
RwInt32 RpGeometryAddMorphTargets(RpGeometry *geometry, RwInt32 mtcount);
RwInt32 RpGeometryAddMorphTarget(RpGeometry *geometry);
RpGeometry  *RpGeometryRemoveMorphTarget(RpGeometry *geometry, RwInt32 morphTarget);
RwInt32 RpGeometryGetNumMorphTargets(const RpGeometry *geometry);
RpMorphTarget  *RpGeometryGetMorphTarget(const RpGeometry *geometry, RwInt32 morphTarget);
RwRGBA  *RpGeometryGetPreLightColors(const RpGeometry *geometry);
RwTexCoords  *RpGeometryGetVertexTexCoords(const RpGeometry *geometry, RwInt32 uvIndex);
RwInt32 RpGeometryGetNumTexCoordSets(const RpGeometry *geometry);
RwInt32 RpGeometryGetNumVertices (const RpGeometry *geometry);
RwV3d  *RpMorphTargetGetVertices(const RpMorphTarget *morphTarget);
RwV3d  *RpMorphTargetGetVertexNormals(const RpMorphTarget *morphTarget);
RpTriangle  *RpGeometryGetTriangles(const RpGeometry *geometry);
RwInt32 RpGeometryGetNumTriangles(const RpGeometry *geometry);
RpMaterial  *RpGeometryGetMaterial(const RpGeometry *geometry, RwInt32 matNum);
const RpGeometry  *RpGeometryTriangleSetVertexIndices(const RpGeometry *geometry, RpTriangle *triangle, RwUInt16 vert1, RwUInt16 vert2, RwUInt16 vert3);
RpGeometry  *RpGeometryTriangleSetMaterial(RpGeometry *geometry, RpTriangle *triangle, RpMaterial *material);
const RpGeometry  *RpGeometryTriangleGetVertexIndices(const RpGeometry *geometry, const RpTriangle *triangle, RwUInt16 *vert1, RwUInt16 *vert2, RwUInt16 *vert3);
RpMaterial   *RpGeometryTriangleGetMaterial(const RpGeometry *geometry, const RpTriangle *triangle);
RwInt32 RpGeometryGetNumMaterials(const RpGeometry *geometry);
RpGeometry  *RpGeometryForAllMaterials(RpGeometry *geometry, RpMaterialCallBack fpCallBack, void *pData);
//const RpGeometry  *RpGeometryForAllMeshes(const RpGeometry *geometry, RpMeshCallBack fpCallBack, void *pData);
RwInt32 RpGeometryRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpGeometryRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB);
RwInt32 RpGeometrySetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB);
RwInt32 RpGeometryGetPluginOffset(RwUInt32 pluginID);
RwBool RpGeometryValidatePlugins(const RpGeometry *geometry);
RwUInt32 RpGeometryStreamGetSize(const RpGeometry *geometry);
const RpGeometry  *RpGeometryStreamWrite(const RpGeometry *geometry, RwStream *stream);
RpGeometry  *RpGeometryStreamRead(RwStream *stream);
//RpGeometryChunkInfo *_rpGeometryChunkInfoRead(RwStream *stream, RpGeometryChunkInfo *geometryChunkInfo, RwInt32 *bytesRead);
RwUInt32 RpGeometryGetFlags(const RpGeometry *geometry);
inline RwUInt32 &RpGeometryGetFlags(RpGeometry *geometry) { return geometry->flags; } // Game code modifies them through this (It's a macro in RenderWare)
RpGeometry  *RpGeometrySetFlags(RpGeometry *geometry, RwUInt32 flags);
const RwSurfaceProperties *_rpGeometryGetSurfaceProperties(const RpGeometry *geometry);
RpGeometry *_rpGeometrySetSurfaceProperties(RpGeometry *geometry, const RwSurfaceProperties *surfaceProperties);


/*
 ***********************************************
 *
 * RpAtomic and RpClump
 *
 ***********************************************
 */

//struct RpAtomic;
typedef rw::Atomic RpAtomic;

enum RpAtomicFlag
{
	rpATOMICCOLLISIONTEST = 0x01,
	rpATOMICRENDER = 0x04,
};

enum RpAtomicSetGeomFlag
{
	rpATOMICSAMEBOUNDINGSPHERE = 0x01,
};

typedef RpAtomic   *(*RpAtomicCallBack) (RpAtomic * atomic, void *data);
typedef RpAtomic   *(*RpAtomicCallBackRender) (RpAtomic * atomic);


//struct RpClump;
typedef rw::Clump RpClump;

struct RpClumpChunkInfo
{
	RwInt32             numAtomics;
	RwInt32             numLights;
	RwInt32             numCameras;
};

typedef RpClump    *(*RpClumpCallBack) (RpClump * clump, void *data);


RpAtomic *AtomicDefaultRenderCallBack(RpAtomic * atomic);
//void _rpAtomicResyncInterpolatedSphere(RpAtomic * atomic);
//const RwSphere *RpAtomicGetWorldBoundingSphere(RpAtomic *  atomic);

RwFrame *RpClumpGetFrame(const RpClump * clump);
RpClump *RpClumpSetFrame(RpClump * clump, RwFrame * frame);
RpClump *RpClumpForAllAtomics(RpClump * clump, RpAtomicCallBack callback, void *pData);
RpClump *RpClumpForAllLights(RpClump * clump, RpLightCallBack callback, void *pData);
RpClump *RpClumpForAllCameras(RpClump * clump, RwCameraCallBack callback, void *pData);
RpClump *RpClumpCreateSpace(const RwV3d * position, RwReal radius);
RpClump *RpClumpRender(RpClump * clump);
RpClump *RpClumpRemoveAtomic(RpClump * clump, RpAtomic * atomic);
RpClump *RpClumpAddAtomic(RpClump * clump, RpAtomic * atomic);
RpClump *RpClumpRemoveLight(RpClump * clump, RpLight * light);
RpClump *RpClumpAddLight(RpClump * clump, RpLight * light);
RpClump *RpClumpRemoveCamera(RpClump * clump, RwCamera * camera);
RpClump *RpClumpAddCamera(RpClump * clump, RwCamera * camera);
RwBool RpClumpDestroy(RpClump * clump);
RpClump *RpClumpCreate(void);
RpClump *RpClumpClone(RpClump * clump);
RpClump *RpClumpSetCallBack(RpClump * clump, RpClumpCallBack callback);
RpClumpCallBack RpClumpGetCallBack(const RpClump * clump);
RwInt32 RpClumpGetNumAtomics(RpClump * clump);
RwInt32 RpClumpGetNumLights(RpClump * clump);
RwInt32 RpClumpGetNumCameras(RpClump * clump);
RwUInt32 RpClumpStreamGetSize(RpClump * clump);
RpClump *RpClumpStreamRead(RwStream * stream);
RpClump *RpClumpStreamWrite(RpClump * clump, RwStream * stream);
RwInt32 RpClumpRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpClumpRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack  readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB);
RwInt32 RpClumpSetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB);
RwInt32 RpClumpGetPluginOffset(RwUInt32 pluginID);
RwBool RpClumpValidatePlugins(const RpClump * clump);

RpAtomic *RpAtomicCreate(void);
RwBool RpAtomicDestroy(RpAtomic * atomic);
RpAtomic *RpAtomicClone(RpAtomic * atomic);
RpAtomic *RpAtomicSetFrame(RpAtomic * atomic, RwFrame * frame);
RpAtomic *RpAtomicSetGeometry(RpAtomic * atomic, RpGeometry * geometry, RwUInt32 flags);

RwFrame *RpAtomicGetFrame(const RpAtomic * atomic);
RpAtomic *RpAtomicSetFlags(RpAtomic * atomic, RwUInt32 flags);
RwUInt32 RpAtomicGetFlags(const RpAtomic * atomic);
RwSphere *RpAtomicGetBoundingSphere(RpAtomic * atomic);
RpAtomic *RpAtomicRender(RpAtomic * atomic);
#define RpAtomicRenderMacro(_atomic) RpAtomicRender(_atomic)
RpClump *RpAtomicGetClump(const RpAtomic * atomic);
//RpInterpolator *RpAtomicGetInterpolator(RpAtomic * atomic);
RpGeometry *RpAtomicGetGeometry(const RpAtomic * atomic);
void RpAtomicSetRenderCallBack(RpAtomic * atomic, RpAtomicCallBackRender callback);
RpAtomicCallBackRender RpAtomicGetRenderCallBack(const RpAtomic * atomic);
RwBool RpAtomicInstance(RpAtomic *atomic);
RwUInt32 RpAtomicStreamGetSize(RpAtomic * atomic);
RpAtomic *RpAtomicStreamRead(RwStream * stream);
RpAtomic *RpAtomicStreamWrite(RpAtomic * atomic, RwStream * stream);
RwInt32 RpAtomicRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpAtomicRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB);
RwInt32 RpAtomicSetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB);
RwInt32 RpAtomicSetStreamRightsCallBack(RwUInt32 pluginID, RwPluginDataChunkRightsCallBack rightsCB);
RwInt32 RpAtomicGetPluginOffset(RwUInt32 pluginID);
RwBool RpAtomicValidatePlugins(const RpAtomic * atomic);

//RwInt32 RpInterpolatorGetEndMorphTarget(const RpInterpolator * interpolator);
//RwInt32 RpInterpolatorGetStartMorphTarget(const RpInterpolator * interpolator);
//RwReal RpInterpolatorGetValue(const RpInterpolator * interpolator);
//RwReal RpInterpolatorGetScale(const RpInterpolator * interpolator);
//RpInterpolator *RpInterpolatorSetEndMorphTarget(RpInterpolator * interpolator, RwInt32 morphTarget, RpAtomic * atomic);
//RpInterpolator *RpInterpolatorSetStartMorphTarget(RpInterpolator * interpolator, RwInt32 morphTarget, RpAtomic * atomic);
//RpInterpolator *RpInterpolatorSetValue(RpInterpolator *  interpolator, RwReal value, RpAtomic *atomic);
//RpInterpolator *RpInterpolatorSetScale(RpInterpolator * interpolator, RwReal scale, RpAtomic *atomic);


RpClump *RpLightGetClump(const RpLight *light);
RpClump *RwCameraGetClump(const RwCamera *camera);

/*
 ***********************************************
 *
 * RpWorld
 *
 ***********************************************
 */

//struct RpWorld;
typedef rw::World RpWorld;

RwBool       RpWorldDestroy(RpWorld * world);
RpWorld     *RpWorldCreate(RwBBox * boundingBox);

RwBool       RpWorldPluginAttach(void);

RpWorld *RpWorldRemoveCamera(RpWorld *world, RwCamera *camera);
RpWorld *RpWorldAddCamera(RpWorld *world, RwCamera *camera);
RpWorld *RwCameraGetWorld(const RwCamera *camera);
RpWorld *RpWorldRemoveAtomic(RpWorld *world, RpAtomic *atomic);
RpWorld *RpWorldAddAtomic(RpWorld *world, RpAtomic *atomic);
RpWorld *RpAtomicGetWorld(const RpAtomic *atomic);
RpWorld *RpWorldAddClump(RpWorld *world, RpClump *clump);
RpWorld *RpWorldRemoveClump(RpWorld *world, RpClump *clump);
RpWorld *RpClumpGetWorld(const RpClump *clump);
RpWorld *RpWorldAddLight(RpWorld *world, RpLight *light);
RpWorld *RpWorldRemoveLight(RpWorld *world, RpLight *light);
RpWorld *RpLightGetWorld(const RpLight *light);
RwCamera *RwCameraForAllClumpsInFrustum(RwCamera *camera, void *data);
RwCamera *RwCameraForAllClumpsNotInFrustum(RwCamera *camera, RwInt32 numClumps, void *data);
//RwCamera *RwCameraForAllSectorsInFrustum(RwCamera *camera, RpWorldSectorCallBack callBack, void *pData);
//RpLight *RpLightForAllWorldSectors(RpLight *light, RpWorldSectorCallBack callback, void *data);
//RpAtomic *RpAtomicForAllWorldSectors(RpAtomic *atomic, RpWorldSectorCallBack callback, void *data);
//RpWorldSector *RpWorldSectorForAllAtomics(RpWorldSector *sector, RpAtomicCallBack callback, void *data);
//RpWorldSector *RpWorldSectorForAllCollisionAtomics(RpWorldSector *sector, RpAtomicCallBack callback, void *data);
//RpWorldSector *RpWorldSectorForAllLights(RpWorldSector *sector, RpLightCallBack callback, void *data);

/*
 ***********************************************
 *
 * Meshes, material lists and the Direct3D 9 pipelines
 *
 ***********************************************
 */

// TODO(librw): the `RxD3D9...` types are RenderWare's layouts, there to make the game's custom pipelines compile.
// librw's are `rw::d3d9::InstanceDataHeader`/`rw::d3d9::InstanceData`.
typedef rw::MeshHeader   RpMeshHeader;
typedef rw::Mesh         RpMesh;
typedef rw::MaterialList RpMaterialList;

#define RpGeometryGetMeshHeader(_geometry) ((_geometry)->meshHeader)
#define RpGeometryGetMesh(_geometry, _index) (&((RpMesh *)(RpGeometryGetMeshHeader(_geometry) + 1))[_index])

RpMaterialList *_rpMaterialListInitialize(RpMaterialList *matList);
RpMaterialList *_rpMaterialListDeinitialize(RpMaterialList *matList);
RwInt32 _rpMaterialListAppendMaterial(RpMaterialList *matList, RpMaterial *material);
RwInt32 _rpMaterialListFindMaterialIndex(const RpMaterialList *matList, const RpMaterial *material);

enum RpD3D9GeometryUsageFlag
{
	rpD3D9GEOMETRYUSAGE_DYNAMICPOSITIONS    = 0x02,
	rpD3D9GEOMETRYUSAGE_DYNAMICNORMALS      = 0x04,
	rpD3D9GEOMETRYUSAGE_DYNAMICPRELIT       = 0x08,
	rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS    = 0x10,
	rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS1   = 0x10,
	rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS2   = 0x20,
	rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS3   = 0x40,
	rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS4   = 0x80,
	rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS5   = 0x0100,
	rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS6   = 0x0200,
	rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS7   = 0x0400,
	rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS8   = 0x0800,
	rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDSALL = 0x0ff0,
	rpD3D9GEOMETRYUSAGE_DYNAMICMASK         = 0x0fff,
	rpD3D9GEOMETRYUSAGE_CREATETANGENTS      = 0x10000,
};
void RpD3D9GeometrySetUsageFlags(RpGeometry *geometry, RpD3D9GeometryUsageFlag flags);
RpD3D9GeometryUsageFlag RpD3D9GeometryGetUsageFlags(const RpGeometry *geometry);

struct RxD3D9ResEntryHeader
{
	RwUInt32                     serialNumber;
	RwUInt32                     numMeshes;
	IDirect3DIndexBuffer9       *indexBuffer;
	D3DPRIMITIVETYPE             primType;
	RxD3D9VertexStream           vertexStream[RWD3D9_MAX_VERTEX_STREAMS];
	RwBool                       useOffsets;
	IDirect3DVertexDeclaration9 *vertexDeclaration;
	RwUInt32                     totalNumIndex;
	RwUInt32                     totalNumVertex;
};

struct RxD3D9InstanceData
{
	RwUInt32                numIndex;
	RwUInt32                minVert;
	RpMaterial             *material;
	RwBool                  vertexAlpha;
	IDirect3DVertexShader9 *vertexShader;
	RwUInt32                baseIndex;
	RwUInt32                numVertices;
	RwUInt32                startIndex;
	RwUInt32                numPrimitives;
};

typedef RwBool (*RxD3D9AllInOneInstanceCallBack)(void *object, RxD3D9ResEntryHeader *resEntryHeader, RwBool reinstance);
typedef RwBool (*RxD3D9AllInOneReinstanceCallBack)(void *object, RwResEntry *resEntry, RxD3D9AllInOneInstanceCallBack instanceCallback);
typedef void (*RxD3D9AllInOneLightingCallBack)(void *object);
typedef void (*RxD3D9AllInOneRenderCallBack)(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags);

RxNodeDefinition *RxNodeDefinitionGetD3D9AtomicAllInOne(void);
void RxD3D9AllInOneSetInstanceCallBack(RxPipelineNode *node, RxD3D9AllInOneInstanceCallBack callback);
RxD3D9AllInOneInstanceCallBack RxD3D9AllInOneGetInstanceCallBack(RxPipelineNode *node);
void RxD3D9AllInOneSetReinstanceCallBack(RxPipelineNode *node, RxD3D9AllInOneReinstanceCallBack callback);
RxD3D9AllInOneReinstanceCallBack RxD3D9AllInOneGetReinstanceCallBack(RxPipelineNode *node);
void RxD3D9AllInOneSetLightingCallBack(RxPipelineNode *node, RxD3D9AllInOneLightingCallBack callback);
RxD3D9AllInOneLightingCallBack RxD3D9AllInOneGetLightingCallBack(RxPipelineNode *node);
void RxD3D9AllInOneSetRenderCallBack(RxPipelineNode *node, RxD3D9AllInOneRenderCallBack callback);
RxD3D9AllInOneRenderCallBack RxD3D9AllInOneGetRenderCallBack(RxPipelineNode *node);

void _rwD3D9EnableClippingIfNeeded(void *object, RwUInt32 type);

void RpClumpGtaCancelStream();

#endif // __GTA_RPWORLD_H__
