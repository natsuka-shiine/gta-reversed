#include "StdInc.h"

#include "Garage.h"

void CGarage::InjectHooks() {
    RH_ScopedClass(CGarage);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(BuildRotatedDoorMatrix, 0x4479F0);
    RH_ScopedInstall(TidyUpGarageClose, 0x449D10);
    RH_ScopedInstall(TidyUpGarage, 0x449C50);
    RH_ScopedInstall(StoreAndRemoveCarsForThisHideOut, 0x449900);
    RH_ScopedInstall(EntityHasASphereWayOutsideGarage, 0x449050);
    RH_ScopedInstall(RemoveCarsBlockingDoorNotInside, 0x449690);
    RH_ScopedInstall(IsEntityTouching3D, 0x448EE0);
    RH_ScopedInstall(IsEntityEntirelyOutside, 0x448D30);
    RH_ScopedInstall(IsStaticPlayerCarEntirelyInside, 0x44A830);
    RH_ScopedInstall(IsEntityEntirelyInside3D, 0x448BE0);
    RH_ScopedOverloadedInstall(IsPointInsideGarage, "", 0x448740, bool(CGarage::*)(CVector));
    RH_ScopedInstall(PlayerArrestedOrDied, 0x4486C0);
    RH_ScopedInstall(OpenThisGarage, 0x447D50);
    RH_ScopedInstall(CloseThisGarage, 0x447D70);
    RH_ScopedInstall(InitDoorsAtStart, 0x447600);
    RH_ScopedOverloadedInstall(IsPointInsideGarage, "radius", 0x4487D0, bool(CGarage::*)(CVector, float));
    RH_ScopedInstall(IsGarageEmpty, 0x44A9C0);
    RH_ScopedInstall(Update, 0x44AA50);
    RH_ScopedInstall(CalcDistToGarageRectangleSquared, 0x447D80);
    RH_ScopedInstall(RightModTypeForThisGarage, 0x447720);
    RH_ScopedInstall(IsPlayerOutsideGarage, 0x448E50);
    RH_ScopedInstall(CountCarsWithCenterPointWithinGarage, 0x4495F0);
    RH_ScopedInstall(IsAnyCarBlockingDoor, 0x4494F0);
    RH_ScopedInstall(IsAnyOtherCarTouchingGarage, 0x449100);
    RH_ScopedInstall(ThrowCarsNearDoorOutOfGarage, 0x449220);
    RH_ScopedInstall(FindDoorsWithGarage, 0x449FF0);
    RH_ScopedInstall(SlideDoorOpen, 0x44A660);
    RH_ScopedInstall(SlideDoorClosed, 0x44A750);
    RH_ScopedInstall(RestoreCarsForThisHideOut, 0x448550);
    RH_ScopedInstall(RestoreCarsForThisImpoundingGarage, 0x4485C0);
    RH_ScopedInstall(NeatlyLineUpStoredCars, 0x448330);
    RH_ScopedInstall(StoreAndRemoveCarsForThisImpoundingGarage, 0x449A50);
}

// 0x4479F0
void CGarage::BuildRotatedDoorMatrix(CEntity* entity, float fDoorPosition) {
    const auto fAngle = fDoorPosition * -HALF_PI;
    const auto fSin = sin(fAngle);
    const auto fCos = cos(fAngle);
    CMatrix& matrix = entity->GetMatrix();

    const auto& vecForward = matrix.GetForward();
    matrix.GetUp() = CVector(-fSin * vecForward.y, fSin * vecForward.x, fCos);
    matrix.GetRight() = CrossProduct(vecForward, matrix.GetUp());
}

// 0x449D10
void CGarage::TidyUpGarageClose() {
    auto* pool = GetVehiclePool();
    // NOTE: The original loop stops at (and skips) slot 0
    for (auto i = pool->GetSize() - 1; i != 0; i--) {
        auto* vehicle = pool->GetAt(i);
        if (!vehicle || (!vehicle->IsAutomobile() && !vehicle->IsBike()))
            continue;
        if (vehicle->GetStatus() != STATUS_WRECKED || !IsEntityTouching3D(vehicle))
            continue;

        // When the door is closed, remove any wrecked car touching the garage; otherwise only
        // remove it once part of it sticks out, so it isn't crushed by the closing door
        if (m_nDoorState == GARAGE_DOOR_CLOSED || EntityHasASphereWayOutsideGarage(vehicle, 0.0f)) {
            CWorld::Remove(vehicle);
            delete vehicle;
        }
    }
}

// 0x449C50
void CGarage::TidyUpGarage() {
    auto* pool = GetVehiclePool();
    // NOTE: The original loop stops at (and skips) slot 0
    for (auto i = pool->GetSize() - 1; i != 0; i--) {
        auto* vehicle = pool->GetAt(i);
        if (!vehicle || (!vehicle->IsAutomobile() && !vehicle->IsBike()))
            continue;

        if (IsPointInsideGarage(vehicle->GetPosition()) &&
            (vehicle->GetStatus() == STATUS_WRECKED || vehicle->GetUp().z < 0.5f)) {
            CWorld::Remove(vehicle);
            delete vehicle;
        }
    }
}

// 0x449900
void CGarage::StoreAndRemoveCarsForThisHideOut(CStoredCar* storedCars, int32 maxSlot) {
    maxSlot = std::min<int32>(maxSlot, NUM_GARAGE_STORED_CARS);

    for (auto i = 0; i < NUM_GARAGE_STORED_CARS; i++)
        storedCars[i].Clear();

    auto pool = GetVehiclePool();
    auto storedCarIdx{0u};
    for (auto i = pool->GetSize(); i; i--) {
        if (auto vehicle = pool->GetAt(i - 1)) {
            if (IsPointInsideGarage(vehicle->GetPosition()) && vehicle->GetCreatedBy() != MISSION_VEHICLE) {
                if (storedCarIdx < static_cast<uint32>(maxSlot) && !EntityHasASphereWayOutsideGarage(vehicle, 1.0f)) {
                    storedCars[storedCarIdx++].StoreCar(vehicle);
                }

                FindPlayerInfo().CancelPlayerEnteringCars(vehicle);
                CWorld::Remove(vehicle);
                delete vehicle;
            }
        }
    }

    // Clear slots with no vehicles in it
    for (auto i = storedCarIdx; i < NUM_GARAGE_STORED_CARS; i++)
        storedCars[i].Clear();
}

// 0x449050
bool CGarage::EntityHasASphereWayOutsideGarage(CEntity* entity, float fRadius) {
    const auto* colModel = entity->GetColModel();
    const auto& matrix = entity->GetMatrix();
    for (const auto& sphere : colModel->GetData()->GetSpheres()) {
        if (!IsPointInsideGarage(matrix.TransformPoint(sphere.m_vecCenter), sphere.m_fRadius + fRadius))
            return true;
    }
    return false;
}

// 0x449690
void CGarage::RemoveCarsBlockingDoorNotInside() {
    auto* pool = GetVehiclePool();
    for (auto i = pool->GetSize(); i != 0; i--) {
        auto* vehicle = pool->GetAt(i - 1);
        if (!vehicle || !IsEntityTouching3D(vehicle))
            continue;
        if (IsPointInsideGarage(vehicle->GetPosition()))
            continue;

        if (!vehicle->bIsLocked && vehicle->CanBeDeleted()) {
            CWorld::Remove(vehicle);
            delete vehicle;
            return; // NOTE: Only ever removes a single car
        }
    }
}

// 0x448EE0
bool CGarage::IsEntityTouching3D(CEntity* entity) {
    const auto* colModel = entity->GetColModel();
    const auto  boundRadius = colModel->GetBoundRadius();
    const auto& pos = entity->GetPosition();

    // Broad phase: entity bounding sphere vs the garage's axis-aligned rectangle
    if (pos.x < m_fLeftCoord - boundRadius || m_fRightCoord + boundRadius < pos.x ||
        pos.y < m_fFrontCoord - boundRadius || m_fBackCoord + boundRadius < pos.y ||
        pos.z < m_vPosn.z - boundRadius || m_fTopZ + boundRadius < pos.z)
        return false;

    // Narrow phase: any collision sphere inside the garage volume means touching
    const auto& matrix = entity->GetMatrix();
    for (const auto& sphere : colModel->GetData()->GetSpheres()) {
        if (IsPointInsideGarage(matrix.TransformPoint(sphere.m_vecCenter), sphere.m_fRadius))
            return true;
    }
    return false;
}

