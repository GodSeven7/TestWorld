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

	/**
	 * Part C: Dual-tangent solver for honeycomb packing.
	 * When the contact circle is fully blocked, find the position where the agent
	 * is tangent to two adjacent obstacles simultaneously, fitting into the "valley"
	 * between them. This naturally produces honeycomb packing for subsequent layers.
	 */
	static bool SolvePartC(const FVector2D& C, float AgentR, const TArray<FSparseGridArcObstacle>& Obs, float PrefAngle, float MaxR, float& OutAngle, float& OutR)
	{
		if (Obs.Num() < 2) return false;

		// Sort obstacles by angle around C
		struct FIndexedAngle { int32 Index; float Angle; };
		TArray<FIndexedAngle> Sorted;
		Sorted.Reserve(Obs.Num());
		for (int32 i = 0; i < Obs.Num(); ++i)
		{
			FVector2D P(Obs[i].Position.X, Obs[i].Position.Y);
			Sorted.Add({i, FMath::Atan2(P.Y - C.Y, P.X - C.X)});
		}
		Sorted.Sort([](const FIndexedAngle& A, const FIndexedAngle& B) { return A.Angle < B.Angle; });

		// Candidate result
		struct FCandidate { float Angle; float R; };
		TArray<FCandidate> Candidates;

		for (int32 k = 0; k < Sorted.Num(); ++k)
		{
			int32 i = Sorted[k].Index;
			int32 j = Sorted[(k + 1) % Sorted.Num()].Index;

			FVector2D Pi(Obs[i].Position.X, Obs[i].Position.Y);
			FVector2D Pj(Obs[j].Position.X, Obs[j].Position.Y);
			float ri = AgentR + Obs[i].Radius;
			float rj = AgentR + Obs[j].Radius;

			// Two-circle intersection: find points tangent to both obstacle circles
			FVector2D D = Pj - Pi;
			float d = D.Size();
			if (d < KINDA_SMALL_NUMBER) continue;
			if (d > ri + rj + KINDA_SMALL_NUMBER) continue;   // Too far apart
			if (d < FMath::Abs(ri - rj) - KINDA_SMALL_NUMBER) continue; // One inside the other

			float a = (ri * ri - rj * rj + d * d) / (2.0f * d);
			float hSq = ri * ri - a * a;
			if (hSq < 0.f) continue;
			float h = FMath::Sqrt(FMath::Max(0.f, hSq));

			FVector2D Mid = Pi + (a / d) * D;
			FVector2D Perp(-D.Y / d, D.X / d);

			FVector2D Cand1 = Mid + h * Perp;
			FVector2D Cand2 = Mid - h * Perp;

			// Pick the one farther from C (the outer valley between obstacles)
			float Dist1 = (Cand1 - C).SizeSquared();
			float Dist2 = (Cand2 - C).SizeSquared();
			FVector2D BestCand = (Dist1 >= Dist2) ? Cand1 : Cand2;

			float CandR = (BestCand - C).Size();
			if (CandR > MaxR) continue;

			// Validate: no collision with any obstacle
			bool bValid = true;
			for (const auto& O : Obs)
			{
				FVector2D OP(O.Position.X, O.Position.Y);
				if ((BestCand - OP).Size() < AgentR + O.Radius - KINDA_SMALL_NUMBER)
				{
					bValid = false;
					break;
				}
			}
			if (!bValid) continue;

			Candidates.Add({FMath::Atan2(BestCand.Y - C.Y, BestCand.X - C.X), CandR});
		}

		if (Candidates.Num() == 0) return false;

		// Select best: minimize R primarily, then angular distance to preferred direction as tiebreaker
		// Score = R + AngDist * Scale, where Scale is small enough that R dominates
		// but large enough to break ties in favor of preferred direction
		constexpr float AngularScale = 1.0f;
		FCandidate Best = Candidates[0];
		float BestScore = MAX_FLT;
		for (const FCandidate& Cand : Candidates)
		{
			float AngDist = AngularDistance(Cand.Angle, PrefAngle);
			float Score = Cand.R + AngDist * AngularScale;
			if (Score < BestScore)
			{
				BestScore = Score;
				Best = Cand;
			}
		}

		OutAngle = Best.Angle;
		OutR = Best.R;
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
	float FinalR = ContactR;

	bool bA = ArcClipSolver::SolvePartA(C, ContactR, AgentRadius, Obstacles, PrefAngle, BestAngle);
	if (bA)
	{
		// Phase 1: free arc found on contact circle, use Part A + Part B
		if (!ArcClipSolver::SolvePartB(C, ContactR, AgentRadius, Obstacles, BestAngle, MaxR, FinalR))
			return false;
	}
	else
	{
		// Phase 2: contact circle fully blocked — try Part C (dual-tangent honeycomb packing)
		float PartCAngle, PartCR;
		if (ArcClipSolver::SolvePartC(C, AgentRadius, Obstacles, PrefAngle, MaxR, PartCAngle, PartCR))
		{
			BestAngle = PartCAngle;
			FinalR = PartCR;
		}
		else
		{
			// Fallback: preferred angle + Part B push-out
			BestAngle = PrefAngle;
			if (!ArcClipSolver::SolvePartB(C, ContactR, AgentRadius, Obstacles, BestAngle, MaxR, FinalR))
				return false;
		}
	}

	OutPosition = FVector(
		TargetPosition.X + FMath::Cos(BestAngle) * FinalR,
		TargetPosition.Y + FMath::Sin(BestAngle) * FinalR,
		TargetPosition.Z);
	return true;
}