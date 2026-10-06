/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"
#include "PathFind.h"

#include <reversiblebugfixes/Bugs.hpp>

// TODO: Move into the class itself
auto& s_pathsNeededPosn = StaticRef<CVector>(0x977B70);
auto& s_bLoadPathsNeeded = StaticRef<bool>(0x96F030);

// TODO: Remove this and use a stack array or smth..
auto& ToBeStreamed = StaticRef<std::array<bool, NUM_PATH_MAP_AREAS>>(0x96EFD0);

auto& XCoorGiven = StaticRef<std::array<float, 64>>(0x96EE80);
auto& YCoorGiven = StaticRef<std::array<float, 64>>(0x96ED80);
auto& ZCoorGiven = StaticRef<std::array<float, 64>>(0x96EC80);
auto& ConnectsToGiven = StaticRef<std::array<std::array<int8, 6>, 64>>(0x96EAC0);
auto& DontWanderGiven = StaticRef<std::array<bool, 64>>(0x96EC40);

auto& aInteriorNodeLinkedToExterior = StaticRef<std::array<int32, NUM_PATH_INTERIOR_AREAS>>(0x96EA98);
auto& aExteriorNodeLinkedTo = StaticRef<std::array<CNodeAddress, NUM_PATH_INTERIOR_AREAS>>(0x977B7C);

auto& aNodesToBeCleared = StaticRef<std::array<CNodeAddress, 5000>>(0x972CD0);

void CPathFind::InjectHooks() {
    RH_ScopedClass(CPathFind);
    RH_ScopedCategoryGlobal();

    // Hooks commented out because most of the functions have no definitions
    // thus it's not possible to take their address
    // And I'm lazy to add stubs

    RH_ScopedInstall(AddNodeToNewInterior, 0x450E90);
    RH_ScopedInstall(FindNearestExteriorNodeToInteriorNode, 0x450F30);
    RH_ScopedInstall(ThisNodeHasToBeSwitchedOff, 0x44D3E0);
    RH_ScopedInstall(These2NodesAreAdjacent, 0x44D230);
    RH_ScopedInstall(FindRegionForCoors, 0x44D830);
    RH_ScopedInstall(FindYRegionForCoors, 0x44D8C0);
    RH_ScopedInstall(FindXRegionForCoors, 0x44D890);
    RH_ScopedInstall(AddInteriorLinkToExternalNode, 0x44DF30);
    RH_ScopedInstall(AddInteriorLink, 0x44DED0);
    RH_ScopedInstall(MarkRegionsForCoors, 0x44DB60);
    RH_ScopedOverloadedInstall(FindStartPointOfRegion, "", 0x44D930, void(CPathFind::*)(size_t, size_t, float&, float&));
    RH_ScopedInstall(FindYCoorsForRegion, 0x44D910);
    RH_ScopedInstall(FindXCoorsForRegion, 0x44D8F0);
    RH_ScopedInstall(IsAreaNodesAvailable, 0x420AA0);
    RH_ScopedInstall(HaveRequestedNodesBeenLoaded, 0x450DB0);
    RH_ScopedInstall(MakeRequestForNodesToBeLoaded, 0x450D70);
    RH_ScopedInstall(UpdateStreaming, 0x450A60);
    RH_ScopedInstall(TakeWidthIntoAccountForWandering, 0x4509A0);
    RH_ScopedInstall(TakeWidthIntoAccountForCoors, 0x44DA30);
    RH_ScopedInstall(GeneratePedCreationCoors, 0x44E790);
    RH_ScopedInstall(Shutdown, 0x450950);
    RH_ScopedOverloadedInstall(FindNodeCoorsForScript, "TwoNodes", 0x450780, CVector(CPathFind::*)(CNodeAddress, CNodeAddress, float&, bool*));
    RH_ScopedOverloadedInstall(FindNodeCoorsForScript, "LinkedNode", 0x4505E0, CVector(CPathFind::*)(CNodeAddress, bool*));
    RH_ScopedInstall(IsWaterNodeNearby, 0x450DE0);
    RH_ScopedInstall(CountNeighboursToBeSwitchedOff, 0x4504F0);
    //RH_ScopedInstall(FindNodeOrientationForCarPlacement, 0x450320);
    //RH_ScopedInstall(FindNodePairClosestToCoors, 0x44FEE0);
    RH_ScopedInstall(FindNodeClosestToCoorsFavourDirection, 0x44FCE0);
    RH_ScopedInstall(FindNodeClosestToCoors, 0x44F460);
    RH_ScopedInstall(RecordNodesClosestToCoors, 0x44FA30);
    RH_ScopedInstall(MarkRoadNodeAsDontWander, 0x450560);
    //RH_ScopedInstall(AddDynamicLinkBetween2Nodes, 0x4512D0);
    RH_ScopedOverloadedInstall(LoadPathFindData, "Area", 0x452F40, void(CPathFind::*)(int32));
    RH_ScopedOverloadedInstall(LoadPathFindData, "FromStream", 0x4529F0, void(CPathFind::*)(RwStream*, int32));
    RH_ScopedInstall(SwitchPedRoadsOffInArea, 0x452F00);
    RH_ScopedInstall(SwitchRoadsOffInArea, 0x452C80);
    RH_ScopedInstall(SwitchRoadsOffInAreaForOneRegion, 0x452820);
    RH_ScopedInstall(ComputeRoute, 0x452760);
    RH_ScopedInstall(CompleteNewInterior, 0x452270);
    RH_ScopedInstall(SwitchOffNodeAndNeighbours, 0x452160);
    RH_ScopedInstall(Find2NodesForCarCreation, 0x452090);
    //RH_ScopedInstall(TestCoorsCloseness, 0x452000);
    //RH_ScopedInstall(FindNextNodeWandering, 0x451B70);
    RH_ScopedInstall(DoPathSearch, 0x4515D0);
    //RH_ScopedInstall(FindParkingNodeInArea, 0x4513F0);
    RH_ScopedInstall(FindLinkBetweenNodes, 0x451350);
    RH_ScopedInstall(ReturnInteriorNodeIndex, 0x451300);
    //RH_ScopedInstall(FindNthNodeClosestToCoors, 0x44F8C0);
    RH_ScopedInstall(FindNodeClosestInRegion, 0x44F2C0);
    RH_ScopedInstall(CalcDistToAnyConnectingLinks, 0x44F190);
    RH_ScopedInstall(CalcRoadDensity, 0x44EFC0);
    RH_ScopedInstall(RemoveBadStartNode, 0x44E4F0);
    RH_ScopedInstall(RemoveInteriorLinks, 0x44DF60);
    RH_ScopedInstall(TestForPedTrafficLight, 0x44D480);
    RH_ScopedInstall(UnMarkAllRoadNodesAsDontWander, 0x44D400);
    RH_ScopedInstall(TidyUpNodeSwitchesAfterMission, 0x44D3B0);
    RH_ScopedInstall(ThisNodeWillLeadIntoADeadEnd, 0x44D310);
    RH_ScopedInstall(AddNodeToList, 0x44D1E0);
    RH_ScopedInstall(RemoveNodeFromList, 0x44D1B0);
    RH_ScopedInstall(UnLoadPathFindData, 0x44D0F0);
    RH_ScopedInstall(Init, 0x44D080);
    RH_ScopedInstall(GetPathNode, 0x420AC0);
    RH_ScopedInstall(TestCrossesRoad, 0x44D790);
    RH_ScopedInstall(ReInit, 0x44E4E0);
    RH_ScopedInstall(RemoveInterior, 0x44E1A0);
    RH_ScopedInstall(AddDynamicLinkBetween2Nodes_For1Node, 0x44E000);
    RH_ScopedInstall(StartNewInterior, 0x44DE80);
    RH_ScopedInstall(LoadSceneForPathNodes, 0x44DE00);
    RH_ScopedInstall(AreNodesLoadedForArea, 0x44DD10);
    RH_ScopedInstall(ReleaseRequestedNodes, 0x44DD00);
    RH_ScopedInstall(SetPathsNeededAtPosition, 0x44DCD0);
    RH_ScopedInstall(SetLinksBridgeLights, 0x44D960);
    RH_ScopedInstall(Save, 0x5D34C0);
    RH_ScopedInstall(Load, 0x5D3500);
}

void CPathNode::InjectHooks() {
    RH_ScopedClass(CPathNode);
    RH_ScopedCategoryGlobal();
    RH_ScopedInstall(GetPosition, 0x420A10);
}

// 0x44D080
void CPathFind::Init() {
    ZoneScoped;

    static int32 NumTempExternalNodes = 0; // Unused
    m_nNumNodeSwitches                = 0;
    m_loadAreaRequestPending = false;

    for (auto i = 0u; i < NUM_TOTAL_PATH_NODE_AREAS; ++i) {
        m_pPathNodes[i] = nullptr;
        m_pNaviNodes[i] = nullptr;
        m_pNodeLinks[i] = nullptr;
        m_pLinkLengths[i] = nullptr;
        m_pPathIntersections[i] = nullptr;
        m_pNaviLinks[i] = nullptr; // BUG: Out of array bounds write, same as in original code
        m_aTempNodes[i] = nullptr;    // BUG: Out of array bounds write, same as in original code
    }

    rng::fill(m_interiorIDs, (uint32)-1);
}

// 0x44E4E0
void CPathFind::ReInit() {
    m_nNumNodeSwitches       = 0;
    m_loadAreaRequestPending = false;
}

// 0x44DD00
void CPathFind::MakeRequestForNodesToBeLoaded(float minX, float maxX, float minY, float maxY) {
    m_loadAreaRequestPending = true;
    m_loadAreaRequestMinX = minX;
    m_loadAreaRequestMaxX = maxX;
    m_loadAreaRequestMinY = minY;
    m_loadAreaRequestMaxY = maxY;
    UpdateStreaming(true);
}

// 0x44DD10
bool CPathFind::AreNodesLoadedForArea(float minX, float maxX, float minY, float maxY) {
    return IterAreasTouchingRect({ minX, minY, maxX, maxY }, [this](auto areaId) -> bool { return IsAreaLoaded(areaId); });
}

// 0x450DB0
bool CPathFind::HaveRequestedNodesBeenLoaded() {
    return AreNodesLoadedForArea(
        m_loadAreaRequestMinX,
        m_loadAreaRequestMaxX,
        m_loadAreaRequestMinY,
        m_loadAreaRequestMaxY
    );
}

// 0x450950
void CPathFind::Shutdown() {
    for (auto x = 0u; x < NUM_PATH_MAP_AREA_X; ++x) {
        for (auto y = 0u; y < NUM_PATH_MAP_AREA_Y; ++y) {
            auto relativeId = x + y * NUM_PATH_MAP_AREA_X;
            if (m_pPathNodes[relativeId]) {
                CStreaming::RemoveModel(DATToModelId(relativeId));
            }
        }
    }
}