// 0x448D30
bool CGarage::IsEntityEntirelyOutside(CEntity* entity, float radius) {
    const auto& pos = entity->GetPosition();

    // Broad phase: if the origin lies within the garage rectangle (XY), it's not outside
    if (m_fLeftCoord - radius < pos.x && pos.x < m_fRightCoord + radius &&
        m_fFrontCoord - radius < pos.y && pos.y < m_fBackCoord + radius)
        return false;

    // Narrow phase: any collision sphere overlapping the garage volume means not outside
    const auto* colModel = entity->GetColModel();
    const auto& matrix = entity->GetMatrix();
    for (const auto& sphere : colModel->GetData()->GetSpheres()) {
        if (IsPointInsideGarage(matrix.TransformPoint(sphere.m_vecCenter), sphere.m_fRadius + radius))
            return false;
    }
    return true;
}

// 0x44A830
bool CGarage::IsStaticPlayerCarEntirelyInside() {
    CVehicle* vehicle = FindPlayerVehicle();
    if (!vehicle)
        return false;

    if (vehicle->GetStatus() != STATUS_PLAYER && vehicle->GetStatus() != STATUS_FORCED_STOP)
        return false;

    // The player must not be in the middle of leaving the car
    if (FindPlayerPed()->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR))
        return false;

    const auto& pos = vehicle->GetPosition();
    if (pos.x < m_fLeftCoord || m_fRightCoord < pos.x || pos.y < m_fFrontCoord || m_fBackCoord < pos.y)
        return false;

    // The car must be (almost) stationary
    const auto& vel = vehicle->m_vecMoveSpeed;
    if (std::fabs(vel.x) > 0.01f || std::fabs(vel.y) > 0.01f || std::fabs(vel.z) > 0.01f || vel.SquaredMagnitude() > 0.0001f)
        return false;

    return IsEntityEntirelyInside3D(vehicle, 0.0f);
}

// 0x448BE0
bool CGarage::IsEntityEntirelyInside3D(CEntity* entity, float radius) {
    const auto& pos = entity->GetPosition();

    // Broad phase: entity origin must lie within the garage's axis-aligned rectangle inflated by `radius`
    if (pos.x < m_fLeftCoord - radius || m_fRightCoord + radius < pos.x ||
        pos.y < m_fFrontCoord - radius || m_fBackCoord + radius < pos.y ||
        pos.z < m_vPosn.z - radius || m_fTopZ + radius < pos.z)
        return false;

    // Narrow phase: every collision sphere must be inside the garage volume
    const auto* colModel = entity->GetColModel();
    const auto& matrix = entity->GetMatrix();
    for (const auto& sphere : colModel->GetData()->GetSpheres()) {
        if (!IsPointInsideGarage(matrix.TransformPoint(sphere.m_vecCenter), radius - sphere.m_fRadius))
            return false;
    }
    return true;
}

// 0x448740
bool CGarage::IsPointInsideGarage(CVector point) {
    if (point.z < m_vPosn.z || m_fTopZ < point.z)
        return false;

    const auto offset = CVector2D{ point } - CVector2D{ m_vPosn };

    const auto distA = offset.Dot(m_vDirectionA);
    if (distA < 0.0f || m_fWidth < distA)
        return false;

    const auto distB = offset.Dot(m_vDirectionB);
    return distB >= 0.0f && distB <= m_fHeight;
}

// 0x4486C0
eGarageDoorState CGarage::PlayerArrestedOrDied() {
    switch (m_nType) {
    case BOMBSHOP_TIMED:
    case BOMBSHOP_ENGINE:
    case BOMBSHOP_REMOTE:
    case PAYNSPRAY:
    case (eGarageType)13: // Reopen these garages
        if (m_nDoorState == GARAGE_DOOR_CLOSED || m_nDoorState == GARAGE_DOOR_CLOSING || m_nDoorState == GARAGE_DOOR_OPENING)
            m_nDoorState = GARAGE_DOOR_OPENING;
        break;
    case INVALID:
        break;
    default: // Close every other (valid) garage
        if (m_nType <= HANGAR_ABANDONED_AIRPORT && m_nDoorState >= GARAGE_DOOR_OPEN && m_nDoorState <= GARAGE_DOOR_OPENING)
            m_nDoorState = GARAGE_DOOR_CLOSING;
        break;
    }
    return m_nDoorState;
}

// 0x447D50
void CGarage::OpenThisGarage() {
  if ( m_nDoorState == GARAGE_DOOR_CLOSED
    || m_nDoorState == GARAGE_DOOR_CLOSING
    || m_nDoorState == GARAGE_DOOR_CLOSED_DROPPED_CAR)
  {
    m_nDoorState = GARAGE_DOOR_OPENING;
  }
}

// 0x447D70
void CGarage::CloseThisGarage() {
    if (m_nDoorState == GARAGE_DOOR_OPEN || m_nDoorState == GARAGE_DOOR_OPENING)
        m_nDoorState = GARAGE_DOOR_CLOSING;
}

// 0x447600
void CGarage::InitDoorsAtStart() {
    m_bInactive           = false;
    m_bUsedRespray        = false;
    m_bDoorClosed         = true;
    m_bRespraysAlwaysFree = false;

    m_nDoorState = GARAGE_DOOR_CLOSED;
    m_nTimeToOpen = 0;

    switch (m_nType) {
    case BOMBSHOP_TIMED:
    case BOMBSHOP_ENGINE:
    case BOMBSHOP_REMOTE:
    case PAYNSPRAY:
        m_nDoorState = GARAGE_DOOR_OPEN;
        m_fDoorPosition = 1.0f;
        break;
    case INVALID:
    case (eGarageType)13:
        break;
    default:
        if (m_nType <= HANGAR_ABANDONED_AIRPORT) {
            m_nDoorState = GARAGE_DOOR_CLOSED;
            m_fDoorPosition = 0.0f;
        }
        break;
    }
}

// 0x4487D0
bool CGarage::IsPointInsideGarage(CVector point, float radius) {
    if (point.z < m_vPosn.z - radius || m_fTopZ + radius < point.z)
        return false;

    const auto offset = CVector2D{ point } - CVector2D{ m_vPosn };

    const auto distA = offset.Dot(m_vDirectionA);
    if (distA < -radius || m_fWidth + radius < distA)
        return false;

    const auto distB = offset.Dot(m_vDirectionB);
    return distB >= -radius && distB <= m_fHeight + radius;
}

