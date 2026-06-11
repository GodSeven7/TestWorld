#include "CrowdSurroundManager.h"
#include "CrowdSurroundUtilities.h"
#include "FlowFieldManager.h"
#include "PathfindingService.h"
#include "SparseGridConfig.h"

int32 UCrowdSurroundManager::CalculateCircularCapacity(float Radius, float Spacing) const
{
    const float EffectiveSpacing = FMath::Max(Spacing, Config ? Config->GetMinCrowdSpacing() : 80.0f);
    if (Radius <= 0.0f || EffectiveSpacing <= 0.0f)
    {
        return 1;
    }

    constexpr float SeparationTolerance = 1.0f;
    const float RequiredSpacing = EffectiveSpacing + SeparationTolerance;
    if (RequiredSpacing > Radius * 2.0f)
    {
        return 1;
    }

    const float HalfAngle = FMath::Asin(FMath::Clamp(RequiredSpacing / (Radius * 2.0f), 0.0f, 1.0f));
    if (HalfAngle <= KINDA_SMALL_NUMBER)
    {
        return 1;
    }

    return FMath::Max(1, FMath::FloorToInt(PI / HalfAngle));
}

bool UCrowdSurroundManager::HasLineOfMovement(const FVector& Position) const
{
    if (Config && !Config->bEnableCrowdWallCheck)
    {
        return true;
    }

    UWorld* World = GetWorld();
    UFlowFieldManager* FlowManager = World ? World->GetSubsystem<UFlowFieldManager>() : nullptr;
    IPathfindingService* PathService = FlowManager ? Cast<IPathfindingService>(FlowManager) : nullptr;
    return !PathService || PathService->IsPointWalkable(CrowdSurroundUtilities::FlattenPosition(Position));
}
