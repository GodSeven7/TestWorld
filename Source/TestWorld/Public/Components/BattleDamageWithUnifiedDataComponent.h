#pragma once

#include "CoreMinimal.h"
#include "BattleDamageComponent.h"
#include "WithUnifiedData.h"
#include "BattleDamageWithUnifiedDataComponent.generated.h"

class UActorUnifiedDataComponent;
class UUnifiedDataExtensionComponent;

UCLASS(ClassGroup = "BattleDamage", meta = (BlueprintSpawnableComponent))
class TESTWORLD_API UBattleDamageWithUnifiedDataComponent 
	: public UBattleDamageComponent
	, public IWithUnifiedData
{
	GENERATED_BODY()

public:
	UBattleDamageWithUnifiedDataComponent();

	virtual UActorUnifiedDataComponent* GetUnifiedDataComponent() const override;

	virtual const FBattleAttributeData& GetAttributeData() const override;
	virtual FBattleAttributeData& GetMutableAttributeData() override;
	virtual void ApplyDamageResult(const FDamageResult& Result) override;
	virtual int32 GetFaction() const override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UActorUnifiedDataComponent> UnifiedDataComponent;

	UPROPERTY()
	TObjectPtr<UUnifiedDataExtensionComponent> ExtensionComponent;
};