// 0x44AA50
void CGarage::Update(int32 garageId) {
    // Work out whether the camera has to stay outside of this garage
    if (m_nType != (eGarageType)13 && m_nDoorState <= GARAGE_DOOR_CLOSED_DROPPED_CAR && FindPlayerPed() && !m_bCameraFollowsPlayer) {
        auto* playerVeh = FindPlayerVehicle();
        auto* player    = FindPlayerPed();

        CEntity* playerEntity = player;
        if (player->IsInVehicle() && player->m_pVehicle->m_nModelIndex == MODEL_KART)
            playerEntity = player->m_pVehicle;

        if (IsEntityEntirelyInside3D(playerEntity, 0.25f)) {
            CGarages::bCamShouldBeOutside = true;
            TheCamera.m_pToGarageWeAreIn  = this;
        }

        if (playerVeh) {
            if (!IsEntityEntirelyOutside(playerVeh, 0.0f))
                TheCamera.m_pToGarageWeAreInForHackAvoidFirstPerson = this;

            if (playerVeh->m_nModelIndex == MODEL_MRWHOOP) {
                const auto& pos = playerVeh->GetPosition();
                if (pos.x > m_fLeftCoord - 0.5f && pos.x < m_fRightCoord + 0.5f && pos.y > m_fFrontCoord - 0.5f && pos.y < m_fBackCoord + 0.5f) {
                    CGarages::bCamShouldBeOutside = true;
                    TheCamera.m_pToGarageWeAreIn  = this;
                }
            }
        }
    }

    if (m_bInactive && m_nDoorState == GARAGE_DOOR_CLOSED)
        return;

    if (m_bDoorOpensUp)
        m_bDoorClosed = !(m_nDoorState == GARAGE_DOOR_OPEN || (m_nDoorState == GARAGE_DOOR_OPENING && m_fDoorPosition > 0.4f));

    // Squared XY distance from the player to the center of the garage
    const auto PlayerDistSqToCenter = [this] {
        const auto playerPos = FindPlayerCoors();
        const auto dx = playerPos.x - (m_fLeftCoord + m_fRightCoord) * 0.5f;
        const auto dy = playerPos.y - (m_fFrontCoord + m_fBackCoord) * 0.5f;
        return dx * dx + dy * dy;
    };
    // The player's vehicle, or the player themselves when on foot
    const auto GetPlayerEntity = []() -> CEntity* {
        if (auto* playerVeh = FindPlayerVehicle())
            return playerVeh;
        return FindPlayerPed();
    };
    const auto SetPlayerAwaitsInGarage = [](bool state) {
        CPad::GetPad(0)->bPlayerAwaitsInGarage        = state;
        FindPlayerWanted()->m_bPoliceBackOffGarage    = state;
    };
    const auto GetStoredCars = [this] {
        return CGarages::GetStoredCarsInSafehouse(CGarages::FindSafeHouseIndexForGarageType(m_nType));
    };
    const auto CallOffChase = [this] {
        CWorld::CallOffChaseForArea(m_fLeftCoord - 10.0f, m_fFrontCoord - 10.0f, m_fRightCoord + 10.0f, m_fBackCoord + 10.0f);
    };

    switch (m_nType) {
    case ONLY_TARGET_VEH: {
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED: {
            if (m_pTargetCar && FindPlayerVehicle() == m_pTargetCar) {
                const auto& pos = m_pTargetCar->GetPosition();
                if (CalcDistToGarageRectangleSquared(pos.x, pos.y) < sq(8.0f))
                    m_nDoorState = GARAGE_DOOR_OPENING;
            }
            break;
        }
        case GARAGE_DOOR_OPEN: {
            if (PlayerDistSqToCenter() <= sq(30.0f)) {
                if (m_pTargetCar && FindPlayerVehicle() != m_pTargetCar && IsEntityEntirelyInside3D(m_pTargetCar, 0.0f) && IsEntityEntirelyOutside(GetPlayerEntity(), 2.0f)) {
                    SetPlayerAwaitsInGarage(true);
                    m_b0x1       = false;
                    m_nDoorState = GARAGE_DOOR_CLOSING;
                }
            } else if ((CTimer::GetFrameCounter() & 0x1F) == 0) {
                if (!m_pTargetCar || !IsEntityTouching3D(m_pTargetCar)) {
                    m_b0x1       = true;
                    m_nDoorState = GARAGE_DOOR_CLOSING;
                }
            }
            break;
        }
        case GARAGE_DOOR_CLOSING: {
            if (m_pTargetCar)
                ThrowCarsNearDoorOutOfGarage(m_pTargetCar);

            if (SlideDoorClosed()) {
                if (m_b0x1) {
                    m_nDoorState = GARAGE_DOOR_CLOSED;
                    break;
                }
                if (m_pTargetCar) {
                    m_nDoorState = GARAGE_DOOR_CLOSED_DROPPED_CAR;
                    m_pTargetCar->DestroyVehicleAndDriverAndPassengers(m_pTargetCar);
                    m_pTargetCar = nullptr;
                } else {
                    m_nDoorState = GARAGE_DOOR_CLOSED;
                }
                SetPlayerAwaitsInGarage(false);
            }
            break;
        }
        case GARAGE_DOOR_OPENING: {
            if (SlideDoorOpen())
                m_nDoorState = GARAGE_DOOR_OPEN;
            break;
        }
        }
        break;
    }
    case BOMBSHOP_TIMED:
    case BOMBSHOP_ENGINE:
    case BOMBSHOP_REMOTE: {
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED: {
            if (CTimer::GetTimeInMS() <= m_nTimeToOpen)
                break;

            if (m_nType == BOMBSHOP_REMOTE && !CStreaming::IsModelLoaded(MODEL_BOMB)) {
                CStreaming::RequestModel(MODEL_BOMB, STREAMING_GAME_REQUIRED);
                break;
            }

            switch (m_nType) {
            case BOMBSHOP_TIMED:  AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_FIT_BOMB_TIMED);             break;
            case BOMBSHOP_ENGINE: AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_FIT_BOMB_BOOBY_TRAPPED);     break;
            case BOMBSHOP_REMOTE: AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_FIT_BOMB_REMOTE_CONTROLLED); break;
            }
            m_nDoorState = GARAGE_DOOR_OPENING;

            if (!CGarages::BombsAreFree) {
                auto& money = FindPlayerInfo().m_nMoney;
                if (money > 0)
                    money = std::max(0, money - 500);
            }

            if (auto* playerVeh = FindPlayerVehicle(); playerVeh && (playerVeh->IsAutomobile() || playerVeh->IsBike())) {
                playerVeh->m_nBombOnBoard          = (uint8)(m_nType - 1); // TIMED -> 1, ENGINE -> 2, REMOTE -> 3
                playerVeh->m_pWhoInstalledBombOnMe = FindPlayerPed();
                if (m_nType == BOMBSHOP_REMOTE)
                    CGarages::GivePlayerDetonator();
                CStats::IncrementStat(STAT_KGS_OF_EXPLOSIVES_USED, 10.0f);
            }

            // Explain the bomb to the player
            const auto padMode = CPad::GetPad(0)->Mode;
            const char* helpMsg = nullptr;
            switch (m_nType) {
            case BOMBSHOP_TIMED:
                if (padMode >= 0 && padMode <= 2) helpMsg = "GA_6";
                else if (padMode == 3)            helpMsg = "GA_6B";
                break;
            case BOMBSHOP_ENGINE:
                if (padMode >= 0 && padMode <= 2) helpMsg = "GA_7";
                else if (padMode == 3)            helpMsg = "GA_7B";
                break;
            case BOMBSHOP_REMOTE:
                helpMsg = "GA_8";
                break;
            }
            if (helpMsg)
                CHud::SetHelpMessage(TheText.Get(helpMsg), false, false, true);
            break;
        }
        case GARAGE_DOOR_OPEN: {
            auto* playerVeh = FindPlayerVehicle();
            if (!IsStaticPlayerCarEntirelyInside() || !playerVeh || playerVeh->IsSubBike() || playerVeh->IsSubBMX())
                break;

            if (playerVeh->m_nBombOnBoard) {
                CGarages::TriggerMessage("GA_5", -1, 4000, -1);
                m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
                AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_ALREADY_RIGGED);
            } else if (!CGarages::BombsAreFree && FindPlayerInfo().m_nMoney < 500) {
                CGarages::TriggerMessage("GA_4", -1, 4000, -1);
                m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
                AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_NO_CASH);
            } else {
                m_nDoorState = GARAGE_DOOR_CLOSING;
                SetPlayerAwaitsInGarage(true);
            }
            break;
        }
        case GARAGE_DOOR_CLOSING: {
            if (auto* playerVeh = FindPlayerVehicle())
                ThrowCarsNearDoorOutOfGarage(playerVeh);

            if (SlideDoorClosed()) {
                m_nDoorState  = GARAGE_DOOR_CLOSED;
                m_nTimeToOpen = CTimer::GetTimeInMS() + 2000;
            }
            if (m_nType == BOMBSHOP_REMOTE)
                CStreaming::RequestModel(MODEL_BOMB, STREAMING_GAME_REQUIRED);
            break;
        }
        case GARAGE_DOOR_OPENING: {
            if (SlideDoorOpen())
                m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
            if (m_fDoorPosition > 0.5f)
                SetPlayerAwaitsInGarage(false);
            break;
        }
        case GARAGE_DOOR_WAITING_PLAYER_TO_EXIT: {
            if (IsPlayerOutsideGarage(0.0f))
                m_nDoorState = GARAGE_DOOR_OPEN;
            break;
        }
        }
        break;
    }
    case PAYNSPRAY: {
        if (FindPlayerCoors().z >= 950.0f)
            break;

        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED: {
            if (CGarages::NoResprays)
                break;

            if (CTimer::GetTimeInMS() > m_nTimeToOpen) {
                m_nDoorState = GARAGE_DOOR_OPENING;

                auto* wanted = FindPlayerWanted();
                const bool bWasWanted = wanted->GetWantedLevel() != WANTED_CLEAN;
                bool bChargePlayer = bWasWanted;
                if (bWasWanted)
                    wanted->ClearWantedLevelAndGoOnParole();

                // NOTE: When the player has no (car/bike) vehicle this is left holding the wanted state (as in the original)
                bool bColourChanged = bWasWanted;

                auto* playerVeh = FindPlayerVehicle();
                if (playerVeh && (playerVeh->IsAutomobile() || playerVeh->IsBike())) {
                    if (playerVeh->m_fHealth < 970.0f)
                        bChargePlayer = true;
                    playerVeh->m_fHealth = std::max(1000.0f, playerVeh->m_fHealth);

                    if (playerVeh->IsAutomobile())
                        playerVeh->AsAutomobile()->m_fBurnTimer = 0.0f;
                    else
                        playerVeh->AsBike()->m_BlowUpTimer = 0.0f;

                    playerVeh->Fix();
                    CStats::IncrementStat(STAT_VEHICLE_RESPRAYS, 1.0f);

                    // Flip the vehicle back onto its wheels
                    if (playerVeh->GetUp().z < 0.0f) {
                        playerVeh->GetUp()    = -playerVeh->GetUp();
                        playerVeh->GetRight() = -playerVeh->GetRight();
                    }

                    bColourChanged = false;
                    // NOTE: Accessed as an automobile even for bikes (as in the original)
                    if (!playerVeh->AsAutomobile()->autoFlags.bShouldNotChangeColour && playerVeh->GetRemapIndex() < 0) {
                        uint8 prim, sec, tert, quat;
                        playerVeh->GetVehicleModelInfo()->ChooseVehicleColour(prim, sec, tert, quat, 1);
                        if (playerVeh->m_nPrimaryColor != prim || playerVeh->m_nSecondaryColor != sec || playerVeh->m_nTertiaryColor != tert || playerVeh->m_nQuaternaryColor != quat)
                            bColourChanged = true;

                        playerVeh->m_nPrimaryColor    = prim;
                        playerVeh->m_nSecondaryColor  = sec;
                        playerVeh->m_nTertiaryColor   = tert;
                        playerVeh->m_nQuaternaryColor = quat;
                        playerVeh->SetRemap(-1);

                        if (bColourChanged) { // Puff some smoke of the new colour
                            FxPrtMult_c fxMult{ 1.0f, 0.0f, 0.0f, 0.6f, 0.7f, 1.0f, 0.4f };
                            const auto& colour   = CVehicleModelInfo::ms_vehicleColourTable[playerVeh->m_nPrimaryColor];
                            fxMult.m_Color.red   = (float)colour.r / 255.0f;
                            fxMult.m_Color.green = (float)colour.g / 255.0f;
                            fxMult.m_Color.blue  = (float)colour.b / 255.0f;

                            for (auto i = 0; i < 10; i++) {
                                CVector pos = playerVeh->GetPosition();
                                pos.x += CGeneral::GetRandomNumberInRange(-3.0f, 3.0f);
                                pos.y += CGeneral::GetRandomNumberInRange(-3.0f, 3.0f);
                                const CVector vel{ 0.0f, 0.0f, CGeneral::GetRandomNumberInRange(0.0f, 0.05f) };
                                g_fx.m_SmokeHuge->AddParticle(pos, vel, 0.0f, fxMult, -1.0f, 1.2f, 0.6f, false);
                            }
                        }
                    }

                    playerVeh->m_fDirtLevel      = 0.0f;
                    playerVeh->bDisableParticles = false;
                }

                if (m_bRespraysAlwaysFree) {
                    CGarages::TriggerMessage("GA_22", -1, 4000, -1);
                } else if (bChargePlayer && !CGarages::RespraysAreFree) {
                    auto& money = FindPlayerInfo().m_nMoney;
                    if (money > 0)
                        money = std::max(0, money - 100);
                    CStats::IncrementStat(STAT_AUTO_REPAIR_AND_PAINTING_BUDGET, 100.0f);
                    CGarages::TriggerMessage(bWasWanted ? "GA_2" : "GA_XX", -1, 4000, -1);
                } else if (bColourChanged) {
                    CGarages::TriggerMessage((CGeneral::GetRandomNumber() & 1) ? "GA_15" : "GA_16", -1, 4000, -1);
                }

                m_bUsedRespray = true;
                if (playerVeh)
                    playerVeh->bHasBeenResprayed = true;
            }
            CallOffChase();
            break;
        }
        case GARAGE_DOOR_OPEN: {
            if (CGarages::NoResprays)
                break;

            auto* playerVeh = FindPlayerVehicle();
            if (IsStaticPlayerCarEntirelyInside()) {
                if (!CGarages::IsCarSprayable(playerVeh)) {
                    CGarages::TriggerMessage(playerVeh->IsSubBMX() ? "GA_1B" : "GA_1", -1, 4000, -1);
                    m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
                    AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_IS_HOT);
                } else if (FindPlayerInfo().m_nMoney >= 100 || CGarages::RespraysAreFree) {
                    m_nDoorState = GARAGE_DOOR_CLOSING;
                    CPad::GetPad(0)->bPlayerAwaitsInGarage = true;
                    playerVeh->m_fDirtLevel = 0.0f;
                } else {
                    CGarages::TriggerMessage("GA_3", -1, 4000, -1);
                    m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
                    AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_NO_CASH);
                }
                FindPlayerWanted()->m_bPoliceBackOffGarage = true;
                CGarages::LastGaragePlayerWasIn = garageId;
            } else if (!IsPlayerOutsideGarage(0.0f)) {
                FindPlayerWanted()->m_bPoliceBackOffGarage = true;
                CGarages::LastGaragePlayerWasIn = garageId;
            } else if (garageId == CGarages::LastGaragePlayerWasIn) {
                FindPlayerWanted()->m_bPoliceBackOffGarage = false;
            }

            if (playerVeh) {
                const auto& pos = playerVeh->GetPosition();
                if (CalcDistToGarageRectangleSquared(pos.x, pos.y) < sq(8.0f))
                    CallOffChase();
            }
            break;
        }
        case GARAGE_DOOR_CLOSING: {
            auto* playerVeh = FindPlayerVehicle();
            if (playerVeh)
                ThrowCarsNearDoorOutOfGarage(playerVeh);

            if (SlideDoorClosed()) {
                m_nDoorState = GARAGE_DOOR_CLOSED;
                AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_RESPRAY);
                m_nTimeToOpen = CTimer::GetTimeInMS() + 2000;
                CStats::IncrementStat(STAT_TOTAL_LEGITIMATE_KILLS, CStats::GetStatValue(STAT_KILLS_SINCE_LAST_CHECKPOINT));
                CStats::SetStatValue(STAT_KILLS_SINCE_LAST_CHECKPOINT, 0.0f);
            }

            if (playerVeh) {
                playerVeh->AsAutomobile()->m_fBurnTimer = 0.0f; // NOTE: Accessed as an automobile regardless of the type (as in the original)
                playerVeh->bDisableParticles = true;
            }
            CallOffChase();
            break;
        }
        case GARAGE_DOOR_OPENING: {
            if (SlideDoorOpen())
                m_nDoorState = GARAGE_DOOR_WAITING_PLAYER_TO_EXIT;
            if (m_fDoorPosition > 0.5f)
                SetPlayerAwaitsInGarage(false);
            break;
        }
        case GARAGE_DOOR_WAITING_PLAYER_TO_EXIT: {
            if (IsPlayerOutsideGarage(0.0f))
                m_nDoorState = GARAGE_DOOR_OPEN;
            break;
        }
        }
        break;
    }
    case UNKN_CLOSESONTOUCH: {
        switch (m_nDoorState) {
        case GARAGE_DOOR_OPEN: {
            if (IsGarageEmpty())
                m_nDoorState = GARAGE_DOOR_CLOSING;
            break;
        }
        case GARAGE_DOOR_CLOSING: {
            if (SlideDoorClosed())
                m_nDoorState = GARAGE_DOOR_CLOSED;
            if (!IsGarageEmpty())
                m_nDoorState = GARAGE_DOOR_OPENING;
            break;
        }
        case GARAGE_DOOR_OPENING: {
            if (SlideDoorOpen())
                m_nDoorState = GARAGE_DOOR_OPEN;
            break;
        }
        }
        break;
    }
    case OPEN_FOR_TARGET_FREEZE_PLAYER:
    case CLOSE_WITH_CAR_DONT_OPEN_AGAIN: {
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED: {
            if (m_pTargetCar && FindPlayerVehicle() == m_pTargetCar) {
                const auto& pos = m_pTargetCar->GetPosition();
                if (CalcDistToGarageRectangleSquared(pos.x, pos.y) < sq(17.0f))
                    m_nDoorState = GARAGE_DOOR_OPENING;
            }
            break;
        }
        case GARAGE_DOOR_OPEN: {
            if (PlayerDistSqToCenter() > sq(30.0f) || !m_pTargetCar) {
                m_b0x1       = true;
                m_nDoorState = GARAGE_DOOR_CLOSING;
                break;
            }
            if (m_pTargetCar == FindPlayerVehicle() && IsStaticPlayerCarEntirelyInside() && !IsAnyCarBlockingDoor()) {
                SetPlayerAwaitsInGarage(true);
                m_b0x1       = false;
                m_nDoorState = GARAGE_DOOR_CLOSING;
            }
            break;
        }
        case GARAGE_DOOR_CLOSING: {
            if (m_pTargetCar)
                ThrowCarsNearDoorOutOfGarage(m_pTargetCar);

            if (SlideDoorClosed()) {
                if (m_b0x1) {
                    m_nDoorState = GARAGE_DOOR_CLOSED;
                    break;
                }
                if (m_pTargetCar) {
                    m_nDoorState  = GARAGE_DOOR_CLOSED_DROPPED_CAR;
                    m_nTimeToOpen = CTimer::GetTimeInMS() + 2000;
                    m_pTargetCar  = nullptr;
                } else {
                    m_nDoorState = GARAGE_DOOR_CLOSED;
                }
                SetPlayerAwaitsInGarage(false);
            }
            break;
        }
        case GARAGE_DOOR_OPENING: {
            if (SlideDoorOpen())
                m_nDoorState = GARAGE_DOOR_OPEN;
            break;
        }
        case GARAGE_DOOR_CLOSED_DROPPED_CAR: {
            if (m_nType == OPEN_FOR_TARGET_FREEZE_PLAYER && CTimer::GetTimeInMS() > m_nTimeToOpen)
                m_nDoorState = GARAGE_DOOR_OPENING;
            break;
        }
        }
        break;
    }
    case SCRIPT_ONLY_OPEN: {
        if (m_nDoorState == GARAGE_DOOR_OPENING && SlideDoorOpen())
            m_nDoorState = GARAGE_DOOR_OPEN;
        break;
    }
    case SAFEHOUSE_GANTON:
    case SAFEHOUSE_SANTAMARIA:
    case SAGEHOUSE_ROCKSHORE:
    case SAFEHOUSE_FORTCARSON:
    case SAFEHOUSE_VERDANTMEADOWS:
    case SAFEHOUSE_DILLIMORE:
    case SAFEHOUSE_PRICKLEPINE:
    case SAFEHOUSE_WHITEWOOD:
    case SAFEHOUSE_PALOMINOCREEK:
    case SAFEHOUSE_REDSANDSWEST:
    case SAFEHOUSE_ELCORONA:
    case SAFEHOUSE_MULHOLLAND:
    case SAFEHOUSE_CALTONHEIGHTS:
    case SAFEHOUSE_PARADISO:
    case SAFEHOUSE_DOHERTY:
    case SAFEHOUSE_HASHBURY:
    case HANGAR_AT400:
    case HANGAR_ABANDONED_AIRPORT: {
        const auto maxStoredCars = m_nType == SAFEHOUSE_GANTON ? 2 : 4; // FindMaxNumStoredCarsForGarage

        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED: {
            const auto playerPos = FindPlayerCoors();
            if (playerPos.z >= 950.0f)
                break;

            const auto distSq   = CalcDistToGarageRectangleSquared(playerPos.x, playerPos.y);
            auto*      playerVeh = FindPlayerVehicle();
            if (distSq >= sq(3.5f) && (distSq >= sq(10.0f) || !playerVeh || playerVeh->IsSubBMX()))
                break;

            if (playerVeh && m_nType != HANGAR_AT400 && CGarages::CountCarsInHideoutGarage(m_nType) >= maxStoredCars) {
                // Garage is full, let the player know if they're waiting at one of the doors
                CObject* door1, * door2;
                FindDoorsWithGarage(&door1, &door2);
                const auto IsPlayerVehAtDoor = [&](CObject* door) {
                    if (!door)
                        return false;
                    const auto dx = door->GetPosition().x - playerVeh->GetPosition().x;
                    const auto dy = door->GetPosition().y - playerVeh->GetPosition().y;
                    return dx * dx + dy * dy < sq(5.0f);
                };
                if (IsPlayerVehAtDoor(door1) || IsPlayerVehAtDoor(door2)) {
                    if (CTimer::GetTimeInMS() - (uint32)CGarages::LastTimeHelpMessage > 18000u) {
                        const auto appearance = playerVeh->GetVehicleAppearance();
                        if (appearance != VEHICLE_APPEARANCE_HELI && appearance != VEHICLE_APPEARANCE_PLANE) {
                            CHud::SetHelpMessage(TheText.Get("GA_21"), false, false, true);
                            CGarages::LastTimeHelpMessage = CTimer::GetTimeInMS();
                        }
                    }
                }
                break;
            }

            if (m_nType == HANGAR_AT400) {
                m_nDoorState = GARAGE_DOOR_OPENING;
                break;
            }
            if (RestoreCarsForThisHideOut(GetStoredCars()))
                m_nDoorState = GARAGE_DOOR_OPENING;
            break;
        }
        case GARAGE_DOOR_OPEN: {
            const auto playerPos = FindPlayerCoors();
            const auto distSq    = CalcDistToGarageRectangleSquared(playerPos.x, playerPos.y);
            auto*      playerVeh = FindPlayerVehicle();

            const auto bPlayerNearby = distSq <= sq(15.0f) && (distSq <= sq(4.0f) || (playerVeh && !playerVeh->IsSubBMX()));
            if (bPlayerNearby || IsAnyCarBlockingDoor()) {
                if (playerVeh && CountCarsWithCenterPointWithinGarage(playerVeh) >= maxStoredCars && IsPlayerOutsideGarage(0.25f)) {
                    m_nDoorState = GARAGE_DOOR_CLOSING;
                } else if (distSq > sq(70.0f)) {
                    m_nDoorState = GARAGE_DOOR_CLOSING;
                    RemoveCarsBlockingDoorNotInside();
                }
            } else {
                m_nDoorState = GARAGE_DOOR_CLOSING;
            }
            break;
        }
        case GARAGE_DOOR_CLOSING: {
            SlideDoorClosed();
            if (!IsPlayerOutsideGarage(0.0f)) {
                m_nDoorState = GARAGE_DOOR_OPENING;
                break;
            }
            if (m_fDoorPosition == 0.0f) {
                m_nDoorState = GARAGE_DOOR_CLOSED;
                if (m_nType != HANGAR_AT400)
                    StoreAndRemoveCarsForThisHideOut(GetStoredCars(), 4);
            }
            break;
        }
        case GARAGE_DOOR_OPENING: {
            if (SlideDoorOpen())
                m_nDoorState = GARAGE_DOOR_OPEN;
            break;
        }
        }
        break;
    }
    case SCRIPT_CONTROLLED: {
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSING: {
            if (SlideDoorClosed())
                m_nDoorState = GARAGE_DOOR_CLOSED;
            break;
        }
        case GARAGE_DOOR_OPENING: {
            if (SlideDoorOpen())
                m_nDoorState = GARAGE_DOOR_OPEN;
            break;
        }
        }
        break;
    }
    case STAY_OPEN_WITH_CAR_INSIDE: {
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED: {
            if (m_pTargetCar && FindPlayerVehicle() == m_pTargetCar) {
                const auto& pos = m_pTargetCar->GetPosition();
                if (CalcDistToGarageRectangleSquared(pos.x, pos.y) < sq(8.0f))
                    m_nDoorState = GARAGE_DOOR_OPENING;
            }
            break;
        }
        case GARAGE_DOOR_OPEN: {
            if (PlayerDistSqToCenter() > sq(30.0f) && m_pTargetCar && IsEntityEntirelyOutside(m_pTargetCar, 0.0f)) {
                m_b0x1       = true;
                m_nDoorState = GARAGE_DOOR_CLOSING;
            }
            break;
        }
        case GARAGE_DOOR_CLOSING: {
            if (m_pTargetCar)
                ThrowCarsNearDoorOutOfGarage(m_pTargetCar);
            if (SlideDoorClosed())
                m_nDoorState = GARAGE_DOOR_CLOSED;
            break;
        }
        case GARAGE_DOOR_OPENING: {
            if (SlideDoorOpen())
                m_nDoorState = GARAGE_DOOR_OPEN;
            break;
        }
        }
        break;
    }
    case SCRIPT_OPEN_FREEZE_WHEN_CLOSING: {
        switch (m_nDoorState) {
        case GARAGE_DOOR_OPEN: {
            if (m_pTargetCar && IsEntityEntirelyInside3D(m_pTargetCar, 0.0f) && !IsAnyCarBlockingDoor() && IsPlayerOutsideGarage(0.0f)) {
                CPad::GetPad(0)->bPlayerAwaitsInGarage = true;
                m_b0x1       = false;
                m_nDoorState = GARAGE_DOOR_CLOSING;
            }
            break;
        }
        case GARAGE_DOOR_CLOSING: {
            if (SlideDoorClosed()) {
                m_nDoorState = GARAGE_DOOR_CLOSED;
                CPad::GetPad(0)->bPlayerAwaitsInGarage = false;
            }
            break;
        }
        case GARAGE_DOOR_OPENING: {
            if (SlideDoorOpen())
                m_nDoorState = GARAGE_DOOR_OPEN;
            break;
        }
        }
        break;
    }
    case IMPOUND_LS:
    case IMPOUND_SF:
    case IMPOUND_LV: {
        const auto playerPos = FindPlayerCoors();
        const auto distSq    = CalcDistToGarageRectangleSquared(playerPos.x, playerPos.y);
        const auto bPlayerAtLevel = playerPos.z < m_fTopZ - 2.0f && playerPos.z > m_vPosn.z;

        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED: {
            if (distSq < sq(60.0f) && bPlayerAtLevel) {
                NeatlyLineUpStoredCars(GetStoredCars());
                if (RestoreCarsForThisImpoundingGarage(GetStoredCars()))
                    m_nDoorState = GARAGE_DOOR_OPEN;
            }
            break;
        }
        case GARAGE_DOOR_OPEN:
        case GARAGE_DOOR_CLOSING: {
            if (distSq > sq(65.0f) || !bPlayerAtLevel || m_nDoorState == GARAGE_DOOR_CLOSING) {
                m_nDoorState = GARAGE_DOOR_CLOSED;
                StoreAndRemoveCarsForThisImpoundingGarage(GetStoredCars(), 3);
            }
            break;
        }
        }
        break;
    }
    case TUNING_LOCO_LOW_CO:
    case TUNING_WHEEL_ARCH_ANGELS:
    case TUNING_TRANSFENDER: {
        switch (m_nDoorState) {
        case GARAGE_DOOR_CLOSED: {
            if (RightModTypeForThisGarage(FindPlayerVehicle())) {
                const auto& pos = FindPlayerVehicle()->GetPosition();
                if (CalcDistToGarageRectangleSquared(pos.x, pos.y) < sq(8.0f))
                    m_nDoorState = GARAGE_DOOR_OPENING;
            }
            break;
        }
        case GARAGE_DOOR_OPEN: {
            if (PlayerDistSqToCenter() > sq(30.0f) && (CTimer::GetFrameCounter() & 0x1F) == 0) {
                auto* playerVeh = FindPlayerVehicle();
                if (!(RightModTypeForThisGarage(playerVeh) && IsEntityTouching3D(playerVeh)) && !IsAnyOtherCarTouchingGarage(nullptr)) {
                    m_b0x1       = true;
                    m_nDoorState = GARAGE_DOOR_CLOSING;
                }
            }
            break;
        }
        case GARAGE_DOOR_CLOSING: {
            if (auto* playerVeh = FindPlayerVehicle())
                ThrowCarsNearDoorOutOfGarage(playerVeh);
            if (SlideDoorClosed())
                m_nDoorState = GARAGE_DOOR_CLOSED;
            break;
        }
        case GARAGE_DOOR_OPENING: {
            if (SlideDoorOpen())
                m_nDoorState = GARAGE_DOOR_OPEN;
            break;
        }
        }
        break;
    }
    case BURGLARY: {
        switch (m_nDoorState) {
        case GARAGE_DOOR_OPEN: {
            if (PlayerDistSqToCenter() > sq(30.0f) && !IsAnyOtherCarTouchingGarage(nullptr))
                m_nDoorState = GARAGE_DOOR_CLOSING;
            break;
        }
        case GARAGE_DOOR_CLOSING: {
            if (auto* playerVeh = FindPlayerVehicle())
                ThrowCarsNearDoorOutOfGarage(playerVeh);
            if (SlideDoorClosed())
                m_nDoorState = GARAGE_DOOR_CLOSED;
            break;
        }
        case GARAGE_DOOR_OPENING: {
            if (SlideDoorOpen())
                m_nDoorState = GARAGE_DOOR_OPEN;
            break;
        }
        }
        break;
    }
    }
}

