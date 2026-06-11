#pragma once

#include "CoreMinimal.h"
#include "NavigationGrid.h"

class SPARSEGRIDPLUGIN_API FPathSmoother
{
public:
    static bool SmoothPath(
        const UNavigationGrid* NavGrid,
        const TArray<FIntVector>& CellPath,
        TArray<FVector>& OutSmoothedPath);

private:
    static void StringPull(
        const UNavigationGrid* NavGrid,
        const TArray<FIntVector>& CellPath,
        TArray<FIntVector>& OutWaypoints);

    static bool HasLineOfSight(
        const UNavigationGrid* NavGrid,
        const FIntVector& From,
        const FIntVector& To);
};
