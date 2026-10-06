#include "StdInc.h"
#include "MentalHealth.h"

#include "Events/EventHealthReallyLow.h"
#include "Events/EventHealthLow.h"
#include "Events/EventHighAngerAtPlayer.h"
#include "Events/EventLowAngerAtPlayer.h"

// 0x421050
// NOTE: In the binary this address is actually `CPedIntelligence::IncrementAngerAtPlayer` (`this` is the `CPedIntelligence`), the code below is the same logic applied on the mental state itself.
void CMentalState::IncrementAnger(int32 anger) {
    if (m_AngerTimer.IsOutOfTime()) {
        m_AngerTimer.Start(3'000);
        m_AngerAtPlayer += (uint8)anger;
    }
}

// 0x6008A0
void CMentalState::Process(const CPed& ped) {
    auto& eg = ped.GetIntelligence()->GetEventGroup();

    m_pedHealth = (uint8)(int32)ped.m_fHealth;
    if (ped.bInVehicle && ped.m_pVehicle) {
        m_vehicleHealth = (uint8)(int32)ped.m_pVehicle->m_fHealth; // NOTE: Truncated, vehicle health goes up to 1000
    }

    if (!ped.bInVehicle) {
        if (m_oldPedHealth >= 50) {
            if (m_pedHealth < 10) {
                CEventHealthReallyLow event{};
                eg.Add(&event);
            } else if (m_pedHealth < 50) {
                CEventHealthLow event{};
                eg.Add(&event);
            }
        } else if (m_oldPedHealth >= 10 && m_pedHealth < 10) {
            CEventHealthReallyLow event{};
            eg.Add(&event);
        }
    } else {
        // 0x600994 - Originally the same thing was done here for the vehicle's health (using 600 and 300 as thresholds),
        // but since the values are stored as `uint8` those checks can never pass, so no event is ever added here.
    }

    // 0x600A43
    if (m_LastAngerAtPlayer <= 3) {
        if (m_AngerAtPlayer > 6) {
            CEventHighAngerAtPlayer event{};
            eg.Add(&event);
        } else if (m_AngerAtPlayer > 3) {
            CEventLowAngerAtPlayer event{};
            eg.Add(&event);
        }
    } else if (m_LastAngerAtPlayer <= 6 && m_AngerAtPlayer > 6) {
        CEventHighAngerAtPlayer event{};
        eg.Add(&event);
    }

    m_LastAngerAtPlayer = m_AngerAtPlayer;
    m_oldPedHealth      = m_pedHealth;
    m_oldVehicleHealth  = m_vehicleHealth;
}
