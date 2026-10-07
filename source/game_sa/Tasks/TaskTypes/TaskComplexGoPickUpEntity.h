#pragma once

#include "TaskComplex.h"

class CEntity;
class CPed;

class NOTSA_EXPORT_VTABLE CTaskComplexGoPickUpEntity : public CTaskComplex {
public:
    static constexpr auto Type = TASK_COMPLEX_GO_PICKUP_ENTITY;

    static inline auto& MAX_GOTO_TIME   = StaticRef<uint32, 0x8D2FF8>();
    static inline auto& MAX_PICKUP_TIME = StaticRef<uint32, 0x8D2FFC>();

    static void InjectHooks();

    CTaskComplexGoPickUpEntity(CEntity* entity, AssocGroupId animGroupId);
    ~CTaskComplexGoPickUpEntity() override;

    CTask*    Clone() const override { return new CTaskComplexGoPickUpEntity{ m_pEntity, m_nAnimGroupId }; } // 0x692C80
    eTaskType GetTaskType() const override { return Type; }                                                // 0x691A40
    CTask*    CreateNextSubTask(CPed* ped) override;
    CTask*    CreateFirstSubTask(CPed* ped) override;
    CTask*    ControlSubTask(CPed* ped) override;

private: // Wrappers for hooks
    // 0x6919C0
    CTaskComplexGoPickUpEntity* Constructor(CEntity* entity, AssocGroupId animGroupId) {
        this->CTaskComplexGoPickUpEntity::CTaskComplexGoPickUpEntity(entity, animGroupId);
        return this;
    }

    // 0x691A50
    CTaskComplexGoPickUpEntity* Destructor() {
        this->CTaskComplexGoPickUpEntity::~CTaskComplexGoPickUpEntity();
        return this;
    }

public:
    CEntity*     m_pEntity;                                // 0xC
    CVector      m_vecPosition;                            // 0x10
    CVector      m_vecPickupPosition;                      // 0x1C
    uint32       m_nTimePassedSinceLastSubTaskCreatedInMs; // 0x28
    AssocGroupId m_nAnimGroupId;                           // 0x2C
    bool         m_bAnimBlockReferenced;                   // 0x30
    char         _pad[3];
};

VALIDATE_SIZE(CTaskComplexGoPickUpEntity, 0x34);
