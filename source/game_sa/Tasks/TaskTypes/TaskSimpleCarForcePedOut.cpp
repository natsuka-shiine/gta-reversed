#include "StdInc.h"

#include "TaskSimpleCarForcePedOut.h"

void CTaskSimpleCarForcePedOut::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleCarForcePedOut, 0x86EE94, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x647710);
    RH_ScopedInstall(Destructor, 0x647790);

    RH_ScopedVMTInstall(Clone, 0x649EE0);
    RH_ScopedVMTInstall(GetTaskType, 0x647770);
    RH_ScopedVMTInstall(MakeAbortable, 0x647780);
    RH_ScopedVMTInstall(ProcessPed, 0x6477F0);
}

// 0x647710
CTaskSimpleCarForcePedOut::CTaskSimpleCarForcePedOut(CVehicle* vehicle, int32 door) :
    m_Vehicle{ vehicle },
    m_Door{ door }
{
    CEntity::SafeRegisterRef(m_Vehicle);
}

// 0x647790
CTaskSimpleCarForcePedOut::~CTaskSimpleCarForcePedOut() {
    CEntity::SafeCleanUpRef(m_Vehicle);
}

// 0x6477F0
bool CTaskSimpleCarForcePedOut::ProcessPed(CPed* ped) {
    if (!m_Vehicle) {
        return true;
    }

    if (!m_Vehicle->IsDriver(ped)) {
        // 0x647829 - If someone is getting out through the opposite door, wait for the
        // occupants that are in the way (the driver, or any passenger before our seat)
        if ((uint8)CCarEnterExit::ComputeOppositeDoorFlag(m_Vehicle, m_Door, false) & m_Vehicle->m_nGettingOutFlags) {
            if (m_Vehicle->m_pDriver) {
                return false;
            }
            const auto psgrIdx = CCarEnterExit::ComputePassengerIndexFromCarDoor(m_Vehicle, m_Door);
            for (int32 i = 0; i < psgrIdx; i++) {
                if (m_Vehicle->m_apPassengers[i]) {
                    return false;
                }
            }
        }
    }

    ped->PositionPedOutOfCollision(m_Door, m_Vehicle, true); // 0x647813, 0x64788A
    return true;
}
