#include "StdInc.h"
#include "Interior_c.h"

#include "Furniture_c.h"
#include "FurnitureManager_c.h"
#include "InteriorGroup_c.h"
#include "InteriorManager_c.h"

//! @notsa Interior tiles are stored as `field_68[x * 30 + y]`
static constexpr int32 TILE_ROW_STRIDE = 30;

/*!
 * @notsa
 * @brief The original code's way of generating a random number in range `[0, max)`:
 *        `(int)((float)(rand() & 0xFFFF) * (1.0f / 32768.0f) * max)`
 */
static int32 RandomNumberInRange(float max) {
    return (int32)((float)(rand() & 0xFFFF) * (1.0f / 32768.0f) * max);
}

/*!
 * @notsa
 * @brief The original `CGeneral::GetRandomNumberInRange(int32, int32)` (0x407180), returns a number in range `[min, max)`
 */
static int32 RandomIntInRange(int32 min, int32 max) {
    return min + (int32)((float)rand() * (1.0f / 32768.0f) * (float)(max - min));
}

/*!
 * @notsa
 * @brief Layout of a goto/exit point slot (See `AddGotoPt`).
 *        - Goto points: 16 slots at `Interior_c + 0x410`, count is `field_40C`
 *        - Exit points: 4 walls x 2 slots at `Interior_c + 0x510` (For each wall: [0] is the exit point, [1] is the door point)
 */
struct InteriorGoToPt {
    int8    TileX, TileY; // 0x0, 0x1
    int8    Prev, Next;   // 0x2, 0x3
    CVector Pos;          // 0x4
};
VALIDATE_SIZE(InteriorGoToPt, 0x10);

void Interior_c::InjectHooks() {
    RH_ScopedClass(Interior_c);
    RH_ScopedCategory("Interior");

    //RH_ScopedInstall(Constructor, 0x5921D0);
    //RH_ScopedInstall(Destructor, 0x591360);

    RH_ScopedInstall(Bedroom_AddTableItem, 0x593F10);
    RH_ScopedInstall(FurnishBedroom, 0x593FC0);
    RH_ScopedInstall(Kitchen_FurnishEdges, 0x596930);
    RH_ScopedInstall(FurnishKitchen, 0x5970B0);
    RH_ScopedInstall(Lounge_AddTV, 0x597240);
    RH_ScopedInstall(Lounge_AddHifi, 0x597430);
    RH_ScopedInstall(Lounge_AddChairInfo, 0x5974E0);
    RH_ScopedInstall(Lounge_AddSofaInfo, 0x5975C0);
    RH_ScopedInstall(FurnishLounge, 0x597740);
    RH_ScopedInstall(Office_PlaceEdgeFillers, 0x599210);
    RH_ScopedInstall(Office_PlaceDesk, 0x5993E0);
    RH_ScopedInstall(Office_PlaceEdgeDesks, 0x5995B0);
    RH_ScopedInstall(Office_FurnishEdges, 0x599770);
    RH_ScopedInstall(Office_PlaceDeskQuad, 0x599960);
    RH_ScopedInstall(Office_FurnishCenter, 0x599A30);
    RH_ScopedInstall(FurnishOffice, 0x599AF0);
    RH_ScopedInstall(Shop_Place3PieceUnit, 0x599BB0);
    RH_ScopedInstall(Shop_PlaceEdgeUnits, 0x599DC0);
    RH_ScopedInstall(Shop_PlaceCounter, 0x599EF0);
    RH_ScopedInstall(Shop_PlaceFixedUnits, 0x59A030);
    RH_ScopedInstall(Shop_FurnishCeiling, 0x59A130);
    RH_ScopedInstall(Shop_AddShelfInfo, 0x59A140);
    RH_ScopedInstall(Shop_FurnishEdges, 0x59A1B0);
    RH_ScopedInstall(GetBoundingBox, 0x593DB0);
    RH_ScopedInstall(Init, 0x593BF0);
    RH_ScopedInstall(ResetTiles, 0x593910);
    RH_ScopedInstall(PlaceObject, 0x5934E0);
    RH_ScopedInstall(GetFurnitureEntity, 0x5913B0);
    RH_ScopedInstall(IsPtInside, 0x5913E0);
    RH_ScopedInstall(CalcMatrix, 0x5914D0);
    RH_ScopedInstall(Furnish, 0x591590);
    RH_ScopedInstall(Unfurnish, 0x5915D0);
    RH_ScopedInstall(CheckTilesEmpty, 0x591680);
    RH_ScopedInstall(SetTilesStatus, 0x591700);
    RH_ScopedInstall(SetCornerTiles, 0x5917C0);
    RH_ScopedInstall(GetTileStatus, 0x5918E0);
    RH_ScopedInstall(GetNumEmptyTiles, 0x591920);
    RH_ScopedInstall(GetRandomTile, 0x591B20);
    RH_ScopedInstall(Shop_FurnishAisles, 0x59A590);
    RH_ScopedInstall(GetTileCentre, 0x591BD0);
    RH_ScopedInstall(AddGotoPt, 0x591D20);
    RH_ScopedInstall(AddInteriorInfo, 0x591E40);
    RH_ScopedInstall(AddPickups, 0x591F90);
    RH_ScopedInstall(Exit, 0x592230);
    RH_ScopedInstall(FindBoundingBox, 0x5922C0);
    RH_ScopedInstall(CalcExitPts, 0x5924A0);
    RH_ScopedInstall(PlaceFurniture, 0x592AA0);
    RH_ScopedInstall(PlaceFurnitureOnWall, 0x593120);
    RH_ScopedInstall(PlaceFurnitureInCorner, 0x593340);
    RH_ScopedInstall(FindEmptyTiles, 0x591C50);
    RH_ScopedInstall(FurnishShop, 0x59A790);
}

// 0x593BF0
int32 Interior_c::Init(const CVector& pos) {
    CalcMatrix(const_cast<CVector*>(&pos));
    ResetTiles();

    // NB: Original takes a copy of the entity's matrix (allocating it if necessary)
    const CVector entityPos = m_pGroup->GetEntity()->GetMatrix().GetPosition();
    if (m_box->m_type != 99) { // Everything but the test room is seeded, so that it's furnished the same way every time
        // Original uses `_ftol` and keeps the low 32 bits only
        const auto FloatToU32 = [](float v) {
            return static_cast<uint32>(static_cast<int64>(v));
        };
        static auto& s_EnEx = StaticRef<CEntryExit*>(0xBB3DAC); // g_interiorMan.m_EnEx
        uint32       seed;
        if (const auto* const enex = s_EnEx) {
            seed = FloatToU32(enex->m_fEntranceZ) * FloatToU32(enex->m_recEntrance.bottom) * FloatToU32(enex->m_recEntrance.left)
                 + FloatToU32(entityPos.z) * FloatToU32(entityPos.y) * FloatToU32(entityPos.x)
                 + m_box->m_seed;
        } else {
            seed = FloatToU32(entityPos.z * entityPos.y * entityPos.x + (float)m_box->m_seed);
        }
        srand(seed);
#ifndef NOTSA_STANDALONE
        // NOTSA: Also seed the game's own CRT (0x821B11 = srand), in case anything still running from the original uses its `rand` (0x821B1E)
        plugin::Call<0x821B11, uint32>(seed);
#endif
    }

    field_40C            = 0;
    m_interiorInfosCount = 0;

    Furnish();
    CalcExitPts();

    // Mark the steal data of this interior as set up
    if (!g_interiorMan.HasInteriorHadStealDataSetup(this)) {
        static auto& s_InteriorCount = StaticRef<int32>(0xBB3914);     // g_interiorMan.m_InteriorCount
        static auto& s_InteriorIds   = StaticRef<int32[64]>(0xBB3918); // g_interiorMan.m_InteriorIds
        if (s_InteriorCount < 64) {
            s_InteriorIds[s_InteriorCount++] = m_interiorId;
        }
    }

    if (m_box->m_type == 2 || m_box->m_type == 3) {
        AddPickups();
    }
    return 1;
}

// 0x592230
void Interior_c::Exit() {
    CPickups::RemovePickUpsInArea(
        m_position.x - 50.0f, m_position.x + 50.0f, m_position.y - 50.0f, m_position.y + 50.0f, m_position.z - 50.0f, m_position.z + 50.0f
    );
    Unfurnish();
}

// 0x593F10
CObject* Interior_c::Bedroom_AddTableItem(int32 a2, int32 a3, int32 a4, int32 a5, int32 a6, int32 a7) {
    float offsetX = (float)a5;
    float offsetY = (float)a6;
    if (a4 == 0 || a4 == 2) {
        offsetX += 0.5f;
    } else if (a4 == 1 || a4 == 3) {
        offsetY += 0.5f;
    }

    return PlaceObject(
        true,
        g_furnitureMan.GetFurniture(a2, a3, -1, m_box->m_status),
        offsetX + 0.5f,
        offsetY + 0.5f,
        0.5f,
        (float)a7 * 90.0f
    );
}

