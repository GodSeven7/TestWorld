#include "Systems/FactionManager.h"

void UFactionManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UFactionManager::Deinitialize()
{
    RelationMap.Empty();
    Super::Deinitialize();
}

EFactionRelation UFactionManager::GetRelation(int32 FactionA, int32 FactionB) const
{
    if (FactionA == -1 || FactionB == -1)
    {
        if (FactionA == FactionB)
            return EFactionRelation::Neutral;
        return EFactionRelation::Hostile;
    }

    if (const auto* InnerMap = RelationMap.Find(FactionA))
    {
        if (const EFactionRelation* Result = InnerMap->Find(FactionB))
        {
            return *Result;
        }
    }

    if (FactionA != FactionB)
        return EFactionRelation::Hostile;

    return EFactionRelation::Friendly;
}

bool UFactionManager::IsHostile(int32 FactionA, int32 FactionB) const
{
    return GetRelation(FactionA, FactionB) == EFactionRelation::Hostile;
}

bool UFactionManager::IsFriendly(int32 FactionA, int32 FactionB) const
{
    return GetRelation(FactionA, FactionB) == EFactionRelation::Friendly;
}

void UFactionManager::SetRelation(int32 FactionA, int32 FactionB, EFactionRelation Relation)
{
    RelationMap.FindOrAdd(FactionA).Add(FactionB, Relation);
}
