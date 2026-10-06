#include "StdInc.h"

#include "Rope.h"
#include "Ropes.h"

void CRope::InjectHooks() {
    RH_ScopedClass(CRope);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(ReleasePickedUpObject, 0x556030);
    RH_ScopedInstall(CreateHookObjectForRope, 0x556070);
    RH_ScopedInstall(UpdateWeightInRope, 0x5561B0);
    RH_ScopedInstall(Remove, 0x556780);
    RH_ScopedInstall(Render, 0x556800);
    RH_ScopedInstall(PickUpObject, 0x5569C0);
    RH_ScopedInstall(Update, 0x557530);
}

// inlined see 0x557959
// use switch like in Android?
// 0x555FB0
bool CRope::DoControlsApply() const {
    return    m_nType == eRopeType::CRANE_MAGNO && CRopes::PlayerControlsCrane == eControlledCrane::MAGNO_CRANE
           || m_nType == eRopeType::WRECKING_BALL && CRopes::PlayerControlsCrane == eControlledCrane::WRECKING_BALL
           || m_nType == eRopeType::CRANE_TROLLEY && CRopes::PlayerControlsCrane == eControlledCrane::LAS_VEGAS_CRANE
           || m_nType == eRopeType::QUARRY_CRANE_ARM && CRopes::PlayerControlsCrane == eControlledCrane::QUARRY_CRANE
           || m_nType == eRopeType::CRANE_MAGNET1
           || m_nType == eRopeType::MAGNET
           || m_nType == eRopeType::CRANE_HARNESS;
}

// 0x556030
void CRope::ReleasePickedUpObject() {
    if (m_pRopeAttachObject) {
        m_pRopeAttachObject->AsPhysical()->physicalFlags.bAttachedToEntity = false;
        m_pRopeAttachObject->AsPhysical()->physicalFlags.bCarriedByRope = false;
        m_pRopeAttachObject = nullptr;
    }
    m_pAttachedEntity->SetUsesCollision(true);
    m_nFlags1 = 60; // 6th, 7th bits set
}

// 0x556070
void CRope::CreateHookObjectForRope() {
    if (m_pAttachedEntity)
        return;

    using namespace ModelIndices;

    const auto modelIndex = [&]() -> ModelIndex {
        switch (m_nType) {
        case eRopeType::CRANE_MAGNET1:
        case eRopeType::CRANE_MAGNO:
        case eRopeType::QUARRY_CRANE_ARM:
        case eRopeType::CRANE_TROLLEY:
            return MI_CRANE_MAGNET;
        case eRopeType::CRANE_HARNESS:
            return MI_CRANE_HARNESS;
        case eRopeType::MAGNET:
            return MI_MINI_MAGNET;
        case eRopeType::WRECKING_BALL:
            return MI_WRECKING_BALL;
        case eRopeType::SWAT:
            return MODEL_INVALID; // Just so the assert below wont be hit.
        default:
            NOTSA_UNREACHABLE(); //assert(0);
        }
    }();
    if (modelIndex == ModelIndex{ MODEL_INVALID }) { // Must do it like this because `ModelIndex` is u16, `MODEL_ID` is i32, and u16 -1 casted to int32 is 0xffff...
        return;
    }

    auto* obj = new CObject(modelIndex, true);
    m_pAttachedEntity = obj;

    obj->RegisterReference(reinterpret_cast<CEntity**>(&m_pAttachedEntity));
    obj->SetPosn(m_aSegments[NUM_ROPE_SEGMENTS - 1]);
    obj->m_nObjectType = OBJECT_TYPE_DECORATION;
    obj->SetIsStatic(false);
    obj->physicalFlags.bAttachedToEntity = true;

    CWorld::Add(m_pAttachedEntity);

    m_pRopeAttachObject = nullptr;
    m_nFlags1 = 0;
}

