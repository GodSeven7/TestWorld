#pragma once

#include "CoreMinimal.h"
#include "GlueBase.h"
#include "EventBus.h"
#include "BattleDamageTypes.h"
#include "AuraTypes.h"
#include "CombatGlue.generated.h"

class UCombatManager;
class USparseGridManager;
class UBattleDamageManager;
class UAuraManager;
class UActorUnifiedDataComponent;
class UCombatComponent;

UCLASS()
class TESTWORLD_API UCombatGlue : public UGlueBase
{
	GENERATED_BODY()

public:
	virtual void Initialize(UWorld* World) override;
	virtual void Shutdown() override;
	virtual void Tick(float DeltaTime) override;

	void Bind(
		UCombatManager* InCombatManager,
		USparseGridManager* InGridManager,
		UBattleDamageManager* InDamageManager,
		UEventBus* InEventBus);

	UCombatManager* GetCombatManager() const { return CombatManager; }
	UAuraManager* GetAuraManager() const { return AuraManager; }

	// Stage 2: Damage processing
	void TickDamageProcessing();

private:
	UPROPERTY()
	TObjectPtr<UCombatManager> CombatManager;

	UPROPERTY()
	TObjectPtr<USparseGridManager> GridManager;

	UPROPERTY()
	TObjectPtr<UBattleDamageManager> DamageManager;

	UPROPERTY()
	TObjectPtr<UEventBus> EventBus;

	UPROPERTY()
	TObjectPtr<UAuraManager> AuraManager;

	UFUNCTION()
	void OnCombatantDeath(AActor* Combatant, const FDamageInfo& KillingBlow);

	UFUNCTION()
	void OnCombatHitProcessed(AActor* Target, const FProcessedHitResult& Result);

	UActorUnifiedDataComponent* FindUnifiedDataComponent(AActor* Actor) const;
	UCombatComponent* FindCombatComponent(AActor* Actor) const;
	void ActivateExternalMotion(AActor* Actor) const;
};