// Not yet reversed helpers used by `Update`

// 0x447720
bool CGarage::RightModTypeForThisGarage(CVehicle* vehicle) {
    if (!vehicle)
        return false;

    const auto& handling = vehicle->GetVehicleModelInfo()->GetHandlingData();
    switch (m_nType) {
    case TUNING_LOCO_LOW_CO:        return handling.m_bLowRider;
    case TUNING_WHEEL_ARCH_ANGELS:  return handling.m_bStreetRacer;
    case TUNING_TRANSFENDER:        return !handling.m_bLowRider && !handling.m_bStreetRacer;
    default:                        return false;
    }
}

// 0x447D80
float CGarage::CalcDistToGarageRectangleSquared(float x, float y) {
    float dx = 0.0f;
    if (x < m_fLeftCoord)
        dx = x - m_fLeftCoord;
    else if (x > m_fRightCoord)
        dx = x - m_fRightCoord;

    float dy = 0.0f;
    if (y < m_fFrontCoord)
        dy = y - m_fFrontCoord;
    else if (y > m_fBackCoord)
        dy = y - m_fBackCoord;

    return dx * dx + dy * dy;
}

// 0x448330
void CGarage::NeatlyLineUpStoredCars(CStoredCar* storedCars) {
    // Center of the garage's floor, raised a bit
    const CVector center = m_vPosn
        + CVector{ m_vDirectionA.x, m_vDirectionA.y, 0.0f } * (m_fWidth * 0.5f)
        + CVector{ m_vDirectionB.x, m_vDirectionB.y, 0.0f } * (m_fHeight * 0.5f)
        + CVector{ 0.0f, 0.0f, 0.5f };

    CVector lineDir{ m_vDirectionA.x, m_vDirectionA.y, 0.0f };
    lineDir.Normalise();

    // NOTE: Only the first 3 cars are lined up
    for (auto i = 0; i < 3 && storedCars[i].HasCar(); i++) {
        auto& storedCar = storedCars[i];
        storedCar.m_vPosn = center + lineDir * (4.0f * (float)(i - 1));
        // Face perpendicular to the line, across the garage
        storedCar.m_nPackedForwardX = (int8)(lineDir.y * 100.0f);
        storedCar.m_nPackedForwardY = (int8)(-lineDir.x * 100.0f);
        storedCar.m_nPackedForwardZ = 0;
    }
}

