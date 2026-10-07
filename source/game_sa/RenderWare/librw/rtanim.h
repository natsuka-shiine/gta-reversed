/*
* RenderWare API on top of librw (https://github.com/aap/librw).
* Started from the `fakerw` layer of re3/reVC (by aap), extended for the RenderWare 3.6 API that San Andreas uses.
* Only used when building with `NOTSA_LIBRW`.
*/
#ifndef __GTA_RTANIM_H__
#define __GTA_RTANIM_H__

#include "rwcore.h"

typedef rw::Animation            RtAnimAnimation;
typedef rw::AnimInterpolator     RtAnimInterpolator;
typedef rw::AnimInterpolatorInfo RtAnimInterpolatorInfo;
typedef rw::KeyFrameHeader       RtAnimKeyFrameHeader;
typedef rw::InterpFrameHeader    RtAnimInterpFrameHeader;

RwBool RtAnimInitialize(void);
RwBool RtAnimRegisterInterpolationScheme(RtAnimInterpolatorInfo *interpolatorInfo);
RtAnimAnimation *RtAnimAnimationCreate(RwInt32 typeID, RwInt32 numFrames, RwInt32 flags, RwReal duration);
RwBool RtAnimAnimationDestroy(RtAnimAnimation *animation);
RtAnimInterpolator *RtAnimInterpolatorCreate(RwInt32 numNodes, RwInt32 maxInterpKeyFrameSize);
void RtAnimInterpolatorDestroy(RtAnimInterpolator *anim);
RwBool RtAnimInterpolatorSetCurrentAnim(RtAnimInterpolator *animI, RtAnimAnimation *anim);

#define RtAnimInterpolatorGetCurrentAnim(animI) ((animI)->currentAnim)
#define rtANIMGETINTERPFRAME(anim, nodeIndex) ((void *)(((RwUInt8 *)&((anim)[1]) + ((nodeIndex) * (anim)->currentInterpKeyFrameSize))))

#endif // __GTA_RTANIM_H__
