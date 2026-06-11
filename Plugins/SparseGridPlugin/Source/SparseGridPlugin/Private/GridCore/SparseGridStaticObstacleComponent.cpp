// Copyright Epic Games, Inc. All Rights Reserved.

#include "SparseGridStaticObstacleComponent.h"
#include "FlowFieldManager.h"
#include "NavigationGrid.h"
#include "GameFramework/Actor.h"

USparseGridStaticObstacleComponent::USparseGridStaticObstacleComponent()
{
    SetNavigationObstacle(true);
    SetEnableSteering(false);
}

void USparseGridStaticObstacleComponent::BeginPlay()
{
    Super::BeginPlay();

    SetNavigationObstacle(true);
    SetEnableSteering(false);
    SetEnableSparseGridCollision(bRegistersLocalAvoidance);
    SetSparseGridObjectActive(bRegistersLocalAvoidance);

    const FBox Bounds = GetStaticObstacleWorldBounds();
    if (GetSparseGridCollisionRadius() <= 0.0f)
    {
        SetSparseGridCollisionRadius(GetLocalAvoidanceRadiusFromBounds(Bounds));
    }

    RegisterStaticObstacleCells();

    if (bRegistersLocalAvoidance && bAutoRegisterToSparseGrid)
    {
        RegisterToGrid();
    }
}

void USparseGridStaticObstacleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnregisterStaticObstacleCells();
    Super::EndPlay(EndPlayReason);
}

void USparseGridStaticObstacleComponent::RefreshStaticObstacle()
{
    UnregisterStaticObstacleCells();
    RegisterStaticObstacleCells();
}

void USparseGridStaticObstacleComponent::RegisterStaticObstacleCells()
{
    if (!bAffectsFlowField || RegisteredStaticCells.Num() > 0)
    {
        return;
    }

    UWorld* World = GetWorld();
    UFlowFieldManager* FlowFieldManager = World ? World->GetSubsystem<UFlowFieldManager>() : nullptr;
    if (!FlowFieldManager)
    {
        return;
    }

    BuildStaticObstacleCells(RegisteredStaticCells);
    if (RegisteredStaticCells.Num() > 0)
    {
        FlowFieldManager->MarkStaticObstacleCells(RegisteredStaticCells);
    }
}

void USparseGridStaticObstacleComponent::UnregisterStaticObstacleCells()
{
    if (RegisteredStaticCells.Num() == 0)
    {
        return;
    }

    UWorld* World = GetWorld();
    UFlowFieldManager* FlowFieldManager = World ? World->GetSubsystem<UFlowFieldManager>() : nullptr;
    if (FlowFieldManager)
    {
        FlowFieldManager->ClearStaticObstacleCells(RegisteredStaticCells);
    }

    RegisteredStaticCells.Reset();
}

void USparseGridStaticObstacleComponent::BuildStaticObstacleCells(TArray<FIntVector>& OutCells) const
{
    OutCells.Reset();

    UWorld* World = GetWorld();
    const UFlowFieldManager* FlowFieldManager = World ? World->GetSubsystem<UFlowFieldManager>() : nullptr;
    const UNavigationGrid* NavGrid = FlowFieldManager ? FlowFieldManager->GetNavigationGrid() : nullptr;
    if (!NavGrid)
    {
        return;
    }

    const FBox Bounds = GetStaticObstacleWorldBounds();
    if (!Bounds.IsValid)
    {
        return;
    }

    const FIntVector MinCell = NavGrid->WorldToGrid(Bounds.Min);
    const FIntVector MaxCell = NavGrid->WorldToGrid(Bounds.Max);
    TSet<FIntVector> UniqueCells;

    if (Shape == ESparseGridStaticObstacleShape::Circle)
    {
        const FVector Center = Bounds.GetCenter();
        const float HalfCellDiagonal = NavGrid->GetCellSize() * 0.70710678f;
        const float EffectiveRadius = FMath::Max(Bounds.GetExtent().X, Bounds.GetExtent().Y) + HalfCellDiagonal;
        const float EffectiveRadiusSq = EffectiveRadius * EffectiveRadius;

        for (int32 X = MinCell.X; X <= MaxCell.X; ++X)
        {
            for (int32 Y = MinCell.Y; Y <= MaxCell.Y; ++Y)
            {
                const FIntVector Cell(X, Y, 0);
                const FVector CellCenter = NavGrid->GetCellCenter(Cell);
                if (FVector::DistSquared2D(Center, CellCenter) <= EffectiveRadiusSq)
                {
                    UniqueCells.Add(Cell);
                }
            }
        }
    }
    else
    {
        for (int32 X = MinCell.X; X <= MaxCell.X; ++X)
        {
            for (int32 Y = MinCell.Y; Y <= MaxCell.Y; ++Y)
            {
                UniqueCells.Add(FIntVector(X, Y, 0));
            }
        }
    }

    OutCells.Reserve(UniqueCells.Num());
    for (const FIntVector& Cell : UniqueCells)
    {
        OutCells.Add(Cell);
    }
}

FBox USparseGridStaticObstacleComponent::GetStaticObstacleWorldBounds() const
{
    FBox Bounds(ForceInit);

    if (bUseOwnerBounds)
    {
        if (const AActor* Owner = GetOwner())
        {
            Bounds = Owner->GetComponentsBoundingBox(true);
        }
    }

    if (!Bounds.IsValid)
    {
        const FVector Center = GetSparseGridPosition();
        const FVector Extent = (Shape == ESparseGridStaticObstacleShape::Circle)
            ? FVector(FMath::Max(0.0f, Radius), FMath::Max(0.0f, Radius), FMath::Max(1.0f, Radius))
            : BoxExtent.GetAbs();
        Bounds = FBox::BuildAABB(Center, Extent);
    }

    if (AgentClearance > 0.0f)
    {
        Bounds.Min.X -= AgentClearance;
        Bounds.Min.Y -= AgentClearance;
        Bounds.Max.X += AgentClearance;
        Bounds.Max.Y += AgentClearance;
    }

    return Bounds;
}

float USparseGridStaticObstacleComponent::GetLocalAvoidanceRadiusFromBounds(const FBox& Bounds) const
{
    if (!Bounds.IsValid)
    {
        return FMath::Max(1.0f, Radius);
    }

    const FVector Extent = Bounds.GetExtent();
    return FMath::Max(1.0f, FMath::Max(Extent.X, Extent.Y));
}
