#pragma once

#include "CoreMinimal.h"
#include "CrowdSurroundTypes.h"

class USparseGridComponent;

// Lightweight agent intent: records which target an agent wants to surround
// and the agent's combat parameters.
struct FCrowdSurroundAgentIntent
{
    int32 TargetObjectID = INDEX_NONE;
    float AttackRange = 0.0f;
    float CollisionRadius = 0.0f;
    double RequestTime = 0.0;
};

enum class ECrowdSurroundSlotType : uint8
{
    Attack,
    Wait,
};

// Locked slot: records an agent that has locked an attack or wait position.
// Used by TickCrowdSurroundCorrection to prevent position overlap and by
// arc solver to avoid assigning agents to occupied positions.
struct FLockedSurroundSlot
{
    int32 ObjectID = INDEX_NONE;
    int32 TargetObjectID = INDEX_NONE;
    ECrowdSurroundSlotType Type = ECrowdSurroundSlotType::Attack;
    FVector LockedPosition = FVector::ZeroVector;
    float CollisionRadius = 0.0f;
    double LockTime = 0.0;
};

// Per-target group: aggregates agents surrounding the same target.
struct FCrowdSurroundGroup
{
    int32 TargetObjectID = INDEX_NONE;
    TWeakObjectPtr<USparseGridComponent> TargetComponent;
    FVector AnchorPosition = FVector::ZeroVector;
    float TargetRadius = 0.0f;
};
