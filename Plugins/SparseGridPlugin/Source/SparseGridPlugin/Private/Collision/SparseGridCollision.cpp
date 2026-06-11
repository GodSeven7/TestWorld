// Copyright Epic Games, Inc. All Rights Reserved.

#include "SparseGridCollision.h"
#include "SparseGridObject.h"
#include "SparseGridManager.h"
#include "GameFramework/Actor.h"
#include "Async/ParallelFor.h"
#include "HAL/PlatformTime.h"

FSparseGridCollisionDetector::FSparseGridCollisionDetector()
{
    CollisionMatrix.DefaultCollisionMask = 0xFFFFFFFF;
}

void FSparseGridCollisionDetector::SetCollisionMatrix(const FSparseGridCollisionMatrix& InMatrix)
{
    CollisionMatrix = InMatrix;
}

void FSparseGridCollisionDetector::ProcessCollisionEvents(TArray<FSparseGridCollisionResult>& Collisions)
{
    if (Collisions.Num() == 0)
    {
        return;
    }

    ParallelFor(Collisions.Num(), [&](int32 i)
    {
        FSparseGridCollisionResult& Collision = Collisions[i];
        if (!Collision.bDispatchEvents)
        {
            return;
        }

        if (ISparseGridObject* ObjA = Collision.ObjectA)
        {
            ObjA->OnSparseGridCollisionBegin(
                Collision.ObjectB,
                Collision.ContactPoint);
        }

        if (ISparseGridObject* ObjB = Collision.ObjectB)
        {
            ObjB->OnSparseGridCollisionBegin(
                Collision.ObjectA,
                Collision.ContactPoint);
        }
    });
}

void FSparseGridCollisionDetector::RemoveCollisionsForObject(ISparseGridObject* Object)
{
    if (!Object)
    {
        return;
    }

    AActor* Actor = Object->GetSparseGridOwnerActor();

    TArray<FSparseGridCollisionPair> ToRemove;
    for (const FSparseGridCollisionPair& Pair : ActiveCollisions)
    {
        if (Actor && (Pair.ActorA == Actor || Pair.ActorB == Actor))
        {
            ToRemove.Add(Pair);
        }
    }

    for (const FSparseGridCollisionPair& Pair : ToRemove)
    {
        ActiveCollisions.Remove(Pair);
    }
}