// 0x5561B0
// Moves the end (weight) of the rope to the given position, and makes the rest of the rope follow it.
// Returns `true` if the rope is fully stretched (in which case `a6` receives the furthest position the weight can be at).
// NOTE: `a2`, `a3`, `a4` is the position of the weight, `a5` (Actually a float: Relative weight) is unused, `a6` is actually a `CVector*`
int8 CRope::UpdateWeightInRope(float a2, float a3, float a4, int32 a5, float* a6) {
    constexpr auto LAST_SEGMENT = (int32)(NUM_ROPE_SEGMENTS - 1);

    // NOTE: The member at 0x30C (`m_fTotalLength` in the header) is actually the length of one segment
    const auto segmentLength = m_fTotalLength;

    // NOTE: `m_nSegments` is actually the index of the node the rope is fixed at
    const auto fixedNode = (int32)m_nSegments;
    const auto fixedPos  = m_aSegments[fixedNode];

    const CVector weightPos{ a2, a3, a4 };
    m_aSegments[LAST_SEGMENT] = weightPos;

    const auto maxLength = (float)(LAST_SEGMENT - fixedNode) * segmentLength;
    auto       fixedToWeight = weightPos - fixedPos;

    // 0x556262 - Rope is fully stretched, so it's just a straight line
    if (const auto dist = fixedToWeight.Magnitude(); dist >= maxLength) {
        *reinterpret_cast<CVector*>(a6) = fixedToWeight * (maxLength / dist) + fixedPos;
        for (auto i = fixedNode + 1; i <= LAST_SEGMENT; i++) {
            fixedToWeight.Normalise();
            fixedToWeight *= segmentLength;
            m_aSegments[i] = fixedToWeight * (float)(i - fixedNode) + fixedPos;
        }
        return true;
    }

    // 0x556279 - Otherwise relax the rope
    for (auto iter = 0; iter < 6; iter++) {
        // Pull segments towards the weight (Starting from the weight)
        for (auto j = LAST_SEGMENT; j > fixedNode + 1; j--) {
            const auto& curr = m_aSegments[j];
            auto&       prev = m_aSegments[j - 1];

            const auto prevOld = prev;
            const auto dist    = (curr - prev).Magnitude();
            if (dist <= segmentLength) {
                break;
            }
            prev            = (prev - curr) * (segmentLength / dist) + curr;
            m_aSpeed[j - 1] = (prev - prevOld) * (1.0f / CTimer::GetTimeStep());
        }

        // Pull segments towards the fixed node (Starting from the fixed node)
        for (auto j = fixedNode + 1; j < LAST_SEGMENT; j++) {
            const auto& prev = m_aSegments[j - 1];
            auto&       curr = m_aSegments[j];

            const auto delta = curr - prev;
            if (const auto dist = delta.Magnitude(); dist > segmentLength) {
                curr = delta * (segmentLength / dist) + prev;
            }
        }
    }

    // 0x556660 - And once more towards the weight (Without affecting the speed this time)
    for (auto j = LAST_SEGMENT; j > fixedNode + 1; j--) {
        const auto& curr = m_aSegments[j];
        auto&       prev = m_aSegments[j - 1];

        const auto dist = (curr - prev).Magnitude();
        if (dist <= segmentLength) {
            break;
        }
        prev = (prev - curr) * (segmentLength / dist) + curr;
    }

    return false;
}

// 0x556780
void CRope::Remove() {
    m_nType = eRopeType::NONE;
    if (m_pRopeAttachObject)
        ReleasePickedUpObject();

    if (m_pAttachedEntity) {
        CWorld::Remove(m_pAttachedEntity);
        delete m_pAttachedEntity;
        m_pAttachedEntity = nullptr;
    }
}

