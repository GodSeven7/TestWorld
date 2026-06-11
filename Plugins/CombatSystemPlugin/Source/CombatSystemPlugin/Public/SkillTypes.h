#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "AuraTypes.h"
#include "SkillTypes.generated.h"

UENUM(BlueprintType)
enum class ESkillType : uint8
{
    Attack,
    Movement
};

UENUM(BlueprintType)
enum class ESkillPhase : uint8
{
    Idle,
    Windup,
    Active,
    FollowThrough
};

UENUM(BlueprintType)
enum class EAttackShape : uint8
{
    Circle,
    Cone,
    Rectangle
};

UENUM(BlueprintType)
enum class ETargetStrategy : uint8
{
    Nearest,
    Random
};

USTRUCT(BlueprintType)
struct FCombatSkillConfig : public FTableRowBase
{
    GENERATED_BODY()

    // ── 技能标识 ──
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Identity")
    FString SkillName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Identity")
    ESkillType SkillType = ESkillType::Attack;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Identity")
    int32 Priority = 0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Identity")
    bool bAutoCast = true;

    // ── 技能周期 ──
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Timing")
    float WindupTime = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Timing")
    float Duration = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Timing")
    float FollowThroughTime = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Timing")
    float Cooldown = 0.0f;

    // ── 攻击行为（仅 Attack 类型技能使用）──
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Attack")
    float Range = 100.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Attack")
    EAttackShape AttackShape = EAttackShape::Circle;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Attack")
    float ConeAngle = 90.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Attack")
    float RectangleWidth = 100.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Attack")
    float AreaRadius = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Attack")
    int32 MaxTargets = 1;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Attack")
    ETargetStrategy TargetStrategy = ETargetStrategy::Nearest;

    // ── 命中效果 ──
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Effects")
    TArray<FAuraEffectEntry> OnHitAuras;

    float GetTotalCycleTime() const
    {
        return WindupTime + Duration + FollowThroughTime + Cooldown;
    }
};

USTRUCT()
struct FScheduledSkill
{
    GENERATED_BODY()

    double AvailableTime = 0.0;
    int32 SkillIndex = INDEX_NONE;
    int32 Priority = 0;

    bool operator<(const FScheduledSkill& Other) const
    {
        if (AvailableTime != Other.AvailableTime)
            return AvailableTime < Other.AvailableTime;
        return Priority > Other.Priority;
    }
};
