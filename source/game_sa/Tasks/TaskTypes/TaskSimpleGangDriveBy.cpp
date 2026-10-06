#include "StdInc.h"

#include "TaskSimpleGangDriveBy.h"
#include "EventDamage.h"
#include "Bike.h"

// Same as the original `CGeneral::GetRandomNumberInRange(int32, int32)` (0x407180).
// NOTSA: Used instead of the templated one because that asserts on empty/inverted ranges, which do occur here (eg.: 100% frequency, weapons with 1 round per clip)
static int32 GetRandomNumberInRangeUnchecked(int32 min, int32 max) {
    return min + (int32)((float)CGeneral::GetRandomNumber() * (1.f / 32768.f) * (float)(max - min));
}

namespace {
// NOTSA: The exact constants the game uses (the ones in `common.h` are less precise)
constexpr float DB_PI         = 3.14159274f;
constexpr float DB_TWO_PI     = 6.28318548f;
constexpr float DB_HALF_PI    = 1.57079637f;
constexpr float DB_QUARTER_PI = 0.785398185f;
constexpr float DB_3_4_PI     = 2.3561945f;
constexpr float DB_2_OVER_PI  = 0.636619747f;

// Direction (relative to the vehicle) to do the driveby in
enum eDriveByDirn : int8 {
    DRIVEBY_DIRN_NONE = -1,
    DRIVEBY_DIRN_FWD  = 0,
    DRIVEBY_DIRN_LHS  = 1,
    DRIVEBY_DIRN_BAK  = 2,
    DRIVEBY_DIRN_RHS  = 3,
};

// NOTSA: Wrap the difference of 2 headings into [-PI, PI]
float LimitHeadingDelta(float angle) {
    if (angle > DB_PI) {
        return angle - DB_TWO_PI;
    }
    if (angle < -DB_PI) {
        return angle + DB_TWO_PI;
    }
    return angle;
}

// NOTSA: Quadrant (`eDriveByDirn`) the angle (relative to the vehicle's heading) is in
eDriveByDirn GetDriveByDirnFromAngle(float angle) {
    angle += DB_QUARTER_PI;
    if (angle < 0.f) {
        angle += DB_TWO_PI;
    }
    return (eDriveByDirn)(int8)(int32)(angle * DB_2_OVER_PI);
}

// NOTSA: Anim to use for the given direction (`dirn` must be valid)
AnimationId GetDriveByAnim(eDriveByDirn dirn, bool seatRHS) {
    switch (dirn) {
    case DRIVEBY_DIRN_FWD: return seatRHS ? ANIM_ID_DRIVEBYRHS_FWD : ANIM_ID_DRIVEBYLHS_FWD;
    case DRIVEBY_DIRN_LHS: return seatRHS ? ANIM_ID_DRIVEBYTOP_RHS : ANIM_ID_DRIVEBYLHS;
    case DRIVEBY_DIRN_BAK: return seatRHS ? ANIM_ID_DRIVEBYRHS_BWD : ANIM_ID_DRIVEBYLHS_BWD;
    case DRIVEBY_DIRN_RHS: return seatRHS ? ANIM_ID_DRIVEBYRHS : ANIM_ID_DRIVEBYTOP_LHS;
    default:               NOTSA_UNREACHABLE();
    }
}
} // namespace

void CTaskSimpleGangDriveBy::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleGangDriveBy, 0x86D944, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(FinishAnimGangDriveByCB, 0x621BE0);
    RH_ScopedInstall(ManageAnim, 0x627B20);
    RH_ScopedInstall(FireGun, 0x627CC0);
    RH_ScopedInstall(PlayerTarget, 0x621960);
    RH_ScopedInstall(LineOfSightClearForAttack, 0x621B10);
    RH_ScopedInstall(LookForTarget, 0x627600);
    RH_ScopedInstall(AimGun, 0x628350);

    RH_ScopedVMTInstall(Clone, 0x6236D0);
    RH_ScopedVMTInstall(GetTaskType, 0x6218B0);
    RH_ScopedVMTInstall(MakeAbortable, 0x62D290);
    RH_ScopedVMTInstall(ProcessPed, 0x62D3B0);
}

CTaskSimpleGangDriveBy::CTaskSimpleGangDriveBy(CEntity* target, const CVector* targetPos, float abortRange, int8 frequencyPercentage, eDrivebyStyle drivebyStyle, bool seatRHS) {
    m_bSeatRHS             = seatRHS;
    m_nDrivebyStyle        = drivebyStyle;
    m_fAbortRange          = abortRange;
    m_pTargetEntity        = target;
    m_nFrequencyPercentage = frequencyPercentage;
    m_bIsFinished          = false;
    m_bAnimsReferenced     = false;
    m_bInRangeToShoot      = false;
    m_bInWeaponRange       = false;
    m_bReachedAbortRange   = false;
    m_bFromScriptCommand   = false;
    m_nBurstShots          = -1;
    m_nFakeShootDirn       = -1;
    m_nAttackTimer         = -1;
    m_nLastCommand         = 0;
    m_nNextCommand         = 1;
    m_nLOSCheckTime        = 0;
    m_nLOSBlocked          = true;
    m_pAnimAssoc           = nullptr;
    m_nRequiredAnimID      = ANIM_ID_NO_ANIMATION_SET;
    m_nRequiredAnimGroup   = ANIM_GROUP_DEFAULT;
    m_pWeaponInfo          = nullptr;
    CEntity::SafeRegisterRef(m_pTargetEntity);
    if (targetPos) {
        m_vecCoords = *targetPos;
    }
}

