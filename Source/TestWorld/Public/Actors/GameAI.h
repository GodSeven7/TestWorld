// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameCharacter.h"
#include "Data/AIConfig.h"
#include "GameAI.generated.h"

class UGOAPAgentWithUnifiedDataComponent;

UCLASS()
class TESTWORLD_API AGameAI : public AGameCharacter
{
    GENERATED_BODY()

public:
    AGameAI();

    void SetVelocity(const FVector& InVelocity) { Velocity = InVelocity; }
    void SetMovementBounds(const FVector& Min, const FVector& Max);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GOAP")
    UGOAPAgentWithUnifiedDataComponent* GOAPComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
    float PatrolWaitTime = 2.0f;

	UFUNCTION(BlueprintCallable, Category = "Config")
	void SetAIConfig(UAIConfig* InConfig) { AIConfig = InConfig; }

	UFUNCTION(BlueprintPure, Category = "Config")
	UAIConfig* GetAIConfig() const { return AIConfig; }

	float GetConfigPatrolWaitTime() const { return AIConfig ? AIConfig->PatrolWaitTime : 2.0f; }
	float GetConfigPatrolRadius() const { return AIConfig ? AIConfig->PatrolRadius : 500.0f; }
	FVector GetConfigBoundsMin() const { return AIConfig ? AIConfig->ActivityBoundsMin : FVector(-2500.0f); }
	FVector GetConfigBoundsMax() const { return AIConfig ? AIConfig->ActivityBoundsMax : FVector(2500.0f); }

protected:
    virtual void BeginPlay() override;
    
    virtual void OnActorPoolSpawn() override;
    virtual void OnActorPoolReturn() override;
    virtual void ResetActorPoolState() override;

    UPROPERTY()
    FVector Velocity;

    UPROPERTY()
    FVector BoundsMin;

    UPROPERTY()
    FVector BoundsMax;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	UAIConfig* AIConfig;
};
