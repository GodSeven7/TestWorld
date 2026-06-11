#pragma once

#include "CoreMinimal.h"
#include "UnifiedDataSnapshot.generated.h"

UENUM()
enum class EUnifiedDataDirtyFlag : uint32
{
	None = 0,
	Position = 1 << 0,
	Rotation = 1 << 1,
	ForwardVector = 1 << 2,
	Faction = 1 << 3,
	IsAlive = 1 << 4,
	ObjectType = 1 << 5,
	TargetInfo = 1 << 6,
	IsMoving = 1 << 7,
	All = 0xFF
};

USTRUCT(BlueprintType)
struct COREGLUE_API FTargetInfo
{
	GENERATED_BODY()

	UPROPERTY()
	int32 TargetID = INDEX_NONE;

	UPROPERTY()
	FVector TargetPosition = FVector::ZeroVector;

	UPROPERTY()
	float TargetDistance = 0.0f;

	bool HasTarget() const { return TargetID != INDEX_NONE; }

	void Clear()
	{
		TargetID = INDEX_NONE;
		TargetPosition = FVector::ZeroVector;
		TargetDistance = 0.0f;
	}
};

USTRUCT(BlueprintType)
struct COREGLUE_API FUnifiedDataSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Position = FVector::ZeroVector;

	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY()
	FVector ForwardVector = FVector::ForwardVector;

	UPROPERTY()
	int32 Faction = 0;

	UPROPERTY()
	bool bIsAlive = true;

	UPROPERTY()
	int32 ObjectType = 0;

	UPROPERTY()
	FTargetInfo TargetInfo;

	UPROPERTY()
	FVector MovementVelocity = FVector::ZeroVector;

	UPROPERTY()
	float MoveSpeed = 0.0f;

	UPROPERTY()
	FVector DesiredPosition = FVector::ZeroVector;

	bool bHasDesiredPosition = false;

	// Crowd-corrected destination, takes priority over DesiredPosition when set
	UPROPERTY()
	FVector CorrectedDesiredPosition = FVector::ZeroVector;

	bool bHasCorrectedDesiredPosition = false;

	UPROPERTY()
	bool bIsMoving = false;

	UPROPERTY()
	bool bIsNavigationObstacle = false;

	// Unified object identifier, allocated once at initialization.
	// Used across all plugins as the canonical reference for this object.
	UPROPERTY()
	int32 ObjectID = INDEX_NONE;

	uint32 DirtyFlags = 0;

	bool IsDirty(EUnifiedDataDirtyFlag Flag) const
	{
		return (DirtyFlags & static_cast<uint32>(Flag)) != 0;
	}

	void MarkDirty(EUnifiedDataDirtyFlag Flag)
	{
		DirtyFlags |= static_cast<uint32>(Flag);
	}

	void ClearDirty()
	{
		DirtyFlags = 0;
	}
};
