#pragma once

#include "CoreMinimal.h"

class UCombatComponent;
class AActor;

namespace CombatDebug
{
    enum class EWeaponDebugState
    {
        Idle,
        Active,
        FollowThrough
    };

    void DrawDebug(UWorld* World, UCombatComponent* Comp);

    bool IsDebugEnabled();
}
