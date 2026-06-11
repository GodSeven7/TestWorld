#pragma once

#include "CoreMinimal.h"
#include "GOAPAgentComponent.h"
#include "WithUnifiedData.h"
#include "GOAPAgentWithUnifiedDataComponent.generated.h"

class UActorUnifiedDataComponent;
class UUnifiedDataExtensionComponent;

UCLASS(ClassGroup = "GOAP", meta = (BlueprintSpawnableComponent))
class TESTWORLD_API UGOAPAgentWithUnifiedDataComponent 
	: public UGOAPAgentComponent
	, public IWithUnifiedData
{
	GENERATED_BODY()

public:
	UGOAPAgentWithUnifiedDataComponent();

	virtual UActorUnifiedDataComponent* GetUnifiedDataComponent() const override;

	virtual int32 GetObjectID() const override;

	virtual FVector QueryWorldPosition() const override;
	virtual bool QueryHasTarget() const override;
	virtual FVector QueryTargetPosition() const override;
	virtual UObject* QueryTargetObject() const override;
	virtual float QueryMoveSpeed() const override;
	virtual int32 QueryFaction() const override;
	virtual bool QueryIsAlive() const override;
	virtual float QueryAttackRange() const override;

	virtual FGOAPWorldState GetGoalState() const override;

	virtual void ApplyWorldPosition(const FVector& Position) override;
	virtual void ApplyWorldRotation(const FRotator& Rotation) override;
	virtual void ApplyForwardVector(const FVector& Forward) override;
	virtual void ApplyContext(const FGOAPAgentContext& Context, float DeltaTime) override;
	virtual void OnTargetFound(UActorUnifiedDataComponent* TargetData) override;
	virtual void OnTargetLost() override;

	UFUNCTION(BlueprintCallable, Category = "GOAP")
	void SetCombatGoalWithTarget(AActor* Target);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UActorUnifiedDataComponent> UnifiedDataComponent;

	UPROPERTY()
	TObjectPtr<UUnifiedDataExtensionComponent> ExtensionComponent;

	// Cached target actor used by QueryTargetObject.
	TWeakObjectPtr<AActor> CachedTargetActor;
};
