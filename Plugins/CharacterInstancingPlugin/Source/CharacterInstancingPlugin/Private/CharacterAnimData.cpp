#include "CharacterAnimData.h"

UAnimSequence* UCharacterAnimData::GetAnimByState(FName StateName) const
{
    for (const auto& Entry : AnimStates)
    {
        if (Entry.StateName == StateName)
        {
            return Entry.AnimSequence;
        }
    }
    return nullptr;
}

const FCharacterAnimStateEntry* UCharacterAnimData::GetAnimStateEntry(FName StateName) const
{
    for (const auto& Entry : AnimStates)
    {
        if (Entry.StateName == StateName)
        {
            return &Entry;
        }
    }
    return nullptr;
}

bool UCharacterAnimData::HasState(FName StateName) const
{
    return GetAnimByState(StateName) != nullptr;
}

int32 UCharacterAnimData::GetAnimStateIndex(FName StateName) const
{
    for (int32 i = 0; i < AnimStates.Num(); i++)
    {
        if (AnimStates[i].StateName == StateName)
        {
            return i;
        }
    }
    return INDEX_NONE;
}
