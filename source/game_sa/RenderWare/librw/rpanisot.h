/*
* RenderWare API on top of librw (https://github.com/aap/librw).
* Started from the `fakerw` layer of re3/reVC (by aap), extended for the RenderWare 3.6 API that San Andreas uses.
* Only used when building with `NOTSA_LIBRW`.
*/
#ifndef __GTA_RPANISOT_H__
#define __GTA_RPANISOT_H__

#include "rwcore.h"

RwInt8      RpAnisotGetMaxSupportedMaxAnisotropy(void);
RwTexture    *RpAnisotTextureSetMaxAnisotropy(RwTexture *tex, RwInt8 val);
RwInt8       RpAnisotTextureGetMaxAnisotropy(RwTexture *tex);
RwBool       RpAnisotPluginAttach(void);

#endif // __GTA_RPANISOT_H__
