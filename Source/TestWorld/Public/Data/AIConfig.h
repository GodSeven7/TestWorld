#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AIConfig.generated.h"

UCLASS(BlueprintType)
class TESTWORLD_API UAIConfig : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, Category = "AI|Behavior")
	float PatrolWaitTime = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Movement")
	FVector ActivityBoundsMin = FVector(-2500.0f, -2500.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, Category = "AI|Movement")
	FVector ActivityBoundsMax = FVector(2500.0f, 2500.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, Category = "AI|Patrol")
	float PatrolRadius = 500.0f;
};
