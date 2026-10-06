/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include "TaskTimer.h"

class C2dEffect;
class CEntity;
class CPed;

class CAttractorScanner {
public:
    bool       m_bActivated;
    CTaskTimer m_Timer;
    C2dEffect* m_pEffectInUse;       //!< Effect the last event was added for (aka. `m_pPreviousEffect`)
    CEntity*   m_pPreviousEntity;    //!< Entity the last event was added for
    CEntity*   m_Entities[10];       //!< Indexed by `ePedAttractorType`
    C2dEffect* m_Effects[10];        //!< Indexed by `ePedAttractorType`
    float      m_MinDistSq[10];      //!< Indexed by `ePedAttractorType`

public:
    static void InjectHooks();

    void Clear();
    void ScanForAttractorsInRange(const CPed& ped);

private:
    template<typename PtrListType>
    void ScanForAttractorsInPtrList(PtrListType& ptrList, const CPed& ped);
    void AddEffect(C2dEffect* effect, CEntity* entity, const CPed& ped);
    void GetNearestAttractorInRange(C2dEffect*& outEffect, CEntity*& outEntity);

    static CPed* GetNearestPedNotUsingAttractor(C2dEffect* effect);
};

VALIDATE_SIZE(CAttractorScanner, 0x90);
