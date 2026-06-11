#pragma once

#include "CoreMinimal.h"
#include "CombatSkill.h"
#include "DashSkill.generated.h"

UCLASS(Blueprintable)
class TESTWORLD_API UDashSkill : public UCombatSkill
{
    GENERATED_BODY()

public:
    UDashSkill();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float DashDistance = 1000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float DashSpeed = 1500.0f;

    float GetConfigDashDistance() const { return DashDistance; }
    float GetConfigDashSpeed() const { return DashSpeed; }

    virtual bool CanExecute(AActor* Owner) const override;
    virtual void OnSkillStarted(AActor* Owner) override;
    virtual void ExecuteInternal(AActor* Owner, float DeltaTime) override;
    virtual void OnSkillCompleted(AActor* Owner) override;

private:
    FVector DashStartLocation = FVector::ZeroVector;
    FVector DashDirection = FVector::ForwardVector;
    FVector DashTargetLocation = FVector::ZeroVector;
    float DashDistanceTraveled = 0.0f;
};
