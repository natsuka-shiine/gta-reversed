#include "StdInc.h"

#include "EventSource.h"

void CEventSource::InjectHooks() {
    RH_ScopedClass(CEventSource);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(ComputeEventSourceType, 0x4ABAC0);
}

// 0x4ABAC0
int32 CEventSource::ComputeEventSourceType(const CEvent& event, const CPed& ped) {
    const auto source = event.GetSourceEntity();
    if (!source || !source->GetIsTypePed()) {
        return 0; // Unknown/Neutral
    }
    const auto sourcePed = static_cast<const CPed*>(source);

    const auto intel = ped.GetIntelligence();
    if (intel->IsThreatenedBy(*sourcePed)) {
        return 3; // Threat
    }
    if (intel->IsFriendlyWith(*sourcePed)) {
        return 2; // Friend
    }
    return sourcePed->IsPlayer() ? 1 : 0; // Player / Neutral
}
