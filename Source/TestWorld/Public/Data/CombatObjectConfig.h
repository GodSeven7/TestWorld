// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActorPool.h"
#include "Actors/GamePawn.h"
#include "CombatObjectConfig.generated.h"

USTRUCT(BlueprintType)
struct TESTWORLD_API FCombatObjectConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    int32 TypeID = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    FString TypeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class")
    TSubclassOf<AGamePawn> ActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
    int32 DefaultFaction = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float DefaultMaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float DefaultAttack = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float DefaultDefense = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float DefaultMoveSpeed = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Pool")
    FActorPoolConfig PoolConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Pool")
    bool bUseObjectPool = true;

    bool IsValid() const
    {
        return TypeID >= 0 && ActorClass != nullptr;
    }
};
