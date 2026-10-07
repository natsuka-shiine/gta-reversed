/*
* RenderWare API on top of librw (https://github.com/aap/librw).
* Started from the `fakerw` layer of re3/reVC (by aap), extended for the RenderWare 3.6 API that San Andreas uses.
* Only used when building with `NOTSA_LIBRW`.
*/
#ifndef __GTA_RTBMP_H__
#define __GTA_RTBMP_H__

#include "rwcore.h"

RwImage *RtBMPImageWrite(RwImage * image, const RwChar * imageName);
RwImage *RtBMPImageRead(const RwChar * imageName);

#endif // __GTA_RTBMP_H__