bool CPathFind::ThisNodeWillLeadIntoADeadEnd(CPathNode* startNode, CPathNode* endNode) {
    auto curr = startNode, prev = endNode;
    while (true) {
        CPathNode* next{}; // If node has no links (or neither links area is loaded) this will be nullptr
        for (auto& linked : GetNodeLinkedNodes(*curr)) {
            if (&linked == prev) { // Obviously don't count the previous node
                continue;
            }
            if (linked.m_nBehaviourType == 4u || linked.m_nBehaviourType > 10u) { // TODO: Enum?
                // I'm unsure what's happening here
                // I think, since this function isn't recursive, they just
                // consider having 2 appropriate links as a non-deadend
                if (next) {
                    return false;
                }
                next = &linked;
            }
        }
        if (!next) { 
            return true;
        }
        prev = curr;
        curr = next;
    }
}

// 0x44D3B0
void CPathFind::TidyUpNodeSwitchesAfterMission() {
    m_nNumNodeSwitches = std::min(54u, m_nNumNodeSwitches); // todo: magic number
}

// 0x44D400
void CPathFind::UnMarkAllRoadNodesAsDontWander() {
    for (auto i = 0u; i < NUM_PATH_MAP_AREAS; ++i) {
        if (!m_pPathNodes[i])
            continue;

        for (auto nodeInd = 0u; nodeInd < m_anNumVehicleNodes[i]; ++nodeInd) {
            m_pPathNodes[i][nodeInd].m_bDontWander = false;
        }
    }
}

// 0x44DD00
void CPathFind::ReleaseRequestedNodes() {
    m_loadAreaRequestPending = false;
}

/*!
* @brief Find intersection info between 2 nodes
* @addr notsa 100% inlined
*/
auto CPathFind::FindIntersection(const CNodeAddress& startNodeAddress, const CNodeAddress& targetNodeAddress) -> CPathIntersectionInfo* {
    // Make sure both nodes areas are loaded
    if (!AreNodeAreasLoaded({ targetNodeAddress, startNodeAddress })) {
        return nullptr;
    }

    const auto& startNode = *GetPathNode(startNodeAddress);
    const auto& nodeLinks = m_pNodeLinks[startNodeAddress.m_wAreaId];
    for (auto i = 0u; i < startNode.m_nNumLinks; i++) {
        const auto linkedNodeIdx = startNode.m_wBaseLinkId + i;
        if (nodeLinks[linkedNodeIdx] == targetNodeAddress) {
            return &m_pPathIntersections[startNodeAddress.m_wAreaId][linkedNodeIdx];
        }
    }

    return nullptr;
}

// 0x44D790
bool CPathFind::TestCrossesRoad(CNodeAddress startNodeAddress, CNodeAddress targetNodeAddress) {
    const auto intersect = FindIntersection(startNodeAddress, targetNodeAddress);
    return intersect && intersect->m_bRoadCross;
}

// 0x44E790
bool CPathFind::GeneratePedCreationCoors(
    float         x,
    float         y,
    float         minDist1,
    float         maxDist1,
    float         minDist2,
    float         maxDist2,
    CVector*      outCoords,
    CNodeAddress* outAddress1,
    CNodeAddress* outAddress2,
    float*        outOrientation,
    bool          bLowTraffic,
    CMatrix*      transformMatrix
) {
    // Nodes with a spawn probability less than (or equal to) this are ignored
    const auto minSpawnProbability = (int32)((float)(rand() & 0xFFFF) * (1.0f / 32768.0f) * 15.0f);
    const auto maxNodeDistSq       = sq(maxDist1 + 30.0f);
    const auto areaId              = FindRegionForCoors({ x, y });

    for (auto attempt = 0; attempt < 300; attempt++) {
        if (!IsAreaLoaded(areaId) || m_anNumPedNodes[areaId] == 0) {
            continue;
        }

        // 0x44E82E - Pick a random ped node in this area
        const CPathNode& node = m_pPathNodes[areaId][(rand() >> 6) % (int32)m_anNumPedNodes[areaId] + m_anNumVehicleNodes[areaId]];

        // 0x44E855 - `m_vPos.x/y` convert to the uncompressed value (int16 / 8)
        const float nodeX      = node.m_vPos.x;
        const float nodeY      = node.m_vPos.y;
        const float nodeDistSq = (nodeY - y) * (nodeY - y) + (nodeX - x) * (nodeX - x);
        if (!(nodeDistSq < maxNodeDistSq)) {
            continue;
        }
        if ((int32)node.m_nSpawnProbability <= minSpawnProbability) {
            continue;
        }

        // 0x44E8D4
        const auto numLinks = (int32)node.m_nNumLinks;
        for (auto linkIdx = 0; linkIdx < numLinks; linkIdx++) {
            const auto linkId = node.m_wBaseLinkId + linkIdx;
            if (m_pPathIntersections[areaId][linkId].m_bRoadCross) { // 0x44E90D
                continue;
            }

            // 0x44E917 - Interior areas are ignored
            const auto linkedAddr = m_pNodeLinks[areaId][linkId];
            if (linkedAddr.m_wAreaId >= NUM_PATH_MAP_AREAS || !IsAreaLoaded(linkedAddr.m_wAreaId)) {
                continue;
            }
            const CPathNode& linkedNode = m_pPathNodes[linkedAddr.m_wAreaId][linkedAddr.m_wNodeId];

            // 0x44E94F
            if ((node.m_isSwitchedOff || linkedNode.m_isSwitchedOff) && !bLowTraffic) {
                continue;
            }
            if ((int32)linkedNode.m_nSpawnProbability <= minSpawnProbability) { // 0x44E96A
                continue;
            }

            // 0x44E97E - Either of the nodes must be closer than `maxDist1`
            const float nodeDist = std::sqrt(nodeDistSq);
            {
                const CVector linkedPos    = linkedNode.GetPosition();
                const float   linkedDistSq = (linkedPos.y - y) * (linkedPos.y - y) + (linkedPos.x - x) * (linkedPos.x - x);
                if (!(nodeDist < maxDist1) && !(std::sqrt(linkedDistSq) < maxDist1)) {
                    continue;
                }
            }

            // 0x44EA30 - Try a few random points on the line between the 2 nodes
            for (auto i = 0; i < 5; i++) {
                const float t = (float)(rand() & 0xFF) * (1.0f / 256.0f);
                *outOrientation = t;

                const CVector pos = t * linkedNode.GetPosition() + (1.0f - t) * node.GetPosition();

                const float dist2D = std::sqrt((pos.y - y) * (pos.y - y) + (pos.x - x) * (pos.x - x));

                // 0x44EB36
                const bool isVisible = transformMatrix
                    ? TheCamera.IsSphereVisible(pos, 2.0f, (RwMatrix*)transformMatrix)
                    : TheCamera.IsSphereVisible(pos, 2.0f);
                if (isVisible) { // 0x44EB6B
                    if (!(dist2D > minDist1) || !(dist2D < maxDist1)) {
                        continue;
                    }
                } else { // 0x44EB91
                    if (!(dist2D > minDist2) || !(dist2D < maxDist2)) {
                        continue;
                    }
                    if ((rand() & 1) == 0) {
                        continue;
                    }
                }

                // 0x44EBBA - NOTE: The out values are written even if the ground isn't found
                *outAddress1 = node.GetAddress();
                *outAddress2 = linkedNode.GetAddress();
                *outCoords   = pos;

                bool        bGroundFound{};
                const float groundZ = CWorld::FindGroundZFor3DCoord({ pos.x, pos.y, pos.z + 2.0f }, &bGroundFound, nullptr);
                if (!bGroundFound) {
                    continue;
                }

                // 0x44EC60
                if (std::abs(groundZ - pos.z) > 3.0f) {
                    return false;
                }
                outCoords->z = groundZ;
                return true;
            }
        }
    }

    return false;
}

// 0x44DA30
void CPathFind::TakeWidthIntoAccountForCoors(CNodeAddress address, CNodeAddress address2, uint16 seed, float* fOut1, float* fOut2) {
    if (!address.IsAreaValid() || !IsAreaNodesAvailable(address)) {
        return;
    }
    if (!address2.IsAreaValid() || !IsAreaNodesAvailable(address2)) {
        return;
    }

    // The narrower of the two nodes decides the offset
    const auto pathWidth = (int32)std::min(
        m_pPathNodes[address.m_wAreaId][address.m_wNodeId].m_nPathWidth,
        m_pPathNodes[address2.m_wAreaId][address2.m_wNodeId].m_nPathWidth
    );

    // Each nibble is remapped to [-7, +8]
    *fOut1 += (float)(((seed & 0xF) - 7) * pathWidth) * 0.00775f;
    *fOut2 += (float)((((seed >> 4) & 0xF) - 7) * pathWidth) * 0.00775f;
}

// 0x44ECA0
bool CPathFind::GeneratePedCreationCoors_Interior(
    float         x,
    float         y,
    CVector*      outCoords,
    CNodeAddress* unused1,
    CNodeAddress* unused2,
    float*        outOrientation
) {
    int   closestInteriorSlot = -1;
    float closestDistSq       = std::numeric_limits<float>::max();
    int   bestNodeIdx         = -1;

    for (int intSlot = 0; intSlot < NUM_PATH_INTERIOR_AREAS; ++intSlot) {
        int areaId = NUM_PATH_MAP_AREAS + intSlot;
        if (!IsAreaLoaded(areaId) || m_interiorIDs[intSlot] == uint32(-1)) {
            continue;
        }

        uint32 numPedNodes = m_anNumPedNodes[areaId];
        if (numPedNodes == 0) {
            continue;
        }
        uint32 firstPedNode = m_anNumVehicleNodes[areaId];
        for (uint32 nodeIdx = firstPedNode; nodeIdx < firstPedNode + numPedNodes; ++nodeIdx) {
            CPathNode& node = m_pPathNodes[areaId][nodeIdx];

            CVector pos    = node.GetPosition();
            float   dx     = x - pos.x;
            float   dy     = y - pos.y;
            float   distSq = dx * dx + dy * dy;
            if (distSq < closestDistSq) {
                closestDistSq       = distSq;
                closestInteriorSlot = intSlot;
                bestNodeIdx         = nodeIdx;
            }
        }
    }

    if (closestInteriorSlot == -1 || bestNodeIdx == -1) {
        return false;
    }

    int        areaId = NUM_PATH_MAP_AREAS + closestInteriorSlot;
    CPathNode& node   = m_pPathNodes[areaId][bestNodeIdx];
    if (node.m_isSwitchedOff) {
        return false;
    }
    if (node.m_onDeadEnd) {
        return false;
    }
    if (node.m_nNumLinks == 0) {
        return false;
    }
    if (outCoords) {
        *outCoords = node.GetPosition();
    }
    if (outOrientation) {
        *outOrientation = 0.0f; 
    }
    if (unused1) {
        *unused1 = CNodeAddress(areaId, node.m_wNodeId);
    }
    if (unused2) {
        *unused2 = CNodeAddress(); 
    }
    return true;
}

// 0x44D480
bool CPathFind::TestForPedTrafficLight(CNodeAddress startNodeAddress, CNodeAddress targetNodeAddress) {
    const auto intersect = FindIntersection(startNodeAddress, targetNodeAddress);
    return intersect && intersect->m_bPedTrafficLight;
}