CTaskSimpleGangDriveBy::~CTaskSimpleGangDriveBy()
{
    if (m_bAnimsReferenced)
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(m_nRequiredAnimGroup));

    if (m_pAnimAssoc)
        m_pAnimAssoc->SetDeleteCallback(CDefaultAnimCallback::DefaultAnimCB, nullptr);

    CEntity::SafeCleanUpRef(m_pTargetEntity);
}

// 0x6236D0
CTask* CTaskSimpleGangDriveBy::Clone() const {
    auto* const task = new CTaskSimpleGangDriveBy{
        m_pTargetEntity,
        &m_vecCoords,
        m_fAbortRange,
        m_nFrequencyPercentage,
        m_nDrivebyStyle,
        m_bSeatRHS
    };
    task->m_bFromScriptCommand = m_bFromScriptCommand;
    return task;
}

// 0x62D290
bool CTaskSimpleGangDriveBy::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event)
{
    if (priority != ABORT_PRIORITY_URGENT && priority != ABORT_PRIORITY_IMMEDIATE) {
        m_nNextCommand = 6;
        return false;
    }

    // Damage that doesn't kill the ped doesn't abort the driveby
    if (event && event->GetEventType() == EVENT_DAMAGE && !static_cast<const CEventDamage*>(event)->HasKilledPed()) {
        return false;
    }

    m_nNextCommand = 7;
    if (ManageAnim(ped)) {
        m_nLastCommand = m_nNextCommand;
        if (priority == ABORT_PRIORITY_IMMEDIATE) {
            m_pAnimAssoc->m_BlendDelta = -1000.0f;
        } else if (!m_pAnimAssoc->IsPlaying()) {
            m_pAnimAssoc->SetFlag(ANIMATION_IS_PLAYING, true);
        }
    }

    if (ped->m_pPlayerData) {
        ped->m_pPlayerData->m_bFreeAiming = true;
    }

    if (ped->IsPlayer()) {
        ped->AsPlayer()->GetPlayerInfoForThisPlayerPed()->m_bCanDoDriveBy = true;
    }

    if (m_pAnimAssoc) {
        if (!m_pAnimAssoc->IsPlaying()) {
            if (m_pAnimAssoc->m_BlendDelta > 0.0f) {
                m_pAnimAssoc->m_BlendDelta = -8.0f;
            } else if (m_pAnimAssoc->m_BlendAmount > 0.0f && m_pAnimAssoc->m_BlendDelta >= 0.0f) {
                m_pAnimAssoc->m_BlendDelta = -8.0f;
            }
        }
        m_pAnimAssoc->SetDeleteCallback(CDefaultAnimCallback::DefaultAnimCB, nullptr);
        m_pAnimAssoc = nullptr;
    }

    m_bIsFinished = true;
    return true;
}

// 0x621BE0
void CTaskSimpleGangDriveBy::FinishAnimGangDriveByCB(CAnimBlendAssociation* anim, void* data) {
    auto* const self = static_cast<CTaskSimpleGangDriveBy*>(data);
    if (self->m_pAnimAssoc == anim) {
        self->m_pAnimAssoc = nullptr;
    }
}

