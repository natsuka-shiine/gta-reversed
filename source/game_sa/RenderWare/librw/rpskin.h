/*
* RenderWare API on top of librw (https://github.com/aap/librw).
* Started from the `fakerw` layer of re3/reVC (by aap), extended for the RenderWare 3.6 API that San Andreas uses.
* Only used when building with `NOTSA_LIBRW`.
*/
#ifndef __GTA_RPSKIN_H__
#define __GTA_RPSKIN_H__

#include "rwcore.h"
#include "rpworld.h"
#include "rphanim.h"

#include <rphanim.h>

//struct RpSkin;
typedef rw::Skin RpSkin;

struct RwMatrixWeights
{
	RwReal w0;
	RwReal w1;
	RwReal w2;
	RwReal w3;
};

RwBool RpSkinPluginAttach(void);

RwUInt32 RpSkinGetNumBones( RpSkin *skin );
RwMatrixWeights *RpSkinGetVertexBoneWeights( RpSkin *skin );
RpSkin *RpSkinCreate( RwUInt32 numVertices, RwUInt32 numBones, RwMatrixWeights *vertexWeights, RwUInt32 *vertexIndices, RwMatrix *inverseMatrices );
RpGeometry *RpSkinGeometrySetSkin( RpGeometry *geometry, RpSkin *skin );

enum RpSkinType
{
	rpNASKINTYPE      = 0,
	rpSKINTYPEGENERIC = 1,
	rpSKINTYPEMATFX   = 2,
	rpSKINTYPETOON    = 3,
};
RpAtomic *RpSkinAtomicSetType( RpAtomic *atomic, RpSkinType type );
RpSkinType RpSkinAtomicGetType( RpAtomic *atomic );
const RwUInt32 *RpSkinGetVertexBoneIndices( RpSkin *skin );
const RwMatrix *RpSkinGetSkinToBoneMatrices( RpSkin *skin );

RpSkin *RpSkinGeometryGetSkin( RpGeometry *geometry );

RpAtomic *RpSkinAtomicSetHAnimHierarchy( RpAtomic *atomic, RpHAnimHierarchy *hierarchy );
RpHAnimHierarchy *RpSkinAtomicGetHAnimHierarchy( const RpAtomic *atomic );

#endif // __GTA_RPSKIN_H__
