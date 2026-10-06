#pragma once

#include "TaskComplex.h"
#include "TaskTimer.h"

class CPed;
class CEvent;

class NOTSA_EXPORT_VTABLE CTaskComplexWaitForPed : public CTaskComplex
{
public:
    static constexpr auto Type = eTaskType::TASK_COMPLEX_WAIT_FOR_PED;

    static void InjectHooks();

    CTaskComplexWaitForPed(CPed* ped, float radius, uint32 timeInMs, bool bRotateOtherPedsToWaitingPed);
    ~CTaskComplexWaitForPed() override;

    CTask*    Clone() const override { return new CTaskComplexWaitForPed{ m_ped, m_radius, m_timeInMs, m_bRotateOtherPedsToWaitingPed }; } // 0x683950
    eTaskType GetTaskType() const override { return Type; }                                                                              // 0x6833C0
    bool      MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override { return true; } // 0x683430
    CTask*    CreateNextSubTask(CPed* ped) override { return nullptr; } // 0x683440
    CTask*    CreateFirstSubTask(CPed* ped) override;
    CTask*    ControlSubTask(CPed* ped) override;

private: // Wrappers for hooks
    // 0x683340
    CTaskComplexWaitForPed* Constructor(CPed* ped, float radius, uint32 timeInMs, bool bRotateOtherPedsToWaitingPed) {
        this->CTaskComplexWaitForPed::CTaskComplexWaitForPed(ped, radius, timeInMs, bRotateOtherPedsToWaitingPed);
        return this;
    }

    // 0x6833D0
    CTaskComplexWaitForPed* Destructor() {
        this->CTaskComplexWaitForPed::~CTaskComplexWaitForPed();
        return this;
    }

public:
    CPed* m_ped;
    float m_radius;
    uint32 m_timeInMs;
    bool m_bRotateOtherPedsToWaitingPed;
private:
    char padding[3];
    CTaskTimer m_timer;
    int32 m_framesToWaitForSettingRotation;
};

VALIDATE_SIZE(CTaskComplexWaitForPed, 0x2C);
