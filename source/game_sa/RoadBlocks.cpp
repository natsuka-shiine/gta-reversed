#include "StdInc.h"

#include "RoadBlocks.h"
#include "PedPlacement.h"
#include "TaskComplexWanderCop.h"
#include "TaskSimpleStandStill.h"
#include <extensions/File.hpp>

void CRoadBlocks::InjectHooks() {
    RH_ScopedClass(CRoadBlocks);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x461100);
    RH_ScopedInstall(ClearScriptRoadBlocks, 0x460EC0);
    RH_ScopedInstall(ClearSpaceForRoadBlockObject, 0x461020);
    RH_ScopedInstall(CreateRoadBlockBetween2Points, 0x4619C0);
    RH_ScopedInstall(GenerateRoadBlockPedsForCar, 0x461170);
    RH_ScopedInstall(GenerateRoadBlocks, 0x4629E0);
    RH_ScopedInstall(GetRoadBlockNodeInfo, 0x460EE0);
    RH_ScopedInstall(RegisterScriptRoadBlock, 0x460DF0);
}

// 0x461100
void CRoadBlocks::Init() {
    rng::fill(InOrOut, true);
    GenerateDynamicRoadBlocks = false;

    if (notsa::File rbx("data\\paths\\roadblox.dat", "rb"); rbx) {
        rbx.Read(&NumRoadBlocks, sizeof(int32));
        assert(NumRoadBlocks <= MAX_ROADBLOCKS);
        rbx.Read(RoadBlockNodes.data(), RoadBlockNodes.size() * sizeof(CNodeAddress));
    } else {
        NOTSA_UNREACHABLE("roadblox.dat couldn't be opened!");
    }
    ClearScriptRoadBlocks();
}

// 0x460EC0
void CRoadBlocks::ClearScriptRoadBlocks() {
    for (auto& srb : aScriptRoadBlocks) {
        srb.IsActive = false;
    }
}

// 0x461020
// Returns true if cleared successfully.
bool CRoadBlocks::ClearSpaceForRoadBlockObject(CVector cornerA, CVector cornerB){
    int16 numEntities{};
    CEntity* entities[2]{};
    CWorld::FindObjectsIntersectingCube(
        cornerA,
        cornerB,
        &numEntities,
        std::size(entities),
        entities,
        false,
        true,
        true,
        true,
        false
    );

    if (numEntities > std::size(entities) || numEntities <= 0) {
        return numEntities <= 0;
    }

    const auto Remove = [](CEntity* e) {
        CWorld::Remove(e);
        delete e;
    };

    for (auto* entity : entities | rngv::take(numEntities)) {
        switch (entity->GetType()) {
        case ENTITY_TYPE_VEHICLE:
            if (auto* v = entity->AsVehicle(); !v->CanBeDeleted()) {
                return false;
            } else if (!v->vehicleFlags.bCreateRoadBlockPeds) {
                Remove(v);
            }
            break;
        case ENTITY_TYPE_PED:
            if (auto* p = entity->AsPed(); p->CanBeDeleted()) {
                Remove(p);
            } else {
                return false;
            }
            break;
        case ENTITY_TYPE_OBJECT:
            if (auto* o = entity->AsObject(); o->CanBeDeleted() && o->m_nObjectType != OBJECT_GAME) {
                Remove(o);
            } else {
                return false;
            }
            break;
        default:
            NOTSA_UNREACHABLE();
        }
    }

    return true;
}