// 0x4509A0
CVector CPathFind::TakeWidthIntoAccountForWandering(CNodeAddress nodeAddress, int16 randomSeed) {
    // Invalid area, or area not loaded
    if (!nodeAddress.IsValid() || !IsAreaNodesAvailable(nodeAddress)) {
        return {};
    }

    auto node         = GetPathNode(nodeAddress);
    auto basePosition = node->GetPosition();
    auto offsetX      = float(node->m_nPathWidth * ((randomSeed % 16) - 7)); // bottom 8 bits remapped to [-7 : +8]
    auto offsetY      = float(node->m_nPathWidth * (((randomSeed / 16) % 16) - 7)); // top 8 bits remapped to [-7 : +8]
    auto offset       = CVector{ offsetX * 0.00775f, offsetY * 0.00775f, 0.f };
    return basePosition + offset;
}

//  0x44F8C0
CNodeAddress CPathFind::FindNthNodeClosestToCoors( CVector pos,uint8 nodeType,float maxDistance,bool bLowTraffic,bool bUnkn,int32 nthNode,bool bBoatsOnly,bool bIgnoreInterior,CNodeAddress* outNode)
{
    struct Candidate {
        float        distSq;
        CNodeAddress addr;
    };
    std::vector<Candidate> candidates;
    size_t areaStart = 0, areaEnd = NUM_PATH_MAP_AREAS;
    if (!bIgnoreInterior) {
        areaEnd += NUM_PATH_INTERIOR_AREAS;
    }
    for (size_t areaId = areaStart; areaId < areaEnd; ++areaId) {
        if (!IsAreaLoaded(areaId)) {
            continue;
        }
        auto nodes = GetPathNodesInArea(areaId, static_cast<ePathType>(nodeType));
        for (size_t i = 0; i < nodes.size(); ++i) {
            auto& node = nodes[i];
            if (bBoatsOnly && !node.m_bWaterNode) {
                continue;
            }
            if (node.m_isSwitchedOff) {
                continue;
            }
            if (bLowTraffic && node.m_nSpawnProbability < 2) {
                continue;
            }
            float distSq = (node.GetPosition() - pos).SquaredMagnitude();
            if (distSq > maxDistance * maxDistance) {
                continue;
            }
            candidates.push_back({
                distSq, { (uint16)areaId, (uint16)i }
            });
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.distSq < b.distSq;
    });

    if (nthNode < 0 || (size_t)nthNode >= candidates.size()) {
        if (outNode) {
            *outNode = {};
        }
        return {};
    }

    auto result = candidates[nthNode].addr;
    if (outNode) {
        *outNode = result;
    }
    return result;
}

// 0x451B70
void CPathFind::FindNextNodeWandering(uint8 nodeType,CVector vecPos,CNodeAddress* originAddress,CNodeAddress* targetAddress,uint8 dir,uint8* outDir) {
    if (!originAddress || !originAddress->IsValid()) {
        if (targetAddress) {
            *targetAddress = CNodeAddress();
        }
        if (outDir) {
            *outDir = 0;
        }
        return;
    }

    CPathNode* originNode = GetPathNode(*originAddress);
    if (!originNode) {
        if (targetAddress) {
            *targetAddress = CNodeAddress();
        }
        if (outDir) {
            *outDir = 0;
        }
        return;
    }

    for (const auto& linked : GetNodeLinkedNodes(*originNode)) {
        if (linked.GetAddress() != *originAddress) {
            if (targetAddress) {
                *targetAddress = linked.GetAddress();
            }
            if (outDir) {
                *outDir = dir;
            }
            return;
        }
    }

    if (targetAddress) {
        *targetAddress = CNodeAddress();
    }
    if (outDir) {
        *outDir = 0;
    }
}

// 0x4515D0
void CPathFind::DoPathSearch(
    ePathType pathType,
    CVector originPos,
    CNodeAddress originAddrAddrHint, // If invalid/area not loaded the closest node to `originPos` is used.
    CVector targetPos,
    CNodeAddress* outResultNodes,
    int16& outNodesCount,
    int32 maxNodesToFind,
    float* outDistance,
    float maxSearchDistance,
    CNodeAddress* targetNodeAddrHint, // If null/invalid/area not loaded the closest node to `targetPos` is used.
    float maxSearchDepth,
    bool sameLaneOnly,
    CNodeAddress forbiddenNodeAddr,
    bool bAllowWaterNodeTransitions,
    bool forBoats
) {
    // Moved this up here, as it's set in every return path
    outNodesCount = 0;

    const auto ResolveNode = [&, this](CVector nodePosn, CNodeAddress* addr) {
        if (addr && addr->IsValid()) {
            // In case area is not loaded we still fall-back to
            // finding the closest node, as that will yield the
            // node closest in a loaded area
            if (IsAreaNodesAvailable(*addr)) {
                return GetPathNode(*addr);
            }
        }
        const auto foundAddr = FindNodeClosestToCoors(
            nodePosn,
            pathType,
            maxSearchDistance,
            false,
            false,
            false,
            forBoats,
            false
        );
        return foundAddr.IsValid()
            ? GetPathNode(foundAddr)
            : nullptr;
    };

    // Resolve addresses to use. Dont use `originAddrAddr` or `targetNodeAddr` after this point
    // NOTE: The target is resolved first (matters only for the order of the `FindNodeClosestToCoors` calls)
    CPathNode *origin{}, *target{};
    if (   !(target = ResolveNode(targetPos, targetNodeAddrHint))
        || !(origin = ResolveNode(originPos, &originAddrAddrHint))
    ) {
    fail:
        outNodesCount = 0;
        if (outDistance) {
            *outDistance = 100'000.f;
        }
        return;
    }

    // Check if the 2 nodes ended up being the same
    if (*origin == *target) {
        outNodesCount = 0;
        if (outDistance) {
            *outDistance = 0.f;
        }
        return;
    }

    // Check if flood fill values match, if not, fail
    if (origin->m_nFloodFill != target->m_nFloodFill) {
        goto fail;
    }

    rng::fill(m_pathFindHashTable, nullptr);
    m_totalNumNodesInPathFindHashTable = 0u;

    AddNodeToList(target, 0);

    size_t numNodesToBeCleared{};
    const auto AddNodeToBeCleared = [&](const CPathNode& node) {
        if (numNodesToBeCleared < std::size(aNodesToBeCleared)) {
            aNodesToBeCleared[numNodesToBeCleared++] = node.GetAddress();
        }
    };
    AddNodeToBeCleared(*target);

    size_t iterDepth{};
    bool finished{};
    while (true) {
        // Dijkstra's algorithm (probably)

        // Find distances
        for (auto node = m_pathFindHashTable[iterDepth % std::size(m_pathFindHashTable)]; node; node = node->m_next) {
            if (*node == *origin) {
                finished = true;
            }

            for (auto linkNum = 0u; linkNum < node->m_nNumLinks; linkNum++) {
                const auto linkIdx    = node->m_wBaseLinkId + linkNum;
                const auto linkedAddr = m_pNodeLinks[node->m_wAreaId][linkIdx];

                if (!IsAreaNodesAvailable(linkedAddr)) {
                    continue;
                }

                auto& linked = *GetPathNode(linkedAddr);

                // Omitted the bool variable and instead used `continue`s

                if (sameLaneOnly) { // 0x451814
                    const auto& naviLinkAddr = m_pNaviLinks[node->m_wAreaId][linkIdx];
                    if (IsAreaLoaded(naviLinkAddr.m_wAreaId)) {
                        const auto& naviLink = GetCarPathLink(naviLinkAddr);
                        // 0x45184A: `(byte[0xB] >> 3) & 7` if attached to the linked node, `byte[0xB] & 7` otherwise
                        if (naviLink.m_attachedTo == linked.GetAddress()) {
                            if (!naviLink.m_numSameDirLanes) {
                                continue;
                            }
                        } else if (!naviLink.m_numOppositeDirLanes) {
                            continue;
                        }
                    }
                }

                if (forbiddenNodeAddr == linked.GetAddress()) {
                    continue;
                }

                // 0x451885
                if (node->m_bWaterNode != linked.m_bWaterNode && !bAllowWaterNodeTransitions) {
                    continue;
                }

                // 0x4518BD
                const auto distToOriginFromLinked = node->m_totalDistFromOrigin + m_pLinkLengths[node->m_wAreaId][linkIdx];

                // If this new route we found is better than the previous re-insert node into hashtable
                if (distToOriginFromLinked < linked.m_totalDistFromOrigin) {
                    if (linked.m_totalDistFromOrigin != SHRT_MAX - 1) { // Why the fuck they used this instead of `SHRT_MAX`?
                        RemoveNodeFromList(&linked);
                    } else {
                        AddNodeToBeCleared(linked);
                    }
                    AddNodeToList(&linked, distToOriginFromLinked);
                }
            }

            // We've visited this node, so remove it
            RemoveNodeFromList(node);
        }

        // No more nodes? Well, too sad.
        if (!m_totalNumNodesInPathFindHashTable) {
            break;
        }

        // Hit limit?
        if (++iterDepth > maxSearchDepth) {
            break;
        }

        if (numNodesToBeCleared >= std::size(aNodesToBeCleared) - 50) {
            break;
        }

        if (!finished) { // Inverted 0x4519C0 
            continue;
        }

        if (outDistance) {
            *outDistance = origin->m_totalDistFromOrigin;
        }
        if (outResultNodes) { // Weird check really, because below it isn't checked :D
            outResultNodes[outNodesCount++] = origin->GetAddress();
        }

        // 0x4519F7 - Walk back from the origin to the target by always stepping onto the linked node that is exactly `linkLength` closer
        if (outNodesCount < maxNodesToFind) {
            for (auto node = origin; node != target;) {
                for (auto linkNum = 0u; linkNum < node->m_nNumLinks; linkNum++) {
                    const auto linkIdx    = node->m_wBaseLinkId + linkNum;
                    const auto linkedAddr = m_pNodeLinks[node->m_wAreaId][linkIdx];
                    if (!IsAreaNodesAvailable(linkedAddr)) {
                        continue;
                    }
                    const auto linked = GetPathNode(linkedAddr);
                    if (node->m_totalDistFromOrigin - m_pLinkLengths[node->m_wAreaId][linkIdx] == linked->m_totalDistFromOrigin) {
                        outResultNodes[outNodesCount++] = linkedAddr;
                        node = linked;
                        break;
                    }
                }
                if (outNodesCount >= maxNodesToFind) {
                    break;
                }
            }
        }
        break;
    }
    for (auto& addr : aNodesToBeCleared | rng::views::take(numNodesToBeCleared)) {
        GetPathNode(addr)->m_totalDistFromOrigin = SHRT_MAX - 1;
    }
}

