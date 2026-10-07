#ifndef NOTSA_LIBRW // These call into the original executable
#include "StdInc.h"

#include "rtslerp.h"

void RtQuatSetupSlerpCache(RtQuat* qpFrom, RtQuat* qpTo, RtQuatSlerpCache* sCache) {
    ((void(__cdecl*)(RtQuat*, RtQuat*, RtQuatSlerpCache*))0x7EC220)(qpFrom, qpTo, sCache);
}

#endif // !NOTSA_LIBRW