// 0x448550
bool CGarage::RestoreCarsForThisHideOut(CStoredCar* storedCars) {
    for (auto i = 0u; i < NUM_GARAGE_STORED_CARS; i++) {
        auto& storedCar = storedCars[i];
        if (!storedCar.HasCar())
            continue;
        if (auto* vehicle = storedCar.RestoreCar()) {
            vehicle->bImpounded = false;
            CWorld::Add(vehicle);
            storedCar.Clear();
        }
    }

    // Done once every car was (successfully) restored
    for (auto i = 0u; i < NUM_GARAGE_STORED_CARS; i++) {
        if (storedCars[i].HasCar())
            return false;
    }
    return true;
}

// 0x4485C0
bool CGarage::RestoreCarsForThisImpoundingGarage(CStoredCar* storedCars) {
    constexpr auto NUM_IMPOUND_STORED_CARS = 3u;

    for (auto i = 0u; i < NUM_IMPOUND_STORED_CARS; i++) {
        auto& storedCar = storedCars[i];
        if (!storedCar.HasCar())
            continue;
        if (auto* vehicle = storedCar.RestoreCar()) {
            vehicle->bImpounded = true;
            CWorld::Add(vehicle);
            if (vehicle->IsSubAutomobile())
                vehicle->AsAutomobile()->PlaceOnRoadProperly();
            else if (vehicle->IsSubBike())
                vehicle->AsBike()->PlaceOnRoadProperly();
            storedCar.Clear();
        }
    }

    // Done once every car was (successfully) restored
    for (auto i = 0u; i < NUM_IMPOUND_STORED_CARS; i++) {
        if (storedCars[i].HasCar())
            return false;
    }
    return true;
}

