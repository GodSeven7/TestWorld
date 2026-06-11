#pragma once

#include "CoreMinimal.h"
#include "GameMode/TestWorldGameMode.h"
#include "InputDrivenGameMode.generated.h"

UCLASS()
class AInputDrivenGameMode : public ATestWorldGameMode
{
    GENERATED_BODY()

public:
    AInputDrivenGameMode();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Gameplay|Suspension")
    bool IsGameplaySuspended() const { return bGameplaySuspended; }

protected:
    void UpdateGameplaySuspension();

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Suspension")
	bool bGameSuspendEnabled = false;

    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Suspension")
    float InputIdleThreshold = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay|Suspension")
    bool bGameplaySuspended = false;

    UPROPERTY()
    TObjectPtr<class UWorldGridSubsystem> WorldGridSubsystem;
};