// 0x556800
void CRope::Render() {
    // Note: Probably needs adjustments if `NUM_ROPE_SEGMENTS` is changed
    if (!TheCamera.IsSphereVisible(m_aSegments[NUM_ROPE_SEGMENTS / 2], 20.0f))
        return;

    if ((TheCamera.GetPosition() - m_aSegments[0]).Magnitude2D() >= 120.0f)
        return;

    DefinedState();

    const auto GetVertex = [](unsigned i) {
        return &TempBufferVertices.m_3d[i];
    };

    const RwRGBA color = { 0, 0, 0, 128 };
    for (auto i = 0u; i < NUM_ROPE_SEGMENTS; i++) {
        RxObjSpace3DVertexSetPreLitColor(GetVertex(i), &color);
        RxObjSpace3DVertexSetPos(GetVertex(i), &m_aSegments[i]);
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER,     RWRSTATE(rwFILTERLINEAR));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,     RWRSTATE(FALSE));

    if (RwIm3DTransform(TempBufferVertices.m_3d, NUM_ROPE_SEGMENTS, nullptr, 0)) {
        RxVertexIndex indices[] = { // *(RxVertexIndex(*)[64])0x8CD818
            0,  1,  1,  2,  2,  3,  3,  4,
            4,  5,  5,  6,  6,  7,  7,  8,
            8,  9,  9,  10, 10, 11, 11, 12,
            12, 13, 13, 14, 14, 15, 15, 16,
            16, 17, 17, 18, 18, 19, 19, 20,
            20, 21, 21, 22, 22, 23, 23, 24,
            24, 25, 25, 26, 26, 27, 27, 28,
            28, 29, 29, 30, 30, 31, 31, 32
        };
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPELINELIST, indices, std::size(indices) - 2); // the last two indexes are not used
        RwIm3DEnd();
    }

    if (m_nType == eRopeType::QUARRY_CRANE_ARM) {
        const CVector pos[] = { m_aSegments[0], { 709.32f, 916.20f, 53.0f } }; // Hunter Quarry
        for (auto i = 0u; i < std::size(pos); i++) {
            RxObjSpace3DVertexSetPreLitColor(GetVertex(i), &color);
            RxObjSpace3DVertexSetPos(GetVertex(i), &pos[i]);
        }
        if (RwIm3DTransform(TempBufferVertices.m_3d, std::size(pos), nullptr, 0)) {
            RxVertexIndex indices[] = { 0, 1 };
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPELINELIST, indices, std::size(indices));
            RwIm3DEnd();
        }
    }
}

// 0x5569C0
void CRope::PickUpObject(CEntity* obj) {
    if (m_pRopeAttachObject == obj)
        return;

    if (m_pRopeAttachObject)
        ReleasePickedUpObject();

    obj->RegisterReference(&m_pAttachedEntity);
    m_pRopeAttachObject = obj;

    // TODO: Move model => world space translation into CEntity
    // MultiplyMatrixWithVector should be used here
    CVector height = { {}, {}, CRopes::FindPickupHeight(obj) };
    m_pAttachedEntity->SetPosn(obj->GetPosition() + obj->GetMatrix().TransformVector(height));
    m_pAttachedEntity->SetUsesCollision(false);

    obj->AsPhysical()->physicalFlags.bAttachedToEntity = true;
    if (obj->GetIsTypeVehicle()) {
        if (obj->GetStatus() == STATUS_SIMPLE)
        {
            obj->SetStatus(STATUS_PHYSICS);
        }
    } else if (obj->GetIsTypeObject()) {
        if (obj->GetIsStatic()) {
            obj->AsObject()->SetIsStatic(false);
            obj->AsObject()->AddToMovingList();
            obj->AsObject()->m_nFakePhysics = 0;
        }
    }
}