// 0x627B20
bool CTaskSimpleGangDriveBy::ManageAnim(CPed* ped) {
    // Make sure the anims are loaded
    auto* block = CAnimManager::GetAnimationBlock(m_nRequiredAnimGroup);
    if (!block) {
        block = CAnimManager::GetAnimationBlock(CAnimManager::GetAnimBlockName(m_nRequiredAnimGroup));
    }
    const auto blockIdx = CAnimManager::GetAnimationBlockIndex(block);
    if (!block->IsLoaded) {
        CStreaming::RequestModel(IFPToModelId(blockIdx), STREAMING_KEEP_IN_MEMORY);
        return false;
    }
    if (!m_bAnimsReferenced) {
        CAnimManager::AddAnimBlockRef(blockIdx);
        m_bAnimsReferenced = true;
    }

    if (m_nRequiredAnimID == ANIM_ID_NO_ANIMATION_SET && !m_pAnimAssoc) {
        return false;
    }

    const auto ShouldStartAnim = [this] {
        if (!m_pAnimAssoc) {
            return m_nAttackTimer <= 0 && m_bInWeaponRange;
        }
        return m_pAnimAssoc->GetAnimId() != m_nRequiredAnimID
            && (m_nNextCommand == 1 || m_nNextCommand == 2 || m_nNextCommand == 3)
            && m_nLastCommand < 4;
    };
    if (ShouldStartAnim()) { // 0x627BD9
        const auto hadAnim = m_pAnimAssoc != nullptr;
        if (hadAnim) {
            m_pAnimAssoc->SetDefaultDeleteCallback();
            m_pAnimAssoc->SetFlag(ANIMATION_IS_PLAYING, false);
        }

        m_pAnimAssoc = CAnimManager::BlendAnimation(ped->GetRpClump(), m_nRequiredAnimGroup, (AnimationId)m_nRequiredAnimID, 4.f);
        m_pAnimAssoc->SetDeleteCallback(FinishAnimGangDriveByCB, this);

        if (m_nNextCommand == 3 && m_nBurstShots <= 1) {
            m_nBurstShots = (int8)GetRandomNumberInRangeUnchecked(2, (int16)m_pWeaponInfo->m_nAmmoClip);
        }

        if (hadAnim) { // Changing direction, skip the lean-out part of the new anim
            m_pAnimAssoc->SetCurrentTime(ANIM_LOOP_START);
            m_pAnimAssoc->SetFlag(ANIMATION_IS_PLAYING, false);
            m_nLastCommand = 1;
            m_nNextCommand = 0;
            if (!ped->IsPlayer()) {
                m_nAttackTimer = 100;
            }
        } else {
            m_nLastCommand = m_nNextCommand;
            m_nNextCommand = 0;
        }
    }

    return m_pAnimAssoc && m_pAnimAssoc->m_BlendAmount > 0.9f && m_pAnimAssoc->m_BlendDelta >= 0.f;
}

// 0x627CC0
void CTaskSimpleGangDriveBy::FireGun(CPed* ped) {
    // Muzzle position
    CVector firePos = m_pWeaponInfo->m_vecFireOffset;
    {
        auto* const hier = GetAnimHierarchyFromSkinClump(ped->GetRpClump());
        const auto  idx  = RpHAnimIDGetIndex(hier, ped->m_apBones[PED_NODE_RIGHT_HAND]->BoneTag);
        RwV3dTransformPoints(&firePos, &firePos, 1, &RpHAnimHierarchyGetMatrixArray(hier)[idx]);
    }

    auto* const veh        = ped->m_pVehicle;
    const auto& vehBB      = CModelInfo::GetModelInfo(veh->m_nModelIndex)->GetColModel()->GetBoundingBox();
    const auto  relFirePos = firePos - veh->GetPosition();

    // Direction to shoot in
    CVector dir;
    if (ped->IsPlayer()) {
        const auto& cam = TheCamera.GetActiveCam();
        float range;
        switch (cam.m_nMode) {
        case MODE_TWOPLAYER_IN_CAR_AND_SHOOTING:
            range = (ped->GetPosition() - cam.m_vecSource).Magnitude() + 20.f;
            break;
        case MODE_AIMWEAPON_FROMCAR:
            range = 20.f;
            break;
        default:
            return;
        }
        CVector camSource, camTarget;
        TheCamera.Find3rdPersonCamTargetVector(range, firePos, camSource, camTarget);
        dir = camTarget - firePos;
        ped->m_pPlayerData->m_fAttackButtonCounter = 0.f;
    } else if (m_pTargetEntity) {
        dir = m_pTargetEntity->GetMatrix().TransformPoint(m_vecCoords) - firePos;
    } else {
        dir = m_vecCoords - firePos;
    }
    dir.Normalise();

    // Move the bullet's origin out of the vehicle (So that the ped doesn't shoot their own vehicle)
    const auto IsAnim = [this](AnimationId a, AnimationId b) {
        return m_pAnimAssoc && (m_pAnimAssoc->GetAnimId() == a || m_pAnimAssoc->GetAnimId() == b);
    };
    const auto& vehRight = veh->m_matrix->GetRight();
    const auto& vehFwd   = veh->m_matrix->GetForward();
    CVector     origin   = firePos;
    if (IsAnim(ANIM_ID_DRIVEBYLHS, ANIM_ID_DRIVEBYTOP_RHS) || m_nFakeShootDirn == 1) { // 0x627F26 - Left
        if (const auto d = -dir.Dot(vehRight); d > 0.1f) {
            origin = firePos + dir * ((vehBB.m_vecMin.x - relFirePos.Dot(vehRight) - 0.2f) / d);
        }
    } else if (IsAnim(ANIM_ID_DRIVEBYRHS, ANIM_ID_DRIVEBYTOP_LHS) || m_nFakeShootDirn == 3) { // 0x627FE8 - Right
        if (const auto d = dir.Dot(vehRight); d > 0.1f) {
            origin = firePos + dir * ((vehBB.m_vecMax.x - relFirePos.Dot(vehRight) + 0.2f) / d);
        }
    } else if (IsAnim(ANIM_ID_DRIVEBYLHS_BWD, ANIM_ID_DRIVEBYRHS_BWD) || m_nFakeShootDirn == 2) { // 0x6280CE - Backwards
        if (const auto d = -dir.Dot(vehFwd); d > 0.1f) {
            origin = firePos + dir * ((vehBB.m_vecMin.y - relFirePos.Dot(vehFwd) - 0.2f) / d);
        }
    } else { // 0x628188 - Forwards
        if (const auto d = dir.Dot(vehFwd); d > 0.1f) {
            origin = firePos + dir * ((vehBB.m_vecMax.y - relFirePos.Dot(vehFwd) + 0.2f) / d);
        }
    }

    ped->m_pedIK.bGunReachedTarget = true;

    // 0x628243
    auto& weapon = ped->GetActiveWeapon();
    if (m_vecCoords.x == 0.f && m_vecCoords.y == 0.f && m_vecCoords.z == 0.f) {
        weapon.Fire(ped, &firePos, &firePos, m_pTargetEntity, nullptr, &origin);
    } else {
        CVector target = m_vecCoords;
        if (m_pTargetEntity) {
            target = m_pTargetEntity->GetMatrix().TransformPoint(target);
        }
        weapon.Fire(ped, &firePos, &firePos, m_pTargetEntity, &target, &origin);
    }
    ped->DoGunFlash(GUN_FLASH_TIME_MS, false);
}