// 0x452760
void CPathFind::ComputeRoute(uint8 nodeType, const CVector& vecStart, const CVector& vecEnd, const CNodeAddress& startAddress, CNodeRoute* route) {
    CNodeAddress outNodes[8]{};
    int16 outCount = 0;
    static auto& forbiddenAddr = StaticRef<CNodeAddress>(0x8A5F44); // Invalid node (area 0xFFFF) = no forbidden node
    DoPathSearch(
        static_cast<ePathType>(nodeType),
        vecStart,
        startAddress,
        vecEnd,
        outNodes,
        outCount,
        8,
        nullptr,
        999999.875f,
        nullptr,
        999999.875f,
        false,
        forbiddenAddr,
        false,
        false
    );
    route->Clear();
    for (int32 i = 0; i < outCount; i++) {
        if (route->GetSize() < 8) {
            route->Add(outNodes[i]);
        }
    }
}

// 0x44D960
void CPathFind::SetLinksBridgeLights(float fXMin, float fXMax, float fYMin, float fYMax, bool value) {
    const auto areaRect = CRect{ {fXMin, fYMin}, {fXMax, fYMax} };
    for (auto areaId = 0u; areaId < NUM_PATH_MAP_AREAS; areaId++) {
        if (!IsAreaLoaded(areaId)) {
            continue;
        }

        for (auto n = 0u; n < m_anNumCarPathLinks[areaId]; n++) {
            auto& node = GetCarPathLink({ areaId, n });
            if (areaRect.IsPointInside(node.GetNodeCoors())) {
                node.m_bridgeLights = value;
            }
        }
    }
}

namespace detail {
// NOTSA
CVector GetPosnBetweenNodesForScript(CPathNode* nodeA, CVector2D dir) {
    return nodeA->GetPosition() + CVector{dir.GetPerpLeft() * ((float)nodeA->m_nPathWidth / 16.f + 2.7f)};
}
};

// 0x4505E0
CVector CPathFind::FindNodeCoorsForScript(CNodeAddress address, bool* bFound) {
    const auto SetFound = [&](bool found) {
        if (bFound) {
            *bFound = found;
        }
    };
    if (!address.IsValid() || !IsAreaNodesAvailable(address)) {
        SetFound(false);
        return {};
    } else {
        SetFound(true);

        const auto node = GetPathNode(address);
        const auto nodePos = node->GetPosition();
 
        // If this node has a link return some kind of position between this and the first link
        if (node->m_nPathWidth && node->m_nNumLinks) {
            if (const auto firstLink = m_pNodeLinks[node->m_wBaseLinkId]) {
                if (const auto firstLinkedNode = GetPathNode(*firstLink)) {
                    auto dir = CVector2D{ firstLinkedNode->GetPosition() - nodePos }.Normalized();

                    // By negating here we invert the direction
                    dir = dir.x >= 0 ? dir : -dir;

                    return detail::GetPosnBetweenNodesForScript(node, dir);
                }
            }
        }

        // Otherwise just return this node's position
        return nodePos;
    }
}

// 0x450780
CVector CPathFind::FindNodeCoorsForScript(CNodeAddress nodeAddrA, CNodeAddress nodeAddrB, float& outHeadingDeg, bool* outFound) {
    const auto SetFound = [&](bool found) {
        if (outFound) {
            *outFound = found;
        }
    };
    if (nodeAddrA.IsValid() && nodeAddrB.IsValid() && AreNodeAreasLoaded({ nodeAddrA, nodeAddrB })) { // Inverted
        SetFound(true);

        const auto nodeA = GetPathNode(nodeAddrA);
        const auto posA  = nodeA->GetPosition();
        const auto dir   = CVector2D{ GetPathNode(nodeAddrB)->GetPosition() - posA }.Normalized();

        outHeadingDeg = RWRAD2DEG(dir.Heading());

        return nodeA->m_nPathWidth
            ? detail::GetPosnBetweenNodesForScript(nodeA, dir)
            : posA;
    } else {
        SetFound(false);
        return {};
    }
}

// 0x450560
void CPathFind::MarkRoadNodeAsDontWander(float x, float y, float z) {
    CVector pos = {x, y, z};
    auto node = FindNodeClosestToCoors(pos, PATH_TYPE_VEH, 999999.88f, 0, 0, 0, 0, 0);
    if (node.IsValid()) {
        m_pPathNodes[node.m_wAreaId][node.m_wNodeId].m_bDontWander = true;
    }
}

// 0x452820
void CPathFind::SwitchRoadsOffInAreaForOneRegion(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax, bool bSwitchOff, bool bCars, int areaId, bool bBackToOriginal) {
    assert(areaId >= 0 && areaId < NUM_PATH_MAP_AREAS);

    if (!IsAreaLoaded(areaId)) {
        return;
    }

    auto start = bCars ? 0 : m_anNumVehicleNodes[areaId];
    auto end   = bCars ? m_anNumVehicleNodes[areaId] : m_anNumNodes[areaId];

    for (auto nodeIdx = start; nodeIdx < end; ++nodeIdx) {
        CPathNode& node     = m_pPathNodes[areaId][nodeIdx];
        const auto position = node.GetPosition();

        if (position.x < xMin || position.x > xMax || position.y < yMin || position.y > yMax || position.z < zMin || position.z > zMax) {
            continue;
        }
        if (!ThisNodeHasToBeSwitchedOff(&node) || node.m_isSwitchedOff == (bBackToOriginal ? node.m_isSwitchedOffOriginal : bSwitchOff)) {
            continue;
        }
        CPathNode* next1{};
        CPathNode* next2{};

        SwitchOffNodeAndNeighbours(&node, next1, &next2, bSwitchOff, bBackToOriginal);

        for (auto iter = next1; iter;) {
            SwitchOffNodeAndNeighbours(iter, iter, nullptr, bSwitchOff, bBackToOriginal);
        }
        for (auto iter = next2; iter;) {
            SwitchOffNodeAndNeighbours(iter, iter, nullptr, bSwitchOff, bBackToOriginal);
        }
    }
}

// NOTSA
CPathNode* CPathFind::GetPathNode(CNodeAddress address) {
    assert(address.IsValid());
    assert(IsAreaNodesAvailable(address));
    return &m_pPathNodes[address.m_wAreaId][address.m_wNodeId];
}

// notsa
CCarPathLinkAddress CPathFind::GetNaviLink(uint16 area, uint16 linkId) const {
    assert(area < NUM_PATH_MAP_AREAS);
    assert(linkId < m_anNumAddresses[area]);
    return m_pNaviLinks[area][linkId];
}

// NOTSA
bool CPathFind::FindNodeCoorsForScript(CVector& outPos, CNodeAddress addr) {
    bool valid{};
    outPos = FindNodeCoorsForScript(addr, &valid);
    return valid;
}

// 0x452F40
void CPathFind::LoadPathFindData(int32 areaId) {
    CTimer::Suspend();
    sprintf_s(gString, "data\\paths\\nodes%d.dat", areaId);
    auto* stream = RwStreamOpen(RwStreamType::rwSTREAMFILENAME, RwStreamAccessType::rwSTREAMREAD, gString);
    LoadPathFindData(stream, areaId);
    CTimer::Resume();
}

// 0x4529F0
void CPathFind::LoadPathFindData(RwStream* stream, int32 areaId) {
    RwStreamRead(stream, &m_anNumNodes[areaId],        sizeof(m_anNumNodes[areaId]));
    RwStreamRead(stream, &m_anNumVehicleNodes[areaId], sizeof(m_anNumVehicleNodes[areaId]));
    RwStreamRead(stream, &m_anNumPedNodes[areaId],     sizeof(m_anNumPedNodes[areaId]));
    RwStreamRead(stream, &m_anNumCarPathLinks[areaId], sizeof(m_anNumCarPathLinks[areaId]));
    RwStreamRead(stream, &m_anNumAddresses[areaId],    sizeof(m_anNumAddresses[areaId]));

    assert(m_anNumNodes[areaId] == m_anNumVehicleNodes[areaId] + m_anNumPedNodes[areaId]);

    auto numNodes = m_anNumNodes[areaId];
    if (numNodes) {
        m_pPathNodes[areaId] = new CPathNode[numNodes];
        RwStreamRead(stream, m_pPathNodes[areaId], sizeof(CPathNode) * numNodes);
    } else {
        m_pPathNodes[areaId] = new CPathNode[1];
    }

    auto numCarPathLinks = m_anNumCarPathLinks[areaId];
    if (numCarPathLinks) {
        m_pNaviNodes[areaId] = new CCarPathLink[numCarPathLinks];
        RwStreamRead(stream, m_pNaviNodes[areaId], sizeof(CCarPathLink) * numCarPathLinks);
    } else {
        m_pNaviNodes[areaId] = nullptr;
    }

    auto numAddresses = m_anNumAddresses[areaId];
    if (numAddresses) {
        auto numToAdd = numAddresses + NUM_DYNAMIC_LINKS_PER_AREA * 12;
        m_pNodeLinks[areaId] = new CNodeAddress[numToAdd];
        m_pNaviLinks[areaId] = new CCarPathLinkAddress[numAddresses];
        m_pLinkLengths[areaId] = new uint8[numToAdd];
        m_pPathIntersections[areaId] = new CPathIntersectionInfo[numToAdd];
        RwStreamRead(stream, m_pNodeLinks[areaId], sizeof(CNodeAddress) * numToAdd);
        RwStreamRead(stream, m_pNaviLinks[areaId], sizeof(CCarPathLinkAddress) * numAddresses);
        RwStreamRead(stream, m_pLinkLengths[areaId], sizeof(uint8) * numToAdd);
        RwStreamRead(stream, m_pPathIntersections[areaId], sizeof(CPathIntersectionInfo) * numToAdd);
    } else {
        m_pNodeLinks[areaId] = nullptr;
        m_pNaviLinks[areaId] = nullptr;
        m_pLinkLengths[areaId] = nullptr;
        m_pPathIntersections[areaId] = nullptr;
    }

    for (auto i = 0u; i < numNodes; ++i) {
        auto& node = m_pPathNodes[areaId][i];
        node.m_isSwitchedOffOriginal = node.m_isSwitchedOff;
    }

    for (auto i = 0u; i < m_nNumNodeSwitches; ++i) {
        auto& area = m_aNodeSwitches[i];
        SwitchRoadsOffInAreaForOneRegion(area.xMin, area.xMax, area.yMin, area.yMax, area.zMin, area.zMax, area.isOff, area.isCars, areaId, false);
    }
    for (auto i = 0u; i < NUM_DYNAMIC_LINKS_PER_AREA; ++i) {
        rng::fill(m_aDynamicLinksBaseIds[i], -1);
        rng::fill(m_aDynamicLinksIds[i], -1);
    }
}

// 0x44D0F0
void CPathFind::UnLoadPathFindData(int32 index) {
    delete[] m_pPathNodes[index];
    delete[] m_pNaviNodes[index];
    delete[] m_pNodeLinks[index];
    delete[] m_pNaviLinks[index];
    delete[] m_pLinkLengths[index];
    delete[] m_pPathIntersections[index];

    m_pPathNodes[index] = nullptr;
    m_pNaviNodes[index] = nullptr;
    m_pNodeLinks[index] = nullptr;
    m_pNaviLinks[index] = nullptr;
    m_pLinkLengths[index] = nullptr;
    m_pPathIntersections[index] = nullptr;
}

