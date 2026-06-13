#pragma once

#include "CoreMinimal.h"
#include "CrowdSurroundTypes.generated.h"

USTRUCT()
struct COREGLUE_API FCrowdSurroundAgentParams
{
    GENERATED_BODY()

    float AttackRange = 0.0f;
    float CollisionRadius = 0.0f;
};

USTRUCT()
struct COREGLUE_API FCrowdSurroundRequest
{
    GENERATED_BODY()

    int32 ObjectID = INDEX_NONE;

    UPROPERTY()
    TObjectPtr<UObject> AgentObject = nullptr;

    UPROPERTY()
    TObjectPtr<UObject> TargetObject = nullptr;

    FCrowdSurroundAgentParams Params;

    bool IsValid() const
    {
        return ObjectID != INDEX_NONE && TargetObject != nullptr;
    }
};

USTRUCT()
struct COREGLUE_API FCrowdSurroundAssignment
{
    GENERATED_BODY()

    // Output: where the agent should move to
    FVector DesiredPosition = FVector::ZeroVector;

    // Distance threshold for direct movement (bypass flow field)
    float DirectMoveRadius = 0.0f;

    bool bHasAssignment = false;
    bool bShouldMove = false;
    bool bIsInPosition = false;
    bool bLocksCrowdPosition = false;
};
