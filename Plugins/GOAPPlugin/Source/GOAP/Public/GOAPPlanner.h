#pragma once

#include "CoreMinimal.h"
#include "GOAPTypes.h"

class GOAP_API FGOAPPlanner
{
public:
    static bool Plan(
        const FGOAPWorldState& Start,
        const FGOAPWorldState& Goal,
        const TArray<FGOAPActionData>& Actions,
        TArray<int32>& OutPlan,
        int32 MaxSearchNodes = 1000);

    static bool IsGoal(const FGOAPWorldState& Current, const FGOAPWorldState& Goal);

private:
    struct FNode
    {
        FGOAPWorldState State;
        TArray<int32> ActionPath;
        float GCost = 0.0f;
        float HCost = 0.0f;

        float FCost() const { return GCost + HCost; }
        bool operator<(const FNode& Other) const { return FCost() > Other.FCost(); }
    };

    static float Heuristic(const FGOAPWorldState& Current, const FGOAPWorldState& Goal);
};
