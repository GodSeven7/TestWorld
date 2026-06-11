#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ProjectileService.generated.h"

USTRUCT(BlueprintType)
struct FProjectileLaunchConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    float Speed = 2000.0f;

    UPROPERTY(BlueprintReadWrite)
    float Height = 200.0f;

    UPROPERTY(BlueprintReadWrite)
    float Damage = 10.0f;

    UPROPERTY(BlueprintReadWrite)
    float CollisionRadius = 10.0f;

    UPROPERTY(BlueprintReadWrite)
    float MaxLifetime = 5.0f;

    UPROPERTY(BlueprintReadWrite)
    int32 Faction = 0;

    UPROPERTY(BlueprintReadWrite)
    float GroundHeight = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    float CollisionHeightThreshold = 50.0f;

    UPROPERTY(BlueprintReadWrite)
    float DestroyDelay = 3.0f;
};

USTRUCT(BlueprintType)
struct FProjectileLaunchParams
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FVector Origin = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite)
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite)
    FProjectileLaunchConfig Config;

    UPROPERTY(BlueprintReadWrite)
    AActor* Instigator = nullptr;
};

UINTERFACE(MinimalAPI, Blueprintable)
class UProjectileService : public UInterface
{
    GENERATED_BODY()
};

class COREGLUE_API IProjectileService
{
    GENERATED_BODY()

public:
    virtual int32 SpawnProjectile(const FProjectileLaunchParams& Params) = 0;
};
