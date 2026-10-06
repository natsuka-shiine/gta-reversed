#include "StdInc.h"

#include "FxFrustumInfo.h"

void FxFrustumInfo_c::InjectHooks() {
    RH_ScopedClass(FxFrustumInfo_c);
    RH_ScopedCategory("Fx");

    RH_ScopedInstall(IsCollision, 0x4AA030);
}

// 0x4AA030
bool FxFrustumInfo_c::IsCollision(FxSphere_c& sphere) {
    // Bounding sphere check first
    if (!(sq(m_Sphere.m_fRadius + sphere.m_fRadius) > (sphere.m_vecCenter - m_Sphere.m_vecCenter).SquaredMagnitude())) {
        return false;
    }

    // Now check the planes, starting with the one that rejected this sphere the last time
    auto planeIdx = sphere.m_nNumPlanesPassed;
    for (auto i = 0u; i < m_Planes.size(); i++, planeIdx++) {
        const auto  idx   = planeIdx % m_Planes.size();
        const auto& plane = m_Planes[idx];
        if (DotProduct(plane.normal, sphere.m_vecCenter) - plane.distance > sphere.m_fRadius) {
            sphere.m_nNumPlanesPassed = idx;
            return false;
        }
    }
    return true;
}