// 0x593FC0
void Interior_c::FurnishBedroom() {
    m_furnitureId = static_cast<int8>(g_furnitureMan.GetRandomId(3, 1, m_box->m_status));
    SetTilesStatus((int32)m_box->m_door - 1, 0, 2, 2, 7, 0);

    // The bed
    // NB: `wall` and `pos` are left uninitialized by the original code if the bed can't be placed
    int32       wall{}, pos{};
    auto* const bed = PlaceFurnitureOnWall(3, 0, -1, 0.0f, 1, -1, -1, 0, &wall, &pos, nullptr, nullptr, nullptr, nullptr);

    // 0x594036 - Bedside table before the bed
    if (pos > 0 && PlaceFurnitureOnWall(3, 1, (int32)m_furnitureId, 0.0f, 1, wall, pos - 1, 0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)) {
        const auto width = (int32)m_box->m_width;
        const auto depth = (int32)m_box->m_depth;
        switch (wall) {
        case 1:
            AddInteriorInfo(3, 1.0f, (float)(pos - 1), 2, bed);
            SetTilesStatus(2, pos - 1, 1, 1, 2, 0);
            break;
        case 3:
            AddInteriorInfo(4, (float)(width - 2), (float)(pos - 1), 2, bed);
            SetTilesStatus(width - 3, pos - 1, 1, 1, 2, 0);
            break;
        case 0:
            AddInteriorInfo(3, (float)(pos - 1), (float)(depth - 2), 1, bed);
            SetTilesStatus(pos - 1, depth - 3, 1, 1, 2, 0);
            break;
        case 2:
            AddInteriorInfo(4, (float)(pos - 1), 1.0f, 1, bed);
            SetTilesStatus(pos - 1, 2, 1, 1, 2, 0);
            break;
        }
    }

    // 0x594146 - Bedside table after the bed
    if (PlaceFurnitureOnWall(3, 1, (int32)m_furnitureId, 0.0f, 1, wall, pos + 2, 0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)) {
        const auto width = (int32)m_box->m_width;
        const auto depth = (int32)m_box->m_depth;
        switch (wall) {
        case 1:
            AddInteriorInfo(4, 1.0f, (float)(pos + 2), 0, bed);
            SetTilesStatus(2, pos + 2, 1, 1, 2, 0);
            break;
        case 3:
            AddInteriorInfo(3, (float)(width - 2), (float)(pos + 2), 0, bed);
            SetTilesStatus(width - 3, pos + 2, 1, 1, 2, 0);
            break;
        case 0:
            AddInteriorInfo(4, (float)(pos + 2), (float)(depth - 2), 3, bed);
            SetTilesStatus(pos + 2, depth - 3, 1, 1, 2, 0);
            break;
        case 2:
            AddInteriorInfo(3, (float)(pos + 2), 1.0f, 3, bed);
            SetTilesStatus(pos + 2, 2, 1, 1, 2, 0);
            break;
        }
    }

    // 0x594253
    PlaceFurnitureOnWall(3, 3, (int32)m_furnitureId, 0.0f, 1, -1, -1, 0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    PlaceFurnitureOnWall(3, 2, (int32)m_furnitureId, 0.0f, 1, -1, -1, 0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    // 0x5942AB - Tables with a hifi and/or a TV on them
    const auto AddTableWithItem = [this](int32 itemSubGroupId) {
        int32 tableWall, tableX, tableY;
        if (PlaceFurnitureOnWall(2, 6, -1, 0.0f, 1, -1, -1, 0, &tableWall, nullptr, &tableX, &tableY, nullptr, nullptr)) {
            Bedroom_AddTableItem(2, itemSubGroupId, tableWall, tableX, tableY, tableWall);
        }
    };
    if (const auto r = RandomNumberInRange(100.0f); r < 25) {
        AddTableWithItem(8);
    } else if (r < 50) {
        AddTableWithItem(3);
    } else if (r < 75) {
        AddTableWithItem(8);
        AddTableWithItem(3);
    }

    // 0x5943BF - The poorer the more junk there is on the floor
    int32 chance;
    if (m_box->m_status >= 75) {
        chance = RandomNumberInRange(20.0f);
    } else if (m_box->m_status >= 50) {
        chance = 20 - RandomNumberInRange(-30.0f);
    } else {
        chance = RandomIntInRange(50, 100);
    }

    // NB: The roll always happens, but the tiles are only searched for if it succeeds
    const auto PlaceJunk = [this, chance](float maxRoll, int32 subGroupId, int32 searchSize, int32 markSize) {
        int32 x, y;
        if (RandomNumberInRange(maxRoll) < chance && FindEmptyTiles(searchSize, searchSize, &x, &y)) {
            PlaceObject(false, g_furnitureMan.GetFurniture(8, subGroupId, -1, m_box->m_status), (float)x + 0.5f, (float)y + 0.5f, 0.05f, 0.0f);
            SetTilesStatus(x, y, markSize, markSize, 2, 0);
        }
    };
    PlaceJunk(60.0f, 2, 2, 2);
    PlaceJunk(100.0f, 5, 1, 1);
    PlaceJunk(100.0f, 4, 1, 1);
    PlaceJunk(100.0f, 3, 1, 1);
    PlaceJunk(100.0f, 6, 2, 1); // Yes, 2x2 tiles are searched for but only 1 is marked

    // 0x594743 - Ceiling light in the middle
    auto* const light = g_furnitureMan.GetFurniture(8, 1, -1, m_box->m_status);
    const auto  y     = (int32)((float)m_box->m_depth * 0.5f - (float)(uint8)light->m_nWidthY * 0.5f);
    const auto  x     = (int32)((float)m_box->m_width * 0.5f - (float)(uint8)light->m_nWidthX * 0.5f);
    int32       sizeX{}, sizeY{};
    PlaceFurniture(light, x, y, 0.0f, 0, 0, &sizeX, &sizeY, false);
}

// 0x596930
CObject* Interior_c::Kitchen_FurnishEdges() {
    const auto id     = (int32)m_furnitureId;
    const auto status = m_box->m_status;
    const auto maxX   = (int32)m_box->m_width - 1;
    const auto maxY   = (int32)m_box->m_depth - 1;

    int32 sizeX{}, sizeY{};

    // 0x596965 - Corner units at the top
    auto* const cornerUnit      = g_furnitureMan.GetFurniture(4, 7, (int16)id, status);
    const bool  hasLeftCorner   = PlaceFurniture(cornerUnit, 0, maxY, 0.0f, 1, 1, &sizeX, &sizeY, false) != nullptr;
    const bool  hasRightCorner  = PlaceFurniture(cornerUnit, maxX, maxY, 0.0f, 1, 0, &sizeX, &sizeY, false) != nullptr;

    // 0x5969C9 - Where the units on the left/right walls start (After the doors/windows)
    const auto leftEnd  = std::max<int32>(std::max(m_box->m_lDoorEnd, m_box->m_lWindowEnd), 0);
    const auto rightEnd = std::max<int32>(std::max(m_box->m_rDoorEnd, m_box->m_rWindowEnd), 0);
    int32      leftY, rightY;
    if (rightEnd <= 0) {
        if (leftEnd <= 0) {
            leftY  = RandomIntInRange(0, maxY);
            rightY = RandomIntInRange(0, maxY);
        } else {
            leftY  = leftEnd;
            rightY = 0;
        }
    } else {
        rightY = rightEnd;
        leftY  = leftEnd <= 0 ? 0 : leftEnd;
    }
    if (leftY > 0) {
        SetTilesStatus(0, 0, 1, leftY, 2, 0);
    }
    if (rightY > 0) {
        SetTilesStatus(maxX, 0, 1, rightY, 2, 0);
    }

    // 0x596A6B - End units
    PlaceFurniture(g_furnitureMan.GetFurniture(4, 0, (int16)id, m_box->m_status), 0, leftY, 0.0f, 1, 1, &sizeX, &sizeY, false);
    PlaceFurniture(g_furnitureMan.GetFurniture(4, 2, (int16)id, m_box->m_status), maxX, rightY, 0.0f, 1, 3, &sizeX, &sizeY, false);

    // 0x596AE1 - The sink (Under the top window, if any)
    int32 wall{}, pos{};
    if (PlaceFurnitureOnWall(4, 3, id, 0.0f, 1, 0, m_box->m_tWindowStart != -1 ? (int32)m_box->m_tWindowStart : -1, 0, &wall, &pos, nullptr, nullptr, nullptr, nullptr)) {
        pos++;
        AddInteriorInfo(5, (float)pos, (float)m_box->m_depth - 1.5f, 2, nullptr);
    }

    // 0x596B5F - Fridge, cooker, washer
    PlaceFurnitureOnWall(4, 5, id, 0.0f, 1, -1, -1, 0, &wall, &pos, nullptr, nullptr, nullptr, nullptr);
    PlaceFurnitureOnWall(4, 4, id, 0.0f, 1, -1, -1, 0, &wall, &pos, nullptr, nullptr, nullptr, nullptr);
    PlaceFurnitureOnWall(4, 6, id, 0.0f, 1, -1, -1, 0, &wall, &pos, nullptr, nullptr, nullptr, nullptr);

    // 0x596BF2 - Fill the rest with worktops, while remembering where they are
    // NB: The original arrays have 32 entries (Without any bounds checks)
    constexpr auto MAX_WORKTOPS = 96;
    float          worktopX[MAX_WORKTOPS], worktopY[MAX_WORKTOPS], worktopRot[MAX_WORKTOPS];
    float          worktopUsed[MAX_WORKTOPS]{};
    int32          numWorktops = 0;

    auto* const worktop = g_furnitureMan.GetFurniture(4, 1, (int16)id, m_box->m_status);
    for (auto y = leftY + 1; y < maxY; y++) { // Left wall
        if (PlaceFurniture(worktop, 0, y, 0.0f, 1, 1, &sizeX, &sizeY, false)) {
            worktopX[numWorktops]   = 0.5f;
            worktopRot[numWorktops] = 90.0f;
            worktopY[numWorktops]   = (float)y + 0.5f;
            numWorktops++;
        }
    }
    for (auto x = 1; x < maxX; x++) { // Top wall
        if (PlaceFurniture(worktop, x, maxY, 0.0f, 1, 0, &sizeX, &sizeY, false)) {
            worktopRot[numWorktops] = 0.0f;
            worktopX[numWorktops]   = (float)x + 0.5f;
            worktopY[numWorktops]   = (float)maxY + 0.5f;
            numWorktops++;
        }
    }
    for (auto y = rightY + 1; y <= maxY - 1; y++) { // Right wall
        if (PlaceFurniture(worktop, maxX, y, 0.0f, 1, 3, &sizeX, &sizeY, false)) {
            worktopRot[numWorktops] = 270.0f;
            worktopX[numWorktops]   = (float)maxX + 0.5f;
            worktopY[numWorktops]   = (float)y + 0.5f;
            numWorktops++;
        }
    }

    // 0x596D70 - Microwave in one of the corners
    constexpr auto WORKTOP_HEIGHT = 1.05f;
    CObject*       result         = nullptr;
    auto* const    microwave      = g_furnitureMan.GetFurniture(4, 10, -1, m_box->m_status);
    if (rand() < 0x3FFF && hasLeftCorner) {
        result = PlaceObject(true, microwave, 0.5f, (float)m_box->m_depth - 0.5f, WORKTOP_HEIGHT, 45.0f);
    } else if (hasRightCorner) {
        result = PlaceObject(true, microwave, (float)m_box->m_width - 0.5f, (float)m_box->m_depth - 0.5f, WORKTOP_HEIGHT, 315.0f);
    }

    if (numWorktops <= 0) {
        return result;
    }

    // 0x596E22 - Pick 2 worktops for the stealable appliances
    const auto fNumWorktops = (float)numWorktops;
    const auto firstIdx     = RandomNumberInRange(fNumWorktops);
    worktopUsed[firstIdx]   = 1.0f;

    auto secondIdx = RandomNumberInRange(fNumWorktops);
    if (worktopUsed[secondIdx] == 1.0f) {
        for (auto tries = 0;;) {
            if (tries >= 30) {
                secondIdx = -1;
                break;
            }
            secondIdx = RandomNumberInRange(fNumWorktops);
            tries++;
            if (worktopUsed[secondIdx] != 1.0f) {
                if (tries == 30) { // NB: Original code discards the one found on the last try
                    secondIdx = -1;
                }
                break;
            }
        }
    }

    // 0x596ECC - The poorer the more junk there is on the worktops
    int32 chance;
    if (m_box->m_status >= 75) {
        chance = RandomNumberInRange(20.0f);
    } else if (m_box->m_status >= 50) {
        chance = 20 - RandomNumberInRange(-30.0f);
    } else {
        chance = 50 - RandomNumberInRange(-50.0f);
    }

    for (auto i = 0; i < numWorktops; i++) {
        if (worktopUsed[i] != 0.0f || RandomNumberInRange(100.0f) >= chance) {
            continue;
        }
        const auto subGroupId = rand() < 0x3FFF ? 3 : 4;
        if (auto* const junk = g_furnitureMan.GetFurniture(8, subGroupId, -1, m_box->m_status)) {
            result = PlaceObject(false, junk, worktopX[i], worktopY[i], WORKTOP_HEIGHT, worktopRot[i]);
        }
    }

    if (firstIdx != -1) {
        result = PlaceObject(true, g_furnitureMan.GetFurniture(4, 8, -1, m_box->m_status), worktopX[firstIdx], worktopY[firstIdx], WORKTOP_HEIGHT, worktopRot[firstIdx]);
    }
    if (secondIdx != -1) {
        result = PlaceObject(true, g_furnitureMan.GetFurniture(4, 9, -1, m_box->m_status), worktopX[secondIdx], worktopY[secondIdx], WORKTOP_HEIGHT, worktopRot[secondIdx]);
    }
    return result; // NB: The return value isn't used anywhere (And is garbage in the original code)
}

// 0x5970B0
void Interior_c::FurnishKitchen() {
    SetTilesStatus((int32)m_box->m_door - 1, 0, 2, 1, 7, 0);
    const auto maxX = (int32)m_box->m_width - 2;
    const auto maxY = (int32)m_box->m_depth - 2;
    for (auto x = 1; x <= maxX; x++) {
        SetTilesStatus(x, maxY, 1, 1, 3, 0);
        SetTilesStatus(x, 0, 1, 1, 3, 0);
    }
    for (auto y = 0; y <= maxY; y++) {
        SetTilesStatus(1, y, 1, 1, 3, 0);
        SetTilesStatus(maxX, y, 1, 1, 3, 0);
    }
    AddGotoPt(1, 1, 0.0f, 0.0f);
    AddGotoPt(1, maxY, 0.0f, 0.0f);
    AddGotoPt(maxX, 1, 0.0f, 0.0f);
    AddGotoPt(maxX, maxY, 0.0f, 0.0f);
    m_furnitureId = static_cast<int8>(g_furnitureMan.GetRandomId(4, 0, m_box->m_status));
    Kitchen_FurnishEdges();
    auto* unit = g_furnitureMan.GetFurniture(8, 1, -1, m_box->m_status);
    // NB: original rolls the depth offset first, then the width offset (C++ arg order is unspecified, so sequence explicitly)
    const auto offsetY = RandomNumberInRange((float)m_box->m_depth * 0.5f - (float)unit->m_nWidthY * 0.5f);
    const auto offsetX = RandomNumberInRange((float)m_box->m_width * 0.5f - (float)unit->m_nWidthX * 0.5f);
    int32 sizeX{}, sizeY{};
    PlaceFurniture(unit, offsetX, offsetY, 0.0f, 0, 0, &sizeX, &sizeY, false);
}

// 0x597240
CObject* Interior_c::Lounge_AddTV(int32 a2, int32 a3, int32 a4, int32 a5) {
    // NOTE: `a3`, `a4`, `a5` are unused in the original code too
    const auto width = (float)m_box->m_width;
    const auto depth = (float)m_box->m_depth;

    // NB: Left uninitialized by the original code if `a2` isn't in range [0, 3]
    float tvX{}, tvY{}, unitX{}, unitY{};
    switch (a2) {
    case 0:
        AddInteriorInfo(0, 1.0f, depth - 2.0f, -1, nullptr);
        tvX   = 0.5f;
        tvY   = depth - 0.5f;
        unitX = 1.5f;
        unitY = depth - 0.5f;
        break;
    case 1:
        AddInteriorInfo(0, 1.0f, 1.0f, -1, nullptr);
        tvX   = 0.5f;
        tvY   = 0.5f;
        unitX = 0.5f;
        unitY = 1.5f;
        break;
    case 2:
        AddInteriorInfo(0, width - 2.0f, 1.0f, -1, nullptr);
        tvX   = width - 0.5f;
        tvY   = 0.5f;
        unitX = width - 1.5f;
        unitY = 0.5f;
        break;
    case 3:
        AddInteriorInfo(0, width - 2.0f, depth - 2.0f, -1, nullptr);
        tvX   = width - 0.5f;
        tvY   = depth - 0.5f;
        unitX = width - 0.5f;
        unitY = depth - 1.5f;
        break;
    }

    const auto rotation = (float)(a2 & 3) * 90.0f;

    // TV
    PlaceObject(
        true,
        g_furnitureMan.GetFurniture(2, 3, -1, m_box->m_status),
        tvX,
        tvY,
        0.5f,
        rotation + 45.0f
    );

    // Video or console next to it
    const auto subGroupId = rand() < 0x3FFF ? 7 : 9;
    return PlaceObject(
        true,
        g_furnitureMan.GetFurniture(2, subGroupId, -1, m_box->m_status),
        unitX,
        unitY,
        0.5f,
        rotation
    );
}

// 0x597430
CObject* Interior_c::Lounge_AddHifi(int32 a2, int32 a3, int32 a4, int32 a5) {
    float offsetX = (float)a3;
    float offsetY = (float)a4;

    if (a2 == 0 || a2 == 2) {
        offsetX += 0.5f;
    } else if (a2 == 1 || a2 == 3) {
        offsetY += 0.5f;
    }

    return PlaceObject(
        true,
        g_furnitureMan.GetFurniture(2, 8, -1, m_box->m_status),
        offsetX + 0.5f,
        offsetY + 0.5f,
        0.5f,
        (float)(a2 & 3) * 90.0f
    );
}

// 0x5974E0
void Interior_c::Lounge_AddChairInfo(int32 a2, int32 a3, CEntity* entityIgnoredCollision) {
    switch (a2) {
    case 0:
        AddInteriorInfo(1, (float)a3 + 0.5f, (float)(m_box->m_depth - 1) - 1.0f, 2, entityIgnoredCollision);
        break;
    case 1:
        AddInteriorInfo(1, 1.0f, (float)a3 + 0.5f, 3, entityIgnoredCollision);
        break;
    case 2:
        AddInteriorInfo(1, (float)a3 + 0.5f, 1.0f, 0, entityIgnoredCollision);
        break;
    case 3:
        AddInteriorInfo(1, (float)(m_box->m_width - 1) - 1.0f, (float)a3 + 0.5f, 1, entityIgnoredCollision);
        break;
    }
}

// 0x5975C0
void Interior_c::Lounge_AddSofaInfo(int32 sitType, int32 offsetX, CEntity* entityIgnoredCollision) {
    switch (sitType) {
    case 0: {
        const auto x = (float)offsetX + 0.5f;
        const auto y = (float)(m_box->m_depth - 1) - 1.0f;
        AddInteriorInfo(1, x, y, 2, entityIgnoredCollision);
        AddInteriorInfo(1, x + 1.0f, y, 2, entityIgnoredCollision);
        break;
    }
    case 1: {
        const auto y = (float)offsetX + 0.5f;
        AddInteriorInfo(1, 1.0f, y, 3, entityIgnoredCollision);
        AddInteriorInfo(1, 1.0f, y + 1.0f, 3, entityIgnoredCollision);
        break;
    }
    case 2: {
        const auto x = (float)offsetX + 0.5f;
        AddInteriorInfo(1, x, 1.0f, 0, entityIgnoredCollision);
        AddInteriorInfo(1, x + 1.0f, 1.0f, 0, entityIgnoredCollision);
        break;
    }
    case 3: {
        const auto x = (float)(m_box->m_width - 1) - 1.0f;
        const auto y = (float)offsetX + 0.5f;
        AddInteriorInfo(1, x, y, 1, entityIgnoredCollision);
        AddInteriorInfo(1, x, y + 1.0f, 1, entityIgnoredCollision);
        break;
    }
    }
}

// 0x597740
void Interior_c::FurnishLounge() {
    const auto perimeter = ((int32)m_box->m_depth + (int32)m_box->m_width) * 2;

    SetTilesStatus((int32)m_box->m_door - 1, 0, 2, 1, 7, 0);
    SetTilesStatus((int32)m_box->m_door - 2, 0, 1, 1, 2, 0);
    SetTilesStatus((int32)m_box->m_door + 1, 0, 1, 1, 2, 0);

    // 0x5977A7 - TV stand in one of the corners
    int32 tvCorner = -1;
    {
        // NB: `corner` is left uninitialized by the original code if the stand can't be placed
        int32 corner = -1, x, y, sizeX, sizeY;
        if (PlaceFurnitureInCorner(2, 2, -1, 0.0f, 1, -1, 0, &corner, &x, &y, &sizeX, &sizeY)) {
            Lounge_AddTV(corner, x, y, 0);
            tvCorner = corner;
        }
        SetCornerTiles(corner, 2, 2, 1);
    }

    // 0x597806 - Walkway around the room
    const auto maxX = (int32)m_box->m_width - 2;
    const auto maxY = (int32)m_box->m_depth - 2;
    for (auto x = 1; x <= maxX; x++) {
        SetTilesStatus(x, maxY, 1, 1, 3, 0);
        SetTilesStatus(x, 1, 1, 1, 3, 0);
    }
    for (auto y = 1; y <= maxY; y++) {
        SetTilesStatus(1, y, 1, 1, 3, 0);
        SetTilesStatus(maxX, y, 1, 1, 3, 0);
    }
    AddGotoPt(1, 1, 0.0f, 0.0f);
    AddGotoPt(1, maxY, 0.0f, 0.0f);
    AddGotoPt(maxX, 1, 0.0f, 0.0f);
    AddGotoPt(maxX, maxY, 0.0f, 0.0f);

    SetCornerTiles(0, 2, 2, 0);
    SetCornerTiles(2, 2, 2, 0);
    SetCornerTiles(1, 2, 2, 0);
    SetCornerTiles(3, 2, 2, 0);

    m_furnitureId = static_cast<int8>(g_furnitureMan.GetRandomId(2, 0, m_box->m_status));

    // 0x59792D - Sofa (With a coffee table in front of it)
    int32 wall, pos;
    if (auto* const sofa = PlaceFurnitureOnWall(2, 0, (int32)m_furnitureId, 0.0f, 1, -1, -1, 0, &wall, &pos, nullptr, nullptr, nullptr, nullptr)) {
        Lounge_AddSofaInfo(wall, pos, sofa);
        if (!PlaceFurnitureOnWall(2, 4, -1, 0.0f, 1, wall, pos, 2, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)) {
            PlaceFurnitureOnWall(2, 4, -1, 0.0f, 1, wall, pos, 3, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
        }
    }

    // 0x5979AA - Chairs (2, or 3 if the room is big enough)
    const auto numChairs = perimeter > 28 ? 3 : 2;
    for (auto i = 0; i < numChairs; i++) {
        if (auto* const chair = PlaceFurnitureOnWall(2, 1, (int32)m_furnitureId, 0.0f, 1, -1, -1, 0, &wall, &pos, nullptr, nullptr, nullptr, nullptr)) {
            Lounge_AddChairInfo(wall, pos, chair);
        }
    }

    // 0x597A7C
    if (tvCorner != 0) {
        SetCornerTiles(0, 2, 0, 0);
    }
    if (tvCorner != 2) {
        SetCornerTiles(2, 2, 0, 0);
    }
    if (tvCorner != 1) {
        SetCornerTiles(1, 2, 0, 0);
    }
    if (tvCorner != 3) {
        SetCornerTiles(3, 2, 0, 0);
    }

    // 0x597AC8 - Table with a hifi on it
    {
        int32 x, y;
        if (PlaceFurnitureOnWall(2, 6, -1, 0.0f, 1, -1, -1, 0, &wall, nullptr, &x, &y, nullptr, nullptr)) {
            Lounge_AddHifi(wall, x, y, 0);
        }
    }

    PlaceFurnitureOnWall(2, 5, -1, 0.0f, 1, -1, -1, 0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    PlaceFurnitureOnWall(8, 0, -1, 0.0f, 1, -1, -1, 0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    PlaceFurnitureOnWall(8, 0, -1, 0.0f, 1, -1, -1, 0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    // 0x597B84 - The poorer the more junk there is on the floor
    int32 chance;
    if (m_box->m_status >= 75) {
        chance = RandomNumberInRange(20.0f);
    } else if (m_box->m_status >= 50) {
        chance = 20 - RandomNumberInRange(-30.0f);
    } else {
        chance = 50 - RandomNumberInRange(-50.0f);
    }

    // NB: The roll always happens, but the tiles are only searched for if it succeeds
    const auto PlaceJunk = [this, chance](float maxRoll, int32 subGroupId, int32 size) {
        int32 x, y;
        if (RandomNumberInRange(maxRoll) < chance && FindEmptyTiles(size, size, &x, &y)) {
            PlaceObject(false, g_furnitureMan.GetFurniture(8, subGroupId, -1, m_box->m_status), (float)x + 0.5f, (float)y + 0.5f, 0.05f, 0.0f);
            SetTilesStatus(x, y, size, size, 2, 0);
        }
    };
    PlaceJunk(60.0f, 2, 2);
    PlaceJunk(100.0f, 5, 1);
    PlaceJunk(100.0f, 4, 1);

    // 0x597DF0 - Ceiling light in the middle
    {
        auto* const light = g_furnitureMan.GetFurniture(8, 1, -1, m_box->m_status);
        const auto  y     = (int32)((float)m_box->m_depth * 0.5f - (float)(uint8)light->m_nWidthY * 0.5f);
        const auto  x     = (int32)((float)m_box->m_width * 0.5f - (float)(uint8)light->m_nWidthX * 0.5f);
        int32       sizeX{}, sizeY{};
        PlaceFurniture(light, x, y, 0.0f, 0, 0, &sizeX, &sizeY, false);
    }

    // 0x597E59 - Places to look out the windows/stand by the walls (Wherever the tiles by the wall are still free)
    const auto IsFree = [](int32 status) { return status == 0 || status == 2; };
    for (auto x = 0; x < (int32)m_box->m_width - 1; x++) {
        const auto bottom = GetTileStatus(x, 0);
        const auto top    = GetTileStatus(x, (int32)m_box->m_depth - 1);
        if (IsFree(bottom)) {
            AddInteriorInfo(2, (float)x, 0.0f, 2, nullptr);
        }
        if (IsFree(top)) {
            AddInteriorInfo(2, (float)x, (float)((int32)m_box->m_depth - 1), 0, nullptr);
        }
    }
    for (auto y = 1; y < (int32)m_box->m_depth - 2; y++) {
        const auto left  = GetTileStatus(0, y);
        const auto right = GetTileStatus((int32)m_box->m_width - 1, y);
        if (IsFree(left)) {
            AddInteriorInfo(2, 0.0f, (float)y, 1, nullptr);
        }
        if (IsFree(right)) {
            AddInteriorInfo(2, (float)((int32)m_box->m_width - 1), (float)y, 3, nullptr);
        }
    }
}

// 0x599210
int32 Interior_c::Office_PlaceEdgeFillers(int32 arg0, int32 a2, int32 a3, int32 a6, int32 a7) {
    // arg0 = filler type (-1 for random), a2 = tile X, a3 = tile Y, a6 = direction (a7 is unused)
    const auto type = arg0, x = a2, y = a3, dir = a6;

    const auto chance = RandomNumberInRange(100.0f);
    if (GetNumEmptyTiles(x, y, (dir == 2 || dir == 0) ? 1 : 2, 1) < 1) {
        return 1;
    }

    // Tile in front of the filler
    int32 infoX = x, infoY = y;
    switch (dir) {
    case 2: infoY = y + 1; break;
    case 0: infoY = y - 1; break;
    case 3: infoX = x - 1; break;
    case 1: infoX = x + 1; break;
    }

    int32 sizeX{}, sizeY{};

    if (type != -1) {
        PlaceFurniture(g_furnitureMan.GetFurniture(1, type, -1, m_box->m_status), x, y, 0.0f, 1, dir, &sizeX, &sizeY, false);
        return sizeX;
    }

    static auto& s_HasPlacedWaterCooler = StaticRef<bool>(0xBB3DC8); // TODO: Proper name (Only 1 of subgroup 2 is placed per office)
    if (chance > 90 && !s_HasPlacedWaterCooler) {
        const auto furniture = g_furnitureMan.GetFurniture(1, 2, -1, m_box->m_status);
        if (furniture) {
            AddInteriorInfo(7, (float)infoX, (float)infoY, (dir - 2) & 3, nullptr);
        }
        s_HasPlacedWaterCooler = true;
        PlaceFurniture(furniture, x, y, 0.0f, 1, dir, &sizeX, &sizeY, false);
        return sizeX;
    }

    if (chance > 75) {
        const auto rot = CGeneral::GetRandomNumberInRange(0, 5); // The original code uses this as the direction too
        PlaceFurniture(g_furnitureMan.GetFurniture(8, 0, -1, m_box->m_status), x, y, 0.0f, 1, rot, &sizeX, &sizeY, false);
        return sizeX;
    }

    if (chance > 25) {
        const auto furniture = g_furnitureMan.GetFurniture(1, 3, -1, m_box->m_status);
        if (furniture) {
            AddInteriorInfo(7, (float)infoX, (float)infoY, (dir - 2) & 3, nullptr);
        }
        PlaceFurniture(furniture, x, y, 0.0f, 1, dir, &sizeX, &sizeY, false);
        return sizeX;
    }

    return 1;
}

// 0x5993E0
int32 Interior_c::Office_PlaceDesk(int32 a3, int32 arg4, int32 offsetY, int32 a5, uint8 a6, int32 b) {
    // a3 = tile X, arg4 = tile Y, offsetY = direction, b = id of the desk (a5, a6 are unused)
    const auto x = a3, y = arg4, dir = offsetY;
    const auto rot = (dir - 2) & 3;

    int32 sizeX{}, sizeY{};

    // The desk
    if (!PlaceFurniture(
        g_furnitureMan.GetFurniture(1, 0, (int16)b, m_box->m_status),
        dir == 1 ? x + 1 : x,
        dir == 2 ? y + 1 : y,
        0.0f,
        1,
        rot,
        &sizeX,
        &sizeY,
        false
    )) {
        return 1;
    }

    // The chair
    int32 chairX = x, chairY = y; // Where the chair is placed
    int32 infoX = x, infoY = y;   // Where the interior info is placed
    int32 tileX = x, tileY = y;   // Tile to be marked
    switch (dir) {
    case 2:
        chairX = infoX = x + 1;
        break;
    case 0:
        chairY = infoY = tileY = y + 1;
        tileX = x + 1;
        break;
    case 3:
        chairY = infoY = y + 1;
        chairX = infoX = tileX = x + 1;
        break;
    case 1:
        tileY = y + 1;
        break;
    }
    const auto chair = PlaceFurniture(
        g_furnitureMan.GetFurniture(1, 1, (int16)field_792, m_box->m_status),
        chairX,
        chairY,
        0.0f,
        1,
        rot,
        &sizeX,
        &sizeY,
        true
    );

    auto infoOffsetX = (float)infoX, infoOffsetY = (float)infoY;
    switch (rot) {
    case 2: infoOffsetY += 0.5f; break;
    case 0: infoOffsetY -= 0.5f; break;
    case 3: infoOffsetX -= 0.5f; break;
    case 1: infoOffsetX += 0.5f; break;
    }
    AddInteriorInfo(6, infoOffsetX, infoOffsetY, (rot - 2) & 3, chair);
    SetTilesStatus(tileX, tileY, 1, 1, 2, 1);

    return 2;
}
// 0x5995B0
int32 Interior_c::Office_PlaceEdgeDesks(int32 a2, int32 a3, int32 a4, int32 a5, int32 a6) {
    // a3 = tile X, a4 = tile Y, a5 = direction, a6 = wall (a2 is unused)
    const auto x = a3, y = a4, dir = a5;

    const auto chance    = RandomNumberInRange(100.0f);
    const auto maxChance = 30 - RandomNumberInRange(-40.0f);

    const auto numEmpty = GetNumEmptyTiles(x, y, (dir == 2 || dir == 0) ? 1 : 2, 1);
    if (numEmpty <= 1) {
        return 1;
    }

    // Each desk takes up 2 tiles
    const auto maxDesks = numEmpty / 2;
    int32      numDesks;
    if (maxDesks >= 2 - RandomNumberInRange(-2.0f)) {
        numDesks = 2 - RandomNumberInRange(-2.0f); // Yes, another random number
    } else {
        numDesks = maxDesks;
    }

    if (chance > maxChance) {
        return 1;
    }

    const auto rot = (dir - 2) & 3;
    int32      offset = 0;
    for (int32 i = 0; i < numDesks; i++) {
        switch (a6) {
        case 0: offset += Office_PlaceDesk(x + offset, y - 1, rot, 0x46, 0, m_furnitureId); break;
        case 3: offset += Office_PlaceDesk(x - 1, y + offset, rot, 0x46, 0, m_furnitureId); break;
        case 1: offset += Office_PlaceDesk(x, y + offset, rot, 0x46, 0, m_furnitureId);     break;
        case 2: offset += Office_PlaceDesk(x + offset, y, rot, 0x46, 0, m_furnitureId);     break;
        }
    }
    return offset + 1;
}

// 0x599770
void Interior_c::Office_FurnishEdges() {
    const auto maxX = (int32)m_box->m_width - 3;
    const auto maxY = (int32)m_box->m_depth - 3;
    for (auto x = 2; x <= maxX; x++) {
        SetTilesStatus(x, maxY, 1, 1, 3, 0);
        SetTilesStatus(x, 2, 1, 1, 3, 0);
    }
    for (auto y = 2; y <= maxY; y++) {
        SetTilesStatus(2, y, 1, 1, 3, 0);
        SetTilesStatus(maxX, y, 1, 1, 3, 0);
    }
    AddGotoPt(2, 2, 0.5f, 0.5f);
    AddGotoPt(2, maxY, 0.5f, -0.5f);
    AddGotoPt(maxX, 2, -0.5f, 0.5f);
    AddGotoPt(maxX, maxY, -0.5f, -0.5f);
    const auto width = (int32)m_box->m_width - 1;
    const auto depth = (int32)m_box->m_depth;
    SetTilesStatus((int32)m_box->m_door - 2, 0, 4, 2, 7, 0);
    for (auto x = 1; x < width;) {
        x += Office_PlaceEdgeDesks(-1, x, 0, 2, 2);
    }
    for (auto x = 1; x < width;) {
        x += Office_PlaceEdgeDesks(-1, x, depth - 1, 0, 0);
    }
    for (auto y = 1; y <= depth - 2;) {
        y += Office_PlaceEdgeDesks(-1, 0, y, 1, 1);
    }
    for (auto y = 1; y <= depth - 2;) {
        y += Office_PlaceEdgeDesks(-1, width, y, 3, 3);
    }
    for (auto x = 1; x < width;) {
        x += Office_PlaceEdgeFillers(-1, x, 0, 2, 2);
    }
    for (auto x = 1; x < width;) {
        x += Office_PlaceEdgeFillers(-1, x, depth - 1, 0, 0);
    }
    for (auto y = 1; y <= depth - 2;) {
        y += Office_PlaceEdgeFillers(-1, 0, y, 1, 1);
    }
    for (auto y = 1; y <= depth - 2;) {
        y += Office_PlaceEdgeFillers(-1, width, y, 3, 3);
    }
}
// 0x599960
int32 Interior_c::Office_PlaceDeskQuad(int32 a2, int32 a3, int32 a4, int32 a5) {
    const auto y = a3 - 2;
    Office_PlaceDesk(a2, y, 2, 0x46, 0, a5);
    Office_PlaceDesk(a2, a3, 0, 0x46, 0, a5);
    Office_PlaceDesk(a2 - 2, a3, 0, 0x46, 0, a5);
    Office_PlaceDesk(a2 - 2, y, 2, 0x46, 0, a5);

    SetTilesStatus(a2 - 3, a3 - 3, 6, 1, 3, 0);
    SetTilesStatus(a2 - 3, a3 + 2, 6, 1, 3, 0);
    SetTilesStatus(a2 - 3, y, 1, 4, 3, 0);
    SetTilesStatus(a2 + 2, y, 1, 4, 3, 0);
    return 6;
}

// 0x599A30
int32 Interior_c::Office_FurnishCenter() {
    const auto w         = (int32)m_box->m_width - 6;
    auto       numDesksX = w / 6;
    const auto d         = (int32)m_box->m_depth - 6;
    const auto numDesksY = d / 6;
    auto       x         = (w % 6) / 2;
    const auto y         = (d % 6) / 2;

    if (w > 0 && d > 0 && numDesksX > 0) {
        for (; numDesksX != 0; --numDesksX) {
            x += 6;
            for (auto i = numDesksY, deskY = y; i != 0; --i, deskY += 6) {
                Office_PlaceDeskQuad(-1, x, deskY + 6, (int32)m_furnitureId);
            }
        }
    }
    return y;
}

// 0x599AF0
void Interior_c::FurnishOffice() {
    SetTilesStatus(0, 0, 2, 2, 2, 0);
    SetTilesStatus(0, m_box->m_depth - 2, 2, 2, 2, 0);
    SetTilesStatus(m_box->m_width - 2, 0, 2, 2, 2, 0);
    SetTilesStatus(m_box->m_width - 2, m_box->m_depth - 2, 2, 2, 2, 0);

    m_furnitureId = static_cast<int8>(g_furnitureMan.GetRandomId(1, 0, m_box->m_status));
    field_792     = static_cast<int8>(g_furnitureMan.GetRandomId(1, 1, m_box->m_status));

    Office_FurnishEdges();
    Office_FurnishCenter();
    Shop_FurnishCeiling();
}

// 0x599BB0
int8 Interior_c::Shop_Place3PieceUnit(int32 a2, int32 a3, int32 a4, int32 a5, int32 a6) {
    // NOTE: `a2` and `a5` double as the out params of `PlaceFurniture` (Just like in the original code)
    Furniture_c *firstUnit, *middleUnit;
    if (a5 != 2 && a5 != 3) {
        firstUnit  = g_furnitureMan.GetFurniture(m_furnitureGroupId, a2, -1, m_box->m_status);
        middleUnit = g_furnitureMan.GetFurniture(m_furnitureGroupId, a2 + 1, firstUnit->m_nId, m_box->m_status);
    } else {
        firstUnit  = g_furnitureMan.GetFurniture(m_furnitureGroupId, a2 + 1, -1, m_box->m_status);
        middleUnit = g_furnitureMan.GetFurniture(m_furnitureGroupId, a2, firstUnit->m_nId, m_box->m_status);
    }
    auto* lastUnit = g_furnitureMan.GetFurniture(m_furnitureGroupId, a2 + 2, firstUnit->m_nId, m_box->m_status);

    const auto dir = a5;
    if (dir != 2 && dir != 0) { // Along Y
        PlaceFurniture(firstUnit, a3, a4, 0.0f, 1, dir, &a5, &a2, false);
        auto pos = a4 + a2; // `a2` now holds the size of the placed unit
        for (auto num = a6 - 2; num > 0; num--) {
            PlaceFurniture(lastUnit, a3, pos, 0.0f, 1, dir, &a5, &a2, false);
            pos += a2;
        }
        PlaceFurniture(middleUnit, a3, pos, 0.0f, 1, dir, &a5, &a2, false);
    } else { // Along X
        PlaceFurniture(firstUnit, a3, a4, 0.0f, 1, dir, &a5, &a2, false);
        auto pos = a3 + a5; // `a5` now holds the size of the placed unit
        for (auto num = a6 - 2; num > 0; num--) {
            PlaceFurniture(lastUnit, pos, a4, 0.0f, 1, dir, &a5, &a2, false);
            pos += a5;
        }
        PlaceFurniture(middleUnit, pos, a4, 0.0f, 1, dir, &a5, &a2, false);
    }
    return 1;
}

// 0x599DC0
int32 Interior_c::Shop_PlaceEdgeUnits(int32 type, int32 x, int32 y, int32 dir) {
    const auto alongX = (dir == 2 || dir == 0) ? 1 : 2;
    const auto numEmpty = GetNumEmptyTiles(x, y, alongX, 1);
    if (numEmpty <= 1) {
        return 1;
    }
    auto size = 2 - RandomNumberInRange(-3.0f);
    if (numEmpty != 3) {
        if (const auto rest = numEmpty - size; rest >= 0 && rest == 1) {
            size--;
        } else if (rest < 0) {
            size = numEmpty;
        }
    } else {
        size = 3;
    }
    static auto& s_Wealth = StaticRef<int32>(0xBB3DE4);
    if (type != -1) {
        Shop_Place3PieceUnit(type, x, y, dir, size);
    } else if (s_Wealth > 50) {
        Shop_Place3PieceUnit(0, x, y, dir, size);
    } else if (s_Wealth > 25) {
        Shop_Place3PieceUnit(3, x, y, dir, size);
    } else if (s_Wealth > 10) {
        Shop_Place3PieceUnit(6, x, y, dir, size);
    } else {
        Shop_Place3PieceUnit(9, x, y, dir, size);
    }
    return size;
}

// 0x599EF0
int32 Interior_c::Shop_PlaceCounter(uint8 a2) {
    auto* bigUnit       = g_furnitureMan.GetFurniture(0, 0xC, -1, m_box->m_status);
    auto* smallUnit     = g_furnitureMan.GetFurniture(0, 0xD, -1, m_box->m_status);

    const auto rotation = RandomNumberInRange(90.0f);

    int32 sizeX{}, sizeY{};
    int32 x;
    int32 x2;
    if (a2 == 0) {
        x = (int32)m_box->m_door + 2;
        PlaceFurniture(bigUnit, x, 1, 0.0f, 1, 0, &sizeX, &sizeY, false);
        SetTilesStatus(x, 0, sizeX + 1, 1, 2, 0);
        x2 = (int32)m_box->m_door - 2;
    } else {
        const auto door = (int32)m_box->m_door;
        x               = door - 5;
        PlaceFurniture(bigUnit, x, 1, 0.0f, 1, 0, &sizeX, &sizeY, false);
        SetTilesStatus(door - 6, 0, sizeX + 1, 1, 2, 0);
        x2 = door + 1;
    }
    PlaceFurniture(smallUnit, x2, 0, 0.0f, 1, rotation, &sizeX, &sizeY, true);

    return x + 2;
}

// 0x59A030
void Interior_c::Shop_PlaceFixedUnits() {
    const auto door = (int32)m_box->m_door;
    if (door == -1) {
        return;
    }
    SetTilesStatus(door - 1, 0, 2, 1, 7, 0);

    const auto spaceLeft  = door - 2;
    const auto spaceRight = (int32)m_box->m_width - door - 2;

    // NOTE: In the original code this is left uninitialized if there's no space for the counter on either side
    int32 counterX = 0;
    if (spaceRight >= 6) {
        counterX = Shop_PlaceCounter(spaceLeft >= 6 ? rand() < 0x3FFF : false);
    } else if (spaceLeft >= 6) {
        counterX = Shop_PlaceCounter(true);
    }

    AddInteriorInfo(9, (float)counterX, 2.0f, 0, nullptr);
    AddInteriorInfo(10, (float)counterX, 0.0f, 2, nullptr);
}
// 0x593DB0
bool Interior_c::GetBoundingBox(FurnitureEntity_c* entity, CVector* corners) {
    const auto type = m_box->m_type;
    if (type != 0 && type != 1 && type != 6) {
        return false;
    }
    const auto tileX = entity->m_tileX;
    const auto tileY = entity->m_tileY;
    int32 visited[900]{};
    visited[tileX * TILE_ROW_STRIDE + tileY] = 1;
    auto minX = (int32)tileX, maxX = (int32)tileX;
    auto minY = (int32)tileY, maxY = (int32)tileY;
    FindBoundingBox(tileX, tileY, &minX, &maxX, &minY, &maxY, visited);
    constexpr auto kOutset = 0.35f;
    GetTileCentre((float)minX - 0.5f - kOutset, (float)maxY + kOutset + 0.5f, &corners[0]);
    GetTileCentre((float)minX - 0.5f - kOutset, (float)minY - 0.5f - kOutset, &corners[1]);
    GetTileCentre((float)maxX + kOutset + 0.5f, (float)minY - 0.5f - kOutset, &corners[2]);
    GetTileCentre((float)maxX + kOutset + 0.5f, (float)maxY + kOutset + 0.5f, &corners[3]);
    return true;
}

// 0x593910
void Interior_c::ResetTiles() {
    std::fill(std::begin(field_68), std::end(field_68), 0);

    // Left doors
    if (m_box->m_lDoorStart != -1) {
        const auto start = (int32)m_box->m_lDoorStart;
        const auto count = (int32)m_box->m_lDoorEnd - start;
        if (count > 0 && start >= 0 && m_box->m_width != 0 && start + count <= (int32)m_box->m_depth) {
            for (auto i = 0; i < count; i++) {
                auto& tile = field_68[start + i]; // Leftmost column (x = 0)
                if (tile != 3 && tile == 0) {
                    tile = 8;
                }
            }
        }
    }

    // Right doors
    if (m_box->m_rDoorStart != -1) {
        const auto start = (int32)m_box->m_rDoorStart;
        const auto count = (int32)m_box->m_rDoorEnd - start;
        const auto x     = (int32)m_box->m_width - 1;
        if (count > 0 && x >= 0 && start >= 0 && start + count <= (int32)m_box->m_depth) {
            for (auto i = 0; i < count; i++) {
                auto& tile = field_68[x * TILE_ROW_STRIDE + start + i];
                if (tile != 3 && tile == 0) {
                    tile = 8;
                }
            }
        }
    }

    // Top doors
    if (m_box->m_tDoorStart != -1) {
        const auto start = (int32)m_box->m_tDoorStart;
        const auto count = (int32)m_box->m_tDoorEnd - start;
        const auto y     = (int32)m_box->m_depth - 1;
        if (count > 0 && start >= 0 && y >= 0 && start + count <= (int32)m_box->m_width) {
            for (auto i = 0; i < count; i++) {
                auto& tile = field_68[(start + i) * TILE_ROW_STRIDE + y];
                if (tile != 3 && tile == 0) {
                    tile = 8;
                }
            }
        }
    }

    // Left windows
    if (m_box->m_lWindowStart != -1) {
        const auto start = (int32)m_box->m_lWindowStart;
        const auto count = (int32)m_box->m_lWindowEnd - start;
        if (count > 0 && start >= 0 && m_box->m_width != 0 && start + count <= (int32)m_box->m_depth) {
            for (auto i = 0; i < count; i++) {
                auto& tile = field_68[start + i]; // Leftmost column (x = 0)
                if (tile != 3 && tile == 0) {
                    tile = 9;
                }
            }
        }
    }

    // Right windows
    if (m_box->m_rWindowStart != -1) {
        const auto start = (int32)m_box->m_rWindowStart;
        const auto count = (int32)m_box->m_rWindowEnd - start;
        const auto x     = (int32)m_box->m_width - 1;
        if (count > 0 && x >= 0 && start >= 0 && start + count <= (int32)m_box->m_depth) {
            for (auto i = 0; i < count; i++) {
                auto& tile = field_68[x * TILE_ROW_STRIDE + start + i];
                if (tile != 3 && tile == 0) {
                    tile = 9;
                }
            }
        }
    }

    // Top windows
    if (m_box->m_tWindowStart != -1) {
        const auto start = (int32)m_box->m_tWindowStart;
        const auto count = (int32)m_box->m_tWindowEnd - start;
        const auto y     = (int32)m_box->m_depth - 1;
        if (count > 0 && start >= 0 && y >= 0 && start + count <= (int32)m_box->m_width) {
            for (auto i = 0; i < count; i++) {
                auto& tile = field_68[(start + i) * TILE_ROW_STRIDE + y];
                if (tile != 3 && tile == 0) {
                    tile = 9;
                }
            }
        }
    }

    // No-go zones
    for (auto i = 0; i < 3; i++) {
        const auto x = (int32)m_box->m_noGoLeft[i];
        if (x == -1) {
            continue;
        }
        const auto y = (int32)m_box->m_noGoBottom[i];
        if (y == -1) {
            continue;
        }
        const auto w = (int32)m_box->m_noGoWidth[i];
        const auto d = (int32)m_box->m_noGoDepth[i];
        if (x < 0 || y < 0 || x + w > (int32)m_box->m_width || y + d > (int32)m_box->m_depth || w <= 0) {
            continue;
        }
        for (auto ix = 0; ix < w; ix++) {
            for (auto iy = 0; iy < d; iy++) {
                auto& tile = field_68[(x + ix) * TILE_ROW_STRIDE + y + iy];
                if (tile != 3 && tile == 0) {
                    tile = 11;
                }
            }
        }
    }
}

// 0x5934E0
CObject* Interior_c::PlaceObject(uint8 isStealable, Furniture_c* furniture, float offsetX, float offsetY, float offsetZ, float rotationZ) {
    // `g_furnitureMan.m_FurnitureList`: The free list (`FurnitureItem` has the same layout as `FurnitureEntity_c`)
    static auto& s_FreeFurnitureList = StaticRef<TList_c<FurnitureEntity_c>>(0xBAD3EC);
    static auto& s_ObjectCount       = StaticRef<int32>(0xBB3A18);              // g_interiorMan.m_ObjectCount
    static auto& s_Objects           = StaticRef<InteriorObject[32]>(0xBB3A1C); // g_interiorMan.m_Objects

    const auto  modelId = (int32)(uint16)furniture->m_nModelId;
    const auto& bbMin   = CModelInfo::GetModelInfo(modelId)->GetColModel()->GetBoundingBox().m_vecMin;

    // Position relative to the centre of the interior
    const CVector pos{
        (float)-(int32)m_box->m_width * 0.5f + offsetX,
        (float)-(int32)m_box->m_depth * 0.5f + offsetY,
        (float)-(int32)m_box->m_height * 0.5f + offsetZ - bbMin.z
    };

    if ((int32)s_FreeFurnitureList.GetNumItems() <= 0) {
        return nullptr;
    }

    CMatrix interiorMat{ &m_matrix, false };

    CMatrix localMat;
    localMat.SetUnity();
    localMat.RotateZ(rotationZ * 0.017453292f); // Degrees to radians
    localMat.GetPosition() += pos;

    CMatrix finalMat;
    finalMat = interiorMat * localMat;

    auto* const item = s_FreeFurnitureList.RemoveHead();
    if (!item) {
        return nullptr;
    }

    // 0x593683
    auto* const obj = new CObject(modelId, false);
    item->m_entity  = obj;
    obj->SetMatrix(finalMat);
    obj->SetAreaCode(static_cast<eAreaCodes>(static_cast<int8>(m_areaCode)));
    obj->m_bDontCastShadowsOn = true; // 0x10000
    obj->m_nObjectType        = OBJECT_TYPE_DECORATION;
    obj->SetIsStatic(true);
    CWorld::Add(obj);

    item->m_tileX = (uint16)(int32)offsetX;
    item->m_tileY = (uint16)(int32)offsetY;
    auto& furnitureEntities = reinterpret_cast<TList_c<FurnitureEntity_c>&>(m_list);
    furnitureEntities.AddItem(item);

    if (isStealable) {
        obj->objectFlags.bIsLiftable = true; // 0x2000

        if (!g_interiorMan.HasInteriorHadStealDataSetup(this)) {
            // First time this interior is set up, so register the object (NB: No bounds check in the original either)
            auto& stealable      = s_Objects[s_ObjectCount];
            stealable.entity     = obj;
            stealable.modelId    = modelId;
            stealable.interiorId = m_interiorId;
            stealable.pos        = pos;
            stealable.wasStolen  = false;
            s_ObjectCount++;
        } else {
            if (const auto id = g_interiorMan.FindStealableObjectId(m_interiorId, modelId, pos); id >= 0 && s_Objects[id].wasStolen) {
                // Already stolen, so don't place it again
                CWorld::Remove(item->m_entity);
                delete item->m_entity;
                item->m_entity = nullptr;
                furnitureEntities.RemoveItem(item);
                s_FreeFurnitureList.AddItem(item);
                return nullptr;
            }
            if (const auto id = g_interiorMan.FindStealableObjectId(m_interiorId, modelId, pos); id >= 0) {
                s_Objects[id].entity = obj;
            }
        }
    }

    return obj;
}

// 0x5913B0
FurnitureEntity_c* Interior_c::GetFurnitureEntity(const CEntity& entity) {
    for (auto* item = reinterpret_cast<TList_c<FurnitureEntity_c>&>(m_list).GetHead(); item; item = item->m_pNext) {
        if (item->m_entity == &entity) {
            return item;
        }
    }
    return nullptr;
}

// 0x5913E0
bool Interior_c::IsPtInside(const CVector& pt, CVector bias) {
    const CVector offset = pt - m_matrix.pos;

    if (std::abs(m_matrix.right.x * offset.x + m_matrix.right.y * offset.y + m_matrix.right.z * offset.z) > bias.x + (float)m_box->m_width * 0.5f) {
        return false;
    }
    if (std::abs(m_matrix.up.x * offset.x + m_matrix.up.y * offset.y + m_matrix.up.z * offset.z) > bias.y + (float)m_box->m_depth * 0.5f) {
        return false;
    }
    return std::abs(m_matrix.at.x * offset.x + m_matrix.at.y * offset.y + m_matrix.at.z * offset.z) <= bias.z + (float)m_box->m_height * 0.5f;
}

// 0x5914D0
void Interior_c::CalcMatrix(CVector* translation) {
    m_matrix.right = { 1.0f, 0.0f, 0.0f };
    m_matrix.up    = { 0.0f, 1.0f, 0.0f };
    m_matrix.at    = { 0.0f, 0.0f, 1.0f };
    m_matrix.pos   = { 0.0f, 0.0f, 0.0f };
    m_matrix.flags |= 0x20003; // rwMATRIXINTERNALIDENTITY | 0x3

    const CVector axis = { 0.0f, 0.0f, 1.0f };
    RwMatrixRotate(&m_matrix, &axis, m_box->m_rot, rwCOMBINEREPLACE);
    RwMatrixTranslate(&m_matrix, translation, rwCOMBINEPOSTCONCAT);

    if (auto* entity = m_pGroup->GetEntity(); entity->GetRwObject()) {
        RwMatrixMultiply(&m_matrix, &m_matrix, entity->GetRwMatrix());
    } else {
        RwMatrixMultiply(&m_matrix, &m_matrix, nullptr);
    }
}

// 0x591590
void Interior_c::Furnish() {
    switch (m_box->m_type) {
    case 0:
        FurnishShop(0);
        break;
    case 1:
        FurnishOffice();
        break;
    case 2:
        FurnishLounge();
        break;
    case 3:
        FurnishBedroom();
        break;
    case 4:
        FurnishKitchen();
        break;
    }
}

// 0x5915D0
void Interior_c::Unfurnish() {
    // `m_list` is the list of this interior's furniture entities (See `GetFurnitureEntity`)
    auto&        furnitureEntities   = reinterpret_cast<TList_c<FurnitureEntity_c>&>(m_list);
    // `g_furnitureMan.m_FurnitureList`: The free list the items are returned to (`FurnitureItem` has the same layout as `FurnitureEntity_c`)
    static auto& s_FreeFurnitureList = StaticRef<TList_c<FurnitureEntity_c>>(0xBAD3EC);

    for (auto* item = furnitureEntities.GetHead(); item;) {
        auto* const next = item->m_pNext;

        auto* const player = FindPlayerPed();
        auto* const held   = player ? player->GetEntityThatThisPedIsHolding() : nullptr;
        if (held && held == item->m_entity && held->GetIsTypeObject() && held->AsObject()->objectFlags.bIsLiftable) {
            // The player is carrying this object, so let it live on as a temporary object
            auto* const obj = held->AsObject();
            CObject::nNoTempObjects++;
            obj->m_nObjectType  = OBJECT_TEMPORARY;
            obj->m_nRemovalTime = CTimer::GetTimeInMS() + 99'999'999;
        } else {
            CWorld::Remove(item->m_entity);
            delete item->m_entity;
        }

        item->m_entity = nullptr;
        furnitureEntities.RemoveItem(item);
        s_FreeFurnitureList.AddItem(item);

        item = next;
    }
}

// 0x591680
int8 Interior_c::CheckTilesEmpty(int32 a1, int32 a2, int32 a3, int32 a4, uint8 a5) {
    if (a1 < 0 || a2 < 0 || (int32)m_box->m_width < a1 + a3 || (int32)m_box->m_depth < a2 + a4) {
        return false;
    }

    for (auto x = 0; x < a3; x++) {
        for (auto y = 0; y < a4; y++) {
            if (const auto tile = field_68[(a1 + x) * TILE_ROW_STRIDE + a2 + y]) {
                if (a5 == 0) {
                    return false;
                }
                if (tile != 9) {
                    return false;
                }
            }
        }
    }
    return true;
}

// 0x591700
void Interior_c::SetTilesStatus(int32 a, int32 b, int32 a3, int32 a4, int32 a5, int8 a6) {
    if (a < 0 || b < 0 || a + a3 > (int32)m_box->m_width || b + a4 > (int32)m_box->m_depth || a3 <= 0) {
        return;
    }

    for (auto x = 0; x < a3; x++) {
        for (auto y = 0; y < a4; y++) {
            auto& tile = field_68[(a + x) * TILE_ROW_STRIDE + b + y];
            if (tile == 9 && a5 == 5) {
                tile = 10;
            } else if (a6 == 0) {
                if (tile == 3) {
                    if (a5 == 3) {
                        return;
                    }
                    if (a5 == 4) {
                        tile = 4;
                    }
                } else if (tile == 0) { // Otherwise some tile types are overwritten
                    tile = (char)a5;
                }
            } else if (tile != 5 && tile != 7 && tile != 8) {
                tile = (char)a5;
            }
        }
    }
}

// 0x5917C0
void Interior_c::SetCornerTiles(int32 a4, int32 a3, int32 a5, uint8 a6) {
    switch (a4) {
    case 0:
        SetTilesStatus(0, m_box->m_depth - 1, a3, 1, a5, a6);
        SetTilesStatus(0, m_box->m_depth - a3, 1, a3, a5, a6);
        break;
    case 1:
        SetTilesStatus(0, 0, a3, 1, a5, a6);
        SetTilesStatus(0, 0, 1, a3, a5, a6);
        break;
    case 2:
        SetTilesStatus(m_box->m_width - a3, 0, a3, 1, a5, a6);
        SetTilesStatus(m_box->m_width - 1, 0, 1, a3, a5, a6);
        break;
    case 3:
        SetTilesStatus(m_box->m_width - a3, m_box->m_depth - 1, a3, 1, a5, a6);
        SetTilesStatus(m_box->m_width - 1, m_box->m_depth - a3, 1, a3, a5, a6);
        break;
    }
}

// 0x5918E0
int32 Interior_c::GetTileStatus(int32 x, int32 y) {
    if (x < (int32)m_box->m_width && y < (int32)m_box->m_depth && x >= 0 && y >= 0) {
        return field_68[x * TILE_ROW_STRIDE + y];
    }
    return 1;
}

// 0x591920
int32 Interior_c::GetNumEmptyTiles(int32 a2, int32 a3, int32 a4, int32 a5) {
    int32      numEmptyTiles = 0;
    const auto step          = a4 == 3 || a4 == 0 ? -1 : 1;

    if (a4 == 3 || a4 == 1) { // Scan along Y
        for (auto x = a2;; x += step) {
            for (auto i = 0, y = a3; i < a5; i++, y++) {
                if (x >= (int32)m_box->m_width || y >= (int32)m_box->m_depth || x < 0 || y < 0 || field_68[x * TILE_ROW_STRIDE + y] != 0) {
                    return numEmptyTiles;
                }
            }
            numEmptyTiles++;
        }
    } else { // Scan along X
        for (auto y = a3;; y += step) {
            for (auto i = 0, x = a2; i < a5; i++, x++) {
                if (x >= (int32)m_box->m_width || y >= (int32)m_box->m_depth || x < 0 || y < 0 || field_68[x * TILE_ROW_STRIDE + y] != 0) {
                    return numEmptyTiles;
                }
            }
            numEmptyTiles++;
        }
    }
}

// 0x591B20
int32 Interior_c::GetRandomTile(int32 a2, int32* a3, int32* a4) {
    int32 x, y;
    do {
        x = RandomNumberInRange((float)m_box->m_width);
        y = RandomNumberInRange((float)m_box->m_depth);
    } while (GetTileStatus(x, y) != a2);

    *a3 = x;
    *a4 = y;
    return y;
}

// 0x59A590
void Interior_c::Shop_FurnishAisles() {
    const auto width = (int32)m_box->m_width;
    const auto depth = (int32)m_box->m_depth;
    const auto numX  = width - 6;
    const auto numY  = depth - 7;
    if (numX <= 0 || numY <= 0) {
        return;
    }

    AddGotoPt(2, 3, -0.5f, -0.5f);
    const auto maxY = depth - 3;
    AddGotoPt(2, maxY, -0.5f, 0.5f);

    int32 x = 0;
    for (; x < numX; x++) {
        auto y           = 4;

        const auto r = RandomNumberInRange(100.0f);
        int32      type;
        if (r > 50) {
            type = 0;
        } else if (r > 25) {
            type = 3;
        } else if (r > 10) {
            type = 6;
        } else {
            type = 9;
        }

        switch (x % 4) {
        case 0:
            if (x != width - 7) {
                for (auto i = numY; i > 0; i--) {
                    y += Shop_PlaceEdgeUnits(type, x + 3, y, 3);
                }
            }
            break;
        case 1:
            for (auto i = numY; i > 0; i--) {
                y += Shop_PlaceEdgeUnits(type, x + 3, y, 1);
            }
            break;
        case 2:
            for (auto i = 0; i < numY; i++) {
                Shop_AddShelfInfo(x + 3, i + 4, 3);
            }
            break;
        case 3:
            SetTilesStatus(x + 3, 4, 1, numY, 3, 0);
            AddGotoPt(x + 3, 3, -0.5f, -0.5f);
            AddGotoPt(x + 3, maxY, -0.5f, 0.5f);
            break;
        }
    }

    AddGotoPt(x + 3, 3, 0.5f, -0.5f);
    AddGotoPt(x + 3, maxY, 0.5f, 0.5f);
}

// 0x591BD0
CVector* Interior_c::GetTileCentre(float offsetX, float offsetY, CVector* pointsIn) {
    pointsIn->x = (float)-m_box->m_width * 0.5f + offsetX + 0.5f;
    pointsIn->y = (float)-m_box->m_depth * 0.5f + offsetY + 0.5f;
    pointsIn->z = (float)-m_box->m_height * 0.5f;
    RwV3dTransformPoints(pointsIn, pointsIn, 1, &m_matrix);
    return pointsIn;
}

// 0x591D20
void Interior_c::AddGotoPt(int32 x, int32 y, float biasX, float biasY) {
    if (field_40C >= 16) {
        return;
    }
    if ((x < (int32)m_box->m_width && y < (int32)m_box->m_depth && x >= 0 && y >= 0 && field_68[x * TILE_ROW_STRIDE + y] == 3)
        || GetTileStatus(x, y) == 7) {
        CVector pos;
        GetTileCentre((float)x + biasX, (float)y + biasY, &pos);

        // Each goto slot is 0x10 bytes at 0x410: int8 tileX at +0, int8 tileY at +1, CVector pos at +4
        auto* slotBase = reinterpret_cast<uint8*>(this) + 0x410 + (int32)field_40C * 0x10;
        slotBase[0x0] = (int8)x;
        slotBase[0x1] = (int8)y;
        *reinterpret_cast<CVector*>(slotBase + 0x4) = pos;

        if (x >= 0 && y >= 0 && x + 1 <= (int32)m_box->m_width && y + 1 <= (int32)m_box->m_depth) {
            auto& tile = field_68[x * TILE_ROW_STRIDE + y];
            if (tile == 3 || tile == 0) {
                tile = 4;
            }
        }
        field_40C++;
    }
}

// 0x591E40
bool Interior_c::AddInteriorInfo(int32 actionType, float offsetX, float offsetY, int32 direction, CEntity* entityIgnoredCollision) {
    if (m_interiorInfosCount >= 16) {
        return false;
    }

    CVector pos;
    GetTileCentre(offsetX, offsetY, &pos);
    pos.z += 0.8f;

    CVector dir{};
    if (direction != -1) {
        switch (direction) {
        case 0:
            dir.y = -1.0f;
            break;
        case 1:
            dir.x = 1.0f;
            break;
        case 2:
            dir.y = 1.0f;
            break;
        case 3:
            dir.x = -1.0f;
            break;
        }
        RwV3dTransformVectors(&dir, &dir, 1, &m_matrix);
    }

    auto& info                  = m_interiorInfos[m_interiorInfosCount];
    info.Type                   = static_cast<eInteriorInfoType>(actionType);
    info.Pos                    = pos;
    info.Dir                    = dir;
    info.IsInUse                = false;
    info.EntityIgnoredCollision = entityIgnoredCollision;
    m_interiorInfosCount++;
    return true;
}

// 0x591F90
void Interior_c::AddPickups() {
    static auto& s_LastPickupTimeMs = StaticRef<uint32>(0xBB3DC4);
    if (CTimer::GetTimeInMS() - s_LastPickupTimeMs <= 179999) {
        return;
    }
    int32 numPlaced = 0;
    for (int32 i = 0; i < 100 && numPlaced <= 0; i++) {
        const auto x = RandomNumberInRange((float)m_box->m_width - 1.0f);
        const auto y = RandomNumberInRange((float)m_box->m_depth - 1.0f);
        if (x >= (int32)m_box->m_width || y >= (int32)m_box->m_depth || x < 0 || y < 0) {
            continue;
        }
        const auto tile = field_68[x * TILE_ROW_STRIDE + y];
        if (tile != 0 && tile != 3 && tile != 4) {
            continue;
        }
        CVector pos;
        GetTileCentre((float)x, (float)y, &pos);
        if (RandomNumberInRange(100.0f) < 75) {
            CPickups::GenerateNewOne(pos, ModelIndices::MI_MONEY, PICKUP_MONEY, 10 - RandomNumberInRange(-40.0f));
        } else {
            pos.z += 0.5f;
            int32 weaponType;
            const auto r = RandomNumberInRange(100.0f);
            if (r < 40) {
                weaponType = WEAPON_BASEBALLBAT;
            } else if (r < 80) {
                weaponType = WEAPON_PISTOL;
            } else {
                weaponType = ((0x59 < r) - 1 & 0xFFFFFFEB) + 0x19;
            }
            CPickups::GenerateNewOne_WeaponType(pos, (eWeaponType)weaponType, PICKUP_ONCE, 3 - RandomNumberInRange(-15.0f), false, nullptr);
        }
        numPlaced++;
    }
}

// 0x5922C0
void Interior_c::FindBoundingBox(int32 x, int32 y, int32* minX, int32* maxX, int32* minY, int32* maxY, int32* visited) {
    int32 nextY = y + 1;
    while (true) {
        // Neighbour (x - 1, y)
        if (x > 0 && x - 1 < (int32)m_box->m_width && y < (int32)m_box->m_depth && x - 1 >= 0 && y >= 0) {
            const auto idx = (x - 1) * TILE_ROW_STRIDE + y;
            if (field_68[idx] == 5 && !visited[idx]) {
                visited[idx] = 1;
                if (x - 1 < *minX) {
                    *minX = x - 1;
                }
                FindBoundingBox(x - 1, y, minX, maxX, minY, maxY, visited);
            }
        }
        // Neighbour (x, y + 1)
        if (y < 29 && x < (int32)m_box->m_width && nextY < (int32)m_box->m_depth && x >= 0 && nextY >= 0) {
            const auto idx = x * TILE_ROW_STRIDE + nextY;
            if (field_68[idx] == 5 && !visited[idx]) {
                visited[idx] = 1;
                if (nextY > *maxY) {
                    *maxY = nextY;
                }
                FindBoundingBox(x, nextY, minX, maxX, minY, maxY, visited);
            }
        }
        // Neighbour (x + 1, y)
        if (x < 29) {
            const auto nextX = x + 1;
            if (nextX < (int32)m_box->m_width && y < (int32)m_box->m_depth && nextX >= 0) {
                if (y < 0) {
                    return;
                }
                const auto idx = nextX * TILE_ROW_STRIDE + y;
                if (field_68[idx] == 5 && !visited[idx]) {
                    visited[idx] = 1;
                    if (nextX > *maxX) {
                        *maxX = nextX;
                    }
                    FindBoundingBox(nextX, y, minX, maxX, minY, maxY, visited);
                }
            }
        }
        // Neighbour (x, y - 1), handled iteratively
        if (y <= 0) {
            return;
        }
        if (x >= (int32)m_box->m_width) {
            return;
        }
        const auto prevY = nextY - 2;
        if (prevY >= (int32)m_box->m_depth) {
            return;
        }
        if (x < 0) {
            return;
        }
        if (prevY < 0) {
            return;
        }
        const auto idx = x * TILE_ROW_STRIDE + prevY;
        if (field_68[idx] != 5) {
            return;
        }
        if (visited[idx]) {
            return;
        }
        visited[idx] = 1;
        if (prevY < *minY) {
            *minY = prevY;
        }
        y--;
        nextY--;
    }
}

// 0x5924A0
void Interior_c::CalcExitPts() {
    auto* const gotoPts    = reinterpret_cast<InteriorGoToPt*>(reinterpret_cast<uint8*>(this) + 0x410);
    auto* const exitPts    = reinterpret_cast<InteriorGoToPt*>(reinterpret_cast<uint8*>(this) + 0x510);
    const auto  numGotoPts = (int32)field_40C;

    // Centre of a door spanning tiles [start, end)
    const auto GetDoorCentre = [](int8 start, int8 end) {
        return (float)((int32)end - (int32)start) * 0.5f + (float)start - 0.5f;
    };

    // 0x5924A7 - Main door (at `y = 0`)
    if (m_box->m_door >= 0) {
        auto&      pt   = exitPts[0];
        auto&      door = exitPts[1];
        const auto x    = (float)m_box->m_door - 0.5f;
        float      y;
        if (numGotoPts > 2) {
            int32 i = 0;
            for (; i < numGotoPts; i += 2) {
                if ((float)gotoPts[i].TileX > x) {
                    break;
                }
            }
            if (i == 0) {
                pt.Prev = 0;
                pt.Next = -1;
            } else if (i == numGotoPts) {
                pt.Prev = (int8)(numGotoPts - 2);
                pt.Next = -1;
            } else {
                pt.Prev = (int8)i;
                pt.Next = (int8)(i - 2);
            }
            y = (float)gotoPts[0].TileY - 0.25f;
        } else {
            pt.Prev = -1;
            pt.Next = -1;
            y       = 0.0f;
        }
        GetTileCentre(x, y, &pt.Pos);
        GetTileCentre(x, -0.25f, &door.Pos);
    }

    // 0x5925DF - Left door (at `x = 0`)
    if (m_box->m_lDoorStart >= 0) {
        auto&      pt   = exitPts[2];
        auto&      door = exitPts[3];
        const auto y    = GetDoorCentre(m_box->m_lDoorStart, m_box->m_lDoorEnd);
        float      x;
        if (numGotoPts > 2) {
            if ((float)gotoPts[0].TileY > y) {
                pt.Prev = 0;
                pt.Next = -1;
            } else if ((float)gotoPts[1].TileY < y) {
                pt.Prev = 1;
                pt.Next = -1;
            } else {
                pt.Prev = 0;
                pt.Next = 1;
            }
            x = (float)gotoPts[0].TileX - 0.25f;
        } else {
            pt.Prev = -1;
            pt.Next = -1;
            x       = 0.0f;
        }
        GetTileCentre(x, y, &pt.Pos);
        GetTileCentre(-0.25f, y, &door.Pos);
    }

    // 0x592714 - Top door (at `y = depth - 1`)
    if (m_box->m_tDoorStart >= 0) {
        auto&      pt   = exitPts[4];
        auto&      door = exitPts[5];
        const auto x    = GetDoorCentre(m_box->m_tDoorStart, m_box->m_tDoorEnd);
        float      y;
        if (numGotoPts > 2) {
            int32 i = 1;
            for (; i < numGotoPts; i += 2) {
                if ((float)gotoPts[i].TileX > x) {
                    break;
                }
            }
            // NB: Original also has an `i == 0` case here (`Prev = 1, Next = -1`), but that's unreachable
            if (i == numGotoPts) {
                pt.Prev = (int8)(numGotoPts - 1);
                pt.Next = -1;
            } else {
                pt.Prev = (int8)i;
                pt.Next = (int8)(i - 2);
            }
            y = (float)gotoPts[1].TileY + 0.25f;
        } else {
            pt.Prev = -1;
            pt.Next = -1;
            y       = (float)((int32)m_box->m_depth - 1);
        }
        GetTileCentre(x, y, &pt.Pos);
        GetTileCentre(x, (float)((int32)m_box->m_depth - 1) + 0.25f, &door.Pos);
    }

    // 0x592879 - Right door (at `x = width - 1`)
    if (m_box->m_rDoorStart >= 0) {
        auto&      pt   = exitPts[6];
        auto&      door = exitPts[7];
        const auto y    = GetDoorCentre(m_box->m_rDoorStart, m_box->m_rDoorEnd);
        float      x;
        if (numGotoPts > 2) {
            const auto& secondToLast = gotoPts[numGotoPts - 2];
            const auto& last         = gotoPts[numGotoPts - 1];
            if ((float)secondToLast.TileY > y) {
                pt.Prev = (int8)(numGotoPts - 2);
                pt.Next = -1;
            } else if ((float)last.TileY < y) {
                pt.Prev = (int8)(numGotoPts - 1);
                pt.Next = -1;
            } else {
                pt.Prev = (int8)(numGotoPts - 1);
                pt.Next = (int8)(numGotoPts - 2);
            }
            x = (float)secondToLast.TileX + 0.25f;
        } else {
            pt.Prev = -1;
            pt.Next = -1;
            x       = (float)((int32)m_box->m_width - 1);
        }
        GetTileCentre(x, y, &pt.Pos);
        GetTileCentre((float)((int32)m_box->m_width - 1) + 0.25f, y, &door.Pos);
    }
}

// 0x5929F0
bool Interior_c::IsVisible() {
    const auto& camPos = TheCamera.GetPosition();
    if (IsPtInside(camPos, { 5.0f, 5.0f, 0.0f })) {
        return true;
    }
    if (m_box->m_door > 0) {
        const auto dx = camPos.x - m_position.x;
        const auto dy = camPos.y - m_position.y;
        if (dx * dx + dy * dy < 100.0f) {
            return true;
        }
    }
    return false;
}

// 0x592AA0
CObject* Interior_c::PlaceFurniture(Furniture_c* a1, int32 a2, int32 a3, float a4, int32 a5, int32 a6, int32* a7, int32* a8, uint8 a9) {
    // a1 = furniture, a2 = tile X, a3 = tile Y, a4 = Z offset, a5 = height info (0 - ceiling, 1 - floor [marks tiles], 2 - floor),
    // a6 = direction, a7 = out placed width, a8 = out placed depth, a9 = don't check if the tiles are empty
    auto* const furniture = a1;
    const auto  x = a2, y = a3, heightInfo = a5, dir = a6;

    // `g_furnitureMan.m_FurnitureList`: The free list (`FurnitureItem` has the same layout as `FurnitureEntity_c`)
    static auto& s_FreeFurnitureList = StaticRef<TList_c<FurnitureEntity_c>>(0xBAD3EC);

    const auto Fail = [&]() -> CObject* {
        *a7 = 0;
        *a8 = 0;
        return nullptr;
    };

    if ((int32)s_FreeFurnitureList.GetNumItems() <= 0) {
        return Fail();
    }

    const auto furnWidth = (int32)(uint8)furniture->m_nWidthX;
    const auto furnDepth = (int32)(uint8)furniture->m_nWidthY;

    // Size in tiles (depends on the direction)
    const auto sizeX = (dir == 1 || dir == 3) ? furnDepth : furnWidth;
    const auto sizeY = (dir == 1 || dir == 3) ? furnWidth : furnDepth;

    if (heightInfo == 1 && !a9 && !CheckTilesEmpty(x, y, sizeX, sizeY, furniture->m_bCanPlaceInFrontOfWindow)) {
        return Fail();
    }

    // 0x592B66 - Random rotation around the centre of the furniture
    CMatrix randomRotMat;
    if ((uint8)furniture->m_nMaxAng > 0) {
        const auto pivotY = 0.5f - (float)furnDepth * 0.5f;
        const auto pivotX = 0.5f - (float)furnWidth * 0.5f;

        CMatrix toPivot;
        toPivot.SetUnity();
        toPivot.SetTranslate(CVector{ pivotX, pivotY, 0.0f });

        CMatrix rotation;
        rotation.SetUnity();
        const auto maxAngle = (int32)(uint8)furniture->m_nMaxAng;
        const auto angle = RandomIntInRange(-maxAngle, maxAngle);
        rotation.RotateZ((float)angle * 0.017453292f); // Degrees to radians

        CMatrix fromPivot;
        fromPivot.SetUnity();
        fromPivot.SetTranslate(CVector{ -pivotX, -pivotY, 0.0f });

        randomRotMat = (fromPivot * rotation) * toPivot;
    } else {
        randomRotMat.SetUnity();
    }

    // 0x592D2F
    CMatrix interiorMat{ &m_matrix, false };

    CMatrix localMat;
    localMat.SetUnity();
    localMat.RotateZ((float)dir * HALF_PI);

    int32 offsetX = 0, offsetY = 0;
    switch (dir) {
    case 1:
        offsetX = sizeX - 1;
        break;
    case 2:
        offsetX = sizeX - 1;
        offsetY = sizeY - 1;
        break;
    case 3:
        offsetY = sizeY - 1;
        break;
    }

    if (heightInfo == 1 || heightInfo == 2 || heightInfo == 0) {
        const auto posZ = heightInfo == 0
            ? (float)m_box->m_height * 0.5f - a4
            : (float)-(int32)m_box->m_height * 0.5f + a4;
        const auto posY = (float)-(int32)m_box->m_depth * 0.5f + (float)offsetY + (float)y + 0.5f;
        const auto posX = (float)-(int32)m_box->m_width * 0.5f + (float)offsetX + (float)x + 0.5f;

        auto& pos = localMat.GetPosition();
        pos.x += posX;
        pos.y += posY;
        pos.z += posZ;
    }

    // 0x592EB5
    CMatrix finalMat;
    finalMat = (interiorMat * localMat) * randomRotMat;

    auto* const item = s_FreeFurnitureList.RemoveHead();
    if (!item) {
        return Fail();
    }

    // 0x592F41
    auto* const building = new CBuilding();
    item->m_entity = building;
    building->SetModelIndexNoCreate((uint16)furniture->m_nModelId);
    building->SetMatrix(finalMat);
    building->SetAreaCode(static_cast<eAreaCodes>(static_cast<int8>(m_areaCode)));
    building->m_bDontCastShadowsOn = true; // 0x10000
    building->m_bIsTempBuilding    = true; // 0x400000
    CWorld::Add(building);

    item->m_tileX = (uint16)x;
    item->m_tileY = (uint16)y;
    reinterpret_cast<TList_c<FurnitureEntity_c>&>(m_list).AddItem(item);

    if (heightInfo == 1) {
        SetTilesStatus(x, y, sizeX, sizeY, furniture->m_bIsTall ? 6 : 5, 1);
    }

    *a7 = sizeX;
    *a8 = sizeY;

    // NB: It's actually a `CBuilding`
    return reinterpret_cast<CObject*>(item->m_entity);
}

// 0x593120
CObject* Interior_c::PlaceFurnitureOnWall(int32 furnitureGroupId, int32 furnitureSubgroupId, int32 furnitureId, float a5, int32 a6, int32 a7, int32 a8, int32 a9, int32* a10, int32* a11, int32* a12, int32* a13, int32* a14, int32* a15) {
    // a5 = Z offset, a6 = height info, a7 = wall (-1 for random), a8 = position along the wall (-1 for random), a9 = distance from the wall
    // a10 = out wall, a11 = out position along the wall, a12 = out tile X, a13 = out tile Y, a14 = out placed width, a15 = out placed depth
    auto* const furniture = g_furnitureMan.GetFurniture(furnitureGroupId, furnitureSubgroupId, (int16)furnitureId, m_box->m_status);

    const auto wantedWall = a7, wantedPos = a8;
    const auto numTries   = (wantedWall != -1 && wantedPos != -1) ? 1 : 100;
    if (!furniture) {
        return nullptr;
    }

    const auto furnWidth = (int32)(uint8)furniture->m_nWidthX;
    const auto furnDepth = (int32)(uint8)furniture->m_nWidthY;

    for (auto i = 0; i < numTries; i++) {
        if (wantedWall == -1) {
            a7 = RandomNumberInRange(4.0f);
        }
        const auto wall = a7;

        int32 x, y;
        if (wall == 1 || wall == 3) {
            if (wantedPos == -1) {
                a8 = RandomNumberInRange((float)((int32)m_box->m_depth - furnWidth));
            }
            y = a8;
            x = wall == 1 ? a9 : (int32)m_box->m_width - furnDepth - a9;
        } else {
            if (wantedPos == -1) {
                a8 = RandomNumberInRange((float)((int32)m_box->m_width - furnWidth));
            }
            x = a8;
            y = wall == 2 ? a9 : (int32)m_box->m_depth - furnDepth - a9;
        }

        int32 sizeX, sizeY;
        if (auto* const obj = PlaceFurniture(furniture, x, y, a5, a6, wall, &sizeX, &sizeY, false)) {
            if (a10) {
                *a10 = a7;
            }
            if (a11) {
                *a11 = a8;
            }
            if (a12) {
                *a12 = x;
            }
            if (a13) {
                *a13 = y;
            }
            if (a14) {
                *a14 = sizeX;
            }
            if (a15) {
                *a15 = sizeY;
            }
            return obj;
        }
    }
    return nullptr;
}

// 0x593340
CObject* Interior_c::PlaceFurnitureInCorner(int32 furnitureGroupId, int32 furnitureSubgroupId, int32 id, float a4, int32 a5, int32 a6, int32 a2, int32* a9, int32* a10, int32* a11, int32* a12, int32* a13) {
    // a4 = Z offset, a5 = height info, a6 = corner (-1 for random), a2 = distance from the corner
    // a9 = out corner, a10 = out tile X, a11 = out tile Y, a12 = out placed width, a13 = out placed depth
    auto* const furniture = g_furnitureMan.GetFurniture(furnitureGroupId, furnitureSubgroupId, (int16)id, m_box->m_status);

    const auto wantedCorner = a6;
    const auto numTries     = wantedCorner != -1 ? 1 : 20;
    if (!furniture) {
        return nullptr;
    }

    const auto furnWidth = (int32)(uint8)furniture->m_nWidthX;
    const auto furnDepth = (int32)(uint8)furniture->m_nWidthY;

    // NB: Left as is by the original code if the corner isn't in range [0, 3]
    int32 x = a6, y = a6;
    for (auto i = 0; i < numTries; i++) {
        if (wantedCorner == -1) {
            a6 = RandomNumberInRange(4.0f);
        }
        switch (a6) {
        case 1:
            x = a2;
            y = 0;
            break;
        case 3:
            x = (int32)m_box->m_width - furnDepth - a2;
            y = (int32)m_box->m_depth - furnWidth;
            break;
        case 2:
            y = a2;
            x = (int32)m_box->m_width - furnWidth;
            break;
        case 0:
            x = 0;
            y = (int32)m_box->m_depth - furnDepth - a2;
            break;
        }

        int32 sizeX, sizeY;
        if (auto* const obj = PlaceFurniture(furniture, x, y, a4, a5, a6, &sizeX, &sizeY, false)) {
            if (a9) {
                *a9 = a6;
            }
            if (a10) {
                *a10 = x;
            }
            if (a11) {
                *a11 = y;
            }
            if (a12) {
                *a12 = sizeX;
            }
            if (a13) {
                *a13 = sizeY;
            }
            return obj;
        }
    }
    return nullptr;
}

// 0x591C50
bool Interior_c::FindEmptyTiles(int32 a3, int32 a4, int32* arg8, int32* a5) {
    for (auto i = 0; i < 100; i++) {
        const auto x = RandomNumberInRange((float)m_box->m_width);
        const auto y = RandomNumberInRange((float)m_box->m_depth);
        if (CheckTilesEmpty(x, y, a3, a4, true)) {
            *arg8 = x;
            *a5   = y;
            return true;
        }
    }
    return false;
}
void Interior_c::FurnishShop(int32 a2) {
    m_furnitureGroupId = static_cast<int8>(a2);

    const auto door    = (int32)m_box->m_door;
    if (door - 1 > 5 || (int32)m_box->m_width - door > 5) {
        SetTilesStatus(0, 0, 1, 1, 2, 0);
        SetTilesStatus(0, m_box->m_depth - 1, 1, 1, 2, 0);
        SetTilesStatus(m_box->m_width - 1, 0, 1, 1, 2, 0);
        SetTilesStatus(m_box->m_width - 1, m_box->m_depth - 1, 1, 1, 2, 0);

        Shop_PlaceFixedUnits();
        Shop_FurnishEdges();
        Shop_FurnishAisles();
    }
}

// 0x59A1B0
void Interior_c::Shop_FurnishEdges() {
    const auto maxX = (int32)m_box->m_width - 1;
    const auto maxY = (int32)m_box->m_depth - 1;

    // Type of units to use (See `Shop_FurnishAisles`)
    const auto GetRandomUnitType = [] {
        const auto r = RandomNumberInRange(100.0f);
        if (r > 50) {
            return 0;
        }
        if (r > 25) {
            return 3;
        }
        return r > 10 ? 6 : 9;
    };

    // 0x59A1C8 - Top wall
    {
        const auto type = GetRandomUnitType();
        for (auto x = 1; x < maxX;) {
            x += Shop_PlaceEdgeUnits(type, x, maxY, 0);
        }
    }

    // 0x59A22F - Left wall
    {
        const auto type = GetRandomUnitType();
        for (auto y = 1; y <= maxY - 1;) {
            y += Shop_PlaceEdgeUnits(type, 0, y, 1);
        }
    }

    // 0x59A2A5 - Right wall
    {
        const auto type = GetRandomUnitType();
        for (auto y = 1; y <= maxY - 1;) {
            y += Shop_PlaceEdgeUnits(type, maxX, y, 3);
        }
    }

    // Inlined `Shop_AddShelfInfo` (0x59A140)
    static auto& s_ShelfInfoCounter = StaticRef<int32>(0x8D0948);
    const auto   AddShelfInfo       = [this](float x, float y, int32 dir) {
        if (s_ShelfInfoCounter > 1 && RandomNumberInRange(100.0f) > 60) {
            AddInteriorInfo(8, x, y, dir, nullptr);
            s_ShelfInfoCounter = 0;
        }
        s_ShelfInfoCounter++;
    };

    // Is `pos` outside of the door spanning `[start, end]` (Or there's no door at all)
    const auto IsOutsideDoor = [](int32 pos, int8 start, int8 end) {
        return start == -1 || pos < (int32)start || pos > (int32)end;
    };

    // 0x59A31C - Shelves on the top wall
    for (auto x = 1; x <= maxX - 1; x++) {
        if (IsOutsideDoor(x, m_box->m_tDoorStart, m_box->m_tDoorEnd)) {
            AddShelfInfo((float)x, (float)(maxY - 1), 2);
        }
        if (GetTileStatus(x, 1) == 0) {
            SetTilesStatus(x, 1, 1, 1, 2, 0);
        }
    }

    // 0x59A3DC - Shelves on the left and right walls
    for (auto y = 2; y <= maxY - 2; y++) {
        if (IsOutsideDoor(y, m_box->m_lDoorStart, m_box->m_lDoorEnd)) {
            AddShelfInfo(1.0f, (float)y, 3);
        }
        if (IsOutsideDoor(y, m_box->m_rDoorStart, m_box->m_rDoorEnd)) {
            AddShelfInfo((float)(maxX - 1), (float)y, 1);
        }
    }

    // 0x59A4F4 - Walkway in front of the units
    for (auto x = 2; x <= maxX - 2; x++) {
        if (GetTileStatus(x, 2) == 0) {
            SetTilesStatus(x, 2, 1, 1, 2, 0);
        }
        SetTilesStatus(x, maxY - 2, 1, 1, 3, 0);
    }
    for (auto y = 3; y <= maxY - 3; y++) {
        SetTilesStatus(2, y, 1, 1, 3, 0);
        SetTilesStatus(maxX - 2, y, 1, 1, 3, 0);
    }
    SetTilesStatus(3, 3, maxX - 5, 1, 3, 0);
}

// 0x59A130 - empty in the original binary (RET only)
void Interior_c::Shop_FurnishCeiling() {
}

// 0x59A140
void Interior_c::Shop_AddShelfInfo(int32 a2, int32 a3, int32 a5) {
    static auto& ctr = StaticRef<int32>(0x8D0948);
    if (ctr > 1) {
        // rand() * (1.0f / 32767.0f) * 100.0f, truncated to int: chance roll must exceed 60
        const auto roll = (int32)((float)rand() * (1.0f / 32767.0f) * 100.0f);
        if (roll > 60) {
            AddInteriorInfo(8, (float)a2, (float)a3, a5, nullptr);
            ctr = 1;
            return;
        }
    }
    ctr++;
}
