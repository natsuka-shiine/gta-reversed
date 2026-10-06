#include "StdInc.h"

#include "EntitySeekPosCalculatorStandard.h"

void CEntitySeekPosCalculatorStandard::InjectHooks() {
    RH_ScopedVirtualClass(CEntitySeekPosCalculatorStandard, 0x859DC4, 2);
    RH_ScopedCategory("Tasks/TaskTypes/SeekPosCalculators");

    RH_ScopedVMTInstall(ComputeEntitySeekPos, 0x46af20);
}

// NOTE: 0x46AC10 is not a member of this class - it's `CTaskComplexSeekEntity<CEntitySeekPosCalculatorStandard>`'s constructor
//       (it sets the task vtable 0x859DF8 and only stores this class' vtable [0x859DC8] at +0x40).
//       It's implemented by the template in `TaskComplexSeekEntity.h` and hooked in `TaskComplexSeekEntityStandard.cpp`.
//       This class has no data members, so its own constructor is implicit (inlined everywhere).

// 0x46AF20
void CEntitySeekPosCalculatorStandard::ComputeEntitySeekPos(const CPed& seeker, const CEntity& target, CVector& outPos) {
    outPos = target.GetPosition();
}
