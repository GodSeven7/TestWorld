#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SpawnerConfig.generated.h"

UENUM(BlueprintType)
enum class ESpawnLocationMode : uint8
{
    FixedPoints UMETA(DisplayName = "Fixed Points"),
    RandomCircle UMETA(DisplayName = "Random Circle")
};

USTRUCT(BlueprintType)
struct TESTWORLD_API FSpawnRule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    int32 TypeID = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|Location")
    ESpawnLocationMode LocationMode = ESpawnLocationMode::FixedPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|Location", meta = (EditCondition = "LocationMode == ESpawnLocationMode::FixedPoints"))
    TArray<FVector> SpawnPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|Location", meta = (EditCondition = "LocationMode == ESpawnLocationMode::RandomCircle"))
    FVector SpawnCenter = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|Location", meta = (EditCondition = "LocationMode == ESpawnLocationMode::RandomCircle"))
    float SpawnRadius = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|Count")
    int32 MaxCount = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|Count")
    int32 SpawnCount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|Timing")
    float SpawnInterval = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|Timing")
    float InitialDelay = 0.0f;

    bool IsValid() const
    {
        return TypeID >= 0 && MaxCount > 0 && SpawnInterval > 0.f;
    }
};

UCLASS(BlueprintType)
class TESTWORLD_API USpawnerConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    TArray<FSpawnRule> SpawnRules;
};