// 0x4619C0
void CRoadBlocks::CreateRoadBlockBetween2Points(CVector a, CVector b, bool isGangRoadBlock) {
    constexpr auto MAX_ROADBLOCK_CARS     = 5;
    constexpr auto MAX_ROADBLOCK_BARRIERS = 8;

    const CVector delta  = b - a;
    const float   length = delta.Magnitude();
    const CVector mid    = (a + b) * 0.5f;

    CVector dir = delta;
    dir.Normalise();

    // Perpendicular to the roadblock, pointing towards the player
    CVector perp{ dir.y, -dir.x, 0.f };
    perp.Normalise();
    if (DotProduct(FindPlayerCoors() - mid, perp) < 0.f) {
        perp *= -1.f;
    }

    // 0x461B72 - Pick the vehicle model
    eModelID vehModel;
    if (isGangRoadBlock) {
        vehModel = CPopulation::PickRiotRoadBlockCar();
        if (vehModel == MODEL_INVALID) {
            return;
        }
    } else {
        if (FindPlayerWanted()->AreArmyRequired()) {
            vehModel = MODEL_BARRACKS;
        } else if (FindPlayerWanted()->AreFbiRequired()) {
            vehModel = MODEL_FBIRANCH;
        } else if (FindPlayerWanted()->AreSwatRequired()) {
            vehModel = MODEL_ENFORCER;
        } else {
            vehModel = CStreaming::GetDefaultCopCarModel(false);
        }
        if (!CStreaming::GetInfo(vehModel).IsLoaded()) {
            vehModel = CStreaming::GetDefaultCopCarModel(false);
        }
        if (vehModel == MODEL_COPBIKE) {
            return;
        }
    }

    // NOTE: `Multiply3x3` (0x59C810) is used here by the original code, which transforms by the *inverse* of the (rotation) matrix
    const auto GetTransformedBoxCorner = [](const CMatrix& mat, const CVector& corner) {
        return mat.GetPosition() + mat.InverseTransformVector(corner);
    };

    // 0x461C2D
    {
        const auto& vehBB    = CModelInfo::GetModelInfo(vehModel)->GetColModel()->GetBoundingBox();
        const float vehSizeX = vehBB.m_vecMax.x - vehBB.m_vecMin.x + 2.f;
        const float vehSizeY = vehBB.m_vecMax.y - vehBB.m_vecMin.y + 0.2f + (isGangRoadBlock ? 0.5f : 0.f);

        // 0x461CAA - Figure out how many vehicles fit, and which way they are facing
        bool  isVehFacingPlayer[MAX_ROADBLOCK_CARS]; // If set the vehicle is placed across the road (its side is parallel to the roadblock line)
        int32 numVehs   = 0;
        float freeSpace = length;
        for (int32 i = 0; i < MAX_ROADBLOCK_CARS; i++) {
            isVehFacingPlayer[i] = vehModel != MODEL_BARRACKS && !isGangRoadBlock && (rand() & 1) != 0;

            const float vehSpace = isVehFacingPlayer[i] ? vehSizeX : vehSizeY;
            if (freeSpace < vehSpace) {
                break;
            }
            freeSpace -= vehSpace;
            numVehs++;
        }

        // 0x461D30
        const float gap  = freeSpace / (float)(numVehs + 1);
        const float step = vehSizeY * 0.5f + gap; // NOTE: Always uses `vehSizeY`, even for vehicles using `vehSizeX`
        float       dist = gap;
        for (int32 i = 0; i < numVehs; i++) {
            CMatrix mat;
            bool    bOnItsSide = false;
            float   randomAngle;
            if (isVehFacingPlayer[i]) { // 0x461D87
                dist += vehSizeX * 0.5f;

                mat.GetRight().Set(perp.y, -perp.x, 0.f);
                mat.GetForward() = perp;
                mat.GetUp().Set(0.f, 0.f, 1.f);

                randomAngle = ((float)(rand() & 0xFF) - 128.f) * (isGangRoadBlock ? 0.004f : 0.002f);
            } else { // 0x461E33
                dist += vehSizeY * 0.5f;

                if (isGangRoadBlock) {
                    bOnItsSide = (rand() & 0xFF) < 0x40;
                }

                mat.GetForward() = dir;
                if (bOnItsSide) { // 0x461E78
                    mat.GetRight().Set(0.f, 0.f, 1.f);
                    mat.GetUp().Set(-dir.y, dir.x, 0.f);
                } else { // 0x461EB1
                    mat.GetRight().Set(dir.y, -dir.x, 0.f);
                    mat.GetUp().Set(0.f, 0.f, 1.f);
                }

                if (rand() & 1) { // 0x461EEC - Turn it around
                    if (bOnItsSide) {
                        mat.RotateY(std::numbers::pi_v<float>);
                    } else {
                        mat.RotateZ(std::numbers::pi_v<float>);
                    }
                }

                randomAngle = ((float)(rand() & 0xFF) - 128.f) * (isGangRoadBlock ? 0.006f : 0.003f);
            }

            // 0x461F46
            mat.RotateZ(randomAngle);
            mat.SetTranslateOnly(a + dir * dist);
            mat.GetPosition().z += 0.3f - (bOnItsSide ? vehBB.m_vecMin.x : vehBB.m_vecMin.z);

            dist += step; // 0x461FFA - For the next one

            // 0x46200E
            const CVector cornerMin = GetTransformedBoxCorner(mat, vehBB.m_vecMin);
            const CVector cornerMax = GetTransformedBoxCorner(mat, vehBB.m_vecMax);
            if (!ClearSpaceForRoadBlockObject(cornerMin, cornerMax)) {
                continue;
            }

            // 0x4620F1 - Riots: Sometimes create a burning car wreck instead
            if (isGangRoadBlock && (bOnItsSide || (rand() & 0xFF) < 0x40)) {
                const auto wreck = new CObject(ModelIndices::MI_ROADBLOCKFUCKEDCAR1, true);
                wreck->GetMatrix() = mat;
                wreck->SetPosn(mat.GetPosition());
                wreck->SetIsStatic(false);
                CObject::nNoTempObjects++;
                wreck->m_nObjectType  = OBJECT_TEMPORARY;
                wreck->m_nRemovalTime = CTimer::m_snTimeInMilliseconds + 600'000;
                CWorld::Add(wreck);
                gFireManager.StartFire(wreck, nullptr, 2.8f, 1, 60'000, 2);
                if (wreck->m_pFire) {
                    wreck->m_pFire->SetRemovalDist(92);
                }
                continue;
            }

            // 0x4621EF
            const auto veh = new CAutomobile(vehModel, RANDOM_VEHICLE, true);
            veh->SetStatus(STATUS_ABANDONED);
            mat.GetPosition().z += veh->GetHeightAboveRoad() - 0.6f;
            veh->GetMatrix() = mat;
            veh->PlaceOnRoadProperly();
            veh->SetIsStatic(false);
            veh->UpdateRwMatrix();
            veh->m_nDoorLock = CARLOCK_UNLOCKED;
            CCarCtrl::JoinCarWithRoadSystem(veh);
            veh->m_autoPilot.m_nCarMission   = MISSION_NONE;
            veh->m_autoPilot.m_nTempAction   = TEMPACT_NONE;
            veh->m_autoPilot.m_nCurrentLane  = 0;
            veh->m_autoPilot.m_nNextLane     = 0;
            veh->m_autoPilot.m_speed         = 0.f;
            veh->m_autoPilot.m_nCruiseSpeed  = 0;
            veh->vehicleFlags.bNeverUseSmallerRemovalRange = true;
            veh->vehicleFlags.bIsLocked                    = false;
            veh->vehicleFlags.bEngineOn                    = false;

            // 0x4622BB
            bool bSetOnFire = false;
            if (isGangRoadBlock) {
                if ((int32)((float)(rand() & 0xFFFF) * (1.f / 32768.f) * 4.f) != 0) { // 0x462326 - Most of them are burning wrecks
                    veh->BlowUpCarCutSceneNoExtras(true, true, true, true);
                    veh->m_nTimeWhenBlowedUp += 1'000'000;
                    bSetOnFire = true;
                } else {
                    veh->SetTotalDamage(true);
                }
            } else if (veh->UsesSiren() && (rand() & 1)) { // 0x462342
                veh->vehicleFlags.bSirenOrAlarm = true;
            }

            // 0x46235D - Not upright?
            if (veh->GetMatrix().GetUp().z <= 0.94f) {
                delete veh;
                continue;
            }

            // 0x462374
            CVisibilityPlugins::SetClumpAlpha(veh->GetRpClump(), 0);
            CWorld::Add(veh);
            veh->vehicleFlags.bCreateRoadBlockPeds = true;
            veh->m_nTimeTillWeNeedThisCar          = CTimer::m_snTimeInMilliseconds + 7000;
            veh->m_nNumPedsForRoadBlock            = numVehs > 3 ? 1 : 2;
            if (isVehFacingPlayer[i]) { // 0x4623C6 - Open the front door(s), the peds will take cover behind them
                veh->m_nPedsPositionForRoadBlock = 2;
                for (const auto door : { DOOR_LEFT_FRONT, DOOR_RIGHT_FRONT }) {
                    if (door == DOOR_RIGHT_FRONT && veh->m_nNumPedsForRoadBlock <= 1) {
                        break;
                    }
                    const auto node = CDamageManager::GetCarNodeIndexFromDoor(door);
                    if (veh->m_aCarNodes[node]) {
                        veh->OpenDoor(nullptr, node, door, 1.f, true);
                    }
                }
            } else { // 0x46242A - Which side is the player on
                veh->m_nPedsPositionForRoadBlock = DotProduct(veh->GetPosition() - FindPlayerCoors(), veh->GetMatrix().GetRight()) < 0.f ? 0 : 1;
            }

            if (bSetOnFire) { // 0x462490
                gFireManager.StartFire(veh, nullptr, 2.8f, 1, 60'000, 2);
                if (veh->m_pFire) {
                    veh->m_pFire->SetRemovalDist(92);
                }
            }
        }
    }

    // 0x4624E1 - Cops also put down barriers in front of the roadblock
    if (isGangRoadBlock) {
        return;
    }

    const auto  barrierModel = ModelIndices::MI_ROADWORKBARRIER1;
    const auto& barrierBB    = CModelInfo::GetModelInfo(barrierModel)->GetColModel()->GetBoundingBox();
    const float barrierSize  = barrierBB.m_vecMax.x - barrierBB.m_vecMin.x + 0.5f;
    const int32 numBarriers  = std::min((int32)(length / barrierSize), MAX_ROADBLOCK_BARRIERS);
    const float barrierGap   = (length - (float)numBarriers * barrierSize) / (float)(numBarriers + 1);

    CObject::DeleteAllTempObjectsInArea(mid, length * 0.5f);

    for (int32 i = 0; i < numBarriers; i++) { // 0x4625E0
        const float dist = ((float)i + 0.5f) * barrierSize + (float)(i + 1) * barrierGap;

        CMatrix mat;
        mat.SetUnity();
        mat.SetTranslate(CVector{ 0.f, 0.f, 0.f });
        mat.GetRight()   = dir;
        mat.GetForward().Set(dir.y, -dir.x, 0.f);
        mat.GetUp().Set(0.f, 0.f, 1.f);
        mat.RotateZ(((float)(rand() & 0xFF) - 128.f) * 0.003f);
        mat.SetTranslateOnly(a + dir * dist + perp * 5.f);
        mat.GetPosition().x += (float)(rand() & 0xF) * 0.05f;
        mat.GetPosition().y += (float)(rand() & 0xF) * 0.05f;

        // 0x4627A9
        bool bGroundFound{};
        mat.GetPosition().z = CWorld::FindGroundZFor3DCoord({ mat.GetPosition().x, mat.GetPosition().y, mat.GetPosition().z + 2.f }, &bGroundFound, nullptr);
        if (!bGroundFound) {
            continue;
        }
        mat.GetPosition().z -= barrierBB.m_vecMin.z;

        // 0x462817
        const CVector cornerMin = GetTransformedBoxCorner(mat, barrierBB.m_vecMin);
        const CVector cornerMax = GetTransformedBoxCorner(mat, barrierBB.m_vecMax);
        if (!ClearSpaceForRoadBlockObject(cornerMin, cornerMax)) {
            continue;
        }

        // 0x4628FA
        const auto barrier = new CObject(barrierModel, true);
        barrier->GetMatrix() = mat;
        barrier->SetPosn(mat.GetPosition());
        CObject::nNoTempObjects++;
        barrier->m_nObjectType  = OBJECT_TEMPORARY;
        barrier->m_nRemovalTime = CTimer::m_snTimeInMilliseconds + 600'000;
        CWorld::Add(barrier);
    }
}

// 0x461170
void CRoadBlocks::GenerateRoadBlockPedsForCar(CVehicle* vehicle, int32 pedsPositionsType, ePedType pedType) {
    const auto Generate = [&](eModelID pedModel = MODEL_INVALID, eCopType copType = COP_TYPE_CITYCOP, bool isSpecialCop = false) {
        static constexpr auto PLACEMENTS = std::to_array<CVector>({
            { -1.5f, +1.9f, 0.0f },
            { -1.5f, -2.6f, 0.0f },
            { +1.5f, +1.9f, 0.0f },
            { +1.5f, -2.6f, 0.0f },
            { -1.5f,  0.0f, 0.0f },
            { +1.5f,  0.0f, 0.0f },
        });

        static constexpr auto SPECIAL_COP_PLACEMENTS = std::to_array<CVector>({
            {  0.0f, +3.2f, 0.0f },
            { +1.5f, -1.8f, 0.0f },
            {  0.0f, +3.2f, 0.0f },
            { -1.5f, -1.8f, 0.0f },
            { -1.5f,  0.0f, 0.0f },
            { +1.5f,  0.0f, 0.0f },
        });

        const auto placementIdx = 2 * pedsPositionsType;
        const auto radiusRatio  = vehicle->GetColModel()->GetBoundingSphere().m_fRadius
            / CModelInfo::GetModelInfo(CStreaming::GetDefaultCopCarModel(false))->GetColModel()->GetBoundingSphere().m_fRadius;

        for (auto i = 0u; i < vehicle->m_nNumPedsForRoadBlock; i++) {
            const auto offset = (isSpecialCop ? SPECIAL_COP_PLACEMENTS : PLACEMENTS)[placementIdx + i] * radiusRatio;
            const auto pos = vehicle->GetMatrix().TransformPoint(offset);

            auto* ped = [&]() -> CPed* {
                if (pedType != PED_TYPE_COP) { // 0x461560
                    return new CCivilianPed(pedType, pedModel);
                } else {
                    auto* p = new CCopPed(CStreaming::IsModelLoaded(pedModel) ? copType : COP_TYPE_CITYCOP);
                    if (copType == COP_TYPE_CITYCOP) {
                        p->SetCurrentWeapon(WEAPON_PISTOL);
                    }
                    return p;
                }
            }();

            ped->SetPosn(std::get<CVector>(CPedPlacement::FindZCoorForPed(pos)));
            ped->GetMatrix().SetRotateKeepPos({ 0.0f, 0.0f, -HALF_PI });

            if (pedType == PED_TYPE_COP) {
                auto* t = new CTaskComplexWanderCop(PEDMOVE_STILL, CGeneral::GetRandomNumberInRange(8ui8));
                t->m_nSubTaskCreatedTimer = {};
                t->m_nScanForStuffTimer   = {};
                ped->GetTaskManager().SetTask(t, TASK_PRIMARY_PRIMARY);
            }
            ped->GetTaskManager().SetTask(new CTaskSimpleStandStill(0, true), TASK_PRIMARY_DEFAULT);

            ped->bStayInSamePlace         = true;
            ped->bNotAllowedToDuck        = true;
            ped->m_nTimeTillWeNeedThisPed = CTimer::GetTimeInMS() + 10'000;
            ped->bCrouchWhenShooting      = !isSpecialCop || pedsPositionsType != 2;
            ped->bCullExtraFarAway        = true;
            CEntity::RegisterReference(ped->m_pVehicle = vehicle);
            CVisibilityPlugins::SetClumpAlpha(ped->GetRpClump(), 0);

            if (pedType != PED_TYPE_COP) {
                const auto weapon = CGangs::Gang[pedType - PED_TYPE_GANG1].GetRandomWeapon(false);
                if (weapon != WEAPON_UNARMED) {
                    ped->GiveDelayedWeapon(weapon, 25'001);
                    ped->SetCurrentWeapon(weapon);
                }
            }
            CWorld::Add(ped);
            ped->GetEventGroup().Add<CEventScriptCommand>({ TASK_PRIMARY_PRIMARY, new CTaskComplexKillPedOnFoot(FindPlayerPed()) });
        }
    };

    if (pedType == PED_TYPE_COP) {
        switch (vehicle->GetModelId()) {
        case MODEL_ENFORCER: Generate(MODEL_SWAT,    COP_TYPE_SWAT1,   true); break;
        case MODEL_BARRACKS: Generate(MODEL_ARMY,    COP_TYPE_ARMY,    true); break;
        case MODEL_FBIRANCH: Generate(MODEL_FBI,     COP_TYPE_FBI,     true); break;
        case MODEL_COPCARRU: Generate(MODEL_INVALID, COP_TYPE_CITYCOP, true); break;
        default:             Generate(MODEL_INVALID, COP_TYPE_CITYCOP, false); break;
        }
    } else if (IsPedTypeGang(pedType)) {
        for (auto i = 0; i < TOTAL_GANGS; i++) {
            if (!CPopCycle::m_pCurrZoneInfo->GangStrength[i]) {
                continue;
            }
            const auto pedModel = CGangs::ChooseGangPedModel((eGangID)i);
            if (pedModel == MODEL_INVALID) {
                continue;
            }
            Generate(pedModel);
            return;
        }
    } else {
        Generate();
    }
}

// 0x4629E0
void CRoadBlocks::GenerateRoadBlocks() {
    ZoneScoped;

    if (FindPlayerWanted()->m_ChanceOnRoadBlock && FindPlayerVehicle()) {
        if (!GenerateDynamicRoadBlocks) {
            rng::fill(InOrOut, true);
            GenerateDynamicRoadBlocks = true;
        }

        const auto counter1      = MAX_ROADBLOCKS * (CTimer::GetFrameCounter() % 16 + 1);
        const auto rbsToGenerate = std::min((uint32)NumRoadBlocks, ((counter1 % 16) + counter1) / 16);
        auto       counter2      = MAX_ROADBLOCKS * (CTimer::GetFrameCounter() % 16) / 16;

        for (; counter2 < rbsToGenerate; counter2++) {
            const auto& mrbNode = RoadBlockNodes[counter2];
            if (!ThePaths.IsAreaLoaded(mrbNode)) {
                continue;
            }
            const auto& mainNode = ThePaths.GetPathNode(mrbNode);
            const auto  playerPos = FindPlayerCoors();
            if (std::abs(playerPos.x - mainNode->GetPosition().x) >= 90.0f ||
                std::abs(playerPos.y - mainNode->GetPosition().y) >= 90.0f ||
                DistanceBetweenPoints2D(playerPos, mainNode->GetPosition()) >= 90.0f)
            {
                InOrOut[counter2] = false;
                continue;
            }

            if (InOrOut[counter2]) {
                continue;
            }
            InOrOut[counter2] = true;

            if (CGeneral::GetRandomNumberInRange(128u) >= FindPlayerWanted()->m_ChanceOnRoadBlock) {
                continue;
            }

            float mrbWidth{};
            CVector mrbDir{};
            if (!GetRoadBlockNodeInfo(mrbNode, mrbWidth, mrbDir)) {
                continue;
            }

            if (mainNode->m_nPathWidth) {
                const auto width = mainNode->m_nPathWidth / 16.0f;
                CreateRoadBlockBetween2Points(
                    mainNode->GetPosition() + mrbDir * (mrbWidth / 2.f + width),
                    mainNode->GetPosition() + mrbDir * width,
                    false
                );
                CreateRoadBlockBetween2Points(
                    mainNode->GetPosition() - mrbDir * width,
                    mainNode->GetPosition() - mrbDir * (mrbWidth / 2.f + width),
                    false
                );
                continue;
            }

            for (auto&& [i, nodeAddr] : rngv::enumerate(RoadBlockNodes)) {
                if (counter2 == i || InOrOut[i] || !ThePaths.IsAreaLoaded(nodeAddr.m_wAreaId)) {
                    continue;
                }
                const auto& node = ThePaths.GetPathNode(nodeAddr);

                if (std::abs(mainNode->GetPosition().x - node->GetPosition().x) >= 30.0f ||
                    std::abs(mainNode->GetPosition().y - node->GetPosition().y) >= 30.0f)
                {
                    continue;
                }

                float   width{};
                CVector dir{}; 
                if (!GetRoadBlockNodeInfo(nodeAddr, width, dir)) {
                    continue;
                }

                if (mrbWidth != width || dir.Dot(mrbDir) <= 0.7f) {
                    continue;
                }

                [[maybe_unused]] CColPoint col{};
                [[maybe_unused]] CEntity*  colEntity{};
                if (CWorld::ProcessLineOfSight(
                    mainNode->GetPosition() + CVector{0.0f, 0.0f, 1.0f},
                    node->GetPosition() + CVector{0.0f, 0.0f, 1.0f},
                    col,
                    colEntity,
                    true,
                    false,
                    false,
                    false,
                    false,
                    false,
                    false,
                    false
                )) {
                    continue;
                }

                const auto dirFromMain = (node->GetPosition() - mainNode->GetPosition()).Normalized();
                CreateRoadBlockBetween2Points(
                    node->GetPosition()     + dirFromMain * (mrbWidth / 2.0f),
                    mainNode->GetPosition() - dirFromMain * (mrbWidth / 2.0f),
                    false
                );
                InOrOut[i] = true;

                if (i == NumRoadBlocks) {
                    CreateRoadBlockBetween2Points(
                        mainNode->GetPosition() - dirFromMain * (mrbWidth / 2.0f),
                        mainNode->GetPosition() + dirFromMain * (mrbWidth / 2.0f),
                        false
                    );
                    break;
                }
            }
        }
    } else {
        GenerateDynamicRoadBlocks = false;
    }

    if (auto& srb = aScriptRoadBlocks[CTimer::GetFrameCounter() % MAX_SCRIPT_ROADBLOCKS]; srb.IsActive) {
        const auto c = CVector::Centroid({ srb.CornerA, srb.CornerB });

        if (DistanceBetweenPoints(FindPlayerCoors(), c) >= 90.0f) {
            srb.IsSafeToCreate = true;
        } else if (srb.IsSafeToCreate) {
            CreateRoadBlockBetween2Points(srb.CornerA, srb.CornerB, srb.IsGangRoadBlock);
            srb.IsSafeToCreate = false;
        }
    }
}

// 0x460EE0
bool CRoadBlocks::GetRoadBlockNodeInfo(CNodeAddress nodeAddress, float& outWidth, CVector& outDir) {
    auto* const node = ThePaths.GetPathNode(nodeAddress);
    assert(node);

    assert(node->m_nNumLinks >= 2);
    const auto naviLinkAddrA = ThePaths.GetNaviLink(nodeAddress.m_wAreaId, node->m_wBaseLinkId + 0),
               naviLinkAddrB = ThePaths.GetNaviLink(nodeAddress.m_wAreaId, node->m_wBaseLinkId + 1);
    if (!ThePaths.IsAreaLoaded(naviLinkAddrA.m_wAreaId) || !ThePaths.IsAreaLoaded(naviLinkAddrB.m_wAreaId)) {
        return false;
    }

    const auto &naviLinkA = ThePaths.GetCarPathLink(naviLinkAddrA),
               &naviLinkB = ThePaths.GetCarPathLink(naviLinkAddrB);

    const auto maxNumLanes = std::max(
        naviLinkA.m_numOppositeDirLanes + naviLinkA.m_numSameDirLanes,
        naviLinkB.m_numOppositeDirLanes + naviLinkB.m_numSameDirLanes
    );

    outWidth = ((float)maxNumLanes + 1.f) * 5.f;
    outDir   = CVector{ (naviLinkB.GetNodeCoors() - naviLinkA.GetNodeCoors()).GetPerpRight(), 0.f }.Normalized();

    return true;
}

// 0x460DF0
void CRoadBlocks::RegisterScriptRoadBlock(CVector cornerA, CVector cornerB, bool isGangRoadBlock) {
    auto free = rng::find_if(aScriptRoadBlocks, [](const auto& srb) { return !srb.IsActive; });
    if (free == aScriptRoadBlocks.end()) {
        // No free script roadblock found
        return;
    }

    free->CornerA         = cornerA;
    free->CornerB         = cornerB;
    free->IsActive        = true;
    free->IsSafeToCreate  = true;
    free->IsGangRoadBlock = isGangRoadBlock;
}
