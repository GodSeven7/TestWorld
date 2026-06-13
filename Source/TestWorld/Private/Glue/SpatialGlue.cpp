#include "Glue/SpatialGlue.h"
#include "SparseGridManager.h"
#include "SparseGridObject.h"
#include "SparseGridComponent.h"
#include "SparseGridCollision.h"
#include "FlowFieldManager.h"
#include "CrowdSurroundManager.h"
#include "CrowdSurroundTypes.h"
#include "ActorUnifiedDataComponent.h"
#include "Components/SparseGridWithUnifiedDataComponent.h"
#include "GameFramework/Actor.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "ActorPool.h"

void USpatialGlue::Initialize(UWorld* World)
{
}

void USpatialGlue::Shutdown()
{
}

void USpatialGlue::Bind(
	USparseGridManager* InGridManager,
	UFlowFieldManager* InFlowFieldManager,
	UCrowdSurroundManager* InCrowdSurroundManager)
{
	GridManager = InGridManager;
	FlowFieldManager = InFlowFieldManager;
	CrowdSurroundManager = InCrowdSurroundManager;
}

// Step 1: Update crowd surround assignments
void USpatialGlue::TickCrowdSurround(float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USpatialGlue_TickCrowdSurround);

	if (!CrowdSurroundManager)
		return;

	CrowdSurroundManager->Update(DeltaTime);
}

// Step 3: Compute flow field directions for all grid objects
void USpatialGlue::TickFlowFieldQuery()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USpatialGlue_TickFlowFieldQuery);

	if (!GridManager)
		return;

	GridManager->ComputeFlowFieldDirections();
}

// Step 4: Apply steering corrections and write ComputedVelocity to UnifiedData
void USpatialGlue::TickSteeringCorrection()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USpatialGlue_TickSteeringCorrection);

	if (!GridManager)
		return;

	GridManager->ApplySteeringCorrections();

	TArray<ISparseGridObject*> AllObjects = GridManager->GetAllObjects();
	for (ISparseGridObject* Obj : AllObjects)
	{
		FSparseGridHandle Handle = Obj->GetGridHandle();
		FVector Velocity = GridManager->GetComputedVelocity(Handle);

		USparseGridWithUnifiedDataComponent* Comp = Cast<USparseGridWithUnifiedDataComponent>(Obj);
		if (Comp && Comp->GetUnifiedDataComponent())
		{
			// Always write back ComputedVelocity, including zero velocity.
			// When obstacle AI's ComputedVelocity=ZeroVector, MovementVelocity must be cleared,
			// otherwise the velocity set by GOAP previously won't be removed and AI keeps moving.
			Comp->GetUnifiedDataComponent()->SetMovementVelocity(Velocity);
		}
	}
}

// Step 5: Sync predicted positions to spatial grid
void USpatialGlue::TickSpatialUpdate(float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USpatialGlue_TickSpatialUpdate);

	if (!GridManager)
		return;

	TArray<ISparseGridObject*> Objects = GridManager->GetAllObjects();

	TArray<FSparseGridHandle> Handles;
	TArray<FVector> Positions;

	Handles.Reserve(Objects.Num());
	Positions.Reserve(Objects.Num());

	for (ISparseGridObject* Object : Objects)
	{
		if (!Object)
			continue;

		FVector Position;
		if (USparseGridWithUnifiedDataComponent* UnifiedComp = Cast<USparseGridWithUnifiedDataComponent>(Object))
		{
			Position = UnifiedComp->GetPredictedPosition(DeltaTime);
		}
		else
		{
			Position = Object->GetSparseGridPosition();
		}

		if (USparseGridComponent* Component = Cast<USparseGridComponent>(Object))
		{
			Handles.Add(Component->GetGridHandle());
			Positions.Add(Position);
		}
	}

	if (Handles.Num() > BATCH_THRESHOLD)
	{
		GridManager->BatchUpdatePositions(Handles, Positions);
	}
	else
	{
		for (int32 i = 0; i < Handles.Num(); i++)
		{
			GridManager->UpdateObjectPosition(Handles[i], Positions[i]);
		}
	}
}

