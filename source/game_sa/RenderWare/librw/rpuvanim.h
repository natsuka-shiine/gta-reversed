/*
* RenderWare API on top of librw (https://github.com/aap/librw).
* Started from the `fakerw` layer of re3/reVC (by aap), extended for the RenderWare 3.6 API that San Andreas uses.
* Only used when building with `NOTSA_LIBRW`.
*/
#ifndef __GTA_RPUVANIM_H__
#define __GTA_RPUVANIM_H__

#include "rwcore.h"
#include "rpworld.h"

#include "rtdict.h"

extern RtDictSchema &RpUVAnimDictSchema;

RwBool RpUVAnimPluginAttach(void);
RwBool RpMaterialUVAnimExists(const RpMaterial *material);
RpMaterial *RpMaterialUVAnimAddAnimTime(RpMaterial *material, RwReal deltaTime);
RpMaterial *RpMaterialUVAnimApplyUpdate(RpMaterial *material);

#endif // __GTA_RPUVANIM_H__