// 0x621960
void CTaskSimpleGangDriveBy::PlayerTarget(CPed* ped) {
    auto& cam = TheCamera.GetActiveCam();
    if (cam.m_nMode == MODE_AIMWEAPON_FROMCAR || cam.m_nMode == MODE_TWOPLAYER_IN_CAR_AND_SHOOTING) {
        m_bInRangeToShoot = true;
        m_bInWeaponRange  = true;

        CVector aimDir = cam.m_vecFront;
        if (cam.m_nMode == MODE_TWOPLAYER_IN_CAR_AND_SHOOTING) {
            cam.Get_TwoPlayer_AimVector(aimDir);
        }

        // 0x6219E1 - Pick the anim according to the aiming direction relative to the vehicle
        const auto angle = LimitHeadingDelta(std::atan2(-aimDir.x, aimDir.y) - ped->m_pVehicle->GetHeading());
        switch (const auto dirn = GetDriveByDirnFromAngle(angle)) {
        case DRIVEBY_DIRN_FWD:
        case DRIVEBY_DIRN_LHS:
        case DRIVEBY_DIRN_BAK:
        case DRIVEBY_DIRN_RHS:
            m_nRequiredAnimID = GetDriveByAnim(dirn, m_bSeatRHS);
            break;
        default:
            break;
        }
    } else {
        m_bInRangeToShoot = false;
        m_bInWeaponRange  = true;
    }

    // 0x621AB6
    if (!CGameLogic::IsCoopGameGoingOn()) {
        TheCamera.SetNewPlayerWeaponMode(MODE_AIMWEAPON_FROMCAR, 0, 0);
    }

    if (ped->m_pPlayerData) {
        ped->m_pPlayerData->m_bFreeAiming = true;
    }

    if (ped->IsPlayer()) {
        ped->AsPlayer()->GetPlayerInfoForThisPlayerPed()->m_bCanDoDriveBy = false;
    }
}

// 0x621B10
bool CTaskSimpleGangDriveBy::LineOfSightClearForAttack(CPed* ped, const CVector& target) {
    if (CTimer::GetTimeInMS() > m_nLOSCheckTime) {
        m_nLOSBlocked = false;
        if (m_pTargetEntity) {
            const auto origin = ped->GetPosition() + CVector{ 0.f, 0.f, 0.5f };
            CWorld::pIgnoreEntity = m_pTargetEntity;
            if (!CWorld::GetIsLineOfSightClear(origin, target, true, false, false, false, false, true, false)) {
                m_nLOSBlocked = true;
            }
            CWorld::pIgnoreEntity = nullptr;
        }
        m_nLOSCheckTime = CTimer::GetTimeInMS() - GetRandomNumberInRangeUnchecked(0, -500) + 1750; // Next check in 1750 - 2250 ms
    }
    return !m_nLOSBlocked;
}

