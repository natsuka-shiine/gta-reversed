/*
* RenderWare API on top of librw (https://github.com/aap/librw).
* Started from the `fakerw` layer of re3/reVC (by aap), extended for the RenderWare 3.6 API that San Andreas uses.
* Only used when building with `NOTSA_LIBRW`.
*/
#ifndef __GTA_RTSLERP_H__
#define __GTA_RTSLERP_H__

#include "rwcore.h"
#include "rtquat.h"

struct RtQuatSlerpCache
{
	RtQuat raFrom;
	RtQuat raTo;
	RwReal omega;
	RwBool nearlyZeroOm;
};

void RtQuatSetupSlerpCache(RtQuat *qpFrom, RtQuat *qpTo, RtQuatSlerpCache *sCache);
void RtQuatSlerp(RtQuat *qpResult, RtQuat *qpFrom, RtQuat *qpTo, RwReal rT, RtQuatSlerpCache *sCache);

#endif // __GTA_RTSLERP_H__
