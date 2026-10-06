/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "StdInc.h"

#include "Rope.h"
#include "Ropes.h"

void CRopes::InjectHooks() {
    RH_ScopedClass(CRopes);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x555DC0);
    RH_ScopedInstall(Shutdown, 0x556B10);
    RH_ScopedInstall(Update, 0x558D70);
    RH_ScopedInstall(Render, 0x556AE0);
    RH_ScopedInstall(RegisterRope, 0x556B40);
    RH_ScopedInstall(FindPickupHeight, 0x556760);
    RH_ScopedInstall(FindRope, 0x556000);
    RH_ScopedInstall(FindCoorsAlongRope, 0x555E40);
    RH_ScopedInstall(CreateRopeForSwatPed, 0x558D10);
    RH_ScopedInstall(IsCarriedByRope, 0x555F80);
    RH_ScopedInstall(SetSpeedOfTopNode, 0x555DF0);
}

// 0x555DC0
void CRopes::Init() {
    for (auto& rope : aRopes) {
        rope.m_nType = eRopeType::NONE;
    }
    PlayerControlsCrane = eControlledCrane::NONE;
}

// 0x556B10
void CRopes::Shutdown() {
    for (auto& rope : aRopes) {
        if (rope.m_nType == eRopeType::NONE)
            continue;

        rope.Remove();
    }
}

// 0x558D70
void CRopes::Update() {
    ZoneScoped;

    if (CReplay::Mode == MODE_PLAYBACK)
        return;

    for (auto& rope : aRopes) {
        if (rope.m_nType != eRopeType::NONE)
            rope.Update();
    }
}

// 0x556AE0
void CRopes::Render() {
    ZoneScoped;

    for (auto& rope : aRopes) {
        if (rope.m_nType != eRopeType::NONE)
            rope.Render();
    }
}

// Must be used in loop to make attached to holder
// 0x556B40
bool CRopes::RegisterRope(uint32 ropeID, uint32 ropeType, CVector startPos, bool bExpires, uint8 segmentCount, uint8 flags, CPhysical* holder, uint32 timeExpire) {
    // Already registered - Just refresh it
    for (auto& rope : aRopes) {
        if (rope.m_nType == eRopeType::NONE || rope.m_nId != ropeID) {
            continue;
        }
        rope.m_aSegments[0] = startPos;
        rope.m_aSpeed[0]    = CVector{};
        rope.m_nFlags2 |= 1;
        rope.m_nSegments = segmentCount;
        assert(segmentCount < NUM_ROPE_SEGMENTS); // NB: Original code doesn't check this, and writes out of bounds
        for (auto s = 0; s <= static_cast<int8>(segmentCount); s++) {
            rope.m_aSegments[s] = startPos;
            rope.m_aSpeed[s]    = CVector{};
        }
        rope.CreateHookObjectForRope();
        return true;
    }

    const auto it = rng::find_if(aRopes, [](const CRope& r) { return r.m_nType == eRopeType::NONE; });
    if (it == aRopes.end()) {
        return false;
    }
    auto* const rope = &*it;

    rope->m_nId               = ropeID;
    rope->m_aSegments[0]      = startPos;
    rope->m_aSpeed[0]         = CVector{};
    rope->m_nSegments         = segmentCount;
    rope->m_nFlags2           = (rope->m_nFlags2 & 0xF9) | ((flags & 1) << 2) | 1;
    rope->m_fGroundZ          = 0.0f;
    rope->m_pAttachedEntity   = nullptr;
    rope->m_pRopeAttachObject = nullptr;
    rope->m_fSegmentLength    = holder && holder->GetIsTypeVehicle() ? 0.9f : 0.5f;
    rope->m_nFlags1           = 0;
    rope->m_pRopeHolder       = holder;
    rope->m_nType             = static_cast<eRopeType>(ropeType);
    if (holder) {
        holder->RegisterReference(&rope->m_pRopeHolder);
    }
    rope->m_nTime = bExpires ? CTimer::GetTimeInMS() + timeExpire : 0;

    // NOTE: `m_fMass` is the total length of the rope, `m_fTotalLength` is the length of one segment (the fields are misnamed)
    switch (rope->m_nType) {
    case eRopeType::MAGNET:
        rope->m_fMass        = 10.0f;
        rope->m_fTotalLength = 10.0f / 31.0f;
        break;
    case eRopeType::CRANE_MAGNO:
        rope->m_fMass        = 50.0f;
        rope->m_fTotalLength = 50.0f / 31.0f;
        break;
    case eRopeType::WRECKING_BALL:
    case eRopeType::QUARRY_CRANE_ARM:
    case eRopeType::CRANE_TROLLEY:
        rope->m_fMass        = 68.0f;
        rope->m_fTotalLength = 68.0f / 31.0f;
        break;
    default:
        rope->m_fMass        = 20.0f;
        rope->m_fTotalLength = 20.0f / 31.0f;
        break;
    }

    if (rope->m_nType >= eRopeType::CRANE_MAGNO && rope->m_nType <= eRopeType::CRANE_TROLLEY) {
        // Crane ropes hang straight down (NB: Their speeds aren't reset)
        for (auto s = 1u; s < NUM_ROPE_SEGMENTS; s++) {
            const auto& prev = rope->m_aSegments[s - 1];
            rope->m_aSegments[s] = CVector{ prev.x, prev.y, prev.z - rope->m_fTotalLength };
        }
    } else {
        // Every other type zigzags along the X axis
        for (auto s = 1u; s < NUM_ROPE_SEGMENTS; s++) {
            const auto& prev = rope->m_aSegments[s - 1];
            rope->m_aSegments[s] = CVector{ (s & 1) ? prev.x + rope->m_fTotalLength : prev.x - rope->m_fTotalLength, prev.y, prev.z };
            rope->m_aSpeed[s]    = CVector{};
        }
    }

    rope->CreateHookObjectForRope();
    return true;
}