// 0x627600
void CTaskSimpleGangDriveBy::LookForTarget(CPed* ped) {
    const auto weaponRange = m_pWeaponInfo->m_fWeaponRange;

    // Direction to the target
    const auto dir = m_pTargetEntity
        ? m_pTargetEntity->GetMatrix().TransformPoint(m_vecCoords) - ped->GetPosition()
        : m_vecCoords - ped->GetPosition();
    const auto dist = dir.Magnitude();

    // Angle of the target relative to the vehicle's heading (positive => on the left)
    const auto angle = LimitHeadingDelta(std::atan2(-dir.x, dir.y) - ped->m_pVehicle->GetHeading());

    m_bInRangeToShoot = false;
    m_bInWeaponRange  = dist < weaponRange;

    // 0x62775D - Once the abort range was reached the driveby is over as soon as the target is out of it again
    if (!m_bReachedAbortRange) {
        if (dist < m_fAbortRange) {
            m_bReachedAbortRange = true;
        }
    } else if (dist > m_fAbortRange) {
        m_nNextCommand = 7;
        return;
    }

    // 0x62778D - Target dead?
    if (m_pTargetEntity) {
        if (m_pTargetEntity->GetIsTypePed()) {
            if (m_pTargetEntity->AsPed()->m_fHealth <= 0.f) {
                m_nNextCommand = 7;
                return;
            }
        } else if (m_pTargetEntity->GetIsTypeVehicle()) {
            if (m_pTargetEntity->AsVehicle()->m_fHealth <= 0.f) {
                m_nNextCommand = 7;
                return;
            }
        }
    }

    // 0x6277D8
    const CVector targetPos = dir + ped->GetPosition();
    if (!LineOfSightClearForAttack(ped, targetPos)) {
        m_bInRangeToShoot = false;
        return;
    }

    // 0x627825
    eDriveByDirn angleDirn = DRIVEBY_DIRN_NONE; // Quadrant the target is in
    int8         sideDirn  = 0;                 // -1 => left, 1 => right (With a 15 deg dead-zone forwards and backwards)
    int8         fwdDirn   = 0;                 // -1 => forwards, 1 => backwards (With a 15 deg dead-zone on both sides)
    switch (m_nDrivebyStyle) {
    case eDrivebyStyle::FIXED_LHS:
    case eDrivebyStyle::FIXED_RHS:
    case eDrivebyStyle::FIXED_FWD:
    case eDrivebyStyle::FIXED_BAK:
        break;
    default: {
        angleDirn = GetDriveByDirnFromAngle(angle);

        if (angle > 0.261799395f && angle < 2.87979341f) {
            sideDirn = -1;
        } else if (angle < -0.261799395f && angle > -2.87979341f) {
            sideDirn = 1;
        }

        if (angle < 1.30899692f && angle > -1.30899692f) {
            fwdDirn = -1;
        } else if (angle > 1.83259583f && angle < -1.83259583f) { // 0x6278FB - BUG (in the original code): Can never be true (Should've been `||`)
            fwdDirn = 1;
        }
        break;
    }
    }

    // 0x627922 - Pick the direction to shoot in according to the style
    const auto hasNoAnim = m_nRequiredAnimID == ANIM_ID_NO_ANIMATION_SET;
    const auto SideDirnFromAngle = [&]() -> eDriveByDirn { // 0x62796D
        switch (angleDirn) {
        case DRIVEBY_DIRN_LHS: return DRIVEBY_DIRN_LHS;
        case DRIVEBY_DIRN_RHS: return DRIVEBY_DIRN_RHS;
        default:               return DRIVEBY_DIRN_NONE;
        }
    };
    eDriveByDirn dirn = DRIVEBY_DIRN_NONE;
    switch (m_nDrivebyStyle) {
    case eDrivebyStyle::FIXED_LHS:
        dirn = DRIVEBY_DIRN_LHS;
        break;
    case eDrivebyStyle::FIXED_RHS:
        dirn = DRIVEBY_DIRN_RHS;
        break;
    case eDrivebyStyle::START_FROM_LHS:
        dirn = hasNoAnim ? DRIVEBY_DIRN_LHS : SideDirnFromAngle();
        break;
    case eDrivebyStyle::START_FROM_RHS:
        dirn = hasNoAnim ? DRIVEBY_DIRN_RHS : SideDirnFromAngle();
        break;
    case eDrivebyStyle::AI_SIDE: // 0x627952
        if (!hasNoAnim) {
            dirn = SideDirnFromAngle();
        } else if (sideDirn == -1) {
            dirn = DRIVEBY_DIRN_LHS;
        } else if (sideDirn == 1) {
            dirn = DRIVEBY_DIRN_RHS;
        }
        break;
    case eDrivebyStyle::FIXED_FWD:
        dirn = DRIVEBY_DIRN_FWD;
        break;
    case eDrivebyStyle::FIXED_BAK:
        dirn = DRIVEBY_DIRN_BAK;
        break;
    case eDrivebyStyle::AI_FWD_BAK: // 0x62797F
        if (hasNoAnim) {
            if (fwdDirn == -1) {
                dirn = DRIVEBY_DIRN_FWD;
            } else if (fwdDirn == 1) {
                dirn = DRIVEBY_DIRN_BAK;
            }
        } else if (angleDirn == DRIVEBY_DIRN_FWD) {
            dirn = DRIVEBY_DIRN_FWD;
        } else if (angleDirn == DRIVEBY_DIRN_BAK) {
            dirn = DRIVEBY_DIRN_BAK;
        }
        break;
    case eDrivebyStyle::AI_ALL_DIRN:
        dirn = angleDirn;
        break;
    default:
        break;
    }

    // 0x6279B1 - Set the anim, and check if the target is in the area covered by it
    switch (dirn) {
    case DRIVEBY_DIRN_FWD: // 0x6279C4
        m_nRequiredAnimID = GetDriveByAnim(dirn, m_bSeatRHS);
        if (dist < weaponRange && angle >= -DB_QUARTER_PI && angle <= DB_QUARTER_PI) {
            m_bInRangeToShoot = true;
        }
        break;
    case DRIVEBY_DIRN_LHS: // 0x627A0F
        m_nRequiredAnimID = GetDriveByAnim(dirn, m_bSeatRHS);
        if (dist < weaponRange && angle >= DB_QUARTER_PI && angle <= DB_3_4_PI) {
            m_bInRangeToShoot = true;
        }
        break;
    case DRIVEBY_DIRN_BAK: // 0x627A56
        m_nRequiredAnimID = GetDriveByAnim(dirn, m_bSeatRHS);
        if (dist < weaponRange && (angle >= -DB_3_4_PI || angle <= DB_3_4_PI)) { // BUG (in the original code): Always true (Should've been `angle <= -3/4 PI || angle >= 3/4 PI`)
            m_bInRangeToShoot = true;
        }
        break;
    case DRIVEBY_DIRN_RHS: // 0x627A96
        m_nRequiredAnimID = GetDriveByAnim(dirn, m_bSeatRHS);
        if (dist < weaponRange && angle >= -DB_3_4_PI && angle <= -DB_QUARTER_PI) {
            m_bInRangeToShoot = true;
        }
        break;
    default:
        break;
    }
}

