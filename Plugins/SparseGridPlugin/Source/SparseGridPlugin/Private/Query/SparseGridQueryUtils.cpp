// Copyright Epic Games, Inc. All Rights Reserved.

#include "SparseGridQueryUtils.h"
#include "SparseGridManager.h"
#include "SparseGridObject.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

// Static members
UWorld* FSparseGridQueryUtils::WorldContext = nullptr;

void FSparseGridQueryUtils::SetWorldContext(UWorld* InWorld)
{
	WorldContext = InWorld;
}

void FSparseGridQueryUtils::ClearWorldContext()
{
	WorldContext = nullptr;
}

TArray<AActor*> FSparseGridQueryUtils::QueryActorsInSphere(const FVector& Center, float Radius)
{
	TArray<AActor*> Results;
	
	if (!WorldContext)
	{
		return Results;
	}

	// Get the SparseGridManager from the world
	if (USparseGridManager* GridManager = WorldContext->GetSubsystem<USparseGridManager>())
	{
		// Query handles from the grid
		TArray<FSparseGridHandle> Handles = GridManager->QuerySphere(Center, Radius);
		
		// Convert handles to actors
		for (const FSparseGridHandle& Handle : Handles)
		{
			if (ISparseGridObject* Object = GridManager->GetObjectInterface(Handle))
			{
				if (AActor* Actor = Object->GetSparseGridOwnerActor())
				{
					// Verify distance
					if (FVector::DistSquared(Center, Actor->GetActorLocation()) <= Radius * Radius)
					{
						Results.Add(Actor);
					}
				}
			}
		}
	}
	
	return Results;
}

AActor* FSparseGridQueryUtils::FindNearestActor(const FVector& Position, float MaxRadius, AActor* IgnoreActor)
{
	if (!WorldContext)
	{
		return nullptr;
	}

	TArray<AActor*> Actors = QueryActorsInSphere(Position, MaxRadius);
	if (Actors.Num() == 0)
	{
		return nullptr;
	}

	AActor* Nearest = nullptr;
	float MinDist = MAX_FLT;
	
	for (AActor* Actor : Actors)
	{
		if (Actor == IgnoreActor)
		{
			continue;
		}
		
		float Dist = FVector::DistSquared(Position, Actor->GetActorLocation());
		if (Dist < MinDist)
		{
			MinDist = Dist;
			Nearest = Actor;
		}
	}
	
	return Nearest;
}

AActor* FSparseGridQueryUtils::FindNearestActorToRay(const FVector& RayOrigin, const FVector& RayDirection, float MaxDistance)
{
	if (!WorldContext)
	{
		return nullptr;
	}

	FVector RayDir = RayDirection.GetSafeNormal();
	
	TArray<AActor*> Actors = QueryActorsInSphere(RayOrigin, MaxDistance);
	if (Actors.Num() == 0)
	{
		return nullptr;
	}

	AActor* Nearest = nullptr;
	float MinDist = MAX_FLT;

	for (AActor* Actor : Actors)
	{
		FVector ActorPos = Actor->GetActorLocation();
		FVector ToActor = ActorPos - RayOrigin;
		
		float ProjectLength = FVector::DotProduct(ToActor, RayDir);
		if (ProjectLength < 0.0f || ProjectLength > MaxDistance)
		{
			continue;
		}

		FVector ClosestPointOnRay = RayOrigin + RayDir * ProjectLength;
		float DistToRay = FVector::DistSquared(ActorPos, ClosestPointOnRay);
		
		if (DistToRay < MinDist)
		{
			MinDist = DistToRay;
			Nearest = Actor;
		}
	}
	
	return Nearest;
}

TArray<FSparseGridRaycastHit> FSparseGridQueryUtils::Raycast(
	const FVector& Origin,
	const FVector& Direction,
	float MaxDistance,
	const TArray<int32>& IgnoreLayers)
{
	if (!WorldContext)
	{
		return TArray<FSparseGridRaycastHit>();
	}

	if (USparseGridManager* GridManager = WorldContext->GetSubsystem<USparseGridManager>())
	{
		FSparseGridRaycastConfig Config;
		Config.MaxDistance = MaxDistance;
		Config.IgnoreLayers = IgnoreLayers;
		Config.bSortByDistance = true;
		Config.bStopAtFirstHit = false;
		
		return GridManager->RaycastThreadSafe(Origin, Direction, Config);
	}
	
	return TArray<FSparseGridRaycastHit>();
}

bool FSparseGridQueryUtils::RaycastSingle(
	const FVector& Origin,
	const FVector& Direction,
	float MaxDistance,
	FSparseGridRaycastHit& OutHit)
{
	if (!WorldContext)
	{
		OutHit.Reset();
		return false;
	}

	if (USparseGridManager* GridManager = WorldContext->GetSubsystem<USparseGridManager>())
	{
		return GridManager->RaycastSingle(Origin, Direction, MaxDistance, OutHit);
	}
	
	OutHit.Reset();
	return false;
}