// 0x44DE00
void CPathFind::LoadSceneForPathNodes(CVector point) {
    rng::fill(ToBeStreamed, false);
    MarkRegionsForCoors(point, 350.f);
    for (const auto [areaId, load] : rngv::enumerate(ToBeStreamed)) {
        if (load) {
            CStreaming::RequestModel(DATToModelId((int32)areaId), STREAMING_DEFAULT);
        }
    }
}

// 0x450DE0
bool CPathFind::IsWaterNodeNearby(CVector position, float radius) {
    for (auto areaId = 0u; areaId < NUM_PATH_MAP_AREAS; areaId++) {
        for (const auto& node : GetPathNodesInArea(areaId, PATH_TYPE_VEH)) {
            if (node.m_bWaterNode) {
                if ((node.GetPosition() - position).SquaredMagnitude() <= sq(radius)) {
                    return true;
                }
            }
        }
    }
    return false;
}

// 0x44F190
float CPathFind::CalcDistToAnyConnectingLinks(CPathNode* node, CVector pos) {
    auto minDistSq = 999999.88f;
    for (auto i = 0u; i < node->m_nNumLinks; i++) {
        const auto linkedAddr = m_pNodeLinks[node->m_wAreaId][node->m_wBaseLinkId + i];
        if (!IsAreaNodesAvailable(linkedAddr)) {
            continue;
        }
        const auto& linked = m_pPathNodes[linkedAddr.m_wAreaId][linkedAddr.m_wNodeId];
        minDistSq = std::min(minDistSq, CCollision::DistToLineSqr(node->GetPosition(), linked.GetPosition(), pos));
    }
    return std::sqrt(minDistSq);
}

// 0x44F2C0
void CPathFind::FindNodeClosestInRegion(CNodeAddress* outAddress, uint16 areaId, CVector pos, uint8 nodeType, float* outDist, bool bLowTraffic, bool bUnkn, bool bBoats, bool bUnused) {
    if (!IsAreaLoaded(areaId)) {
        return;
    }

    // NOTE: For any other `nodeType` the original code uses garbage as the range
    const auto begin = nodeType == PATH_TYPE_PED ? m_anNumVehicleNodes[areaId] : 0u;
    const auto end   = nodeType == PATH_TYPE_VEH ? m_anNumVehicleNodes[areaId] : m_anNumNodes[areaId];
    for (auto i = begin; i < end; i++) {
        auto& node = m_pPathNodes[areaId][i];
        if (bLowTraffic && node.m_isSwitchedOff) {
            continue;
        }
        if (bUnkn && node.unk1) { // Already recorded (See `RecordNodesClosestToCoors`)
            continue;
        }
        if (bBoats != static_cast<bool>(node.m_bWaterNode)) {
            continue;
        }
        const auto nodePos = node.GetPosition();
        auto dist = (std::abs(nodePos.z - pos.z) * 3.0f + std::abs(nodePos.y - pos.y) + std::abs(nodePos.x - pos.x)) * 0.3f;
        if (dist >= *outDist) {
            continue;
        }
        dist += CalcDistToAnyConnectingLinks(&node, pos) * 0.2f;
        if (dist < *outDist) {
            *outDist    = dist;
            *outAddress = CNodeAddress{ areaId, static_cast<uint16>(i) };
        }
    }
}

// 0x44F460
CNodeAddress CPathFind::FindNodeClosestToCoors(CVector pos, ePathType nodeType, float maxDistance, uint16 bLowTraffic, int32, uint16 bIgnoreRecorded, uint16 bBoatsOnly, int32 bIgnoreInteriors) {
    CNodeAddress bestAddress;
    float        bestDist = maxDistance;

    const auto SearchRegion = [&](size_t areaId) {
        FindNodeClosestInRegion(&bestAddress, static_cast<uint16>(areaId), pos, nodeType, &bestDist, bLowTraffic != 0, bIgnoreRecorded != 0, bBoatsOnly != 0, bIgnoreInteriors != 0);
    };
    const auto IsInRange = [](int32 v) { return v >= 0 && v < NUM_PATH_MAP_AREA_X; };

    const auto regionX = std::clamp(static_cast<int32>((pos.x + 3000.0f) / 750.0f), 0, NUM_PATH_MAP_AREA_X - 1);
    const auto regionY = std::clamp(static_cast<int32>((pos.y + 3000.0f) / 750.0f), 0, NUM_PATH_MAP_AREA_Y - 1);

    // Distance to the closest edge of the region the point is in
    auto distToEdge = std::min({
        pos.x - (regionX * 750.0f - 3000.0f),
        (regionX + 1) * 750.0f - 3000.0f - pos.x,
        pos.y - (regionY * 750.0f - 3000.0f),
        (regionY + 1) * 750.0f - 3000.0f - pos.y,
    });

    SearchRegion(regionX + regionY * NUM_PATH_MAP_AREA_X);

    // Search the regions around in growing rings, as long as a closer node may still be found there
    for (auto ring = 1; ring < 5 && bestDist > distToEdge; ring++, distToEdge += 750.0f) {
        const auto minX = regionX - ring, maxX = regionX + ring;
        const auto minY = regionY - ring, maxY = regionY + ring;
        for (const auto x : { minX, maxX }) {
            if (!IsInRange(x)) {
                continue;
            }
            for (auto y = minY; y <= maxY; y++) {
                if (IsInRange(y)) {
                    SearchRegion(x + y * NUM_PATH_MAP_AREA_X);
                }
            }
        }
        for (const auto y : { minY, maxY }) {
            if (!IsInRange(y)) {
                continue;
            }
            for (auto x = minX + 1; x < maxX; x++) {
                if (IsInRange(x)) {
                    SearchRegion(x + y * NUM_PATH_MAP_AREA_X);
                }
            }
        }
    }

    if (!bIgnoreInteriors) {
        for (auto areaId = NUM_PATH_MAP_AREAS; areaId < NUM_TOTAL_PATH_NODE_AREAS; areaId++) {
            SearchRegion(areaId);
        }
    }

    return bestAddress;
}

// 0x44FA30
void CPathFind::RecordNodesClosestToCoors(CVector pos, uint8 nodeType, int count, CNodeAddress* outAddresses, float maxDist, bool bLowTraffic, bool bUnkn, bool bBoats, bool bIgnoreInteriors) {
    // Clear the "recorded" flag of all the nodes of this type
    for (auto areaId = 0; areaId < NUM_TOTAL_PATH_NODE_AREAS; areaId++) {
        if (!IsAreaLoaded(areaId)) {
            continue;
        }
        const auto begin = nodeType == PATH_TYPE_PED ? m_anNumVehicleNodes[areaId] : 0u;
        const auto end   = nodeType == PATH_TYPE_VEH ? m_anNumVehicleNodes[areaId] : m_anNumNodes[areaId];
        for (auto i = begin; i < end; i++) {
            m_pPathNodes[areaId][i].unk1 = false;
        }
    }

    for (auto i = 0; i < count; i++) {
        const auto addr = FindNodeClosestToCoors(pos, static_cast<ePathType>(nodeType), maxDist, bLowTraffic, bUnkn, true, bBoats, bIgnoreInteriors);
        if (!addr.IsAreaValid()) {
            break;
        }
        m_pPathNodes[addr.m_wAreaId][addr.m_wNodeId].unk1 = true;
        outAddresses[i] = addr;
    }
}

// 0x450A60
void CPathFind::UpdateStreaming(bool bForceStreaming) {
    ZoneScoped;

    // The time thingy I think is some kind of `% 512`, not sure yet, will have to figure it out.
    if (!s_bLoadPathsNeeded && !bForceStreaming && (CTimer::m_snTimeInMilliseconds ^ CTimer::m_snPreviousTimeInMilliseconds) < 512) {
        return;
    }

    rng::fill(ToBeStreamed, false);
    std::array<bool, NUM_PATH_MAP_AREAS> ToBeStreamedForScript{};

    // Mark areas around the player
    if (FindPlayerPed()) {
        MarkRegionsForCoors(FindPlayerCoors(), 350.f);
    }

    // Mark areas requested by `SetPathsNeededAtPosition`
    if (s_bLoadPathsNeeded) {
        MarkRegionsForCoors(s_pathsNeededPosn, 300.f);
        s_bLoadPathsNeeded = false;
    }

    // Mark paths around some specific mission vehicles
    for (const auto& veh : GetVehiclePool()->GetAllValid()) {
        if (!veh.IsMissionVehicle()) {
            continue;
        }

        switch (veh.m_nVehicleSubType) {
        case VEHICLE_TYPE_HELI:
        case VEHICLE_TYPE_PLANE:
        case VEHICLE_TYPE_BOAT:
        case VEHICLE_TYPE_TRAIN:
        case VEHICLE_TYPE_FPLANE:
            break;
        default:
            MarkRegionsForCoors(veh.GetPosition(), 300.f);
        }
    }

    // Mark areas inside load request rect
    if (m_loadAreaRequestPending) {
        IterAreasTouchingRect(
            { m_loadAreaRequestMinX, m_loadAreaRequestMinY, m_loadAreaRequestMaxX, m_loadAreaRequestMaxY },
            [&, this](auto areaId) {
                ToBeStreamed[areaId] = ToBeStreamedForScript[areaId] = true;
                return true;
            }
        );
    }

    // Load/unload areas as per `ToBeStreamed`
    for (const auto [areaId, shouldBeLoaded] : rngv::enumerate(ToBeStreamed)) {
        if (shouldBeLoaded) {
            if (!IsAreaLoaded(areaId)) {
                CStreaming::RequestModel(
                    DATToModelId(areaId),
                    ToBeStreamedForScript[areaId]
                        ? STREAMING_MISSION_REQUIRED
                        : STREAMING_KEEP_IN_MEMORY
                );
                NOTSA_LOG_DEBUG("Requested area: {}", (int)areaId);
            }
        } else if (IsAreaLoaded(areaId)) {
            CStreaming::RemoveModel(DATToModelId(areaId));
            NOTSA_LOG_DEBUG("Removed area: {}", (int)areaId);
        }
    }
}

// 0x44DE80
void CPathFind::StartNewInterior(int32 interiorNum) {
    InteriorIDBeingBuilt = interiorNum;
    bInteriorBeingBuilt = true;
    NumNodesGiven = 0;
    NumLinksToExteriorNodes = 0;

    // BUG: Possible endless loop if 8 interiors are loaded i think
    NewInteriorSlot = 0;
    while (m_interiorIDs[NewInteriorSlot] != (uint32)-1) {
        NewInteriorSlot++;
        assert(NewInteriorSlot < 8);
    }
}

// 0x450E90
CNodeAddress CPathFind::AddNodeToNewInterior(
    float x,
    float y,
    float z,
    bool bDontWander,
    int8 con0,
    int8 con1,
    int8 con2,
    int8 con3,
    int8 con4,
    int8 con5
) {
    const auto idx = NumNodesGiven++;
    XCoorGiven[idx] = x;
    YCoorGiven[idx] = y;
    ZCoorGiven[idx] = z;
    DontWanderGiven[idx] = bDontWander;
    rng::copy(std::array{ con0, con1, con2, con3, con4, con5 }, ConnectsToGiven[idx].begin());
    return { (uint16)(NUM_PATH_MAP_AREAS + NewInteriorSlot), (uint16)idx };
}

