#include "StdInc.h"

#include "AttractorScanner.h"
#include "Scripted2dEffects.h"
#include "EventAttractor.h"
#include "EventScriptedAttractor.h"
#include "PedAttractorManager.h"

void CAttractorScanner::InjectHooks() {
    RH_ScopedClass(CAttractorScanner);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Clear, 0x5FFF90);
    RH_ScopedInstall(ScanForAttractorsInRange, 0x6060A0);
    RH_ScopedInstall(ScanForAttractorsInPtrList<CPtrListSingleLink<CEntity*>>, 0x6034B0);
    RH_ScopedInstall(AddEffect, 0x5FFFD0);
    RH_ScopedInstall(GetNearestAttractorInRange, 0x600180);
    RH_ScopedInstall(GetNearestPedNotUsingAttractor, 0x603570);
}

// Search distance for attractors (0x86C52C)
constexpr auto ATTRACTOR_SEARCH_DISTANCE = 15.0f;

// 0x5FE960 - tUserList::IsMemberByPedType (Inlined here, as it's trivial)
static bool IsUserListMemberByPedType(const tUserList& list, int32 pedType) {
    for (auto i = 0u; i < list.m_UserTypes.size(); i++) {
        if (list.m_UserTypes[i] == -2 && list.m_UserTypesByPedType[i] == pedType) {
            return true;
        }
    }
    return false;
}

// 0x6034B0
template<typename PtrListType>
void CAttractorScanner::ScanForAttractorsInPtrList(PtrListType& ptrList, const CPed& ped) {
    // NOTE: Not reset per-entity, so non-object entities use the value of the last processed object (Original behaviour)
    bool bEnableDisabledAttractors = false;

    for (auto* const entity : ptrList) {
        if (entity->GetIsTypeObject()) {
            const auto obj = entity->AsObject();
            bEnableDisabledAttractors = obj->objectFlags.bEnableDisabledAttractors;
            if (!entity->m_bIsStatic && !entity->m_bIsStaticWaitingForCollision) {
                continue;
            }
            if (obj->objectFlags.bIsExploded) {
                continue;
            }
        }

        const auto mi = entity->GetModelInfo();
        for (int32 i = 0; i < mi->m_n2dfxCount; i++) {
            const auto effect = mi->Get2dEffect(i);
            if (effect->m_Type != EFFECT_ATTRACTOR) {
                continue;
            }
            if ((effect->pedAttractor.m_nFlags & 1) && !bEnableDisabledAttractors) {
                continue;
            }
            AddEffect(effect, entity, ped);
        }
    }
}

// 0x5FFFD0
void CAttractorScanner::AddEffect(C2dEffect* effect, CEntity* entity, const CPed& ped) {
    const auto type = effect->pedAttractor.m_nAttractorType;

    // When it's raining peds are only interested in shelters, otherwise in everything but shelters.
    if (CWeather::Rain < 0.2f) {
        if (type == PED_ATTRACTOR_SHELTER) {
            return;
        }
    } else if (type != PED_ATTRACTOR_SHELTER) {
        return;
    }

    const auto effectPos = entity
        ? entity->TransformFromObjectSpace(effect->m_Pos)
        : effect->m_Pos;
    const auto distSq = (ped.GetPosition() - effectPos).SquaredMagnitude();
    if (distSq >= m_MinDistSq[type]) {
        return;
    }

    const auto attractorFx = reinterpret_cast<C2dEffectPedAttractor*>(effect);

    if (type == PED_ATTRACTOR_SCRIPTED) {
        const auto radius = CScripted2dEffects::ms_radii[CScripted2dEffects::GetIndex(attractorFx)];
        if (radius >= 0.f && distSq >= sq(radius)) {
            return;
        }
    }

    // 0x6000F2
    if (!GetPedAttractorManager()->HasEmptySlot(attractorFx, entity)) {
        return;
    }

    CMatrix mat;
    if (entity) {
        mat = entity->GetMatrix();
    } else {
        mat.SetScale(1.f);
    }

    if (CPedAttractorManager::IsApproachable(attractorFx, mat, 0, const_cast<CPed*>(&ped))) {
        m_Entities[type]  = entity;
        m_MinDistSq[type] = distSq;
        m_Effects[type]   = effect;
    }
}

// 0x600180
void CAttractorScanner::GetNearestAttractorInRange(C2dEffect*& outEffect, CEntity*& outEntity) {
    outEffect = nullptr;
    outEntity = nullptr;

    // Shelters (if there's any) take priority
    if (m_Entities[PED_ATTRACTOR_SHELTER]) {
        outEffect = m_Effects[PED_ATTRACTOR_SHELTER];
        outEntity = m_Entities[PED_ATTRACTOR_SHELTER];
        return;
    }

    auto minDistSq = std::numeric_limits<float>::max();
    for (auto i = 0; i < 10; i++) {
        if (m_MinDistSq[i] < minDistSq && m_Effects[i]) {
            minDistSq = m_MinDistSq[i];
            outEffect = m_Effects[i];
            outEntity = m_Entities[i];
        }
    }
}

