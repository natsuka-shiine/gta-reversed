#include "StdInc.h"
#include "SearchLight.h"
#include "TheScripts.h"

void CSearchLight::InjectHooks() {
    RH_ScopedClass(CSearchLight);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(IsSpottedEntity, 0x493900);
    RH_ScopedInstall(IsLookingAtPos, 0x493280);
}

void CSearchLight::SetTravelToPoint() {
    assert(0);
}

void CSearchLight::SetFollowEntity() {
    assert(0);
}

void CSearchLight::SetPathBetween() {
    assert(0);
}

// 0x493280
bool CSearchLight::IsLookingAtPos(const CVector& pos, int32 index) {
    const auto& light = CTheScripts::ScriptSearchLightArray[index];

    // Axes of the light's spot (ellipse) on the ground
    CVector    axisA    = light.vf64;
    CVector    axisB    = light.vf70;
    const auto axisALen = axisA.NormaliseAndMag();
    const auto axisBLen = axisB.NormaliseAndMag();

    const auto spotToPos = pos - light.m_TargetSpot;
    const auto a         = DotProduct(axisA, spotToPos) / axisALen;
    const auto b         = DotProduct(axisB, spotToPos) / axisBLen;

    return sq(b) + sq(a) <= 1.f;
}

void CSearchLight::GetOnEntity() {
    assert(0);
}

// 0x493900
bool CSearchLight::IsSpottedEntity(uint32 index, const CEntity& entity) {
    const auto actualIdx = CTheScripts::GetActualScriptThingIndex(static_cast<int32>(index), SCRIPT_THING_SEARCH_LIGHT);
    if (actualIdx < 0) {
        return false;
    }
    const CVector pos = entity.GetPosition();
    return IsLookingAtPos(pos, actualIdx);
}
