#pragma once

#include "CoreMinimal.h"
#include "CrowdSurroundTypes.generated.h"

UENUM()
enum class ECrowdSurroundAssignmentType : uint8
{
    None,
    AttackAnchor,
    WaitPoint,
};

UENUM()
enum class ECrowdSurroundAssignmentState : uint8
{
    None,
    MovingToAttack,
    Attacking,
    MovingToWait,
    Waiting,
    Reassigning,
};

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

    ECrowdSurroundAssignmentType Type = ECrowdSurroundAssignmentType::None;
    ECrowdSurroundAssignmentState State = ECrowdSurroundAssignmentState::None;
    int32 AnchorID = -1;
    int32 WaitIndex = -1;
    int32 WaitLayer = -1;
    int32 WaitBranchIndex = -1;
    int32 ParentObjectID = -1;
    FVector DesiredPosition = FVector::ZeroVector;
    FVector PathTargetPosition = FVector::ZeroVector;
    float DirectMoveRadius = 0.0f;
    float DesiredRadius = 0.0f;
    float CurrentRadius = 0.0f;
    float BandMin = 0.0f;
    float BandMax = 0.0f;
    float Spacing = 0.0f;
    bool bHasAssignment = false;
    bool bHasAttackPermission = false;
    bool bHasAttackReservation = false;
    bool bShouldMove = false;
    bool bIsInPosition = false;
    bool bLocksCrowdPosition = false;
    bool bIsRefillCandidate = false;
};