// 0x628350
void CTaskSimpleGangDriveBy::AimGun(CPed* ped) {
    ped->m_pedIK.bUseArm = false;

    // Position to aim at
    CVector target{};
    if (ped->IsPlayer()) {
        const auto& cam = TheCamera.GetActiveCam();
        float range;
        switch (cam.m_nMode) {
        case MODE_TWOPLAYER_IN_CAR_AND_SHOOTING:
            range = (ped->GetPosition() - cam.m_vecSource).Magnitude() + 20.f;
            break;
        case MODE_AIMWEAPON_FROMCAR:
            range = 20.f;
            break;
        default:
            return;
        }
        CVector source = ped->GetPosition() + CVector{ 0.f, 0.f, 0.7f };
        TheCamera.Find3rdPersonCamTargetVector(range, source, source, target);
    } else if (m_pTargetEntity) { // 0x628473
        if (m_vecCoords.x != 0.f || m_vecCoords.y != 0.f || m_vecCoords.z != 0.f) {
            target = m_pTargetEntity->GetMatrix().TransformPoint(m_vecCoords);
        } else if (m_pTargetEntity->GetIsTypePed()) {
            m_pTargetEntity->AsPed()->GetBonePosition(&target, BONE_SPINE1, false);
        } else {
            target = m_pTargetEntity->GetPosition();
        }
    } else if (m_vecCoords.x != 0.f || m_vecCoords.y != 0.f || m_vecCoords.z != 0.f) { // 0x628520
        target = m_vecCoords;
    }

    // 0x628565 - Only the height of the upper arm is used
    CVector armPos{};
    ped->GetBonePosition(&armPos, BONE_R_UPPER_ARM, false);
    armPos.x = ped->GetPosition().x;
    armPos.y = ped->GetPosition().y;

    auto  heading = CGeneral::GetRadianAngleBetweenPoints(target.x, target.y, armPos.x, armPos.y);
    float pitch   = 0.f;
    if (auto* const veh = ped->m_pVehicle; veh && veh->m_nVehicleType == VEHICLE_TYPE_BIKE) { // 0x6285E7 - Compensate for the bike's lean and pitch
        const auto dist2D          = std::sqrt(sq(armPos.x - target.x) + sq(armPos.y - target.y));
        const auto relativeHeading = heading - veh->GetHeading();
        pitch = CGeneral::GetRadianAngleBetweenPoints(target.z, dist2D, armPos.z, 0.f)
            + std::sin(relativeHeading) * static_cast<CBike*>(veh)->m_RideAnimData.LeanAngle;
        pitch += std::asin(std::clamp(veh->m_matrix->GetForward().z, -1.f, 1.f)) * std::cos(relativeHeading);
    }

    // 0x6286E3 - The anims point the gun to the side/backwards already
    bool useArm = false;
    switch (m_pAnimAssoc->GetAnimId()) {
    case ANIM_ID_DRIVEBYLHS:
    case ANIM_ID_DRIVEBYTOP_RHS: // 0x628734
        useArm   = true;
        heading -= DB_HALF_PI;
        pitch    = -pitch;
        break;
    case ANIM_ID_DRIVEBYRHS:
    case ANIM_ID_DRIVEBYTOP_LHS: // 0x62871F
        useArm   = true;
        heading += DB_HALF_PI;
        break;
    case ANIM_ID_DRIVEBYLHS_BWD:
    case ANIM_ID_DRIVEBYRHS_BWD: // 0x628713
        heading -= DB_PI;
        pitch    = -pitch;
        break;
    default:
        break;
    }
    ped->m_pedIK.PointGunInDirection(heading, pitch, useArm, -1.f);
}