// 0x557530
// NOTE: Some members are misnamed in the header (not renamed here, other files use them):
//  - `m_fMass`          (0x308) is the total length of the rope
//  - `m_fTotalLength`   (0x30C) is the length of one segment
//  - `m_fSegmentLength` (0x31C) is the winch height (0.01 - 0.9)
//  - `m_nSegments`      (0x324) is the index of the node the rope is fixed at
//  - `m_nFlags1`        (0x326) is a timer (in frames) during which the winch can't pick up anything
//  - `m_nFlags2`        (0x327) are the actual flags: 1 = updated this frame (kept alive), 4 = do ground checks
void CRope::Update() {
    constexpr auto LAST_SEGMENT = (int32)(NUM_ROPE_SEGMENTS - 1);

    auto& winchHeight        = m_fSegmentLength;
    auto& winchDisabledTimer = m_nFlags1;

    const auto damping = std::pow(0.8f, CTimer::GetTimeStep());

    if ((TheCamera.GetPosition() - m_aSegments[0]).Magnitude2D() >= 200.0f) {
        return;
    }

    // 0x5575B6 - Not kept alive anymore, the top of the rope falls
    if (!(m_nFlags2 & 1) && CTimer::GetTimeInMS() > m_nTime) {
        m_aSpeed[0].z -= CTimer::GetTimeStep() * 0.0015f;
        m_aSegments[0] += m_aSpeed[0] * CTimer::GetTimeStep();
    }

    // 0x557611
    if ((m_nFlags2 & 4) && (CTimer::GetFrameCounter() & 7) == 2) {
        m_fGroundZ = CWorld::FindGroundZFor3DCoord(m_aSegments[0], nullptr, nullptr);
    }

    // 0x557647
    if ((m_nFlags2 & 4) && m_pRopeAttachObject) {
        const auto mi = m_pRopeAttachObject->m_nModelIndex;
        if (   m_pRopeAttachObject->GetIsTypeVehicle()
            || mi == ModelIndices::MI_OBJECTFORMAGNOCRANE1
            || mi == ModelIndices::MI_OBJECTFORMAGNOCRANE2
            || mi == ModelIndices::MI_OBJECTFORMAGNOCRANE3
            || mi == ModelIndices::MI_OBJECTFORMAGNOCRANE5
        ) {
            m_fGroundZ = m_pAttachedEntity->GetPosition().z - 0.5f;
        }
    }

    // Value of the stack slot at `esp+0x2C`. The compiler shares it between several vectors (`z` component),
    // and the ped pickup code below (0x5586DB) reads it without it being set to anything meaningful there. (Original bug)
    float staleZ = 0.0f;

    // 0x5576AE - Simulate the free part of the rope
    for (auto i = (int32)m_nSegments + 1; i <= LAST_SEGMENT; i++) {
        auto& pos   = m_aSegments[i];
        auto& speed = m_aSpeed[i];

        const auto oldPos = pos;
        staleZ = oldPos.z;

        speed.x += (float)((int32)(CGeneral::GetRandomNumber() & 0xF) - 8) * 0.001f;
        speed.y += (float)((int32)(CGeneral::GetRandomNumber() & 0xF) - 8) * 0.001f;
        speed = speed * damping + m_aSpeed[i - 1] * (1.0f - damping);

        pos.z -= CTimer::GetTimeStep() * 0.15f;
        if (m_nFlags2 & 4) {
            pos.z = std::max(pos.z, m_fGroundZ + 0.3f);
        }

        const auto& prev  = m_aSegments[i - 1];
        const auto  delta = pos - prev;
        pos = prev + delta * (m_fTotalLength / delta.Magnitude());

        speed = (pos - oldPos) * (1.0f / CTimer::GetTimeStep());
    }

    // 0x55792B - Winch
    const auto ProcessWinch = [&] {
        switch (m_nType) {
        case eRopeType::CRANE_MAGNET1:
        case eRopeType::CRANE_HARNESS:
        case eRopeType::MAGNET:
        case eRopeType::CRANE_MAGNO:
        case eRopeType::WRECKING_BALL:
        case eRopeType::QUARRY_CRANE_ARM:
        case eRopeType::CRANE_TROLLEY:
            break;
        default:
            return;
        }

        const auto IsCraneType = [this] {
            switch (m_nType) {
            case eRopeType::CRANE_MAGNO:
            case eRopeType::WRECKING_BALL:
            case eRopeType::QUARRY_CRANE_ARM:
            case eRopeType::CRANE_TROLLEY:
                return true;
            default:
                return false;
            }
        };

        // 0x557959 - Raise/lower
        if (DoControlsApply()) {
            if (!IsCraneType()) { // 0x557B04 - Helis
                const auto upDown = CPad::GetPad(0)->GetCarGunUpDown();
                if (upDown < 0 ? CTheScripts::bEnableCraneRaise : (upDown > 0 && CTheScripts::bEnableCraneLower)) {
                    winchHeight -= (float)upDown * CTimer::GetTimeStep() * 0.00001f;
                }
                winchHeight = std::min(winchHeight, 0.84f);
            } else {
                if (CTheScripts::bEnableCraneRaise) { // 0x5579AD
                    const auto timeStep = CTimer::GetTimeStep();
                    const auto change   = (float)CPad::GetPad(0)->NewState.ButtonSquare * timeStep * 0.00001f;
                    if (IsCraneType() && change > 0.0f && change + winchHeight < 0.9f) {
                        AudioEngine.ReportMissionAudioEvent(AE_CRANE_WINCH_MOVE, m_pRopeHolder->AsPhysical(), 0.0f, 1.0f);
                    }
                    winchHeight += change;
                }
                if (CTheScripts::bEnableCraneLower) { // 0x557A58
                    const auto timeStep = CTimer::GetTimeStep();
                    const auto change   = (float)CPad::GetPad(0)->NewState.ButtonCross * timeStep * 0.00001f;
                    if (IsCraneType() && change > 0.0f && winchHeight - change > 0.01f) {
                        AudioEngine.ReportMissionAudioEvent(AE_CRANE_WINCH_MOVE, m_pRopeHolder->AsPhysical(), 0.0f, 1.0f);
                    }
                    winchHeight -= change;
                }
            }
        }

        // 0x557B87
        winchHeight = std::min(std::max(winchHeight, 0.01f), 0.9f);

        // 0x557BBD - The weight at the end of the rope is either the object carried or the hook itself
        CPhysical* weight;
        float      relativeWeight;
        if (m_pRopeAttachObject) {
            weight = m_pRopeAttachObject->AsPhysical();
            const auto mass = weight->m_nModelIndex == MODEL_SECURICA
                ? 750.0f
                : weight->m_fMass;
            relativeWeight = std::min(mass * (0.13f * 0.000833333354f) + 0.06f, 0.5f); // 0x8CD89C, 0x863E30, 0x8CD898
            weight->m_nFakePhysics = 0;
        } else {
            weight         = m_pAttachedEntity->AsPhysical();
            relativeWeight = 0.06f; // 0x8CD898
            weight->SetUsesCollision(true);
        }

        if (weight) { // 0x557C4D
            // 0x557C89 - NOTE: 4th argument is actually a float (0.1f), unused by the callee
            CVector stretchedPos;
            const auto weightPosBefore = weight->GetPosition();
            if (UpdateWeightInRope(weightPosBefore.x, weightPosBefore.y, weightPosBefore.z, 0x3DCCCCCD, reinterpret_cast<float*>(&stretchedPos))) {
                weight->SetPosn(stretchedPos);

                auto* const holder = m_pRopeHolder->AsPhysical();

                // 0x557D37
                auto holderSpeed = holder->GetSpeed(m_aSegments[0] - holder->GetPosition());
                if (IsCraneType()) {
                    holderSpeed = CVector{ 0.0f, 0.0f, 0.0f };
                }

                // 0x557D82 - Remove the part of the (relative) speed that would stretch the rope even more
                auto relSpeed = weight->m_vecMoveSpeed - holderSpeed;
                auto ropeDir  = weight->GetPosition() - m_aSegments[0];
                ropeDir.Normalise();
                if (const auto stretchSpeed = DotProduct(relSpeed, ropeDir); stretchSpeed > 0.0f) {
                    relSpeed -= ropeDir * stretchSpeed;
                }
                staleZ = relSpeed.z;

                // 0x557E3B - Distribute the change of speed between the weight and the holder
                const auto speedChange = holderSpeed + relSpeed - weight->m_vecMoveSpeed;
                weight->m_vecMoveSpeed += speedChange * (1.0f - relativeWeight);
                holder->m_vecMoveSpeed -= speedChange * relativeWeight;

                // 0x557EF0 - Make the weight hang in the direction of the rope
                if (!m_pRopeAttachObject) {
                    weight->GetMatrix().ForceUpVector(-ropeDir);
                } else {
                    auto& mat = weight->GetMatrix();
                    const CMatrix before{ mat };
                    mat.ForceUpVector(-ropeDir);
                    const CMatrix after{ mat };
                    mat.GetRight()   = before.GetRight() * 0.9f + after.GetRight() * 0.1f;
                    mat.GetForward() = before.GetForward() * 0.9f + after.GetForward() * 0.1f;
                    mat.GetUp()      = before.GetUp() * 0.9f + after.GetUp() * 0.1f;
                }
            }

            // 0x558121 - The hook follows the carried object
            if (m_pRopeAttachObject) {
                auto* const carried = m_pRopeAttachObject->AsPhysical();
                auto* const hook    = m_pAttachedEntity->AsPhysical();
                hook->GetMatrix() = carried->GetMatrix();
                hook->m_vecMoveSpeed = carried->m_vecMoveSpeed;
                hook->SetPosn(carried->GetMatrix().TransformPoint(CVector{ 0.0f, 0.0f, CRopes::FindPickupHeight(carried) }));
            }
        }

        const auto ropeEnd = m_aSegments[LAST_SEGMENT];

        // 0x5581E4 - Something is carried, check if it should be released
        if (m_pRopeAttachObject) {
            if (DoControlsApply() && CTheScripts::bEnableCraneRelease && CPad::GetPad(0)->CarGunJustDown()) {
                ReleasePickedUpObject();
            }
            if (m_pRopeAttachObject && m_pRopeAttachObject->AsPhysical()->physicalFlags.bRenderScorched) { // 0x55824F
                ReleasePickedUpObject(); // Inlined
            }
            if (m_pRopeAttachObject && m_pRopeAttachObject->GetIsTypeVehicle()) { // 0x558299
                const auto* const veh = m_pRopeAttachObject->AsVehicle();
                if (veh->m_nVehicleSubType == VEHICLE_TYPE_BIKE && veh->m_pDriver) {
                    ReleasePickedUpObject();
                }
            }
            return;
        }

        // 0x5582D4 - Nothing is carried, but something was released recently
        if (winchDisabledTimer) {
            winchDisabledTimer--;
            return;
        }

        // 0x5582EB - Nothing is carried, try picking up something
        const auto IsInPickUpRange = [&](const CVector& pos) {
            return (pos - ropeEnd).Magnitude() < 2.5f;
        };
        const auto GetPickUpPos = [](CEntity* entity) {
            return entity->GetMatrix().TransformPoint(CVector{ 0.0f, 0.0f, CRopes::FindPickupHeight(entity) });
        };
        const auto ShortenRopeBy = [&](float dist) { // Makes the rope shorter, so the hook is at the height of the entity picked up
            if (dist > 0.0f) {
                const auto invRopeLength = 1.0f / m_fMass;
                winchHeight = std::max(winchHeight - dist * invRopeLength - invRopeLength * m_fTotalLength, 0.01f);
            }
        };

        bool onlyRCTiger{}, pickMagnoCraneObjs{}, pickBuildingSiteObjs{}, pickQuarryObjs{}, pickMiniMagnetObjs{};
        switch (m_nType) {
        case eRopeType::CRANE_MAGNET1: // 0x55835C
            break;
        case eRopeType::CRANE_HARNESS: { // 0x5585A4 - Picks up peds only
            auto* const pool = GetPedPool();
            for (auto i = (int32)pool->GetSize() - 1; i >= 0; i--) {
                auto* const ped = pool->GetAt(i);
                if (!ped || ped->m_nPedState == PEDSTATE_DEAD || ped->IsPlayer() || ped->bInVehicle) {
                    continue;
                }
                if (!IsInPickUpRange(ped->GetPosition())) {
                    continue;
                }
                m_pRopeAttachObject = ped;
                ped->RegisterReference(&m_pRopeAttachObject);
                m_pRopeAttachObject->AsPhysical()->physicalFlags.bCarriedByRope = true;
                m_pAttachedEntity->SetPosn(GetPickUpPos(m_pRopeAttachObject));
                m_pAttachedEntity->SetUsesCollision(false);
                ShortenRopeBy(ropeEnd.z - staleZ); // 0x5586DB - See note at `staleZ`
                break;
            }
            return;
        }
        case eRopeType::MAGNET: // 0x558343
            pickMiniMagnetObjs = true;
            onlyRCTiger        = true;
            break;
        case eRopeType::CRANE_MAGNO: // 0x55832B
            pickMagnoCraneObjs = true;
            break;
        case eRopeType::WRECKING_BALL: // 0x558CA3
            return;
        case eRopeType::QUARRY_CRANE_ARM: // 0x55833C
            pickQuarryObjs = true;
            break;
        case eRopeType::CRANE_TROLLEY: // 0x558332
            pickBuildingSiteObjs = true;
            break;
        default:
            return;
        }

        // 0x55835C - Vehicles
        if (m_nType != eRopeType::CRANE_TROLLEY) {
            auto* const pool = GetVehiclePool();
            for (auto i = (int32)pool->GetSize() - 1; i >= 0; i--) {
                auto* const veh = pool->GetAt(i);
                if (!veh) {
                    continue;
                }
                const auto isPickableType = [veh] {
                    switch (veh->m_nVehicleSubType) {
                    case VEHICLE_TYPE_AUTOMOBILE:
                    case VEHICLE_TYPE_MTRUCK:
                        return true;
                    case VEHICLE_TYPE_BIKE:
                        if (!veh->m_pDriver) {
                            return true;
                        }
                        break;
                    }
                    return veh->m_nModelIndex == MODEL_DINGHY || veh->m_nModelIndex == MODEL_VORTEX;
                }();
                if (!isPickableType) {
                    continue;
                }
                if (veh->physicalFlags.bRenderScorched || !veh->vehicleFlags.bWinchCanPickMeUp) {
                    continue;
                }
                if (onlyRCTiger && veh->m_nModelIndex != MODEL_RCTIGER) {
                    continue;
                }
                if (veh->m_ropeType != 0) { // Has a winch itself
                    continue;
                }
                const auto pickUpPos = GetPickUpPos(veh);
                if (!IsInPickUpRange(pickUpPos)) {
                    continue;
                }
                m_pRopeAttachObject = veh;
                veh->RegisterReference(&m_pRopeAttachObject);
                m_pRopeAttachObject->AsPhysical()->physicalFlags.bCarriedByRope = true;
                if (veh->GetStatus() == STATUS_SIMPLE) {
                    veh->SetStatus(STATUS_PHYSICS);
                }
                m_pAttachedEntity->SetPosn(GetPickUpPos(m_pRopeAttachObject));
                m_pAttachedEntity->SetUsesCollision(false);
                ShortenRopeBy(ropeEnd.z - pickUpPos.z);
                break;
            }

            // 0x55873B
            if (!pickMagnoCraneObjs && !pickQuarryObjs && !pickMiniMagnetObjs) {
                return;
            }
        }

        // 0x55876F - Objects
        if (m_pRopeAttachObject) {
            return;
        }
        auto* const pool = GetObjectPool();
        for (auto i = (int32)pool->GetSize() - 1; i >= 0; i--) {
            auto* const obj = pool->GetAt(i);
            if (!obj || !obj->objectFlags.bCanBeAttachedToMagnet) {
                continue;
            }

            using namespace ModelIndices;

            const auto mi = obj->m_nModelIndex;
            const auto isPickable = [&] {
                if (pickMagnoCraneObjs) {
                    if (mi == MI_OBJECTFORMAGNOCRANE1 || mi == MI_OBJECTFORMAGNOCRANE2 || mi == MI_OBJECTFORMAGNOCRANE3 || mi == MI_OBJECTFORMAGNOCRANE4 || mi == MI_OBJECTFORMAGNOCRANE5) {
                        return true;
                    }
                }
                if (pickBuildingSiteObjs) {
                    if (mi == MI_OBJECTFORBUILDINGSITECRANE1) {
                        return true;
                    }
                }
                if (pickQuarryObjs) {
                    if (mi == MI_QUARY_ROCK1 || mi == MI_QUARY_ROCK2 || mi == MI_QUARY_ROCK3 || mi == MI_DEAD_TIED_COP) {
                        if (!obj->m_pAttachedTo) {
                            return true;
                        }
                    }
                }
                if (pickMiniMagnetObjs) {
                    if (mi == MI_WONG_DISH || mi == MI_KMB_ROCK || mi == MI_KMB_PLANK || mi == MI_KMB_BOMB) {
                        return true;
                    }
                }
                return false;
            }();
            if (!isPickable) {
                continue;
            }

            // 0x5588ED - Find the pickup position.
            // The containers can be picked up on any of 4 sides, whichever is the topmost at the moment.
            // If it's not the one facing "up" in model space the object is rotated (below) after it's picked up.
            auto  pickUpPos    = GetPickUpPos(obj);
            int32 numRotations = 0;
            if (mi == MI_OBJECTFORMAGNOCRANE1 || mi == MI_OBJECTFORMAGNOCRANE2 || mi == MI_OBJECTFORMAGNOCRANE3 || mi == MI_OBJECTFORMAGNOCRANE5) {
                const auto height = CRopes::FindPickupHeight(obj);
                const auto TrySide = [&](CVector offset, int32 rotations) {
                    if (const auto pos = obj->GetMatrix().TransformPoint(offset); pos.z > pickUpPos.z) {
                        pickUpPos    = pos;
                        numRotations = rotations;
                    }
                };
                TrySide(CVector{ 0.0f, 0.0f, -height }, 2);
                TrySide(CVector{ height, 0.0f, 0.0f }, 3);
                TrySide(CVector{ -height, 0.0f, 0.0f }, 1);
            }

            // 0x558B03
            if (!IsInPickUpRange(pickUpPos)) {
                continue;
            }

            m_pRopeAttachObject = obj;
            obj->RegisterReference(&m_pRopeAttachObject);
            auto* const carried = m_pRopeAttachObject->AsPhysical();
            carried->physicalFlags.bCarriedByRope = true;
            m_pAttachedEntity->SetPosn(GetPickUpPos(carried));
            m_pAttachedEntity->SetUsesCollision(false);
            if (carried->m_bIsStatic || carried->m_bIsStaticWaitingForCollision) {
                carried->SetIsStatic(false);
                carried->AddToMovingList();
            }
            carried->physicalFlags.bAttachedToEntity = true;
            ShortenRopeBy(ropeEnd.z - pickUpPos.z);

            // 0x558C3D - Rotate it, so the side picked up at becomes the top
            for (auto n = numRotations; n > 0; n--) {
                auto&      mat   = carried->GetMatrix();
                const auto right = mat.GetRight();
                mat.GetRight() = mat.GetUp();
                mat.GetUp()    = -right;
            }
            break;
        }
    };
    ProcessWinch();

    // 0x558CA3 - Not kept alive anymore and has fallen out of the world
    if (!(m_nFlags2 & 1) && m_aSegments[0].z < -50.0f) {
        Remove();
    }
    m_nFlags2 &= 0xFE;
}
