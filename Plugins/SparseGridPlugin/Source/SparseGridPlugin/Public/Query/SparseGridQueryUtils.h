// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SparseGridRaycast.h"

class AActor;
class ISparseGridObject;

/**
 * Static utility class for SparseGrid queries.
 * Provides a centralized way for plugins to perform spatial queries
 * without direct dependency on WorldGridSubsystem.
 */
class SPARSEGRIDPLUGIN_API FSparseGridQueryUtils
{
public:
	/**
	 * Set the world context for queries.
	 * Called by WorldGridSubsystem during initialization.
	 */
	static void SetWorldContext(UWorld* InWorld);
	
	/**
	 * Clear the world context.
	 * Called during shutdown.
	 */
	static void ClearWorldContext();
	
	/**
	 * Query actors in a sphere.
	 * @param Center Center of the sphere
	 * @param Radius Radius of the sphere
	 * @return Array of actors in the sphere
	 */
	static TArray<AActor*> QueryActorsInSphere(const FVector& Center, float Radius);
	
	/**
	 * Find the nearest actor to a position.
	 * @param Position The position to search from
	 * @param MaxRadius Maximum search radius
	 * @param IgnoreActor Optional actor to ignore
	 * @return The nearest actor, or nullptr if none found
	 */
	static AActor* FindNearestActor(const FVector& Position, float MaxRadius, AActor* IgnoreActor = nullptr);
	
	/**
	 * Find the nearest actor to a ray.
	 * @param RayOrigin Ray start point
	 * @param RayDirection Ray direction (will be normalized)
	 * @param MaxDistance Maximum search distance
	 * @return The nearest actor to the ray, or nullptr if none found
	 */
	static AActor* FindNearestActorToRay(const FVector& RayOrigin, const FVector& RayDirection, float MaxDistance = 10000.0f);
	
	/**
	 * Raycast - return all hits along the ray.
	 * @param Origin Ray start point
	 * @param Direction Ray direction (will be normalized)
	 * @param MaxDistance Maximum ray distance
	 * @param IgnoreLayers Optional layers to ignore
	 * @return Array of hit results
	 */
	static TArray<FSparseGridRaycastHit> Raycast(
		const FVector& Origin,
		const FVector& Direction,
		float MaxDistance = 10000.0f,
		const TArray<int32>& IgnoreLayers = TArray<int32>());
	
	/**
	 * RaycastSingle - return first hit only (optimized).
	 * @param Origin Ray start point
	 * @param Direction Ray direction (will be normalized)
	 * @param MaxDistance Maximum ray distance
	 * @param OutHit Output hit result
	 * @return True if a hit was found
	 */
	static bool RaycastSingle(
		const FVector& Origin,
		const FVector& Direction,
		float MaxDistance,
		FSparseGridRaycastHit& OutHit);
	
	/**
	 * Check if world context is available.
	 */
	static bool HasWorldContext() { return WorldContext != nullptr; }
	
private:
	static UWorld* WorldContext;
};
