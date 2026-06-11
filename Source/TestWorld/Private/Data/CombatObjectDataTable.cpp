// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/CombatObjectDataTable.h"

bool UCombatObjectDataTable::GetConfigByTypeID(int32 TypeID, FCombatObjectConfig& OutConfig) const
{
    for (const FCombatObjectConfig& Config : CombatObjectConfigs)
    {
        if (Config.TypeID == TypeID)
        {
            OutConfig = Config;
            return true;
        }
    }
    return false;
}

bool UCombatObjectDataTable::HasTypeID(int32 TypeID) const
{
    for (const FCombatObjectConfig& Config : CombatObjectConfigs)
    {
        if (Config.TypeID == TypeID)
        {
            return true;
        }
    }
    return false;
}

TArray<int32> UCombatObjectDataTable::GetAllTypeIDs() const
{
    TArray<int32> TypeIDs;
    TypeIDs.Reserve(CombatObjectConfigs.Num());
    
    for (const FCombatObjectConfig& Config : CombatObjectConfigs)
    {
        TypeIDs.Add(Config.TypeID);
    }
    
    return TypeIDs;
}
