#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SkillConfig.generated.h"

UCLASS(BlueprintType)
class TESTWORLD_API USkillConfig : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashDistance = 600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashSpeed = 1500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Timing")
	float WindupTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Timing")
	float Duration = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Timing")
	float FollowThroughTime = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Timing")
	float Cooldown = 5.0f;
};
