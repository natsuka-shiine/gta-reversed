/*
* RenderWare API on top of librw (https://github.com/aap/librw).
* Started from the `fakerw` layer of re3/reVC (by aap), extended for the RenderWare 3.6 API that San Andreas uses.
* Only used when building with `NOTSA_LIBRW`.
*/
#ifndef __GTA_RPHANIM_H__
#define __GTA_RPHANIM_H__

#include "rwcore.h"
#include "rpworld.h"
#include "rtquat.h"
#include "rtanim.h"

#include "rtquat.h"

//struct RpHAnimHierarchy;
typedef rw::HAnimHierarchy RpHAnimHierarchy;
//struct RpHAnimAnimation;
typedef rw::Animation RpHAnimAnimation;

#define rpHANIMSTDKEYFRAMETYPEID 0x1

// same as rw::HAnimKeyFrame, but we need RtQuat in this one
struct RpHAnimStdKeyFrame
{
	RpHAnimStdKeyFrame *prevFrame;
	RwReal        time;
	RtQuat           q;
	RwV3d            t;
};
// same story, this one only exists in later RW versions
// but we need it for 64 bit builds because offset and size differs!
struct RpHAnimStdInterpFrame
{
	RpHAnimStdKeyFrame *keyFrame1;
	RpHAnimStdKeyFrame *keyFrame2;
	RtQuat           q;
	RwV3d            t;
};

enum RpHAnimHierarchyFlag
{
	rpHANIMHIERARCHYSUBHIERARCHY =              rw::HAnimHierarchy::SUBHIERARCHY,
	rpHANIMHIERARCHYNOMATRICES =                rw::HAnimHierarchy::NOMATRICES,

	rpHANIMHIERARCHYUPDATEMODELLINGMATRICES = rw::HAnimHierarchy::UPDATEMODELLINGMATRICES,
	rpHANIMHIERARCHYUPDATELTMS =              rw::HAnimHierarchy::UPDATELTMS,
	rpHANIMHIERARCHYLOCALSPACEMATRICES =      rw::HAnimHierarchy::LOCALSPACEMATRICES
};

#define rpHANIMPOPPARENTMATRIX      rw::HAnimHierarchy::POP
#define rpHANIMPUSHPARENTMATRIX     rw::HAnimHierarchy::PUSH

RwBool RpHAnimPluginAttach(void);

RwBool RpHAnimFrameSetID(RwFrame *frame, RwInt32 id);
RwInt32 RpHAnimFrameGetID(RwFrame *frame);

RwInt32 RpHAnimIDGetIndex(RpHAnimHierarchy *hierarchy, RwInt32 ID);

RwBool RpHAnimFrameSetHierarchy(RwFrame *frame, RpHAnimHierarchy *hierarchy);
RpHAnimHierarchy *RpHAnimFrameGetHierarchy(RwFrame *frame);

RpHAnimHierarchy *RpHAnimHierarchySetFlags(RpHAnimHierarchy *hierarchy, RpHAnimHierarchyFlag flags);
RpHAnimHierarchyFlag RpHAnimHierarchyGetFlags(RpHAnimHierarchy *hierarchy);

RwBool RpHAnimHierarchySetCurrentAnim(RpHAnimHierarchy *hierarchy, RpHAnimAnimation *anim);
RwBool RpHAnimHierarchySetCurrentAnimTime(RpHAnimHierarchy *hierarchy, RwReal time);
RwBool RpHAnimHierarchySubAnimTime(RpHAnimHierarchy *hierarchy, RwReal time);
RwBool RpHAnimHierarchyAddAnimTime(RpHAnimHierarchy *hierarchy, RwReal time);

RwMatrix *RpHAnimHierarchyGetMatrixArray(RpHAnimHierarchy *hierarchy);
RwMatrix *RpHAnimHierarchyGetNodeMatrix(RpHAnimHierarchy *hierarchy, RwInt32 nodeID);
RpHAnimHierarchy *RpHAnimHierarchyCreateFromHierarchy(RpHAnimHierarchy *hierarchy, RpHAnimHierarchyFlag flags, RwInt32 maxInterpKeyFrameSize);

// Fields that are named differently in librw
#define RpHAnimHierarchyGetNodeFlags(_hier, _index)  ((_hier)->nodeInfo[_index].flags)
#define RpHAnimHierarchyGetNodeID(_hier, _index)     ((_hier)->nodeInfo[_index].id)
#define RpHAnimHierarchyGetInterpolator(_hier)       ((_hier)->interpolator)

typedef RpHAnimStdKeyFrame RpHAnimKeyFrame;
void RpHAnimKeyFrameApply(void *matrix, void *voidIFrame);
void RpHAnimKeyFrameBlend(void *voidOut, void *voidIn1, void *voidIn2, RwReal alpha);
void RpHAnimKeyFrameInterpolate(void *voidOut, void *voidIn1, void *voidIn2, RwReal time, void *customData);
void RpHAnimKeyFrameAdd(void *voidOut, void *voidIn1, void *voidIn2);
void RpHAnimKeyFrameMulRecip(void *voidFrame, void *voidStart);
RwBool RpHAnimHierarchyUpdateMatrices(RpHAnimHierarchy *hierarchy);

#define rpHANIMHIERARCHYGETINTERPFRAME( hierarchy, nodeIndex )    \
        ( (void *)( ( (RwUInt8 *)&(hierarchy->interpolator[1]) +                \
                      ((nodeIndex) *                               \
                       hierarchy->interpolator->currentInterpKeyFrameSize) ) ) )


RpHAnimAnimation *RpHAnimAnimationCreate(RwInt32 typeID, RwInt32 numFrames, RwInt32 flags, RwReal duration);
RpHAnimAnimation  *RpHAnimAnimationDestroy(RpHAnimAnimation *animation);
RpHAnimAnimation  *RpHAnimAnimationStreamRead(RwStream *stream);

#endif // __GTA_RPHANIM_H__
