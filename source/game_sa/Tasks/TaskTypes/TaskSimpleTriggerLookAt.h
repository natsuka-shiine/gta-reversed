/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include "TaskSimple.h"
#include "AnimBlendAssociation.h"
#include "Entity.h"
#include <Enums/eBoneTag.h>

class CPed;
class CEvent;

class NOTSA_EXPORT_VTABLE CTaskSimpleTriggerLookAt : public CTaskSimple {
public:
    CEntity*   m_pEntity;
    int32      m_time;
    eBoneTag32 m_nOffsetBoneTag;
    CVector    m_vOffsetPos;
    bool       m_bUseTorso;
    float      m_Speed;
    int32      m_nBlendTime;
    bool       m_bEntityExist;
    int8       m_nPriority;

public:
    static void InjectHooks();

    static constexpr auto Type = TASK_SIMPLE_TRIGGER_LOOK_AT;

    CTaskSimpleTriggerLookAt(CEntity* entity,
                             int32 time,
                             eBoneTag32 offsetBoneTag,
                             CVector offsetPos,
                             bool bUseTorso = true,
                             float speed = 0.25f,
                             int32 blendTime = 1000,
                             int32 priority = 3);
    ~CTaskSimpleTriggerLookAt() override;

    CTask*    Clone() const override;
    eTaskType GetTaskType() const override { return Type; }
    bool      MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override { return true; }
    bool      ProcessPed(CPed* ped) override;

private: // Wrappers for hooks
    // 0x634440
    CTaskSimpleTriggerLookAt* Constructor(CEntity* entity, int32 time, eBoneTag32 offsetBoneTag, RwV3d offsetPos, bool bUseTorso, float speed, int32 blendTime, int32 priority) {
        this->CTaskSimpleTriggerLookAt::CTaskSimpleTriggerLookAt(entity, time, offsetBoneTag, offsetPos, bUseTorso, speed, blendTime, priority);
        return this;
    }
    // 0x634500
    CTaskSimpleTriggerLookAt* Destructor() {
        this->CTaskSimpleTriggerLookAt::~CTaskSimpleTriggerLookAt();
        return this;
    }
};

VALIDATE_SIZE(CTaskSimpleTriggerLookAt, 0x30);
