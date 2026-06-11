#include "PathSmoother.h"

bool FPathSmoother::SmoothPath(
    const UNavigationGrid* NavGrid,
    const TArray<FIntVector>& CellPath,
    TArray<FVector>& OutSmoothedPath)
{
    if (!NavGrid || CellPath.Num() == 0)
        return false;

    if (CellPath.Num() <= 2)
    {
        OutSmoothedPath.Reserve(CellPath.Num());
        for (const FIntVector& Cell : CellPath)
        {
            OutSmoothedPath.Add(NavGrid->GetCellCenter(Cell));
        }
        return true;
    }

    TArray<FIntVector> Waypoints;
    StringPull(NavGrid, CellPath, Waypoints);

    OutSmoothedPath.Reserve(Waypoints.Num());
    for (const FIntVector& Waypoint : Waypoints)
    {
        OutSmoothedPath.Add(NavGrid->GetCellCenter(Waypoint));
    }

    return OutSmoothedPath.Num() >= 2;
}

void FPathSmoother::StringPull(
    const UNavigationGrid* NavGrid,
    const TArray<FIntVector>& CellPath,
    TArray<FIntVector>& OutWaypoints)
{
    if (CellPath.Num() == 0)
        return;

    OutWaypoints.Add(CellPath[0]);

    if (CellPath.Num() == 1)
        return;

    int32 Current = 0;
    while (Current < CellPath.Num() - 1)
    {
        int32 Farthest = Current + 1;

        for (int32 Candidate = CellPath.Num() - 1; Candidate > Current + 1; --Candidate)
        {
            if (HasLineOfSight(NavGrid, CellPath[Current], CellPath[Candidate]))
            {
                Farthest = Candidate;
                break;
            }
        }

        OutWaypoints.Add(CellPath[Farthest]);
        Current = Farthest;
    }
}

bool FPathSmoother::HasLineOfSight(
    const UNavigationGrid* NavGrid,
    const FIntVector& From,
    const FIntVector& To)
{
    int32 DX = To.X - From.X;
    int32 DY = To.Y - From.Y;
    int32 Steps = FMath::Max(FMath::Abs(DX), FMath::Abs(DY));

    if (Steps == 0)
        return true;

    float StepX = static_cast<float>(DX) / static_cast<float>(Steps);
    float StepY = static_cast<float>(DY) / static_cast<float>(Steps);

    for (int32 i = 1; i < Steps; ++i)
    {
        FIntVector Cell(
            FMath::RoundToInt(static_cast<float>(From.X) + StepX * i),
            FMath::RoundToInt(static_cast<float>(From.Y) + StepY * i),
            0
        );

        if (!NavGrid->IsWalkable(Cell))
            return false;
    }

    return true;
}
