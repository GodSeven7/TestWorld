// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SparseGridRaycast.generated.h"

class AActor;

/**
 * Raycast hit result containing information about a single hit.
 */
USTRUCT(BlueprintType)
struct SPARSEGRIDPLUGIN_API FSparseGridRaycastHit
{
	GENERATED_BODY()

	/** The actor that was hit */
	UPROPERTY(BlueprintReadOnly)
	AActor* HitActor = nullptr;

	/** The world position where the ray hit */
	UPROPERTY(BlueprintReadOnly)
	FVector HitPoint = FVector::ZeroVector;

	/** The normal at the hit point (approximated for sphere) */
	UPROPERTY(BlueprintReadOnly)
	FVector HitNormal = FVector::UpVector;

	/** The distance from ray origin to hit point */
	UPROPERTY(BlueprintReadOnly)
	float HitDistance = 0.0f;

	/** The layer of the hit object */
	UPROPERTY(BlueprintReadOnly)
	int32 HitLayer = 0;

	/** Check if this hit result is valid */
	bool IsValid() const { return HitActor != nullptr; }

	/** Reset to default values */
	void Reset()
	{
		HitActor = nullptr;
		HitPoint = FVector::ZeroVector;
		HitNormal = FVector::UpVector;
		HitDistance = 0.0f;
		HitLayer = 0;
	}
};

/**
 * Configuration for raycast operations.
 */
USTRUCT(BlueprintType)
struct SPARSEGRIDPLUGIN_API FSparseGridRaycastConfig
{
	GENERATED_BODY()

	/** Maximum distance the ray will travel */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxDistance = 10000.0f;

	/** Layers to ignore during raycast */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> IgnoreLayers;

	/** Whether to sort results by distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSortByDistance = true;

	/** Whether to stop at the first hit (for RaycastSingle optimization) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bStopAtFirstHit = false;

	/** Check if a layer should be ignored */
	bool ShouldIgnoreLayer(int32 Layer) const
	{
		return IgnoreLayers.Contains(Layer);
	}
};

/**
 * Internal structure for DDA algorithm traversal.
 */
struct FSparseGridDDAState
{
	/** Current cell coordinates */
	FIntVector CurrentCell;

	/** Step direction for each axis (+1 or -1) */
	FIntVector Step;

	/** tMax: distance to next cell boundary for each axis */
	FVector tMax;

	/** tDelta: distance along ray to move one cell for each axis */
	FVector tDelta;

	/** Current t value along ray */
	float CurrentT = 0.0f;

	/** Initialize DDA state from ray origin and direction */
	void Initialize(const FVector& RayOrigin, const FVector& RayDir, float CellSize);
	
	/** Step to next cell, returns the axis that was stepped (0=X, 1=Y, 2=Z) */
	int32 StepToNextCell();
};
