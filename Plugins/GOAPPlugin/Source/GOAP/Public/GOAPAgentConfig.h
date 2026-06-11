#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GOAPAgentConfig.generated.h"

UCLASS(BlueprintType)
class GOAP_API UGOAPAgentConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "GOAP|Movement")
    float MoveSpeed = 400.0f;

    UPROPERTY(EditAnywhere, Category = "GOAP|Combat")
    float AttackRange = 100.0f;

    UPROPERTY(EditAnywhere, Category = "GOAP|Combat")
    float AbandonChaseRange = 1000.0f;

    UPROPERTY(EditAnywhere, Category = "GOAP|Patrol")
    float ArrivalDistance = 50.0f;

    UPROPERTY(EditAnywhere, Category = "GOAP|Patrol")
    float PatrolWaitTime = 2.0f;

    UPROPERTY(EditAnywhere, Category = "GOAP|Patrol")
    float AtHomeDistance = 100.0f;

    UPROPERTY(EditAnywhere, Category = "GOAP|Combat")
    float AttackCooldown = 1.0f;

    UPROPERTY(EditAnywhere, Category = "GOAP|Combat")
    float ApproachMoveSpeedRatio = 0.5f;

    UPROPERTY(EditAnywhere, Category = "GOAP|Planning")
    int32 ActionSetID = 1;
};