// 0x62D3B0
bool CTaskSimpleGangDriveBy::ProcessPed(CPed* ped) {
    // Only instant-hit weapons can be used for drivebys
    if (const auto* const wi = CWeaponInfo::GetWeaponInfo(ped->GetActiveWeapon().m_Type, ped->GetWeaponSkill()); wi && wi->m_nWeaponFire != WEAPON_FIRE_INSTANT_HIT) {
        return true;
    }
    if (ped->m_pVehicle && !ped->m_pVehicle->CanPedLeanOut(ped)) {
        return true;
    }
    if (m_bIsFinished) {
        return true;
    }

    if (!m_nLOSCheckTime) {
        m_nLOSCheckTime = CTimer::GetTimeInMS() + GetRandomNumberInRangeUnchecked(0, 1000);
    }
    if (!m_pWeaponInfo) {
        m_pWeaponInfo = CWeaponInfo::GetWeaponInfo(ped->GetActiveWeapon().m_Type, ped->GetWeaponSkill());
    }

    if (!ped->bInVehicle || !ped->m_pVehicle || ped->m_fHealth < 1.f) {
        MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr);
        return true;
    }

    if (!ped->IsPlayer()) {
        ped->bTestForShotInVehicle = true;
    }

    // 0x62D4A1 - Pick the anim group (and seat side) according to the vehicle
    if (m_nRequiredAnimGroup == ANIM_GROUP_DEFAULT) {
        auto* const veh = ped->m_pVehicle;
        if (veh->m_nVehicleSubType == VEHICLE_TYPE_QUAD) {
            m_nRequiredAnimGroup = ANIM_GROUP_QUAD_DBZ;
            m_bSeatRHS           = false;
        } else if (veh->m_nVehicleType == VEHICLE_TYPE_BIKE) {
            m_nRequiredAnimGroup = static_cast<CBike*>(veh)->m_RideAnimData.AnimGroup == ANIM_GROUP_BIKES && veh->m_pDriver == ped
                ? ANIM_GROUP_COP_DBZ
                : ANIM_GROUP_BIKE_DBZ;
            m_bSeatRHS = false;
        } else {
            if (veh->m_nVehicleSubType == VEHICLE_TYPE_AUTOMOBILE) {
                if (ped == veh->m_apPassengers[0]) {
                    m_bSeatRHS = true;
                    ped->m_pVehicle->SetWindowOpenFlag(CAR_DOOR_RF);
                } else if (ped == veh->m_apPassengers[1]) {
                    m_bSeatRHS = false;
                    ped->m_pVehicle->SetWindowOpenFlag(CAR_DOOR_LR);
                } else if (ped == veh->m_apPassengers[2]) {
                    m_bSeatRHS = true;
                    ped->m_pVehicle->SetWindowOpenFlag(CAR_DOOR_RR);
                }
            }
            m_nRequiredAnimGroup = ANIM_GROUP_DRIVEBYS;
        }

        if (!ped->IsPlayer() && !ped->m_pWeaponObject) {
            ped->AddWeaponModel(CWeaponInfo::GetWeaponInfo(ped->GetActiveWeapon().m_Type, eWeaponSkill::STD)->m_nModelId1);
        }
    }

    // 0x62D58D
    if (ped->IsPlayer()) {
        PlayerTarget(ped);
    } else {
        LookForTarget(ped);
    }

    if (m_nRequiredAnimGroup == ANIM_GROUP_COP_DBZ && ped->m_pVehicle->m_nVehicleType == VEHICLE_TYPE_BIKE) {
        static_cast<CBike*>(ped->m_pVehicle)->m_nFixLeftHand = true;
    }

    // Has the anim just passed the given time?
    const auto HasAnimJustPassed = [this](float time) {
        return m_pAnimAssoc->m_CurrentTime > time && m_pAnimAssoc->m_CurrentTime - m_pAnimAssoc->m_TimeStep <= time;
    };
    const auto IsFireCommand = [](int8 cmd) {
        return cmd == 2 || cmd == 3;
    };

    if (ManageAnim(ped)) { // 0x62D5D3
        if (m_pAnimAssoc->IsPlaying() && HasAnimJustPassed(ANIM_LOOP_FIRE) && IsFireCommand(m_nLastCommand)) {
            FireGun(ped);
        }

        const auto isPlaying        = m_pAnimAssoc->IsPlaying();
        const auto isRequiredAnim   = [this] { return m_pAnimAssoc->GetAnimId() == m_nRequiredAnimID; };
        if (   !isPlaying
            && m_nAttackTimer <= 0
            && IsFireCommand(m_nNextCommand)
            && (m_pAnimAssoc->m_BlendHier->m_fTotalTime > m_pAnimAssoc->m_CurrentTime || m_pAnimAssoc->m_BlendDelta >= 0.f)
        ) { // 0x62D656 - Start firing
            m_pAnimAssoc->SetFlag(ANIMATION_IS_PLAYING, true);
            m_nBurstShots  = (int8)GetRandomNumberInRangeUnchecked(1, (int32)((float)(int16)m_pWeaponInfo->m_nAmmoClip * 0.5f));
            m_nLastCommand = m_nNextCommand;
            m_nNextCommand = 0;
        } else if (!isPlaying) { // 0x62D692
            if (m_nNextCommand == 7) {
                m_pAnimAssoc->SetFlag(ANIMATION_IS_PLAYING, true);
                m_nLastCommand = m_nNextCommand;
                m_nNextCommand = 7;
            } else if (!isRequiredAnim()) {
                m_pAnimAssoc->SetFlag(ANIMATION_IS_PLAYING, true);
            }
        } else if (m_nLastCommand == 1 && isRequiredAnim() && HasAnimJustPassed(ANIM_LOOP_START)) { // 0x62D6BB - Hold the aiming pose
            m_pAnimAssoc->SetFlag(ANIMATION_IS_PLAYING, false);
            m_pAnimAssoc->SetCurrentTime(ANIM_LOOP_START);
        }

        // 0x62D703 - End of the fire loop reached
        if (isRequiredAnim() && HasAnimJustPassed(ANIM_LOOP_END)) {
            if (ped->GetActiveWeapon().m_State == WEAPONSTATE_RELOADING && m_nNextCommand <= 2) {
                m_nNextCommand = 2;
                m_nAttackTimer = 2000;
            } else if ((IsFireCommand(m_nNextCommand) || (m_nLastCommand == 3 && m_nBurstShots > 0)) && m_bInRangeToShoot) { // 0x62D782 - Loop
                m_pAnimAssoc->SetCurrentTime(ANIM_LOOP_START);
                m_pAnimAssoc->SetFlag(ANIMATION_IS_PLAYING, true);
                if (m_nNextCommand > m_nLastCommand) {
                    m_nLastCommand = m_nNextCommand;
                }
                m_nNextCommand = 0;
                if (m_nLastCommand == 3) {
                    m_nBurstShots--;
                } else {
                    m_nBurstShots = 0;
                }
            } else if (m_nNextCommand == 1) { // 0x62D7B8 - Back to aiming
                m_pAnimAssoc->SetCurrentTime(ANIM_LOOP_START);
                m_pAnimAssoc->SetFlag(ANIMATION_IS_PLAYING, false);
                m_nLastCommand = 1;
                m_nNextCommand = 0;
                m_nAttackTimer = -1;
            }
        }
    }

    // 0x62D7D8
    if (m_nLastCommand != 0 && m_nLastCommand < 4 && m_pAnimAssoc) {
        if (m_pAnimAssoc->m_BlendAmount > 0.5f || (m_pAnimAssoc->m_BlendDelta > 0.f && !m_pAnimAssoc->IsPlaying())) {
            AimGun(ped);
        }
    }

    // 0x62D818
    const auto timeStepMs = [] { return (int32)(CTimer::GetTimeStep() * 0.02f * 1000.f); };
    if (m_nAttackTimer < 0) {
        m_nAttackTimer = (int16)GetRandomNumberInRangeUnchecked(200, (100 - m_nFrequencyPercentage) * 100);
    } else {
        m_nAttackTimer = (int16)(m_nAttackTimer - timeStepMs());
    }

    if (ped->IsPlayer() && m_nNextCommand < 4) { // 0x62D871
        m_nAttackTimer = 0;
        m_nNextCommand = ped->AsPlayer()->GetPadFromPlayer()->GetWeapon(ped) ? 2 : 1;
    } else if (m_nNextCommand == 0) {
        switch (m_nLastCommand) {
        case 1: { // 0x62D8A9
            if (m_nFrequencyPercentage == 0 && m_bInWeaponRange) {
                m_nNextCommand = 1;
            } else if (m_bInRangeToShoot) {
                m_nNextCommand = 3;
            } else if (m_nAttackTimer <= 0) {
                m_nAttackTimer = (int16)(timeStepMs() * 2);
            }
            break;
        }
        case 6:
        case 7:
            m_nNextCommand = 7;
            break;
        default: // 0x62D8EF
            if (m_bInWeaponRange) {
                m_nNextCommand = 1;
            }
            break;
        }
    }

    // 0x62D8FA
    if (m_nLastCommand != 4 && !m_pAnimAssoc && (m_nLastCommand == 7 || m_nNextCommand == 7)) {
        m_bIsFinished = true;
    }

    if (ped->IsPlayer()) {
        ped->GetActiveWeapon().Update(ped);
    }

    const auto& vehSpeed   = ped->m_pVehicle->m_vecMoveSpeed;
    const auto  vehSpeedSq = vehSpeed.x * vehSpeed.x + vehSpeed.y * vehSpeed.y;
    if (vehSpeedSq > 0.5f) {
        ped->Say(CTX_GLOBAL_CAR_DRIVEBY_TOO_FAST);
    } else if (vehSpeedSq < 0.01f) {
        ped->Say(CTX_GLOBAL_CAR_DRIVEBY_BURN_RUBBER);
    }

    return false;
}
