#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WithUnifiedData.h"
#include "CombatComponent.h"
#include "CombatWithUnifiedDataComponent.generated.h"

class UActorUnifiedDataComponent;
class UUnifiedDataExtensionComponent;

UCLASS(ClassGroup = "Combat", meta = (BlueprintSpawnableComponent))
class TESTWORLD_API UCombatWithUnifiedDataComponent : public UCombatComponent, public IWithUnifiedData
{
    GENERATED_BODY()

public:
    UCombatWithUnifiedDataComponent();

    virtual UActorUnifiedDataComponent* GetUnifiedDataComponent() const override;

    virtual FVector GetCombatWorldPosition() const override;
    virtual FVector GetCombatForwardVector() const override;
    virtual int32 GetCombatFaction() const override;
    virtual int32 GetFaction() const override;
    virtual UObject* GetCombatParent() const override;
    virtual void OnCombatAttackExecuted(float Damage, bool bIsCritical) override;
    virtual void OnCombatHitReceived(float Damage) override;

    virtual void SetAttackPhase(EAttackPhase Phase) override;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    TObjectPtr<UActorUnifiedDataComponent> UnifiedDataComponent;

    UPROPERTY()
    TObjectPtr<UUnifiedDataExtensionComponent> ExtensionComponent;
};