// 0x451300 unused
CNodeAddress CPathFind::ReturnInteriorNodeIndex(int32 unkn, uint32 intId, int16 nodeId) {
    for (auto i = 0; i < NUM_PATH_INTERIOR_AREAS; ++i) {
        if (m_interiorIDs[i] == intId) {
            return CNodeAddress(NUM_PATH_MAP_AREAS + i, nodeId);
        }
    }
    return {};
}

// 0x451350
CCarPathLinkAddress CPathFind::FindLinkBetweenNodes(CNodeAddress nodeAddrA, CNodeAddress nodeAddrB) {
    if (AreNodeAreasLoaded({ nodeAddrA, nodeAddrB })) {
        const auto nodeA = GetPathNode(nodeAddrA);
        for (auto i = 0u; i < nodeA->m_nNumLinks; i++) {
            const auto linkIdx = nodeA->m_wBaseLinkId + i;
            if (m_pNodeLinks[nodeA->m_wAreaId][linkIdx] == nodeAddrB) {
                return m_pNaviLinks[nodeA->m_wAreaId][linkIdx];
            }
        }
    }
    return {};
}

// 0x4513F0
CVector CPathFind::FindParkingNodeInArea(float minX, float maxX, float minY, float maxY, float minZ, float maxZ) {
    // Loop over all areas
    for (size_t areaId = 0; areaId < NUM_PATH_MAP_AREAS; ++areaId) {
        // Get all vehicle nodes in this area
        auto nodes = GetPathNodesInArea(areaId, PATH_TYPE_VEH);

        for (const auto& node : nodes) {
            CVector pos = node.GetPosition();

            // Check if node position is within the bounds
            if (pos.x < minX || pos.x > maxX) {
                continue;
            }
            if (pos.y < minY || pos.y > maxY) {
                continue;
            }
            if (pos.z < minZ || pos.z > maxZ) {
                continue;
            }

            // Check if it's a parking node (behaviour type 2)
            if (node.m_nBehaviourType == 2) {
                return pos;
            }
        }
    }
    // Return zero vector if not found
    return CVector(0.0f, 0.0f, 0.0f);
}


// 0x450F30
CNodeAddress CPathFind::FindNearestExteriorNodeToInteriorNode(int32 interiorId) {
    return FindNodeClosestToCoors(
        { XCoorGiven[interiorId], YCoorGiven[interiorId], ZCoorGiven[interiorId] },
        ePathType::PATH_TYPE_PED,
        3.f,
        0,
        0,
        0,
        0,
        true
    );
}

// 0x44E000
void CPathFind::AddDynamicLinkBetween2Nodes_For1Node(CNodeAddress first, CNodeAddress second) {
    assert(IsAreaNodesAvailable(first));

    auto& firstPathInfo = m_pPathNodes[first.m_wAreaId][first.m_wNodeId];
    auto numAddresses = m_anNumAddresses[first.m_wAreaId];

    uint32 firstLinkId;
    if (static_cast<uint32>(firstPathInfo.m_wBaseLinkId) >= numAddresses)
        firstLinkId = firstPathInfo.m_wBaseLinkId;
    else {
        auto* nodeLink = &m_pNodeLinks[first.m_wAreaId][numAddresses];
        auto linkCounter = 0u;
        while (!nodeLink->IsValid()) {
            nodeLink += 12; // No clue why we jump 12 objects each time (Search: MAGIC_NUM_12)
            ++linkCounter;
        }

        firstLinkId = numAddresses + 12 * linkCounter;
        for (auto i = 0u; i < firstPathInfo.m_nNumLinks; ++i) {
            m_pNodeLinks[first.m_wAreaId][firstLinkId + i] = m_pNodeLinks[first.m_wAreaId][firstPathInfo.m_wBaseLinkId + i];
            m_pLinkLengths[first.m_wAreaId][firstLinkId + i] = m_pLinkLengths[first.m_wAreaId][firstPathInfo.m_wBaseLinkId + i];
            m_pPathIntersections[first.m_wAreaId][firstLinkId + i] = m_pPathIntersections[first.m_wAreaId][firstPathInfo.m_wBaseLinkId + i];
        }

        if (first.m_wAreaId < NUM_PATH_MAP_AREAS) {
            auto& linkInfo = m_aDynamicLinksBaseIds[first.m_wAreaId];
            for (auto i = 0u; i < NUM_DYNAMIC_LINKS_PER_AREA; ++i) {
                if (linkInfo[i] == -1) {
                    m_aDynamicLinksBaseIds[first.m_wAreaId][i] = firstPathInfo.m_wBaseLinkId;
                    m_aDynamicLinksIds[first.m_wAreaId][i] = firstLinkId;
                    break;
                }
            }
        }
    }

    m_pNodeLinks[first.m_wAreaId][firstLinkId + firstPathInfo.m_nNumLinks] = second;
    m_pLinkLengths[first.m_wAreaId][firstLinkId + firstPathInfo.m_nNumLinks] = 5;
    m_pPathIntersections[first.m_wAreaId][firstLinkId + firstPathInfo.m_nNumLinks].Clear();
    firstPathInfo.m_nNumLinks++;
    firstPathInfo.m_wBaseLinkId = firstLinkId;
}

// 0x44D230
bool CPathFind::These2NodesAreAdjacent(CNodeAddress nodeAddress1, CNodeAddress nodeAddress2) {
    const auto node1 = GetPathNode(nodeAddress1);
    for (auto i = 0u; i < node1->m_nNumLinks; i++) {
        if (m_pNodeLinks[node1->m_wAreaId][node1->m_wBaseLinkId + i] == nodeAddress2) {
            return true;
        }
    }
    return false;
}

// 0x44FCE0
CNodeAddress CPathFind::FindNodeClosestToCoorsFavourDirection(CVector pos, ePathType nodeType, CVector2D dir) {
    dir = dir.Normalized(); // In-place normalize
    
    CNodeAddress closest{};
    float        scoreOfClosest{std::numeric_limits<float>::max()};
    for (auto areaId{ 0u }; areaId < NUM_TOTAL_PATH_NODE_AREAS; areaId++) {
        for (const auto& node : GetPathNodesInArea(areaId, nodeType)) { // NOTE: Function takes care of checking whenever the area is loaded
            const auto playerToNodeDirection = node.GetPosition() - pos;

            const auto dotScore = (abs(playerToNodeDirection) * CVector { 1.f, 1.f, 3.f }).ComponentwiseSum();
            if (dotScore >= scoreOfClosest) {
                continue;
            }

            const auto score = dotScore - (dir.Dot(CVector2D{ playerToNodeDirection }.Normalized()) - 1.f) * 20.f;
            if (score > scoreOfClosest) {
                continue;
            }

            scoreOfClosest = score;
            closest = node.GetAddress();
        }
    }
    return closest;
}

// 0x5D34C0
bool CPathFind::Save() {
    CGenericGameStorage::SaveDataToWorkBuffer(m_nNumNodeSwitches);
    for (auto& area : std::span{ m_aNodeSwitches, m_nNumNodeSwitches }) {
        CGenericGameStorage::SaveDataToWorkBuffer(area);
    }
    return true;
}

// 0x5D3500
bool CPathFind::Load() {
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nNumNodeSwitches);
    for (auto& area : std::span{ m_aNodeSwitches, m_nNumNodeSwitches }) {
        CGenericGameStorage::LoadDataFromWorkBuffer(area);
    }
    return true;
}

bool CPathFind::AreNodeAreasLoaded(const std::initializer_list<CNodeAddress>& addrs) const {
    return rng::all_of(addrs, [this](auto&& addr) { return IsAreaNodesAvailable(addr); });
}

// 0x44DCD0
void CPathFind::SetPathsNeededAtPosition(const CVector& posn) {
    s_pathsNeededPosn = posn;
    s_bLoadPathsNeeded = true;
}

namespace detail {
constexpr size_t RegionValueOf(float p, size_t nareas) {
    return std::clamp((uint32)((p + 3000.f) / (6000.f / (float)nareas)), 0u, (uint32)nareas - 1);
}
}; // namespace detail

size_t CPathFind::FindXRegionForCoors(float x) const {
    return detail::RegionValueOf(x, NUM_PATH_MAP_AREA_X);
}

size_t CPathFind::FindYRegionForCoors(float y) const {
    return detail::RegionValueOf(y, NUM_PATH_MAP_AREA_Y);
}

// 0x44DB60
void CPathFind::MarkRegionsForCoors(CVector pos, float radius) {
    // HACK: Since the below function isnt `static` (TODO...) we gotta use the class instance here...
    ThePaths.IterAreasTouchingRect(
        { pos, radius },
        [](auto areaId) {
            ToBeStreamed[areaId] = true;
            return true;
        }
    );
}

// 0x44D3E0 - Moved to CPathNode
bool CPathFind::ThisNodeHasToBeSwitchedOff(CPathNode* node) {
    return node->HasToBeSwitchedOff();
}

// 0x4504F0
// This function is only called from `SwitchOffNodeAndNeighbours` but when unhooked
// it doesn't spoil `eax` which makes the former crash
// so hopefully the `__asm mov eax, this` fixes it
// If not just lock both :D
size_t CPathFind::CountNeighboursToBeSwitchedOff(const CPathNode& node) {
    const auto ret = (size_t)rng::count_if(GetNodeLinkedNodes(node), &CPathNode::HasToBeSwitchedOff);
    __asm mov eax, this // It has to be `this`
    return ret;
}

