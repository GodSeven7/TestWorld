#pragma once

#include "CoreMinimal.h"
#include "DamageTypes.h"
#include "BattleDamageTypes.generated.h"

USTRUCT(BlueprintType)
struct FBattleAttributeData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CurrentHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AttackPower = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Defense = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MagicResistance = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CriticalChance = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CriticalMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MoveSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AttackSpeed = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AttackRange = 100.0f;
};

USTRUCT(BlueprintType)
struct FDamageInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float BaseDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    EDamageType DamageType = EDamageType::Physical;

    UPROPERTY(BlueprintReadOnly)
    AActor* Instigator = nullptr;

    UPROPERTY(BlueprintReadOnly)
    AActor* Target = nullptr;

    UPROPERTY(BlueprintReadOnly)
    FVector ImpactLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    bool bCanCritical = true;
};

USTRUCT(BlueprintType)
struct FDamageResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float FinalDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float DamageMitigated = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bIsCritical = false;

    UPROPERTY(BlueprintReadOnly)
    bool bKilled = false;

    UPROPERTY(BlueprintReadOnly)
    float Overkill = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float HealthRemaining = 0.0f;
};