// Step 6: Collision detection — event dispatch only, no push vectors
void USpatialGlue::TickCollisionDetection(const TArray<TObjectPtr<UActorUnifiedDataComponent>>& InUnifiedDataComponents)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USpatialGlue_TickCollisionDetection);

	// Collision detection processing (event-only, no push vector writes)
	if (!GridManager)
		return;

	FCollisionTickResult TickResult = GridManager->ProcessCollisions();

	// Dispatch collision begin events
	for (const FSparseGridCollisionResult& Collision : TickResult.NewCollisions)
	{
		if (!Collision.IsValid())
			continue;

		if (ISparseGridObject* ObjectA = Collision.ObjectA)
		{
			if (!ObjectA->IsPendingSparseGridDestroy())
			{
				ObjectA->OnSparseGridCollisionBegin(Collision.ObjectB, Collision.ContactPoint);
			}
		}

		if (ISparseGridObject* ObjectB = Collision.ObjectB)
		{
			if (!ObjectB->IsPendingSparseGridDestroy())
			{
				ObjectB->OnSparseGridCollisionBegin(Collision.ObjectA, Collision.ContactPoint);
			}
		}
	}

	// Dispatch collision end events
	for (const FSparseGridCollisionPair& Pair : TickResult.EndedCollisions)
	{
		AActor* ActorA = Pair.ActorA.Get();
		AActor* ActorB = Pair.ActorB.Get();

		if (!ActorA || !ActorB)
			continue;

		ISparseGridObject* GridObjectA = Cast<ISparseGridObject>(ActorA);
		ISparseGridObject* GridObjectB = Cast<ISparseGridObject>(ActorB);

		if (!GridObjectA || !GridObjectB)
			continue;

		if (!GridObjectA->IsPendingSparseGridDestroy())
		{
			GridObjectA->OnSparseGridCollisionEnd(GridObjectB);
		}

		if (!GridObjectB->IsPendingSparseGridDestroy())
		{
			GridObjectB->OnSparseGridCollisionEnd(GridObjectA);
		}
	}

	// Process objects that need destruction
	for (ISparseGridObject* Object : TickResult.ObjectsToDestroy)
	{
		if (!Object)
			continue;

		AActor* Actor = Cast<AActor>(Object);
		if (Actor && !Actor->IsPendingKillPending())
		{
			if (GridManager)
			{
				GridManager->NotifyActorRemoved(Actor);
			}

			IPoolableActor* PoolableActor = Cast<IPoolableActor>(Actor);
			if (!PoolableActor)
			{
				if (Actor && !Actor->IsPendingKillPending())
				{
					Actor->Destroy();
				}
			}
			else
			{
				PoolableActor->RecycleSelf();
			}
		}
	}
}

// Step 7: Flow field dynamic obstacle and completed field processing
void USpatialGlue::TickNavigationUpdate()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USpatialGlue_TickNavigationUpdate);

	if (FlowFieldManager)
	{
		FlowFieldManager->UpdateDynamicObstacles();
		FlowFieldManager->ProcessCompletedFlowFields();
	}
}

// Step 2: Bridge CrowdSurround locked slots and assignments to UnifiedData CorrectedDesiredPosition.
// Locked agents receive their locked position. Non-locked agents with an active assignment
// receive the assignment's DesiredPosition. Others have their correction cleared.
void USpatialGlue::TickCrowdSurroundCorrection()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USpatialGlue_TickCrowdSurroundCorrection);

	if (!GridManager || !CrowdSurroundManager)
		return;

	TArray<ISparseGridObject*> AllObjects = GridManager->GetAllObjects();
	for (ISparseGridObject* Obj : AllObjects)
	{
		USparseGridWithUnifiedDataComponent* Comp = Cast<USparseGridWithUnifiedDataComponent>(Obj);
		if (!Comp || !Comp->GetUnifiedDataComponent())
			continue;

		UActorUnifiedDataComponent* UnifiedData = Comp->GetUnifiedDataComponent();

		int32 ObjectID = Obj->GetObjectID();
		if (ObjectID == INDEX_NONE)
			continue;

		// Locked agents: use their locked position directly
		FVector LockedPos;
		if (CrowdSurroundManager->GetFixedSlotPosition(ObjectID, LockedPos))
		{
			UnifiedData->SetCorrectedDesiredPosition(LockedPos);
			continue;
		}

		// Non-locked agents with an assignment that requires movement
		FCrowdSurroundAssignment Assignment;
		if (CrowdSurroundManager->GetSurroundAssignment(ObjectID, Assignment)
			&& Assignment.bShouldMove)
		{
			UnifiedData->SetCorrectedDesiredPosition(Assignment.DesiredPosition);
		}
		else
		{
			UnifiedData->ClearCorrectedDesiredPosition();
		}
	}
}

void USpatialGlue::NotifyFactionChanged(UActorUnifiedDataComponent* Component, int32 OldFaction, int32 NewFaction)
{
	// Sync update to SparseGridManager (same frame, zero latency)
	if (GridManager && Component)
	{
		AActor* Owner = Component->GetOwner();
		if (Owner)
		{
			if (USparseGridWithUnifiedDataComponent* SparseComp =
				Owner->FindComponentByClass<USparseGridWithUnifiedDataComponent>())
			{
				FSparseGridHandle Handle = SparseComp->GetGridHandle();
				GridManager->UpdateObjectFaction(Handle, OldFaction, NewFaction);
			}
		}
	}
}
