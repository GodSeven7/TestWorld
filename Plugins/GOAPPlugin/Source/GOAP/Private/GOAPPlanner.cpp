#include "GOAPPlanner.h"

bool FGOAPPlanner::Plan(
    const FGOAPWorldState& Start,
    const FGOAPWorldState& Goal,
    const TArray<FGOAPActionData>& Actions,
    TArray<int32>& OutPlan,
    int32 MaxSearchNodes)
{
    OutPlan.Empty();

    if (IsGoal(Start, Goal))
    {
        return true;
    }

    TArray<FNode> OpenSet;
    TSet<uint32> ClosedSet;

    FNode StartNode;
    StartNode.State = Start;
    StartNode.GCost = 0.0f;
    StartNode.HCost = Heuristic(Start, Goal);
    OpenSet.Add(StartNode);

    int32 SearchCount = 0;

    while (OpenSet.Num() > 0 && SearchCount < MaxSearchNodes)
    {
        SearchCount++;

        int32 BestIndex = 0;
        float BestFCost = OpenSet[0].FCost();
        for (int32 i = 1; i < OpenSet.Num(); ++i)
        {
            if (OpenSet[i].FCost() < BestFCost)
            {
                BestFCost = OpenSet[i].FCost();
                BestIndex = i;
            }
        }

        FNode Current = OpenSet[BestIndex];
        OpenSet.RemoveAtSwap(BestIndex);

        uint32 CurrentHash = Current.State.GetHash();
        if (ClosedSet.Contains(CurrentHash))
        {
            continue;
        }
        ClosedSet.Add(CurrentHash);

        if (IsGoal(Current.State, Goal))
        {
            OutPlan = Current.ActionPath;
            return true;
        }

        for (const FGOAPActionData& Action : Actions)
        {
            if (!Action.CheckPreconditions(Current.State))
            {
                continue;
            }

            FNode Neighbor;
            Neighbor.State = Current.State;
            Action.ApplyEffects(Neighbor.State);
            Neighbor.GCost = Current.GCost + Action.BaseCost;
            Neighbor.HCost = Heuristic(Neighbor.State, Goal);
            Neighbor.ActionPath = Current.ActionPath;
            Neighbor.ActionPath.Add(Action.ActionID);

            uint32 NeighborHash = Neighbor.State.GetHash();
            if (ClosedSet.Contains(NeighborHash))
            {
                continue;
            }

            OpenSet.Add(Neighbor);
        }
    }

    return false;
}

float FGOAPPlanner::Heuristic(const FGOAPWorldState& Current, const FGOAPWorldState& Goal)
{
    uint32 DiffBits = (Current.StateBits ^ Goal.StateBits) & Goal.StateBits;
    int32 Diff = 0;
    while (DiffBits)
    {
        Diff += DiffBits & 1u;
        DiffBits >>= 1;
    }
    return static_cast<float>(Diff);
}

bool FGOAPPlanner::IsGoal(const FGOAPWorldState& Current, const FGOAPWorldState& Goal)
{
    return Current.Matches(Goal);
}