void FSparseGridCollisionDetector::DetectCollisionsFromObjects(
    const TArray<ISparseGridObject*>& Objects,
    USparseGrid* Grid,
    USparseGridManager* Manager,
    TArray<FSparseGridCollisionResult>& OutNewCollisions,
    TArray<FSparseGridCollisionPair>& OutEndedCollisions,
    TArray<FSparseGridCollisionResult>& OutActiveCollisions)
{
    if (!bCollisionEnabled || Objects.Num() == 0 || !Grid || !Manager)
    {
        return;
    }

    const double StartTime = FPlatformTime::Seconds();
    const float CellSize = Grid->GetCellSize();
    const float InvCellSize = 1.0f / CellSize;

    TMap<FIntVector, TArray<int32>> CellToObjects;
    for (int32 i = 0; i < Objects.Num(); ++i)
    {
        ISparseGridObject* Obj = Objects[i];
        if (!Obj || Obj->IsPendingSparseGridDestroy() || !Obj->EnableSparseGridCollision())
        {
            continue;
        }

        const FVector Pos = Obj->GetSparseGridPosition();
        const FIntVector CellCoord(
            FMath::FloorToInt(Pos.X * InvCellSize),
            FMath::FloorToInt(Pos.Y * InvCellSize),
            0);
        CellToObjects.FindOrAdd(CellCoord).Add(i);
    }

    TArray<TArray<FSparseGridCollisionResult>> ThreadCollisions;
    ThreadCollisions.SetNum(Objects.Num());

    ParallelFor(Objects.Num(), [&](int32 Index)
    {
        ISparseGridObject* ObjectA = Objects[Index];
        if (!ObjectA || ObjectA->IsPendingSparseGridDestroy() || !ObjectA->EnableSparseGridCollision())
        {
            return;
        }

        const float RadiusA = ObjectA->GetSparseGridCollisionRadius();
        const int32 LayerA = ObjectA->GetSparseGridCollisionLayer();
        const FVector PosA = ObjectA->GetSparseGridPosition();
        const int32 ExtraRange = FMath::CeilToInt(RadiusA / CellSize);
        const int32 Range = 1 + ExtraRange;
        const FIntVector CellA(
            FMath::FloorToInt(PosA.X * InvCellSize),
            FMath::FloorToInt(PosA.Y * InvCellSize),
            0);

        TSet<int32> CheckedIndices;

        for (int32 DX = -Range; DX <= Range; ++DX)
        {
            for (int32 DY = -Range; DY <= Range; ++DY)
            {
                const FIntVector NeighborCell(CellA.X + DX, CellA.Y + DY, 0);
                TArray<int32>* NeighborIndices = CellToObjects.Find(NeighborCell);
                if (!NeighborIndices)
                {
                    continue;
                }

                for (int32 NeighborIdx : *NeighborIndices)
                {
                    if (NeighborIdx <= Index || CheckedIndices.Contains(NeighborIdx))
                    {
                        continue;
                    }
                    CheckedIndices.Add(NeighborIdx);

                    ISparseGridObject* ObjectB = Objects[NeighborIdx];
                    if (!ObjectB || ObjectB->IsPendingSparseGridDestroy() || !ObjectB->EnableSparseGridCollision())
                    {
                        continue;
                    }

                    const int32 TeamA = ObjectA->GetTeamFlag();
                    const int32 TeamB = ObjectB->GetTeamFlag();
                    const bool bDispatchEvents = !(TeamA != 0 && TeamA == TeamB);

                    const int32 LayerB = ObjectB->GetSparseGridCollisionLayer();
                    if (!CollisionMatrix.ShouldCollide(LayerA, LayerB))
                    {
                        continue;
                    }

                    const FVector PosB = ObjectB->GetSparseGridPosition();
                    const float RadiusB = ObjectB->GetSparseGridCollisionRadius();
                    const float CombinedRadius = RadiusA + RadiusB;
                    const float DistSq = FVector::DistSquared2D(PosA, PosB);
                    if (DistSq > CombinedRadius * CombinedRadius)
                    {
                        continue;
                    }

                    FSparseGridCollisionResult Result;
                    Result.ObjectA = ObjectA;
                    Result.ObjectB = ObjectB;
                    Result.ActorA = ObjectA->GetSparseGridOwnerActor();
                    Result.ActorB = ObjectB->GetSparseGridOwnerActor();
                    Result.CombinedRadius = CombinedRadius;
                    Result.Distance = FMath::Sqrt(DistSq);
                    Result.ContactPoint = (PosA + PosB) * 0.5f;
                    Result.bDispatchEvents = bDispatchEvents;
                    ThreadCollisions[Index].Add(Result);
                }
            }
        }
    });

    TArray<FSparseGridCollisionResult> PotentialCollisions;
    for (const TArray<FSparseGridCollisionResult>& LocalResults : ThreadCollisions)
    {
        PotentialCollisions.Append(LocalResults);
    }

    TSet<FSparseGridCollisionPair> NewEventActiveCollisions;
    Stats.TotalChecks += PotentialCollisions.Num();

    for (const FSparseGridCollisionResult& Result : PotentialCollisions)
    {
        ISparseGridObject* ObjectA = Result.ObjectA;
        ISparseGridObject* ObjectB = Result.ObjectB;
        if (!ObjectA || !ObjectB)
        {
            continue;
        }

        FSparseGridCollisionPair NewPair(ObjectA->GetSparseGridOwnerActor(), ObjectB->GetSparseGridOwnerActor());
        OutActiveCollisions.Add(Result);

        if (!Result.bDispatchEvents)
        {
            continue;
        }

        NewEventActiveCollisions.Add(NewPair);
        if (!ActiveCollisions.Contains(NewPair))
        {
            OutNewCollisions.Add(Result);
        }
    }

    for (const FSparseGridCollisionPair& OldPair : ActiveCollisions)
    {
        if (!NewEventActiveCollisions.Contains(OldPair) && OldPair.ActorA.IsValid() && OldPair.ActorB.IsValid())
        {
            OutEndedCollisions.Add(OldPair);
        }
    }

    ActiveCollisions = NewEventActiveCollisions;
    Stats.DetectionTimeMs = static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0);
    Stats.ActiveCollisions = ActiveCollisions.Num();
}

FCollisionTickResult FSparseGridCollisionDetector::Tick(USparseGridManager* Manager)
{
    FCollisionTickResult Result;

    if (!bCollisionEnabled || !Manager)
    {
        return Result;
    }

    const double StartTime = FPlatformTime::Seconds();
    TArray<ISparseGridObject*> Objects = Manager->GetAllObjects();
    if (Objects.Num() == 0)
    {
        return Result;
    }

    // Event-only collision pipeline: detect collisions, dispatch events, track active pairs
    TArray<FSparseGridCollisionResult> ActiveCollisionsForEvents;
    TArray<FSparseGridCollisionPair> EndedCollisions;

    DetectCollisionsFromObjects(Objects, Manager->GetSparseGrid(), Manager, Result.NewCollisions, EndedCollisions, ActiveCollisionsForEvents);
    ProcessCollisionEvents(ActiveCollisionsForEvents);

    Result.EndedCollisions = MoveTemp(EndedCollisions);
    Stats.DetectionTimeMs = static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0);
    Stats.ActiveCollisions = ActiveCollisions.Num();

    return Result;
}
