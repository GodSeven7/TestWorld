#include "Components/BattleDamageWithUnifiedDataComponent.h"
#include "ActorUnifiedDataComponent.h"
#include "Core/UnifiedDataExtensionComponent.h"
#include "GameFramework/Actor.h"

UBattleDamageWithUnifiedDataComponent::UBattleDamageWithUnifiedDataComponent()
{
}

void UBattleDamageWithUnifiedDataComponent::BeginPlay()
{
	UnifiedDataComponent = GetOwner()->FindComponentByClass<UActorUnifiedDataComponent>();
	ExtensionComponent = GetOwner()->FindComponentByClass<UUnifiedDataExtensionComponent>();
	if (ExtensionComponent)
	{
		ExtensionComponent->GetMutableBattleAttributes() = Attributes;
	}

	Super::BeginPlay();
}

UActorUnifiedDataComponent* UBattleDamageWithUnifiedDataComponent::GetUnifiedDataComponent() const
{
	return UnifiedDataComponent;
}

const FBattleAttributeData& UBattleDamageWithUnifiedDataComponent::GetAttributeData() const
{
	if (ExtensionComponent)
	{
		return ExtensionComponent->GetBattleAttributes();
	}
	return Super::GetAttributeData();
}

FBattleAttributeData& UBattleDamageWithUnifiedDataComponent::GetMutableAttributeData()
{
	if (ExtensionComponent)
	{
		return ExtensionComponent->GetMutableBattleAttributes();
	}
	return Super::GetMutableAttributeData();
}

void UBattleDamageWithUnifiedDataComponent::ApplyDamageResult(const FDamageResult& Result)
{
	if (ExtensionComponent)
	{
		ExtensionComponent->GetMutableBattleAttributes().CurrentHealth = Result.HealthRemaining;
	}
	if (UnifiedDataComponent)
	{
		UnifiedDataComponent->SetAlive(Result.HealthRemaining > 0.0f);
	}
	Super::ApplyDamageResult(Result);
}

int32 UBattleDamageWithUnifiedDataComponent::GetFaction() const
{
	if (UnifiedDataComponent)
	{
		return UnifiedDataComponent->GetFaction();
	}
	return -1;
}
