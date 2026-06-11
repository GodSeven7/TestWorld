#pragma once

#include "CoreMinimal.h"
#include "ProjectileTypes.generated.h"

UENUM(BlueprintType)
enum class EProjectilePhase : uint8
{
    Flying = 0,
    Collision = 1,
    Destroyed = 2
};

UENUM(BlueprintType)
enum class EProjectileCollisionType : uint8
{
    Penetration = 0,
    Stop = 1,
    Bounce = 2
};

USTRUCT(BlueprintType)
struct PROJECTILEPLUGIN_API FProjectileConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Speed = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Height = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Damage = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CollisionRadius = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxLifetime = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Faction = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float GroundHeight = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CollisionHeightThreshold = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DestroyDelay = 3.0f;
};

USTRUCT(BlueprintType)
struct PROJECTILEPLUGIN_API FProjectileSpawnParams
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FVector Origin = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite)
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite)
    FProjectileConfig Config;

    UPROPERTY(BlueprintReadWrite)
    AActor* Instigator = nullptr;

    UPROPERTY(BlueprintReadWrite)
    int32 ProjectileID = -1;
};

USTRUCT(BlueprintType)
struct PROJECTILEPLUGIN_API FProjectileHitResult
{
    GENERATED_BODY()

    UPROPERTY()
    int32 ProjectileID = -1;

    UPROPERTY()
    int32 InstanceIndex = -1;

    UPROPERTY()
    AActor* HitActor = nullptr;

    UPROPERTY()
    FVector HitLocation = FVector::ZeroVector;

    UPROPERTY()
    FVector HitNormal = FVector::UpVector;

    UPROPERTY()
    float Damage = 0.0f;

    UPROPERTY()
    bool bIsValid = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileHitDelegate, const FProjectileHitResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileSpawnedDelegate, int32, ProjectileID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileDestroyedDelegate, int32, ProjectileID);
