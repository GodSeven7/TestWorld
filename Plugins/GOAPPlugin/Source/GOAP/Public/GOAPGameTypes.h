#pragma once

#include "CoreMinimal.h"
#include "GOAPTypes.h"

namespace EGOAPGameAction
{
    enum Type : int32
    {
        None = 0,
        Patrol = 501,
        Chase = 502,
        Attack = 503,
        ReturnToPatrol = 504,
    };
}

namespace EGOAPGameStateKey
{
    enum Type : int32
    {
        HasTarget = 0,
        TargetInAttackRange = 1,
        TargetOutOfRange = 2,
        HasPatrolPath = 3,
        AtHomeLocation = 4,
        EnemyDead = 5,
        IsPatrolling = 6,
    };
}
