#include "StdInc.h"

#include "TaskUtilityLineUpPedWithCar.h"
#include "VehicleAnimGroupData.h"
#include "Bike.h"
#include "PedPlacement.h"

#include "rtslerp.h"

void CTaskUtilityLineUpPedWithCar::InjectHooks() {
    RH_ScopedClass(CTaskUtilityLineUpPedWithCar);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x64FBB0);
    RH_ScopedInstall(Destructor, 0x64FC00);

    RH_ScopedInstall(GetLocalPositionToOpenCarDoor, 0x64FC10);
    RH_ScopedInstall(GetPositionToOpenCarDoor, 0x650A80);
    RH_ScopedInstall(ProcessPed, 0x6513A0);
}

// 0x64FBB0
CTaskUtilityLineUpPedWithCar::CTaskUtilityLineUpPedWithCar(const CVector& offset, int32 time, int32 doorOpenPosType, int32 doorIdx) {
    m_Offset = offset;
    m_fDoorOpenPosZ = -999.99f;
    m_fTime = time;
    m_nDoorOpenPosType = doorOpenPosType;
    m_nDoorIdx = doorIdx;
}

// The following 2 functions seem to have copy elision on the returned CVector, that the compiled functions
// took a vector ptr as their first arg. Now, hopefully our code will compile to the same stuff.
// If not, it might crash here, in that case a wrapper function should be used.