// 0x603570
CPed* CAttractorScanner::GetNearestPedNotUsingAttractor(C2dEffect* effect) {
    const auto attractorFx = reinterpret_cast<C2dEffectPedAttractor*>(effect);

    CPed* nearestPed{};
    auto  minDistSq = std::numeric_limits<float>::max();

    const auto pool = GetPedPool();
    for (auto i = (int32)pool->GetSize(); i-- > 0;) {
        const auto ped = pool->GetAt(i);
        if (!ped) {
            continue;
        }

        if (const auto activeTask = ped->GetIntelligence()->GetTaskManager().GetActiveTask()) {
            if (activeTask->GetTaskType() == TASK_COMPLEX_USE_EFFECT) {
                continue;
            }
        }

        const auto distSq = (effect->m_Pos - ped->GetPosition()).SquaredMagnitude();
        if (distSq >= minDistSq) {
            continue;
        }

        const auto& userList = CScripted2dEffects::ms_userLists[CScripted2dEffects::GetIndex(attractorFx)];
        if (userList.m_bUseList) {
            const auto isUserByModel = rng::contains(userList.m_UserTypes, static_cast<int32>(static_cast<int16>(ped->m_nModelIndex)));
            if (!isUserByModel && !IsUserListMemberByPedType(userList, static_cast<int32>(ped->m_nPedType))) {
                continue;
            }
        }

        CMatrix mat;
        mat.SetScale(1.f);
        if (CPedAttractorManager::IsApproachable(attractorFx, mat, 0, ped)) {
            minDistSq  = distSq;
            nearestPed = ped;
        }
    }

    return nearestPed;
}

// 0x5FFF90
void CAttractorScanner::Clear() {
    for (auto i = 0; i < 10; i++) {
        m_Entities[i] = nullptr;
        m_Effects[i]  = nullptr;
        switch (i) {
        case PED_ATTRACTOR_SHELTER:
        case PED_ATTRACTOR_SCRIPTED:
            m_MinDistSq[i] = sq(ATTRACTOR_SEARCH_DISTANCE);
            break;
        default:
            m_MinDistSq[i] = sq(5.0f);
            break;
        }
    }
}

// 0x6060A0
void CAttractorScanner::ScanForAttractorsInRange(const CPed& ped) {
    if (!m_bActivated) {
        return;
    }
    if (ped.bInVehicle) {
        return;
    }

    // Don't scan (for a while) if the ped is already using an effect
    if (const auto activeTask = ped.GetIntelligence()->GetTaskManager().GetActiveTask()) {
        if (activeTask->GetTaskType() == TASK_COMPLEX_USE_EFFECT) {
            m_Timer.Start(3000);
            return;
        }
    }

    m_Timer.StartIfNotAlready(1500);
    if (!m_Timer.IsOutOfTime()) {
        return;
    }
    m_Timer.Start(1500);

    Clear();

    // 0x60617B - Scan the sectors around the ped
    const auto& pedPos = ped.GetPosition();
    const auto  minX   = std::max(CWorld::GetSectorX(pedPos.x - ATTRACTOR_SEARCH_DISTANCE), 0);
    const auto  minY   = std::max(CWorld::GetSectorY(pedPos.y - ATTRACTOR_SEARCH_DISTANCE), 0);
    const auto  maxX   = std::min(CWorld::GetSectorX(pedPos.x + ATTRACTOR_SEARCH_DISTANCE), MAX_SECTORS_X - 1);
    const auto  maxY   = std::min(CWorld::GetSectorY(pedPos.y + ATTRACTOR_SEARCH_DISTANCE), MAX_SECTORS_Y - 1);
    for (auto y = minY; y <= maxY; y++) {
        for (auto x = minX; x <= maxX; x++) {
            ScanForAttractorsInPtrList(CWorld::GetSector(x, y).Buildings, ped);
            ScanForAttractorsInPtrList(CWorld::GetRepeatSector(x, y).Objects, ped);
        }
    }

    // 0x606411 - Scripted effects
    for (auto i = 0u; i < NUM_SCRIPTED_2D_EFFECTS; i++) {
        if (!CScripted2dEffects::ms_activated[i]) {
            continue;
        }
        const auto& userList = CScripted2dEffects::ms_userLists[i];
        if (userList.m_bUseList) {
            const auto isUserByModel = rng::contains(userList.m_UserTypes, static_cast<int32>(static_cast<int16>(ped.m_nModelIndex)));
            if (!isUserByModel && !IsUserListMemberByPedType(userList, static_cast<int32>(ped.m_nPedType))) {
                continue;
            }
        }
        AddEffect(&CScripted2dEffects::ms_effects[i], nullptr, ped);
    }

    // 0x60647F - Now find the nearest attractor, and (try to) make the ped use it
    C2dEffect* effect{};
    CEntity*   entity{};
    GetNearestAttractorInRange(effect, entity);
    if (!effect) {
        return;
    }
    if (effect == m_pEffectInUse && entity == m_pPreviousEntity) {
        return;
    }

    const auto AddEvent = [&](CEvent& event) {
        if (ped.GetIntelligence()->GetEventGroup().Add(&event, false)) {
            m_pPreviousEntity = entity;
            m_pEffectInUse    = effect;
        }
    };

    const auto attractorFx = reinterpret_cast<C2dEffectPedAttractor*>(effect);
    if (effect->pedAttractor.m_nAttractorType == PED_ATTRACTOR_SCRIPTED) {
        if (GetNearestPedNotUsingAttractor(effect) == &ped) {
            CEventScriptedAttractor event{ attractorFx, entity, false };
            AddEvent(event);
        }
    } else {
        CEventAttractor event{ attractorFx, entity, false };
        AddEvent(event);
    }
}
