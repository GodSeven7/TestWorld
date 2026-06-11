#include "Components/CombatWithUnifiedDataComponent.h"
#include "ActorUnifiedDataComponent.h"
#include "Core/UnifiedDataExtensionComponent.h"
#include "CharacterSkelMeshComponent.h"
#include "GameFramework/Actor.h"

UCombatWithUnifiedDataComponent::UCombatWithUnifiedDataComponent()
{
}

void UCombatWithUnifiedDataComponent::BeginPlay()
{
    UnifiedDataComponent = GetOwner()->FindComponentByClass<UActorUnifiedDataComponent>();
    ExtensionComponent = GetOwner()->FindComponentByClass<UUnifiedDataExtensionComponent>();
    if (UnifiedDataComponent)
    {
        UnifiedDataComponent->SetFaction(Faction);
    }

	Super::BeginPlay();
}

UActorUnifiedDataComponent* UCombatWithUnifiedDataComponent::GetUnifiedDataComponent() const
{
    return UnifiedDataComponent;
}

FVector UCombatWithUnifiedDataComponent::GetCombatWorldPosition() const
{
    if (UnifiedDataComponent)
    {
        return UnifiedDataComponent->GetPosition();
    }
    return Super::GetCombatWorldPosition();
}

FVector UCombatWithUnifiedDataComponent::GetCombatForwardVector() const
{
    if (UnifiedDataComponent)
    {
        return UnifiedDataComponent->GetForwardVector();
    }
    return Super::GetCombatForwardVector();
}

int32 UCombatWithUnifiedDataComponent::GetCombatFaction() const
{
    if (UnifiedDataComponent)
    {
        return UnifiedDataComponent->GetFaction();
    }
    return Super::GetCombatFaction();
}

int32 UCombatWithUnifiedDataComponent::GetFaction() const
{
    if (UnifiedDataComponent)
    {
        return UnifiedDataComponent->GetFaction();
    }
    return -1;
}

UObject* UCombatWithUnifiedDataComponent::GetCombatParent() const
{
    return GetOwner();
}

void UCombatWithUnifiedDataComponent::OnCombatAttackExecuted(float Damage, bool bIsCritical)
{
    if (ExtensionComponent)
    {
        FCombatSnapshotData& CombatData = ExtensionComponent->GetMutableCombatData();
        CombatData.LastAttackTime = GetWorld()->GetTimeSeconds();
        CombatData.LastDamageDealt = Damage;
        CombatData.TotalDamageDealt += Damage;
        CombatData.TotalAttacksCount++;
    }
    Super::OnCombatAttackExecuted(Damage, bIsCritical);
}

void UCombatWithUnifiedDataComponent::OnCombatHitReceived(float Damage)
{
    if (ExtensionComponent)
    {
        FCombatSnapshotData& CombatData = ExtensionComponent->GetMutableCombatData();
        CombatData.TotalHitsCount++;
    }
    Super::OnCombatHitReceived(Damage);
}

void UCombatWithUnifiedDataComponent::SetAttackPhase(EAttackPhase Phase)
{
    Super::SetAttackPhase(Phase);

    if (AActor* Owner = GetOwner())
    {
        if (UCharacterSkelMeshComponent* MeshComp = Owner->FindComponentByClass<UCharacterSkelMeshComponent>())
        {
            MeshComp->SetAttacking(Phase != EAttackPhase::Idle);
        }
    }
}
