#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "DamageTypes.h"
#include "CombatTypes.generated.h"

USTRUCT(BlueprintType)
struct FWeaponStats : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Core")
    float Damage = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Core")
    EDamageType DamageType = EDamageType::Physical;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Critical")
    float CritChance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Critical")
    float CritMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Effects")
    float Lifesteal = 0.0f;

    // 武器默认攻击技能（方案A：武器决定技能），编辑器中下拉选择 SkillTable 行名
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Skill")
    FDataTableRowHandle DefaultSkillHandle;
};

USTRUCT(BlueprintType)
struct FCharacterCombatConfig : public FTableRowBase
{
    GENERATED_BODY()

    // 角色的技能列表（方案B：角色决定技能），编辑器中下拉选择 SkillTable 行名
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Skill")
    TArray<FDataTableRowHandle> SkillHandles;

    // 角色的默认武器（方案A：武器决定技能），编辑器中下拉选择 WeaponTable 行名
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Weapon")
    FDataTableRowHandle DefaultWeaponHandle;
};

USTRUCT(BlueprintType)
struct FCombatEventData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    AActor* Attacker = nullptr;

    UPROPERTY(BlueprintReadOnly)
    AActor* Target = nullptr;

    UPROPERTY(BlueprintReadOnly)
    FVector ImpactLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float Damage = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bIsCritical = false;

    UPROPERTY(BlueprintReadOnly)
    FName WeaponID;

    UPROPERTY(BlueprintReadOnly)
    EDamageType DamageType = EDamageType::Physical;
};

USTRUCT(BlueprintType)
struct FCombatSnapshotData
{
    GENERATED_BODY()

    UPROPERTY()
    int32 Faction = 0;

    UPROPERTY()
    float LastAttackTime = 0.0f;

    UPROPERTY()
    float LastDamageDealt = 0.0f;

    UPROPERTY()
    float TotalDamageDealt = 0.0f;

    UPROPERTY()
    int32 TotalAttacksCount = 0;

    UPROPERTY()
    int32 TotalHitsCount = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttackDelegate, AActor*, Attacker, const FCombatEventData&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHitDelegate, AActor*, Target, const FCombatEventData&, Event);
