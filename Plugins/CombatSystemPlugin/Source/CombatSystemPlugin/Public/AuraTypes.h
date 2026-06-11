#pragma once

#include "CoreMinimal.h"
#include "AuraTypes.generated.h"

UENUM(BlueprintType)
enum class EAuraType : uint8
{
    None,
    Knockback,
    Launch,
    DeathLaunch
};

UENUM(BlueprintType)
enum class EAuraCurve : uint8
{
    Linear,
    EaseOut,
    EaseIn
};

USTRUCT(BlueprintType)
struct FKnockbackParams
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|Knockback")
    float Distance = 200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|Knockback")
    float Duration = 0.2f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|Knockback")
    EAuraCurve CurveType = EAuraCurve::EaseOut;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|Knockback")
    bool bInterruptible = true;
};

USTRUCT(BlueprintType)
struct FLaunchParams
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|Launch")
    float HorizontalDistance = 300.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|Launch")
    float LaunchHeight = 200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|Launch")
    float Duration = 0.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|Launch")
    float RiseTimeRatio = 0.4f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|Launch")
    EAuraCurve CurveType = EAuraCurve::EaseOut;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|Launch")
    bool bInterruptible = true;
};

USTRUCT(BlueprintType)
struct FDeathLaunchParams
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|DeathLaunch")
    float HorizontalDistance = 500.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|DeathLaunch")
    float LaunchHeight = 300.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|DeathLaunch")
    float Duration = 0.8f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|DeathLaunch")
    float RiseTimeRatio = 0.3f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|DeathLaunch")
    EAuraCurve CurveType = EAuraCurve::EaseOut;
};

USTRUCT(BlueprintType)
struct FAuraEffectEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura")
    EAuraType AuraType = EAuraType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura")
    FKnockbackParams KnockbackParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura")
    FLaunchParams LaunchParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura")
    FDeathLaunchParams DeathLaunchParams;
};

USTRUCT()
struct FAuraInstance
{
    GENERATED_BODY()

    int32 AuraID = INDEX_NONE;
    TWeakObjectPtr<AActor> Target;
    TWeakObjectPtr<AActor> Caster;
    FVector StartPosition = FVector::ZeroVector;
    FVector Direction = FVector::ForwardVector;
    float ElapsedTime = 0.0f;
    EAuraType AuraType = EAuraType::None;
    bool bActive = false;

    FKnockbackParams KnockbackParams;
    FLaunchParams LaunchParams;
    FDeathLaunchParams DeathLaunchParams;
};

USTRUCT()
struct FAuraUpdate
{
    GENERATED_BODY()

    TWeakObjectPtr<AActor> Target;
    FVector NewPosition = FVector::ZeroVector;
    EAuraType AuraType = EAuraType::None;
    bool bComplete = false;
};