// 0x64FC10
CVector CTaskUtilityLineUpPedWithCar::GetLocalPositionToOpenCarDoor(CVehicle* vehicle, float animProgress, CAnimBlendAssociation* assoc) {
    const auto mi = vehicle->GetVehicleModelInfo();

    const auto seatOffset = vehicle->vehicleFlags.bIsVan && (m_nDoorIdx == TARGET_DOOR_REAR_LEFT || m_nDoorIdx == TARGET_DOOR_REAR_RIGHT)
        ? 0.0f
        : animProgress * vehicle->m_pHandlingData->m_fSeatOffsetDistance;

    // Figure out which of the anim group's door offsets to use
    // NOTE: For anims not listed here the PC version leaves this value uninitialized (and then passes it to `ComputeAnimDoorOffsets`),
    //       the Android version treats them the same way as the `ENTER_FRONT` ones, so that's what's done here too.
    auto doorOffsetType = ENTER_FRONT;
    if (assoc) {
        switch (assoc->m_AnimId) {
        case ANIM_ID_DEFAULT_CAR_CRAWLOUTRHS_0:
        case ANIM_ID_DEFAULT_CAR_CRAWLOUTRHS_1:
        case ANIM_ID_CAR_GETOUT_LHS_0:
        case ANIM_ID_CAR_GETOUT_RHS_0:
        case ANIM_ID_CAR_GETOUT_LHS_1:
        case ANIM_ID_CAR_GETOUT_RHS_1:
        case ANIM_ID_CAR_CLOSE_LHS_0:
        case ANIM_ID_CAR_CLOSE_RHS_0:
        case ANIM_ID_CAR_CLOSE_LHS_1:
        case ANIM_ID_CAR_CLOSE_RHS_1:
        case ANIM_ID_CAR_FALLOUT_LHS:
        case ANIM_ID_CAR_FALLOUT_RHS:
            doorOffsetType = EXIT_FRONT;
            break;
        default: // [ANIM_ID_CAR_ALIGN_LHS, ANIM_ID_CAR_SHUFFLE_RHS_1], ANIM_ID_CAR_JACKED(L/R)HS, ANIM_ID_CAR_DOORLOCKED_(L/R)HS
            break;
        }
    }
    switch (m_nDoorIdx) {
    case TARGET_DOOR_DRIVER:
    case TARGET_DOOR_FRONT_RIGHT: doorOffsetType = ENTER_FRONT;      break;
    case TARGET_DOOR_REAR_LEFT:
    case TARGET_DOOR_REAR_RIGHT:  doorOffsetType = ENTER_REAR;       break;
    case TARGET_DOOR_UNK:         doorOffsetType = ENTER_BIKE_FRONT; break;
    }

    auto& animGroup = CVehicleAnimGroupData::GetVehicleAnimGroup(vehicle->m_pHandlingData->m_nAnimGroup);

    CVector doorOffset;
    if (assoc && (assoc->m_AnimId == ANIM_ID_CAR_DOORLOCKED_LHS || assoc->m_AnimId == ANIM_ID_CAR_DOORLOCKED_RHS) && animGroup.m_specialFlags.bRunSpecialLockedDoor) {
        // Blend between this group's and the default group's offsets
        const auto groupOffset   = animGroup.ComputeAnimDoorOffsets(doorOffsetType) * (1.0f - assoc->m_BlendAmount);
        const auto defaultOffset = CVehicleAnimGroupData::GetVehicleAnimGroup(0).ComputeAnimDoorOffsets(doorOffsetType) * assoc->m_BlendAmount;
        doorOffset = defaultOffset + groupOffset;
    } else {
        doorOffset = animGroup.ComputeAnimDoorOffsets(doorOffsetType);
    }

    const auto& frontSeatPos = mi->m_pVehicleStruct->m_avDummyPos[mi->m_nVehicleType == VEHICLE_TYPE_BOAT ? DUMMY_LIGHT_FRONT_MAIN : DUMMY_SEAT_FRONT];
    const auto& rearSeatPos  = mi->m_pVehicleStruct->m_avDummyPos[DUMMY_SEAT_REAR];

    CVector seatPos, pos;
    switch (m_nDoorIdx) {
    case TARGET_DOOR_FRONT_RIGHT:
    case TARGET_DOOR_REAR_RIGHT: {
        seatPos      = m_nDoorIdx == TARGET_DOOR_FRONT_RIGHT ? frontSeatPos : rearSeatPos;
        seatPos.x    = seatPos.x + seatOffset;
        doorOffset.x = -doorOffset.x;
        pos          = seatPos - doorOffset;
        break;
    }
    case TARGET_DOOR_DRIVER:
    case TARGET_DOOR_REAR_LEFT: {
        seatPos   = m_nDoorIdx == TARGET_DOOR_DRIVER ? frontSeatPos : rearSeatPos;
        seatPos.x = -(seatPos.x + seatOffset);
        pos       = seatPos - doorOffset;
        break;
    }
    case TARGET_DOOR_UNK: {
        seatPos = frontSeatPos;
        pos     = seatPos + doorOffset;
        break;
    }
    default: {
        seatPos    = frontSeatPos;
        doorOffset = CVector{ 0.0f, 0.0f, 0.0f };
        pos        = seatPos;
        break;
    }
    }

    if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_BIKE && m_nDoorIdx != TARGET_DOOR_UNK) {
        vehicle->AsBike()->GetCorrectedWorldDoorPosition(pos, doorOffset, seatPos);
        pos = vehicle->GetMatrix().InverseTransformPoint(pos); // Back to local space
    }

    return pos;
}

// 0x650A80
CVector CTaskUtilityLineUpPedWithCar::GetPositionToOpenCarDoor(CVehicle* vehicle, float animProgress, CAnimBlendAssociation* assoc) {
    CMatrix localMatrix;
    localMatrix.CopyOnlyMatrix(*vehicle->m_matrix);
    const CVector localPos = GetLocalPositionToOpenCarDoor(vehicle, animProgress, assoc);
    return vehicle->GetPosition() + localMatrix.TransformVector(localPos);
}