// 0x448E50
bool CGarage::IsPlayerOutsideGarage(float fRadius) {
    if (auto* playerVeh = FindPlayerVehicle())
        return IsEntityEntirelyOutside(playerVeh, fRadius);
    return IsEntityEntirelyOutside(FindPlayerPed(), fRadius);
}

// 0x449100
bool CGarage::IsAnyOtherCarTouchingGarage(CVehicle* ignoredVehicle) {
    auto* pool = GetVehiclePool();
    for (auto i = pool->GetSize(); i != 0; i--) {
        auto* vehicle = pool->GetAt(i - 1);
        if (!vehicle || vehicle == ignoredVehicle || vehicle->GetStatus() == STATUS_WRECKED || !IsEntityTouching3D(vehicle))
            continue;

        // NOTE: Redundant with `IsEntityTouching3D`'s narrow phase, but this is what the original does
        const auto& matrix = vehicle->GetMatrix();
        for (const auto& sphere : vehicle->GetColModel()->GetData()->GetSpheres()) {
            if (IsPointInsideGarage(matrix.TransformPoint(sphere.m_vecCenter), sphere.m_fRadius))
                return true;
        }
    }
    return false;
}

// 0x449220
void CGarage::ThrowCarsNearDoorOutOfGarage(CVehicle* ignoredVehicle) {
    auto* pool = GetVehiclePool();
    for (auto i = pool->GetSize(); i != 0; i--) {
        auto* vehicle = pool->GetAt(i - 1);
        if (!vehicle || vehicle == ignoredVehicle || !IsEntityTouching3D(vehicle))
            continue;

        const auto& matrix = vehicle->GetMatrix();
        for (const auto& sphere : vehicle->GetColModel()->GetData()->GetSpheres()) {
            if (IsPointInsideGarage(matrix.TransformPoint(sphere.m_vecCenter), 0.0f))
                continue;

            // Part of the car sticks out: shove it away from the garage's center
            CVector dir{
                vehicle->GetPosition().x - (m_fLeftCoord + m_fRightCoord) * 0.5f,
                vehicle->GetPosition().y - (m_fFrontCoord + m_fBackCoord) * 0.5f,
                0.0f
            };
            dir.Normalise();
            vehicle->m_vecMoveSpeed += dir * (0.02f * CTimer::GetTimeStep());
            break;
        }
    }
}

