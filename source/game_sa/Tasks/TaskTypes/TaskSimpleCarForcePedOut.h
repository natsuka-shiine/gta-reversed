#pragma once

#include "TaskSimple.h"

class CVehicle;
class CPed;
class CEvent;

class NOTSA_EXPORT_VTABLE CTaskSimpleCarForcePedOut : public CTaskSimple {
public:
    static constexpr auto Type = eTaskType::TASK_SIMPLE_CAR_FORCE_PED_OUT;

    static void InjectHooks();

    CTaskSimpleCarForcePedOut(CVehicle* vehicle, int32 door);
    ~CTaskSimpleCarForcePedOut() override;

    CTask*    Clone() const override { return new CTaskSimpleCarForcePedOut{ m_Vehicle, m_Door }; }                                           // 0x649EE0
    eTaskType GetTaskType() const override { return Type; }                                                                                // 0x647770
    bool      MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override { return false; } // 0x647780
    bool      ProcessPed(CPed* ped) override;

private: // Wrappers for hooks
    // 0x647710
    CTaskSimpleCarForcePedOut* Constructor(CVehicle* vehicle, int32 door) {
        this->CTaskSimpleCarForcePedOut::CTaskSimpleCarForcePedOut(vehicle, door);
        return this;
    }

    // 0x647790
    CTaskSimpleCarForcePedOut* Destructor() {
        this->CTaskSimpleCarForcePedOut::~CTaskSimpleCarForcePedOut();
        return this;
    }

private:
    CVehicle* m_Vehicle{}; // 0x8
    int32     m_Door{};    // 0xC - See `eTargetDoor`
};
VALIDATE_SIZE(CTaskSimpleCarForcePedOut, 0x10);