// 0x450320
// Returns the orientation in degrees for car placement at the given node.
// Reverse engineered from FUN_00450320, 100% faithful version.
float CPathFind::FindNodeOrientationForCarPlacement(CNodeAddress nodeInfo) {
    // Get area and node id from input address
    uint16 areaId = nodeInfo.m_wAreaId;
    uint16 nodeId = nodeInfo.m_wNodeId;

    // Sanity: get node array for area
    CPathNode* nodes = m_pPathNodes[areaId];
    if (!nodes) {
        return 0.0f; // Fallback: node array not loaded
    }

    // Sanity: get node
    CPathNode& node = nodes[nodeId];

    // The node must have at least one link
    if (node.m_nNumLinks == 0) {
        return 0.0f;
    }

    // Find the first valid link (with flags filter)
    int foundIdx = -1;
    for (uint8_t i = 0; i < node.m_nNumLinks; ++i) {
        CNodeAddress linkAddr   = m_pNodeLinks[areaId][node.m_wBaseLinkId + i];
        uint16       linkAreaId = linkAddr.m_wAreaId;

        // Area of linked node must be loaded
        if (!IsAreaLoaded(linkAreaId)) {
            continue;
        }

        // Get linked node
        CPathNode* linkNodes  = m_pPathNodes[linkAreaId];
        CPathNode& linkedNode = linkNodes[linkAddr.m_wNodeId];

        // Check node ids and flags (matches ASM: checks if nodeId/areaId pair matches and flag bits)
        // In the original ASM, it checks a byte at offset 0x18 & 0x0F != 0
        uint8 flags = node.m_nBehaviourType; // CPathNode offset 0x18 is the fourth byte after all bitfields, likely behaviourType
        if ((flags & 0x0F) == 0) {
            continue;
        }

        foundIdx = i;
        break;
    }

    // If no valid link found, return 0.0f (just like the fallback in ASM)
    if (foundIdx == -1) {
        return 0.0f;
    }

    // Get first valid link address and nodes
    CNodeAddress linkAddr   = m_pNodeLinks[areaId][node.m_wBaseLinkId + foundIdx];
    uint16       linkAreaId = linkAddr.m_wAreaId;

    if (!IsAreaLoaded(linkAreaId)) {
        return 0.0f;
    }

    CPathNode* linkNodes  = m_pPathNodes[linkAreaId];
    CPathNode& linkedNode = linkNodes[linkAddr.m_wNodeId];

    // Get world positions
    CVector pos1 = node.GetPosition();
    CVector pos2 = linkedNode.GetPosition();

    // Calculate difference
    float dx = pos2.x - pos1.x;
    float dy = pos2.y - pos1.y;

    // Calculate angle in radians and convert to degrees
    float angle = RadiansToDegrees(std::atan2(dy, dx));
    return angle;
}


// 0x452160
void CPathFind::SwitchOffNodeAndNeighbours(CPathNode* node, CPathNode*& outNext1, CPathNode** outNext2, bool bWhatToSwitchTo, bool bBackToOriginal) {
    node->m_isSwitchedOff = bBackToOriginal ? node->m_isSwitchedOffOriginal : bWhatToSwitchTo;
   
    outNext1 = nullptr;
    if (outNext2) {
        *outNext2 = nullptr;
    }

    if (CountNeighboursToBeSwitchedOff(*node) > 2) {
        return;
    }

    for (auto& linked : GetNodeLinkedNodes(*node)) {
        if (!linked.HasToBeSwitchedOff()) {
            continue;
        }
        if (linked.m_isSwitchedOff == bWhatToSwitchTo) {
            continue;
        }
        if (CountNeighboursToBeSwitchedOff(*node) > 2) {
            continue;
        }
        if (!outNext1) {
            outNext1 = &linked;
        }
#ifdef FIX_BUGS // Above it was checked whenever it's set so I assume this was a bug
        else if (outNext2) // Don't get confused, `outNext2` is a ptr to a ptr
#else
        else
#endif
        {
            *outNext2 = &linked;
        }
    }
}

// 0x44DF30
void CPathFind::AddInteriorLinkToExternalNode(int32 interiorNodeIdx, CNodeAddress externalNodeAddr) {
    const auto idx = NumLinksToExteriorNodes++;
    aInteriorNodeLinkedToExterior[idx] = interiorNodeIdx;
    aExteriorNodeLinkedTo[idx] = externalNodeAddr;
}

// 0x44E1A0
void CPathFind::RemoveInterior(uint32 intId) {
    for (auto intSlot = 0u; intSlot < NUM_PATH_INTERIOR_AREAS; intSlot++) {
        const auto intSlotAreaId = NUM_PATH_MAP_AREAS + intSlot;

        if (m_interiorIDs[intSlot] != intId) {
            continue;
        }

        for (auto areaId = 0u; areaId < NUM_TOTAL_PATH_NODE_AREAS; areaId++) {
            for (auto& node : GetPathNodesInArea(areaId, PATH_TYPE_PED)) {
                // I assume this checks if the link is an interior link?
                if ((int32)node.m_wBaseLinkId < (int32)m_anNumAddresses[areaId]) {
                    continue;
                }

                // Remove all link of this node that point to a node int the current interior
                bool foundNodeFromOtherInt{}, foundNodeFromThisInt{};
                (void)rng::remove_if(GetNodeLinkedNodes(node, false), [&](CPathNode& linkedNode) {
                    if (linkedNode.m_wAreaId == intSlotAreaId) {
                        node.m_nNumLinks--;
                        foundNodeFromOtherInt = true;
                        return true;
                    } else if (linkedNode.m_wAreaId >= NUM_PATH_MAP_AREAS) {
                        foundNodeFromThisInt = true;
                    }
                    return false;
                });

                // If we found a linked node and there was no other node from another interior
                if (foundNodeFromOtherInt || !foundNodeFromThisInt) {
                    continue;
                }

                // TODO: Magic number `12` (Search: MAGIC_NUM_12)
                // Null out all links of this node
                rng::fill(std::span{ &m_pNodeLinks[node.m_wAreaId][node.m_wBaseLinkId], 12 }, CNodeAddress{});

                // Delete dynamic link of this area
                // Honestly, this doesn't make much sense... As in, I don't think these are dynamic areas? We'll see.. TODO
                const auto& dynLinks  = m_aDynamicLinksIds[intSlot];
                const auto  dynLinkIt = rng::find(dynLinks, (int32)node.m_wBaseLinkId);
                if (dynLinkIt != rng::end(dynLinks)) {
                    const auto dynLinkIdx = rng::distance(rng::begin(dynLinks), dynLinkIt);
                    node.m_wBaseLinkId = m_aDynamicLinksBaseIds[intSlot][dynLinkIdx];
                    m_aDynamicLinksBaseIds[intSlot][dynLinkIdx] = -1;
                    m_aDynamicLinksIds[intSlot][dynLinkIdx] = -1;
                }
            }
        }

        // Finally, unload area and related data
        const auto FreeAndNull = [](auto& ptr) {
            CMemoryMgr::Free(ptr);
            ptr = nullptr;
        };
        FreeAndNull(m_pPathIntersections[intSlotAreaId]);
        FreeAndNull(m_pLinkLengths[intSlotAreaId]);
        FreeAndNull(m_pPathNodes[intSlotAreaId]);
        FreeAndNull(m_pLinkLengths[intSlotAreaId]);

        m_anNumAddresses[intSlotAreaId] = 0;
        m_anNumCarPathLinks[intSlotAreaId] = 0;
        m_anNumPedNodes[intSlotAreaId] = 0;
        m_anNumVehicleNodes[intSlotAreaId] = 0;
        m_anNumNodes[intSlotAreaId] = 0;
    }
}

// 0x44D930
void CPathFind::FindStartPointOfRegion(size_t x, size_t y, float& outX, float& outY) {
    const auto pos = FindStartPointOfRegion(x, y);
    outX = pos.x;
    outY = pos.y;
}

// notsa
CVector2D CPathFind::FindStartPointOfRegion(size_t x, size_t y) {
    return { FindXCoorsForRegion(x), FindYCoorsForRegion(y) };
}

namespace detail {
constexpr auto GetCoorsOfRegion(size_t p, size_t nareas) {
    return (6000.f / (float)nareas) * (float)p - 3000.f;
}
};

// 0x44D8F0
float CPathFind::FindXCoorsForRegion(size_t x) {
    return detail::GetCoorsOfRegion(x, NUM_PATH_MAP_AREA_X);
}

// 0x44D910
float CPathFind::FindYCoorsForRegion(size_t y) {
    return detail::GetCoorsOfRegion(y, NUM_PATH_MAP_AREA_Y);
}

// 0x44DED0
void CPathFind::AddInteriorLink(int32 intNodeA, int32 intNodeB) {
    const auto AddLink = [](int32 intIdx, int32 linkTo) {
        const auto it = rng::find(ConnectsToGiven[intIdx], -1); 
        assert(!(*it >= 0)); // NOTE: Original code did a `while (*it >= 0), if anything goes bad use `>= 0` for `rng::find`
        *it = linkTo;
    };
    AddLink(intNodeA, intNodeB);
    AddLink(intNodeB, intNodeA);
}

// notsa
std::span<CPathNode> CPathFind::GetPathNodesInArea(size_t areaId, ePathType ptype) const {
    if (const auto allNodes = m_pPathNodes[areaId]) {
        const auto numVehNodes = m_anNumVehicleNodes[areaId];
        switch (ptype) {
        case ePathType::PATH_TYPE_VEH: // Vehicles, boats, race tracks
            return std::span{ allNodes, m_anNumVehicleNodes[areaId] };
        case ePathType::PATH_TYPE_PED: // Peds only
            assert(m_anNumPedNodes[areaId] == m_anNumNodes[areaId] - numVehNodes); // Pirulax: I'm assuming this is true, so if this doesnt assert for a long time remove it
            return std::span{ allNodes + numVehNodes, m_anNumPedNodes[areaId] };
        case ePathType::PATH_TYPE_ALL: // All of the above
            return std::span{ allNodes, m_anNumNodes[areaId] };
        default:
            NOTSA_UNREACHABLE("Invalid pathType: {}", (int)ptype);
        }
    }
    return {}; // Area not loaded, return nothing.. Perhaps assert here instead?
}

// 0x44D1B0
void CPathFind::RemoveNodeFromList(CPathNode* node) {
    node->m_prev->m_next = node->m_next;
    if (node->m_next) {
        node->m_next->m_prev = node->m_prev;
    }

    m_totalNumNodesInPathFindHashTable--;
}

void CPathFind::AddNodeToList(CPathNode* node, int32 distFromOrigin) {
    // Insert the node as the head into it's bucket

    auto& head = m_pathFindHashTable[distFromOrigin % std::size(m_pathFindHashTable)];

    node->m_next = head;

    // Make this node's `next` point to the head in the hash table
    // I guess this works as long as you only access the `m_prev` variable
    // as that's at offset 0
    // This is a really bad hack to avoid having to do special handling
    // for the head in `RemoveNodeFromList`...
    node->m_prev = reinterpret_cast<CPathNode*>(&head);

    if (head) {
        head->m_prev = node;
    }

    head = node;

    assert(distFromOrigin <= std::numeric_limits<decltype(node->m_totalDistFromOrigin)>::max()); // Prevent bugs from overflow
    node->m_totalDistFromOrigin = (int16)distFromOrigin;

    m_totalNumNodesInPathFindHashTable++;
}