// 0x556760
float CRopes::FindPickupHeight(CEntity* entity) {
    return CModelInfo::GetModelInfo(entity->m_nModelIndex)->GetColModel()->GetBoundingBox().m_vecMax.z;
}

// Returns id to array
// 0x556000
int32 CRopes::FindRope(uint32 id) {
    for (auto ropeId = 0; ropeId < MAX_NUM_ROPES; ropeId++) {
        if (aRopes[ropeId].m_nType != eRopeType::NONE && aRopes[ropeId].m_nId == id)
            return ropeId;
    }
    return -1;
}

// a4 always nullptr
// 0x555E40
// fDistAlongRope is a fraction along the rope (callers pass e.g. m_fCoorAlongRope
// advanced by CTimer::GetTimeStep() * 0.003f); the binary clamps it into [0, 0.999].
bool CRopes::FindCoorsAlongRope(uint32 ropeId, float fDistAlongRope, CVector* outPosn, CVector* outSpeed) {
    const auto idx = FindRope(ropeId);
    if (idx < 0)
        return false;
    auto& rope = GetRope(idx);
    const float t = std::clamp(fDistAlongRope, 0.0f, 0.999f);
    const float segFloat = t * 31.0f;
    const auto seg = static_cast<uint32>(segFloat); // binary truncates via float->int conversion
    const float frac = segFloat - static_cast<float>(seg);
    *outPosn = rope.m_aSegments[seg] * (1.0f - frac) + rope.m_aSegments[seg + 1] * frac;
    if (outSpeed)
        *outSpeed = rope.m_aSpeed[seg + 1];
    return true;
}

// 0x558D10
int32 CRopes::CreateRopeForSwatPed(const CVector& startPos) {
    int32 newRopeId = m_nRopeIdCreationCounter + 100;
    if (RegisterRope(newRopeId, static_cast<uint32>(eRopeType::SWAT), startPos, true, 0, 0, nullptr, 4000)) {
        return -1;
    }

    m_nRopeIdCreationCounter += 1;
    return newRopeId;
}

// 0x555F80
bool CRopes::IsCarriedByRope(CPhysical* entity) {
    if (!entity)
        return false;

    for (auto& rope : aRopes) {
        if (rope.m_nType != eRopeType::NONE && rope.m_pRopeAttachObject == entity)
            return true;
    }
    return false;
}

// 0x555DF0
void CRopes::SetSpeedOfTopNode(uint32 ropeId, CVector dirSpeed) {
    for (auto& rope : aRopes) {
        if (rope.m_nType != eRopeType::NONE && rope.m_nId == ropeId) {
            rope.m_aSpeed[0] = dirSpeed;
            return;
        }
    }
}
