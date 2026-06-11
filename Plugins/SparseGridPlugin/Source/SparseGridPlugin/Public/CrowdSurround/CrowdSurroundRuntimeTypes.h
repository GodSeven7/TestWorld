#pragma once

#include "CoreMinimal.h"
#include "CrowdSurroundTypes.h"

class USparseGridComponent;

enum class ECrowdAttackAnchorState : uint8
{
    Free,
    // Soft claim while moving toward an attack point. It can be stolen by a better request.
    Reserved,
    // Hard claim for an attacker already in range. It cannot be stolen or wait-assigned.
    Occupied,
    Blocked,
    Cooldown,
};

struct FCrowdAttackAnchor
{
    int32 AnchorID = INDEX_NONE;
    FVector Direction = FVector::ForwardVector;
    FVector Position = FVector::ZeroVector;
    float Radius = 0.0f;
    ECrowdAttackAnchorState State = ECrowdAttackAnchorState::Free;
    int32 OwnerObjectID = INDEX_NONE;
    double ReservationTime = 0.0;
    double LastProgressTime = 0.0;
    float LastProgressDistance = TNumericLimits<float>::Max();
};

struct FCrowdWaitPoint
{
    int32 AnchorID = INDEX_NONE;
    int32 WaitIndex = INDEX_NONE;
    int32 WaitLayer = INDEX_NONE;
    int32 WaitBranchIndex = INDEX_NONE;
    int32 OwnerObjectID = INDEX_NONE;
    int32 ParentObjectID = INDEX_NONE;
    FVector Position = FVector::ZeroVector;
};

struct FCrowdWaitCluster
{
    int32 AnchorID = INDEX_NONE;
    TArray<FCrowdWaitPoint> WaitPoints;
};

// Lightweight agent intent: records which target an agent wants to surround
// and the agent's combat parameters. Replaces the heavy FCrowdSurroundParticipant.
struct FCrowdSurroundAgentIntent
{
    int32 TargetObjectID = INDEX_NONE;
    float AttackRange = 0.0f;
    float CollisionRadius = 0.0f;
    double RequestTime = 0.0;
};

// Locked slot: records an agent that has locked an attack position.
// Used for cross-frame lock preservation and progress tracking.
struct FLockedSurroundSlot
{
    int32 ObjectID = INDEX_NONE;
    FVector LockedPosition = FVector::ZeroVector;
    float CollisionRadius = 0.0f;
    double LockTime = 0.0;
    // Progress tracking for timeout detection
    double LastProgressTime = 0.0;
    float LastProgressDistance = TNumericLimits<float>::Max();
};

struct FCrowdSurroundCandidate
{
    int32 ObjectID = INDEX_NONE;
    FVector Position = FVector::ZeroVector;
    float DistanceToTarget = 0.0f;
    float AttackRange = 0.0f;
    float DesiredAttackRadius = 0.0f;
    float BandMin = 0.0f;
    float BandMax = 0.0f;
    float CollisionRadius = 0.0f;
    float ContactPadding = 0.0f;
    float Spacing = 0.0f;
    double RequestTime = 0.0;
};

struct FCrowdSurroundOccupiedPoint
{
    int32 OwnerObjectID = INDEX_NONE;
    int32 AnchorID = INDEX_NONE;
    int32 WaitIndex = INDEX_NONE;
    FVector Position = FVector::ZeroVector;
    float Radius = 0.0f;
    float ContactPadding = 0.0f;
    bool bAttackPoint = false;
    bool bLockedAttackPoint = false;
};

struct FCrowdSurroundGroup
{
    int32 TargetObjectID = INDEX_NONE;
    TWeakObjectPtr<USparseGridComponent> TargetComponent;
    FVector AnchorPosition = FVector::ZeroVector;
    FVector LastAnchorPosition = FVector::ZeroVector;
    float TargetRadius = 0.0f;
    float AttackBandMin = 0.0f;
    float AttackBandMax = 0.0f;
    float AverageSpacing = 0.0f;
    int32 AttackCapacity = 0;
    int32 AnchorCount = 0;
    TArray<FCrowdAttackAnchor> AttackAnchors;
    TMap<int32, FCrowdWaitCluster> WaitClusters;
};