// 0x4494F0
bool CGarage::IsAnyCarBlockingDoor() {
    auto* pool = GetVehiclePool();
    for (auto i = pool->GetSize(); i != 0; i--) {
        auto* vehicle = pool->GetAt(i - 1);
        if (vehicle && IsEntityTouching3D(vehicle) && EntityHasASphereWayOutsideGarage(vehicle, 0.0f))
            return true;
    }
    return false;
}

// 0x4495F0
int32 CGarage::CountCarsWithCenterPointWithinGarage(CVehicle* ignoredVehicle) {
    auto* pool = GetVehiclePool();
    int32 count = 0;
    for (auto i = pool->GetSize(); i != 0; i--) {
        auto* vehicle = pool->GetAt(i - 1);
        if (vehicle && vehicle != ignoredVehicle && IsPointInsideGarage(vehicle->GetPosition()))
            count++;
    }
    return count;
}

// 0x449A50
void CGarage::StoreAndRemoveCarsForThisImpoundingGarage(CStoredCar* storedCars, int32 iMaxSlot) {
    // NOTE: The original is an exact copy of `StoreAndRemoveCarsForThisHideOut`
    StoreAndRemoveCarsForThisHideOut(storedCars, iMaxSlot);
}

// 0x449FF0
void CGarage::FindDoorsWithGarage(CObject** ppFirstDoor, CObject** ppSecondDoor) {
    *ppFirstDoor  = nullptr;
    *ppSecondDoor = nullptr;

    const auto garageIdx = (int8)(this - CGarages::aGarages);
    const auto centerX   = m_vPosn.x + m_vDirectionA.x * m_fWidth * 0.5f + m_vDirectionB.x * m_fHeight * 0.5f;
    const auto centerY   = m_vPosn.y + m_vDirectionA.y * m_fWidth * 0.5f + m_vDirectionB.y * m_fHeight * 0.5f;

    float firstDist = 99999.9f, secondDist = 99999.9f;

    auto* pool = GetObjectPool();
    for (auto i = pool->GetSize(); i != 0; i--) {
        auto* object = pool->GetAt(i - 1);
        if (!object || object->m_nGarageDoorGarageIndex != garageIdx)
            continue;

        const auto dist = std::hypot(centerX - object->GetPosition().x, centerY - object->GetPosition().y);
        if (!*ppFirstDoor) {
            *ppFirstDoor = object;
            firstDist    = dist;
        } else if (dist < firstDist) {
            *ppSecondDoor = *ppFirstDoor;
            secondDist    = firstDist;
            *ppFirstDoor  = object;
            firstDist     = dist;
        } else if (!*ppSecondDoor || dist < secondDist) {
            *ppSecondDoor = object;
            secondDist    = dist;
        }
    }
}

// 0x44A660
bool CGarage::SlideDoorOpen() {
    const auto speed = (m_nType == HANGAR_AT400 || m_nType == HANGAR_ABANDONED_AIRPORT) ? 0.0011f : 0.011f;
    m_fDoorPosition += speed * CTimer::GetTimeStep();

    const auto bFullyOpen = m_fDoorPosition >= 1.0f;
    if (bFullyOpen)
        m_fDoorPosition = 1.0f;

    CObject* door1, * door2;
    FindDoorsWithGarage(&door1, &door2);
    if (door1)
        m_GarageAudio.AddAudioEvent(bFullyOpen ? AE_GARAGE_DOOR_OPENED : AE_GARAGE_DOOR_OPENING, door1->GetPosition(), 0.0f, 1.0f);

    return bFullyOpen;
}

// 0x44A750
bool CGarage::SlideDoorClosed() {
    const auto speed = (m_nType == HANGAR_AT400 || m_nType == HANGAR_ABANDONED_AIRPORT) ? 0.0013f : 0.013f;
    m_fDoorPosition -= speed * CTimer::GetTimeStep();

    const auto bFullyClosed = m_fDoorPosition <= 0.0f;
    if (bFullyClosed)
        m_fDoorPosition = 0.0f;

    CObject* door1, * door2;
    FindDoorsWithGarage(&door1, &door2);
    if (door1)
        m_GarageAudio.AddAudioEvent(bFullyClosed ? AE_GARAGE_DOOR_CLOSED : AE_GARAGE_DOOR_CLOSING, door1->GetPosition(), 0.0f, 1.0f);

    return bFullyClosed;
}

bool CGarage::IsHideOut() const {
    switch (m_nType) {
    case eGarageType::SAFEHOUSE_GANTON:
    case eGarageType::SAFEHOUSE_SANTAMARIA:
    case eGarageType::SAGEHOUSE_ROCKSHORE:
    case eGarageType::SAFEHOUSE_FORTCARSON:
    case eGarageType::SAFEHOUSE_VERDANTMEADOWS:
    case eGarageType::SAFEHOUSE_DILLIMORE:
    case eGarageType::SAFEHOUSE_PRICKLEPINE:
    case eGarageType::SAFEHOUSE_WHITEWOOD:
    case eGarageType::SAFEHOUSE_PALOMINOCREEK:
    case eGarageType::SAFEHOUSE_REDSANDSWEST:
    case eGarageType::SAFEHOUSE_ELCORONA:
    case eGarageType::SAFEHOUSE_MULHOLLAND:
    case eGarageType::SAFEHOUSE_CALTONHEIGHTS:
    case eGarageType::SAFEHOUSE_PARADISO:
    case eGarageType::SAFEHOUSE_DOHERTY:
    case eGarageType::SAFEHOUSE_HASHBURY:
    case eGarageType::HANGAR_ABANDONED_AIRPORT:
        return true;
    default:
        return false;
    }
}

// 0x44A9C0
bool CGarage::IsGarageEmpty() {
    CVector cornerA = { m_fLeftCoord, m_fFrontCoord, m_vPosn.z };
    CVector cornerB = { m_fRightCoord, m_fBackCoord, m_fTopZ   };

    int16 outCount[2];
    CEntity* outEntities[16];
    CWorld::FindObjectsIntersectingCube(cornerA, cornerB, outCount, static_cast<int16>(std::size(outEntities)), outEntities, false, true, true, false, false);
    if (outCount[0] <= 0)
        return true;

    int16 entityIndex = 0;

    while (!IsEntityTouching3D(outEntities[entityIndex])) {
        if (++entityIndex >= outCount[0])
            return true;
    }
    return false;
}

/*
void CGarage::CenterCarInGarage(CEntity* entity) {
    auto vehicle = FindPlayerVehicle();
    if (IsAnyOtherCarTouchingGarage(vehicle))
        return;

    auto player = FindPlayerPed();
    if (IsAnyOtherPedTouchingGarage(player))
        return;

    auto pos = entity->GetPosition();

    const auto halfX = (m_fRightCoord + m_fLeftCoord) * 0.5f;
    const auto halfY = (m_fBackCoord + m_fFrontCoord) * 0.5f;
    CVector p1{
        halfX - pos.x,
        halfY - pos.y,
        pos.z - pos.z
    };

    auto dist = p1.Magnitude();
    if (dist >= 0.4f) {
        auto x = halfX - pos.x * 0.4f / dist + pos.x;
        auto y = 0.4f / dist * halfY - pos.y + pos.y;
    } else {
        auto x = halfX;
        auto y = halfY;
    }

    if (!IsEntityEntirelyInside3D(entity, 0.3f))
        entity->SetPosn(entity->GetPosition());
}
*/

// 0x5D3020
void CSaveGarage::CopyGarageIntoSaveGarage(Const CGarage& g) {
    m_nType         = g.m_nType;
    m_nDoorState    = g.m_nDoorState;
    m_nFlags        = g.m_nFlags;
    m_vPosn         = g.m_vPosn;
    m_vDirectionA   = g.m_vDirectionA;
    m_vDirectionB   = g.m_vDirectionB;
    m_fTopZ         = g.m_fTopZ;
    m_fWidth        = g.m_fWidth;
    m_fHeight       = g.m_fHeight;
    m_fLeftCoord    = g.m_fLeftCoord;
    m_fRightCoord   = g.m_fRightCoord;
    m_fFrontCoord   = g.m_fFrontCoord;
    m_fBackCoord    = g.m_fBackCoord;
    m_fDoorPosition = g.m_fDoorPosition;
    m_nTimeToOpen   = g.m_nTimeToOpen;
    m_nOriginalType = g.m_nOriginalType;
    strcpy_s(m_anName, g.m_anName);
}

