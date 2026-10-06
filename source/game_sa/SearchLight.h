#pragma once
#include "Base.h"
class CEntity;
class CVector;

class CSearchLight {
public:

    static void InjectHooks();
    static void SetTravelToPoint();
    static void SetFollowEntity();
    static void SetPathBetween();
    static bool IsLookingAtPos(const CVector& pos, int32 index);
    static void GetOnEntity();
    static bool IsSpottedEntity(uint32 index, const CEntity& entity);
};