// NOTSA: Code inlined (twice) in `ProcessPed`
// Slerps from `from` to `to`, and sets the resulting rotation (with the given position) as the ped's matrix
static void SetPedMatrixFromSlerp(CPed* ped, RtQuat& from, RtQuat& to, float t, const CVector& pos) {
    RtQuatSlerpCache cache;
    RtQuatSetupSlerpCache(&from, &to, &cache);

    RtQuat quat;
    RtQuatSlerp(&quat, &from, &to, t, &cache);

    RwMatrix rwMat;
    RtQuatConvertToMatrix(&quat, &rwMat);

    CMatrix mat;
    mat.UpdateMatrix(&rwMat);
    mat.GetPosition() = pos;
    ped->SetMatrix(mat);
}

// 0x6513A0
bool CTaskUtilityLineUpPedWithCar::ProcessPed(CPed* ped, CVehicle* vehicle, CAnimBlendAssociation* assoc) {
    if (m_nDoorOpenPosType == 0) {
        ped->m_vecMoveSpeed = CVector{ 0.0f, 0.0f, 0.0f };
    }

    // Heading
    bool isVehUpsideDown = false;
    if (vehicle->GetUp().z <= -0.8f) {
        isVehUpsideDown = true;
        ped->m_fAimingRotation = m_nDoorIdx == TARGET_DOOR_FRONT_RIGHT || m_nDoorIdx == TARGET_DOOR_REAR_RIGHT
            ? vehicle->GetHeading() - PI
            : vehicle->GetHeading();
    } else if (m_nDoorIdx == TARGET_DOOR_UNK) {
        ped->m_fAimingRotation = vehicle->GetHeading() + PI;
    } else if (m_nDoorOpenPosType != 2) {
        ped->m_fAimingRotation = vehicle->GetHeading();
    }

    const auto GetAssocProgress = [assoc] {
        return assoc->m_CurrentTime / assoc->m_BlendHier->m_fTotalTime;
    };

    // Figure out the anim progress (used for the door position), and how much to blend the Z position
    float animProgress = 0.0f;
    float zBlend       = 0.0f;
    if (assoc) {
        const auto animId    = static_cast<AnimationId>(assoc->m_AnimId);
        auto&      animGroup = CVehicleAnimGroupData::GetVehicleAnimGroup(vehicle->m_pHandlingData->m_nAnimGroup);
        switch (animId) {
        case ANIM_ID_DEFAULT_CAR_CRAWLOUTRHS_0:
        case ANIM_ID_DEFAULT_CAR_CRAWLOUTRHS_1: {
            animProgress = GetAssocProgress();
            zBlend       = 0.0f;
            break;
        }
        case ANIM_ID_CAR_ALIGN_LHS:
        case ANIM_ID_CAR_ALIGN_RHS:
        case ANIM_ID_CAR_ALIGNHI_LHS:
        case ANIM_ID_CAR_ALIGNHI_RHS: {
            animProgress = 1.0f;

            const auto criticalTime = animGroup.ComputeCriticalBlendTime(animId);
            if (std::fabs(criticalTime) < 10.0f) {
                zBlend = 0.0f;
            } else {
                const auto blendTime = std::fabs(criticalTime) - 11.0f;
                const auto progress  = GetAssocProgress();
                if (criticalTime - 11.0f > 0.0f) {
                    zBlend = progress >= blendTime
                        ? 1.0f
                        : progress / blendTime;
                } else {
                    zBlend = progress >= blendTime
                        ? (progress - blendTime) / (1.0f - blendTime)
                        : 0.0f;
                }
            }
            if (vehicle->m_nModelIndex == MODEL_AT400) {
                zBlend = 1.0f;
            }
            break;
        }
        case ANIM_ID_CAR_OPEN_LHS:
        case ANIM_ID_CAR_OPEN_RHS:
        case ANIM_ID_CAR_OPEN_LHS_1:
        case ANIM_ID_CAR_OPEN_RHS_1: {
            animProgress = 1.0f;
            zBlend       = std::fabs(animGroup.ComputeCriticalBlendTime(animId)) < 10.0f ? 0.0f : 1.0f;
            if (vehicle->m_nModelIndex == MODEL_AT400) {
                zBlend = 1.0f;
            }
            break;
        }
        case ANIM_ID_CAR_GETIN_LHS_0:
        case ANIM_ID_CAR_GETIN_RHS_0:
        case ANIM_ID_CAR_GETIN_LHS_1:
        case ANIM_ID_CAR_GETIN_RHS_1:
        case ANIM_ID_CAR_GETIN_BIKE_FRONT: {
            animProgress = 1.0f - GetAssocProgress();

            const auto criticalTime = animGroup.ComputeCriticalBlendTime(animId);
            const auto blendTime    = std::fabs(criticalTime);
            if (blendTime < 10.0f) {
                const auto progress = GetAssocProgress();
                if (criticalTime > 0.0f) {
                    zBlend = progress >= blendTime
                        ? 1.0f
                        : progress / blendTime;
                } else {
                    zBlend = progress >= blendTime
                        ? (progress - blendTime) / (1.0f - blendTime)
                        : 0.0f;
                }
            } else {
                zBlend = 1.0f;
            }
            if (vehicle->m_nModelIndex == MODEL_AT400) {
                zBlend = 1.0f;
            }
            break;
        }
        case ANIM_ID_CAR_PULLOUT_LHS:
        case ANIM_ID_CAR_PULLOUT_RHS:
        case ANIM_ID_UNKNOWN_15:
        case ANIM_ID_CAR_CLOSE_LHS_0:
        case ANIM_ID_CAR_CLOSE_RHS_0:
        case ANIM_ID_CAR_CLOSE_LHS_1:
        case ANIM_ID_CAR_CLOSE_RHS_1: {
            animProgress = 1.0f;
            zBlend       = 0.0f;
            break;
        }
        case ANIM_ID_CAR_CLOSEDOOR_LHS_0:
        case ANIM_ID_CAR_CLOSEDOOR_RHS_0:
        case ANIM_ID_CAR_CLOSEDOOR_LHS_1:
        case ANIM_ID_CAR_CLOSEDOOR_RHS_1:
        case ANIM_ID_CAR_SHUFFLE_RHS_0:
        case ANIM_ID_CAR_SHUFFLE_RHS_1:
        case ANIM_ID_CAR_ROLLDOOR: {
            animProgress = 0.0f;
            zBlend       = 1.0f;
            break;
        }
        case ANIM_ID_CAR_GETOUT_LHS_0:
        case ANIM_ID_CAR_GETOUT_RHS_0:
        case ANIM_ID_CAR_GETOUT_LHS_1:
        case ANIM_ID_CAR_GETOUT_RHS_1:
        case ANIM_ID_CAR_JACKEDLHS:
        case ANIM_ID_CAR_JACKEDRHS:
        case ANIM_ID_CAR_ROLLOUT_LHS:
        case ANIM_ID_CAR_ROLLOUT_RHS:
        case ANIM_ID_CAR_FALLOUT_LHS:
        case ANIM_ID_CAR_FALLOUT_RHS: {
            animProgress = GetAssocProgress();

            const auto criticalTime = animGroup.ComputeCriticalBlendTime(animId);
            const auto blendTime    = std::fabs(criticalTime);
            const auto progress     = GetAssocProgress();
            if (criticalTime > 0.0f) {
                zBlend = progress >= blendTime
                    ? 0.0f
                    : 1.0f - progress / blendTime;
            } else {
                zBlend = progress >= blendTime
                    ? 1.0f - (progress - blendTime) / (1.0f - blendTime)
                    : 1.0f;
            }
            break;
        }
        case ANIM_ID_CAR_DOORLOCKED_LHS:
        case ANIM_ID_CAR_DOORLOCKED_RHS: {
            animProgress = 1.0f;
            zBlend       = animGroup.m_specialFlags.bRunSpecialLockedDoor ? 1.0f : 0.0f;
            break;
        }
        default:
            break;
        }
    }

    // Position the ped should be at (now), and the position of the ped at the end of the anim (used for finding the ground Z)
    CVector pos = m_nDoorOpenPosType == 2
        ? ped->GetPosition()
        : GetPositionToOpenCarDoor(vehicle, animProgress, assoc);
    CVector doorPos = pos;
    if (!vehicle->IsBike() && m_nDoorOpenPosType != 2) {
        doorPos = GetPositionToOpenCarDoor(vehicle, 1.0f, assoc);
    }

    if (!vehicle->physicalFlags.bSubmergedInWater) {
        const auto right      = vehicle->GetRightVector();
        const auto sideOffset = (doorPos - vehicle->GetPosition()).Dot(right);
        const auto vehPosZ    = vehicle->GetPosition().z;
        const auto maxZ       = vehPosZ - vehicle->GetHeightAboveRoad() + right.z * sideOffset + 1.0f;
        const auto originalZ  = doorPos.z;
        doorPos = std::get<CVector>(CPedPlacement::FindZCoorForPed(doorPos));
        if (maxZ - 0.5f > doorPos.z) { // Found ground is too low
            doorPos.z = originalZ;
        }
    } else if (vehicle->IsBoat() && vehicle->IsUpsideDown()) {
        doorPos.z += 1.0f;
    }
    m_fDoorOpenPosZ = doorPos.z;

    if (m_nDoorOpenPosType == 1 || m_nDoorOpenPosType == 2) { // Let the ped fall (until they reach the ground)
        const auto pedPosZ   = ped->GetPosition().z;
        const auto newZSpeed = ped->m_vecMoveSpeed.z - CTimer::GetTimeStep() * 0.008f;
        if (pedPosZ + newZSpeed >= doorPos.z) {
            ped->m_vecMoveSpeed.z = newZSpeed;
            pos.z                 = ped->GetPosition().z;
        } else {
            pos.z               = doorPos.z;
            ped->m_vecMoveSpeed = CVector{ 0.0f, 0.0f, 0.0f };
        }
    }

    if (m_fDoorOpenPosZ > pos.z) {
        if (vehicle->IsBike() && assoc && assoc->m_AnimId != ANIM_ID_CAR_GETIN_BIKE_FRONT) {
            float t = 0.0f;
            switch (assoc->m_AnimId) {
            case ANIM_ID_CAR_GETOUT_LHS_0:
            case ANIM_ID_CAR_GETOUT_RHS_0:
            case ANIM_ID_CAR_GETOUT_LHS_1:
            case ANIM_ID_CAR_GETOUT_RHS_1: {
                t = 1.0f - animProgress;
                break;
            }
            case ANIM_ID_CAR_GETIN_LHS_0:
            case ANIM_ID_CAR_GETIN_RHS_0:
            case ANIM_ID_CAR_GETIN_LHS_1:
            case ANIM_ID_CAR_GETIN_RHS_1: {
                t = 1.0f >= GetAssocProgress() * 2.0f
                    ? GetAssocProgress() * 2.0f
                    : 1.0f;
                break;
            }
            default:
                break;
            }
            const auto vehPosZ = vehicle->GetPosition().z;
            pos.z = (vehPosZ - vehicle->GetHeightAboveRoad() + 1.0f - m_fDoorOpenPosZ) * t + m_fDoorOpenPosZ;
        } else {
            pos.z = (pos.z - m_fDoorOpenPosZ) * zBlend + m_fDoorOpenPosZ;
        }
    } else if (m_nDoorOpenPosType == 0) {
        pos.z = (pos.z - m_fDoorOpenPosZ) * zBlend + m_fDoorOpenPosZ;
    }

    // Blend from the initial position/heading towards the target
    if (CTimer::GetTimeInMS() < static_cast<uint32>(m_fTime)) {
        auto       targetRot = CGeneral::LimitRadianAngle(ped->m_fAimingRotation);
        const auto currRot   = ped->m_fCurrentRotation;
        const auto t         = static_cast<float>(static_cast<uint32>(m_fTime) - CTimer::GetTimeInMS()) * (1.0f / 600.0f);
        if (t <= 0.0f) {
            m_Offset.x = 0.0f;
            m_Offset.y = 0.0f;
        }
        m_Offset.z = 0.0f;
        pos -= m_Offset * t;

        if (currRot + PI < targetRot) {
            targetRot -= TWO_PI;
        } else if (currRot - PI > targetRot) {
            targetRot += TWO_PI;
        }
        ped->m_fCurrentRotation = currRot - (currRot - targetRot) * (1.0f - t);
    } else {
        ped->m_fCurrentRotation = ped->m_fAimingRotation;
    }

    if (assoc) {
        switch (assoc->m_AnimId) {
        case ANIM_ID_CAR_GETIN_LHS_0:
        case ANIM_ID_CAR_GETIN_RHS_0:
        case ANIM_ID_CAR_GETIN_LHS_1:
        case ANIM_ID_CAR_GETIN_RHS_1:
        case ANIM_ID_CAR_GETIN_BIKE_FRONT: { // 0x651D43 - Slerp from the ped's orientation to the vehicle's
            CMatrix vehMat{ *vehicle->m_matrix };
            if (assoc->m_AnimId == ANIM_ID_CAR_GETIN_BIKE_FRONT) {
                CMatrix rotMat;
                rotMat.SetRotateZ(PI);
                vehMat *= rotMat;
            }
            RwMatrix vehRwMat;
            vehMat.CopyToRwMatrix(&vehRwMat);
            RtQuat vehQuat;
            RtQuatConvertFromMatrix(&vehQuat, &vehRwMat);

            CMatrix  pedMat{ *ped->m_matrix };
            RwMatrix pedRwMat;
            pedMat.CopyToRwMatrix(&pedRwMat);
            RtQuat pedQuat;
            RtQuatConvertFromMatrix(&pedQuat, &pedRwMat);

            SetPedMatrixFromSlerp(ped, pedQuat, vehQuat, GetAssocProgress(), pos);
            return false;
        }
        case ANIM_ID_CAR_GETOUT_LHS_0:
        case ANIM_ID_CAR_GETOUT_RHS_0:
        case ANIM_ID_CAR_GETOUT_LHS_1:
        case ANIM_ID_CAR_GETOUT_RHS_1:
        case ANIM_ID_UNKNOWN_26: { // 0x6521F2 - Slerp from the vehicle's orientation to the ped's (upright) heading
            CMatrix  vehMat{ *vehicle->m_matrix };
            RwMatrix vehRwMat;
            vehMat.CopyToRwMatrix(&vehRwMat);
            RtQuat vehQuat;
            RtQuatConvertFromMatrix(&vehQuat, &vehRwMat);

            CMatrix pedMat{ *ped->m_matrix };
            pedMat.SetRotateZOnly(ped->m_fCurrentRotation);
            RwMatrix pedRwMat;
            pedMat.CopyToRwMatrix(&pedRwMat);
            RtQuat pedQuat;
            RtQuatConvertFromMatrix(&pedQuat, &pedRwMat);

            SetPedMatrixFromSlerp(ped, vehQuat, pedQuat, GetAssocProgress(), pos);
            return false;
        }
        default:
            break;
        }
    }

    // 0x652640
    if (animProgress <= 0.2f && !isVehUpsideDown && !vehicle->IsBike() && !vehicle->IsSubQuad()) {
        CMatrix mat{ *vehicle->m_matrix };
        mat.GetPosition() += mat.TransformVector(GetLocalPositionToOpenCarDoor(vehicle, 0.0f, assoc));
        ped->SetMatrix(mat);
    } else {
        ped->GetPosition() = pos;
        ped->SetOrientation(0.0f, 0.0f, ped->m_fCurrentRotation);
    }

    return false;
}