// 0x5D30C0
void CSaveGarage::CopyGarageOutOfSaveGarage(CGarage& g) const {
    g.m_nType         = m_nType;
    g.m_nDoorState    = m_nDoorState;
    g.m_nFlags        = m_nFlags;
    g.m_vPosn         = m_vPosn;
    g.m_vDirectionA   = m_vDirectionA;
    g.m_vDirectionB   = m_vDirectionB;
    g.m_fTopZ         = m_fTopZ;
    g.m_fWidth        = m_fWidth;
    g.m_fHeight       = m_fHeight;
    g.m_fLeftCoord    = m_fLeftCoord;
    g.m_fRightCoord   = m_fRightCoord;
    g.m_fFrontCoord   = m_fFrontCoord;
    g.m_fBackCoord    = m_fBackCoord;
    g.m_fDoorPosition = m_fDoorPosition;
    g.m_nTimeToOpen   = m_nTimeToOpen;
    g.m_nOriginalType = m_nOriginalType;
    g.m_pTargetCar    = nullptr;
    strcpy_s(g.m_anName, m_anName);
}

// todo move
// 0x449760
void CStoredCar::StoreCar(CVehicle* vehicle) {
    m_wModelIndex = (uint16)vehicle->m_nModelIndex;
    m_vPosn = vehicle->GetPosition();

    const auto& forward = vehicle->GetForward();
    m_nPackedForwardX = (int8)(forward.x * 100.0f);
    m_nPackedForwardY = (int8)(forward.y * 100.0f);
    m_nPackedForwardZ = (int8)(forward.z * 100.0f);

    m_nPrimaryColor    = vehicle->m_nPrimaryColor;
    m_nSecondaryColor  = vehicle->m_nSecondaryColor;
    m_nTertiaryColor   = vehicle->m_nTertiaryColor;
    m_nQuaternaryColor = vehicle->m_nQuaternaryColor;
    m_nRadioStation    = (uint8)vehicle->m_vehicleAudio.m_AuSettings.RadioStation;
    m_nHandlingFlags   = (uint32)vehicle->m_nHandlingFlagsIntValue;
    m_anCompsToUse[0]  = vehicle->m_anExtras[0];
    m_anCompsToUse[1]  = vehicle->m_anExtras[1];

    m_nStoredCarFlags = 0;
    if (vehicle->bBulletProof)                  m_nStoredCarFlags |= 0x01;
    if (vehicle->bFireProof)                    m_nStoredCarFlags |= 0x02;
    if (vehicle->bExplosionProof)               m_nStoredCarFlags |= 0x04;
    if (vehicle->bCollisionProof)               m_nStoredCarFlags |= 0x08;
    if (vehicle->bMeleeProof)                   m_nStoredCarFlags |= 0x10;
    if (vehicle->bUpgradedStereo)               m_nStoredCarFlags |= 0x20;
    if (vehicle->handlingFlags.bHydraulicInst)  m_nStoredCarFlags |= 0x40;
    if (vehicle->handlingFlags.bNosInst)        m_nStoredCarFlags |= 0x80;

    if (vehicle->IsAutomobile() || vehicle->IsBike())
        m_nBombType = vehicle->m_nBombOnBoard;

    for (auto i = 0; i < CVehicle::NUM_VEHICLE_UPGRADES; i++)
        m_awCarMods[i] = vehicle->m_anUpgrades[i];

    m_nPaintJob    = (uint8)vehicle->GetRemapIndex();
    m_nNitroBoosts = vehicle->m_nNitroBoosts;
}

// 0x447E40
CVehicle* CStoredCar::RestoreCar() {
    CStreaming::RequestModel(m_wModelIndex, STREAMING_KEEP_IN_MEMORY);
    for (const auto mod : m_awCarMods) {
        if (mod != -1)
            CStreaming::RequestVehicleUpgrade(mod, STREAMING_DEFAULT);
    }

    if (!CStreaming::IsModelLoaded(m_wModelIndex))
        return nullptr;
    for (const auto mod : m_awCarMods) {
        if (mod != -1 && !CStreaming::HasVehicleUpgradeLoaded(mod))
            return nullptr;
    }

    CVehicleModelInfo::ms_compsToUse[0] = m_anCompsToUse[0];
    CVehicleModelInfo::ms_compsToUse[1] = m_anCompsToUse[1];

    CVehicle* vehicle;
    switch (CModelInfo::GetVehicleModelInfo(m_wModelIndex)->m_nVehicleType) {
    case VEHICLE_TYPE_MTRUCK:  vehicle = new CMonsterTruck(m_wModelIndex, RANDOM_VEHICLE); break;
    case VEHICLE_TYPE_QUAD:    vehicle = new CQuadBike(m_wModelIndex, RANDOM_VEHICLE); break;
    case VEHICLE_TYPE_HELI:    vehicle = new CHeli(m_wModelIndex, RANDOM_VEHICLE); break;
    case VEHICLE_TYPE_PLANE:   vehicle = new CPlane(m_wModelIndex, RANDOM_VEHICLE); break;
    case VEHICLE_TYPE_BOAT:    vehicle = new CBoat(m_wModelIndex, RANDOM_VEHICLE); break;
    case VEHICLE_TYPE_BIKE:
        vehicle = new CBike(m_wModelIndex, RANDOM_VEHICLE);
        vehicle->AsBike()->bikeFlags.bOnSideStand = true;
        break;
    case VEHICLE_TYPE_BMX:
        vehicle = new CBmx(m_wModelIndex, RANDOM_VEHICLE);
        vehicle->AsBike()->bikeFlags.bOnSideStand = true;
        break;
    case VEHICLE_TYPE_TRAILER: vehicle = new CTrailer(m_wModelIndex, RANDOM_VEHICLE); break;
    default:                   vehicle = new CAutomobile(m_wModelIndex, RANDOM_VEHICLE, true); break;
    }

    vehicle->SetPosn(m_vPosn);

    const CVector forward = {
        (float)m_nPackedForwardX / 100.0f,
        (float)m_nPackedForwardY / 100.0f,
        (float)m_nPackedForwardZ / 100.0f,
    };
    vehicle->GetForward() = forward;
    vehicle->SetStatus(STATUS_ABANDONED);
    vehicle->GetRight() = CVector{ forward.y, -forward.x, 0.0f };
    vehicle->GetUp() = CVector{ 0.0f, 0.0f, 1.0f };

    vehicle->m_pDriver = nullptr;
    vehicle->m_vehicleAudio.m_AuSettings.RadioStation = (eRadioID)m_nRadioStation;
    vehicle->m_nHandlingFlagsIntValue = (eVehicleHandlingFlags)m_nHandlingFlags;
    vehicle->bFreebies = false;
    vehicle->bHasBeenOwnedByPlayer = true;
    vehicle->m_nDoorLock = CARLOCK_UNLOCKED;

    if (vehicle->IsAutomobile())
        vehicle->m_nBombOnBoard = m_nBombType;

    if (m_nStoredCarFlags & 0x01) vehicle->bBulletProof = true;
    if (m_nStoredCarFlags & 0x02) vehicle->bFireProof = true;
    if (m_nStoredCarFlags & 0x04) vehicle->bExplosionProof = true;
    if (m_nStoredCarFlags & 0x08) vehicle->bCollisionProof = true;
    if (m_nStoredCarFlags & 0x10) vehicle->bMeleeProof = true;
    if (m_nStoredCarFlags & 0x20) {
        vehicle->bUpgradedStereo = true;
        vehicle->m_vehicleAudio.m_AuSettings.BassSetting = eBassSetting::BOOST;
    }
    if (m_nStoredCarFlags & 0x40) vehicle->handlingFlags.bHydraulicInst = true;
    if (m_nStoredCarFlags & 0x80) vehicle->handlingFlags.bNosInst = true;

    for (auto i = 0; i < CVehicle::NUM_VEHICLE_UPGRADES; i++)
        vehicle->m_anUpgrades[i] = m_awCarMods[i];

    vehicle->SetupUpgradesAfterLoad();
    vehicle->SetRemap(m_nPaintJob);
    vehicle->bEngineOn = false;
    vehicle->m_nNitroBoosts = m_nNitroBoosts;

    vehicle->bDontSetColourWhenRemapping = true;
    vehicle->m_nPrimaryColor    = m_nPrimaryColor;
    vehicle->m_nSecondaryColor  = m_nSecondaryColor;
    vehicle->m_nTertiaryColor   = m_nTertiaryColor;
    vehicle->m_nQuaternaryColor = m_nQuaternaryColor;

    return vehicle;
}
