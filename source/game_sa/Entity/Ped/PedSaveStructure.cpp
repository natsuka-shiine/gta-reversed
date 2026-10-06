#include "StdInc.h"
#include "PedSaveStructure.h"
#include "EntryExitManager.h"
#include "WeaponInfo.h"
#include "Streaming.h"

void CPedSaveStructure::InjectHooks() {
    RH_ScopedClass(CPedSaveStructure);
    RH_ScopedCategory("Entity/Ped");

    RH_ScopedInstall(Extract, 0x5D44B0);
    RH_ScopedInstall(Construct, 0x5D43D0);
}

// 0x5D44B0
void CPedSaveStructure::Extract(CPed* ped) {
    ped->GetPosition() = m_pos; // NOTE: Doesn't call `SetPosn` (it's virtual)

    ped->m_fHealth           = m_health;
    ped->m_fArmour           = m_armor;
    ped->m_nActiveWeaponSlot = (uint8)m_activeWeaponSlot;
    ped->SetCharCreatedBy(m_createdBy);
    ped->m_nFightingStyle      = (eFightingStyle)m_nFightingStyle;
    ped->m_nAllowedAttackMoves = m_nAllowedAttackMoves;

    for (auto&& [slot, weapon] : rngv::enumerate(m_weapons)) {
        if (weapon.m_Type == WEAPON_UNARMED) {
            continue;
        }

        // NOTE: `GetWeaponInfo` is really called twice in the original code
        if (const auto modelId = CWeaponInfo::GetWeaponInfo(weapon.m_Type, eWeaponSkill::STD)->m_nModelId1; modelId != MODEL_INVALID) {
            CStreaming::RequestModel(modelId, STREAMING_KEEP_IN_MEMORY);
            CStreaming::LoadAllRequestedModels(false);
        }
        if (const auto modelId = CWeaponInfo::GetWeaponInfo(weapon.m_Type, eWeaponSkill::STD)->m_nModelId2; modelId != MODEL_INVALID) {
            CStreaming::RequestModel(modelId, STREAMING_KEEP_IN_MEMORY);
            CStreaming::LoadAllRequestedModels(false);
        }

        ped->GiveWeapon(weapon.m_Type, weapon.m_TotalAmmo, false);
        ped->m_aWeapons[slot].m_AmmoInClip = weapon.m_AmmoInClip; // NOTE: Indexed by the save's slot, not by the one returned by `GiveWeapon`
    }
    ped->SetCurrentWeapon((int32)(uint8)m_activeWeaponSlot);

    ped->SetAreaCode((eAreaCodes)m_areaCode);

    ped->m_pEnex = m_nExitIndex != -1
        ? CEntryExitManager::GetInSlot(m_nExitIndex) // Returns `nullptr` if the slot is free
        : nullptr;
}

// 0x5D43D0
void CPedSaveStructure::Construct(CPed* ped) {
    m_pos                 = ped->GetPosition();
    m_health              = ped->m_fHealth;
    m_armor               = ped->m_fArmour;
    m_createdBy           = (ePedCreatedBy)ped->GetCreatedBy();
    m_activeWeaponSlot    = (int8)ped->m_nActiveWeaponSlot;
    m_areaCode            = (int8)ped->GetAreaCode();
    m_nFightingStyle      = (uint8)ped->m_nFightingStyle;
    m_nAllowedAttackMoves = (uint8)ped->m_nAllowedAttackMoves;

    m_nExitIndex = -1;
    if (const auto enex = ped->m_pEnex) {
        if (enex->GetLinkedOrThis()->m_nArea != AREA_CODE_NORMAL_WORLD) {
            m_nExitIndex = (int32)CEntryExitManager::GetPool()->GetIndex(enex);
        }
    }

    // Raw copy (0x16C bytes), just like the original
    static_assert(sizeof(m_weapons) == sizeof(ped->m_aWeapons));
    std::memcpy(static_cast<void*>(m_weapons), static_cast<const void*>(ped->m_aWeapons.data()), sizeof(m_weapons));
}
