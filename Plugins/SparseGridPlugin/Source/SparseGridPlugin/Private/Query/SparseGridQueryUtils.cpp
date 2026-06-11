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
// ---------------------------------------------------------------------------
// Arc-Clipping Position Solver
// ---------------------------------------------------------------------------

namespace ArcClipSolver
{
	struct FArcInterval
	{
		float Start = 0.0f;
		float End = 0.0f;
	};

	static float NormalizeAngle(float Angle)
	{
		constexpr float TwoPi = 2.0f * PI;
		float Result = FMath::Fmod(Angle, TwoPi);
		if (Result < 0.0f) Result += TwoPi;
		return Result;
	}

	static float AngularDistance(float A, float B)
	{
		float Diff = FMath::Abs(NormalizeAngle(A) - NormalizeAngle(B));
		if (Diff > PI) Diff = 2.0f * PI - Diff;
		return Diff;
	}

	static TArray<FArcInterval> MergeArcs(TArray<FArcInterval>&& InArcs)
	{
		TArray<FArcInterval> Unwrapped;
		Unwrapped.Reserve(InArcs.Num() * 2);
		constexpr float TwoPi = 2.0f * PI;
		for (FArcInterval& Arc : InArcs)
		{
			if (Arc.Start <= Arc.End) Unwrapped.Add(Arc);
			else { Unwrapped.Add({Arc.Start, TwoPi}); Unwrapped.Add({0.0f, Arc.End}); }
		}
		Unwrapped.Sort([](const FArcInterval& A, const FArcInterval& B) { return A.Start < B.Start; });
		TArray<FArcInterval> Merged;
		for (const FArcInterval& Arc : Unwrapped)
		{
			if (Merged.Num() == 0 || Arc.Start > Merged.Last().End + KINDA_SMALL_NUMBER) Merged.Add(Arc);
			else Merged.Last().End = FMath::Max(Merged.Last().End, Arc.End);
		}
		return Merged;
	}

	static TArray<FArcInterval> ComplementArcs(const TArray<FArcInterval>& Merged)
	{
		constexpr float TwoPi = 2.0f * PI;
		TArray<FArcInterval> Free;
		if (Merged.Num() == 0) { Free.Add({0.0f, TwoPi}); return Free; }
		if (Merged[0].Start > KINDA_SMALL_NUMBER) Free.Add({0.0f, Merged[0].Start});
		for (int32 i = 1; i < Merged.Num(); ++i)
		{
			if (Merged[i].Start - Merged[i-1].End > KINDA_SMALL_NUMBER)
				Free.Add({Merged[i-1].End, Merged[i].Start});
		}
		if (Merged.Last().End < TwoPi - KINDA_SMALL_NUMBER) Free.Add({Merged.Last().End, TwoPi});
		return Free;
	}

	static bool FindBestAngleInFreeArcs(const TArray<FArcInterval>& FreeArcs, float PreferredAngle, float& OutAngle)
	{
		PreferredAngle = NormalizeAngle(PreferredAngle);
		for (const FArcInterval& Arc : FreeArcs)
		{
			if (PreferredAngle >= Arc.Start - KINDA_SMALL_NUMBER && PreferredAngle <= Arc.End + KINDA_SMALL_NUMBER)
			{
				OutAngle = FMath::Clamp(PreferredAngle, Arc.Start, Arc.End);
				return true;
			}
		}
		float BestAngle = PreferredAngle, BestDist = MAX_FLT;
		for (const FArcInterval& Arc : FreeArcs)
		{
			float D1 = AngularDistance(PreferredAngle, Arc.Start);
			float D2 = AngularDistance(PreferredAngle, Arc.End);
			if (D1 < BestDist) { BestDist = D1; BestAngle = Arc.Start; }
			if (D2 < BestDist) { BestDist = D2; BestAngle = Arc.End; }
		}
		if (BestDist < MAX_FLT) { OutAngle = BestAngle; return true; }
		return false;
	}

	static bool SolvePartA(const FVector2D& C, float R, float AgentR, const TArray<FSparseGridArcObstacle>& Obs, float PrefAngle, float& OutAngle)
	{
		TArray<FArcInterval> Blocked;
		for (const auto& O : Obs)
		{
			FVector2D P(O.Position.X, O.Position.Y);
			float d = (P - C).Size(), s = AgentR + O.Radius;
			if (d > R + s + KINDA_SMALL_NUMBER) continue;
			if (d < KINDA_SMALL_NUMBER || s >= d + R - KINDA_SMALL_NUMBER) { Blocked.Add({0.0f, 2.0f*PI}); break; }
			float k = (R*R + d*d - s*s) / (2*R*d);
			float ha = FMath::Acos(FMath::Clamp(k, -1.f, 1.f));
			float a = FMath::Atan2(P.Y-C.Y, P.X-C.X);
			Blocked.Add({NormalizeAngle(a-ha), NormalizeAngle(a+ha)});
		}
		auto Merged = MergeArcs(MoveTemp(Blocked));
		auto Free = ComplementArcs(Merged);
		if (Free.Num() == 0) return false;
		return FindBestAngleInFreeArcs(Free, PrefAngle, OutAngle);
	}

	static bool SolvePartB(const FVector2D& C, float BaseR, float AgentR, const TArray<FSparseGridArcObstacle>& Obs, float Angle, float MaxR, float& OutR)
	{
		float Rmin = BaseR;
		for (const auto& O : Obs)
		{
			FVector2D D(O.Position.X-C.X, O.Position.Y-C.Y);
			float d = D.Size();
			if (d < KINDA_SMALL_NUMBER) continue;
			float s = AgentR + O.Radius;
			float da = Angle - FMath::Atan2(D.Y, D.X);
			float cp = d*FMath::Cos(da), sp = d*FMath::Sin(da);
			float disc = s*s - sp*sp;
			if (disc < 0.f) continue;
			Rmin = FMath::Max(Rmin, cp + FMath::Sqrt(disc));
		}
		if (Rmin > MaxR) return false;
		OutR = Rmin;
		return true;
	}
}

bool FSparseGridQueryUtils::SolveArcClippingPosition(
	const FVector& TargetPosition, float TargetRadius, float AgentRadius,
	const FVector& PreferredDirection, const TArray<FSparseGridArcObstacle>& Obstacles,
	FVector& OutPosition)
{
	if (TargetRadius < 0.f || AgentRadius <= 0.f) return false;
	const FVector2D C(TargetPosition.X, TargetPosition.Y);
	const FVector2D P = FVector2D(PreferredDirection.X, PreferredDirection.Y).GetSafeNormal();
	const float PrefAngle = FMath::Atan2(P.Y, P.X);
	const float ContactR = TargetRadius + AgentRadius;
	constexpr float MaxR = 5000.f;

	float BestAngle = PrefAngle;
	bool bA = ArcClipSolver::SolvePartA(C, ContactR, AgentRadius, Obstacles, PrefAngle, BestAngle);
	if (!bA) BestAngle = PrefAngle;

	float FinalR = ContactR;
	if (!ArcClipSolver::SolvePartB(C, ContactR, AgentRadius, Obstacles, BestAngle, MaxR, FinalR))
		return false;

	OutPosition = FVector(
		TargetPosition.X + FMath::Cos(BestAngle) * FinalR,
		TargetPosition.Y + FMath::Sin(BestAngle) * FinalR,
		TargetPosition.Z);
	return true;
}