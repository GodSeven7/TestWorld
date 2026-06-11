// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatObjectConfig.h"
#include "CombatObjectDataTable.generated.h"

UCLASS(BlueprintType)
class TESTWORLD_API UCombatObjectDataTable : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Objects")
    TArray<FCombatObjectConfig> CombatObjectConfigs;

    bool GetConfigByTypeID(int32 TypeID, FCombatObjectConfig& OutConfig) const;

    const TArray<FCombatObjectConfig>& GetAllConfigs() const { return CombatObjectConfigs; }

    bool HasTypeID(int32 TypeID) const;

    TArray<int32> GetAllTypeIDs() const;
};
