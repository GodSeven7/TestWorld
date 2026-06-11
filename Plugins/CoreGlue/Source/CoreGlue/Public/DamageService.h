#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DamageTypes.h"
#include "DamageService.generated.h"

USTRUCT(BlueprintType)
struct FProcessedDamageResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    AActor* Target = nullptr;

    UPROPERTY(BlueprintReadOnly)
    AActor* Instigator = nullptr;

    UPROPERTY(BlueprintReadOnly)
    float RawDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float FinalDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float DamageMitigated = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bIsCritical = false;

    UPROPERTY(BlueprintReadOnly)
    bool bKilled = false;
};

UINTERFACE(MinimalAPI, Blueprintable)
class UDamageService : public UInterface
{
    GENERATED_BODY()
};

class COREGLUE_API IDamageService
{
    GENERATED_BODY()

public:
    virtual FProcessedDamageResult ApplyDamageHit(
        AActor* Target,
        float RawDamage,
        EDamageType DamageType,
        AActor* Instigator,
        const FVector& ImpactLocation,
        bool bCanCritical = true) = 0;

    virtual int32 FlushDamageNotifications() = 0;

    virtual int32 FlushPendingKills() = 0;
};