// 0x452C80
void CPathFind::SwitchRoadsOffInArea(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax, bool bSwitchOff, bool bCars, bool bBackToOriginal) {
    for (auto areaId = 0u; areaId < NUM_PATH_MAP_AREAS; ++areaId) {
        SwitchRoadsOffInAreaForOneRegion(xMin, xMax, yMin, yMax, zMin, zMax, bSwitchOff, bCars, areaId, bBackToOriginal);
    }

    for (auto i = 0u; i < m_nNumNodeSwitches; ++i) {
        auto* pArea = &m_aNodeSwitches[i];

        if (notsa::bugfixes::CPathFind_SwitchRoadsOffInArea_StrayAreas) {
            // some missions create both types of switches at the same area, so we store them separately
            if (pArea->isCars != bCars) {
                continue;
            }

            // avoid creating stray areas, potentially leaving no space for important areas later in game
            // ideally, the script would use SWITCH_ROADS_BACK_TO_ORIGINAL or SWITCH_PED_ROADS_BACK_TO_ORIGINAL but that's not always the case
            // so we consider toggling the same area as "back to original"
            if (pArea->xMin == xMin && pArea->yMin == yMin && pArea->zMin == zMin && pArea->xMax == xMax && pArea->yMax == yMax && pArea->zMax == zMax && pArea->isOff != bSwitchOff) {
                bBackToOriginal = true;
            }
        }

        // If the existing area is completely inside the area we are switching off, remove it
        if (pArea->xMin < xMin || pArea->yMin < yMin || pArea->zMin < zMin || pArea->xMax > xMax || pArea->yMax > yMax || pArea->zMax > zMax) {
            continue;
        }

        for (auto j = i; j < m_nNumNodeSwitches - 1; ++j) {
            if (notsa::bugfixes::CPathFind_SwitchRoadsOffInArea_StrayAreas) {
                m_aNodeSwitches[j] = m_aNodeSwitches[j + 1];
            } else {
                // R* bug, they messed up with the index
                m_aNodeSwitches[i] = m_aNodeSwitches[i + 1];
            }
        }

        --m_nNumNodeSwitches;
        --i;
    }

    if (!bBackToOriginal && m_nNumNodeSwitches < NUM_PATH_MAP_AREAS) {
        auto& area  = m_aNodeSwitches[m_nNumNodeSwitches];
        area.xMin   = xMin;
        area.xMax   = xMax;
        area.yMin   = yMin;
        area.yMax   = yMax;
        area.zMin   = zMin;
        area.zMax   = zMax;
        area.isOff  = bSwitchOff;
        area.isCars = bCars;
        m_nNumNodeSwitches++;
    }
}

// 0x452F00
void CPathFind::SwitchPedRoadsOffInArea(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax, bool bSwitchOff, bool bBackToOriginal) {
    SwitchRoadsOffInArea(xMin, xMax, yMin, yMax, zMin, zMax, bSwitchOff, false, bBackToOriginal);
}

// 0x44E4F0
void CPathFind::RemoveBadStartNode(CVector pos, CNodeAddress* address, int16* numPathFindNodes) {
    if (*numPathFindNodes < 2) {
        return;
    }
    if (!IsAreaNodesAvailable(address[0]) || !IsAreaNodesAvailable(address[1])) {
        return;
    }

    // If the position is between the first two nodes the first one is behind us, so drop it
    const auto first  = CVector2D{ GetPathNode(address[0])->GetPosition() } - CVector2D{ pos };
    const auto second = CVector2D{ GetPathNode(address[1])->GetPosition() } - CVector2D{ pos };
    if (first.x * second.x + first.y * second.y >= 0.0f) {
        return;
    }

    (*numPathFindNodes)--;
    for (auto i = 0; i < *numPathFindNodes; i++) {
        address[i] = address[i + 1];
    }
}

// 0x44EFC0
float CPathFind::CalcRoadDensity(float x, float y) {
    auto density = 0.0f;
    for (auto areaId = 0; areaId < NUM_PATH_MAP_AREAS; areaId++) {
        if (!IsAreaLoaded(areaId)) {
            continue;
        }
        for (auto nodeId = 0u; nodeId < m_anNumVehicleNodes[areaId]; nodeId++) {
            const auto& node    = m_pPathNodes[areaId][nodeId];
            const auto  nodePos = node.GetPosition();
            if (std::abs(nodePos.x - x) >= 80.0f || std::abs(nodePos.y - y) >= 80.0f) {
                continue;
            }
            for (auto i = 0u; i < node.m_nNumLinks; i++) {
                const auto linkedAddr = m_pNodeLinks[areaId][node.m_wBaseLinkId + i];
                if (!IsAreaNodesAvailable(linkedAddr)) {
                    continue;
                }
                const auto linkedPos = m_pPathNodes[linkedAddr.m_wAreaId][linkedAddr.m_wNodeId].GetPosition();
                const auto dist      = (CVector2D{ nodePos } - CVector2D{ linkedPos }).Magnitude();
                const auto naviAddr  = m_pNaviLinks[areaId][node.m_wBaseLinkId + i];
                if (!IsAreaLoaded(naviAddr.m_wAreaId)) {
                    continue;
                }
                const auto& navi = m_pNaviNodes[naviAddr.m_wAreaId][naviAddr.m_wCarPathLinkId];
                density += static_cast<float>(navi.m_numOppositeDirLanes) * dist + static_cast<float>(navi.m_numSameDirLanes) * dist;
            }
        }
    }
    return density * 0.0004f;
}

// 0x452090
void CPathFind::Find2NodesForCarCreation(CVector pos, CNodeAddress* outAddress1, CNodeAddress* outAddress2, bool bLowTraffic) {
    std::array<CNodeAddress, 4> nodes{};
    RecordNodesClosestToCoors(pos, PATH_TYPE_VEH, static_cast<int>(nodes.size()), nodes.data(), 999999.88f, bLowTraffic, false, false, true);

    if (!nodes[0].IsAreaValid()) {
        outAddress1->ResetAreaId();
        outAddress2->ResetAreaId();
        return;
    }

    *outAddress1 = nodes[0];
    for (auto i = 1u; i < nodes.size(); i++) { // NB: If none is found `outAddress2` is left untouched
        if (nodes[i].IsAreaValid() && !These2NodesAreAdjacent(nodes[0], nodes[i])) {
            *outAddress2 = nodes[i];
            return;
        }
    }
}

// 0x44DF60
void CPathFind::RemoveInteriorLinks(uint32 intIdx) {
    for (auto node = 0u; node < NumNodesGiven; node++) {
        for (auto& connection : ConnectsToGiven[node]) {
            if (node == intIdx || connection == static_cast<int32>(intIdx)) {
                connection = -1;
            }
        }
    }
}

// 0x452270
void CPathFind::CompleteNewInterior(CNodeAddress* outAddress) {
    if (outAddress) {
        outAddress->ResetAreaId();
    }

    if (NumNodesGiven) {
        const auto areaId = static_cast<uint16>(NUM_PATH_MAP_AREAS + NewInteriorSlot);

        auto floodFill = static_cast<uint8>(NewInteriorSlot + 100);
        if (NumLinksToExteriorNodes > 0) {
            floodFill = GetPathNode(aExteriorNodeLinkedTo[0])->m_nFloodFill;
        }

        m_interiorIDs[NewInteriorSlot] = InteriorIDBeingBuilt;

        // Create the nodes
        m_pPathNodes[areaId] = static_cast<CPathNode*>(CMemoryMgr::Malloc(NumNodesGiven * sizeof(CPathNode)));
        for (auto i = 0u; i < NumNodesGiven; i++) {
            auto& node = m_pPathNodes[areaId][i];
            node.m_vPos                  = CVector{ XCoorGiven[i], YCoorGiven[i], ZCoorGiven[i] };
            node.m_wNodeId               = static_cast<uint16>(i);
            node.m_wAreaId               = areaId;
            node.m_nPathWidth            = 0;
            node.m_nFloodFill            = floodFill;
            node.m_onDeadEnd             = false;
            node.m_isSwitchedOff         = true;
            node.m_isSwitchedOffOriginal = true;
            node.m_bRoadBlocks           = false;
            node.m_bWaterNode            = false;
            node.unk1                    = false;
            node.m_bDontWander           = DontWanderGiven[i];
            node.unk2                    = true;
            node.m_bNotHighway           = false;
            node.m_bHighway              = false;
            node.m_nSpawnProbability     = 15;
            node.m_nBehaviourType        = 0;
            node.m_totalDistFromOrigin   = SHRT_MAX - 1;
        }

        // Make all the connections two-way
        for (auto i = 0u; i < NumNodesGiven; i++) {
            for (const auto connection : ConnectsToGiven[i]) {
                if (connection < 0) {
                    continue;
                }
                auto& others = ConnectsToGiven[connection];
                if (rng::find(others, static_cast<int8>(i)) != others.end()) {
                    continue;
                }
                if (const auto it = rng::find_if(others, [](int8 c) { return c < 0; }); it != others.end()) {
                    *it = static_cast<int8>(i);
                }
            }
        }

        auto numLinks = 0u;
        for (auto i = 0u; i < NumNodesGiven; i++) {
            numLinks += static_cast<uint32>(rng::count_if(ConnectsToGiven[i], [](int8 c) { return c >= 0; }));
        }

        // Some extra space is left for dynamic links (See `AddDynamicLinkBetween2Nodes_For1Node`)
        constexpr auto NUM_EXTRA_LINKS = 192u;
        m_pNodeLinks[areaId]         = static_cast<CNodeAddress*>(CMemoryMgr::Malloc((numLinks + NUM_EXTRA_LINKS) * sizeof(CNodeAddress)));
        m_pLinkLengths[areaId]       = static_cast<uint8*>(CMemoryMgr::Malloc(numLinks + NUM_EXTRA_LINKS));
        m_pPathIntersections[areaId] = static_cast<CPathIntersectionInfo*>(CMemoryMgr::Malloc(numLinks + NUM_EXTRA_LINKS));

        // Create the links
        auto linkId = 0u;
        for (auto i = 0u; i < NumNodesGiven; i++) {
            auto& node = m_pPathNodes[areaId][i];
            node.m_wBaseLinkId = static_cast<int16>(linkId);
            node.m_nNumLinks   = 0;
            for (const auto connection : ConnectsToGiven[i]) {
                if (connection < 0) {
                    continue;
                }
                // 0x450FB0 - Inlined
                const auto& linked = m_pPathNodes[areaId][connection];
                const auto  length = static_cast<int32>(std::min((node.GetPosition() - linked.GetPosition()).Magnitude(), 255.0f));
                m_pNodeLinks[areaId][linkId]   = CNodeAddress{ areaId, static_cast<uint16>(connection) };
                m_pLinkLengths[areaId][linkId] = static_cast<uint8>(std::max(length, 1));
                m_pPathIntersections[areaId][linkId].Clear();
                node.m_nNumLinks++;
                linkId++;
            }
        }
        for (auto i = numLinks; i < numLinks + NUM_EXTRA_LINKS; i++) {
            m_pNodeLinks[areaId][i] = CNodeAddress{ static_cast<uint16>(-1), 0 };
        }

        m_anNumNodes[areaId]        = NumNodesGiven;
        m_anNumVehicleNodes[areaId] = 0;
        m_anNumPedNodes[areaId]     = NumNodesGiven;
        m_anNumCarPathLinks[areaId] = 0;
        m_anNumAddresses[areaId]    = numLinks;

        // Link the interior to the outside world
        for (auto i = 0; i < NumLinksToExteriorNodes; i++) {
            const auto exterior = aExteriorNodeLinkedTo[i];
            const auto interior = CNodeAddress{ areaId, static_cast<uint16>(aInteriorNodeLinkedToExterior[i]) };
            AddDynamicLinkBetween2Nodes_For1Node(interior, exterior);
            AddDynamicLinkBetween2Nodes_For1Node(exterior, interior);
        }
    }

    bInteriorBeingBuilt = false;
}
