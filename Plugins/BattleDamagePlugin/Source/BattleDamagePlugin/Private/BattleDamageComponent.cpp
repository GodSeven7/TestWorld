#include "BattleDamageComponent.h"
#include "BattleDamageManager.h"
#include "GameFramework/Actor.h"

UBattleDamageComponent::UBattleDamageComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UBattleDamageComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoRegister)
    {
        if (UBattleDamageManager* Manager = GetManager())
        {
            Manager->RegisterCombatant(this);
        }
    }
}

void UBattleDamageComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bAutoRegister)
    {
        if (UBattleDamageManager* Manager = GetManager())
        {
            Manager->UnregisterCombatant(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

const FBattleAttributeData& UBattleDamageComponent::GetAttributeData() const
{
    return Attributes;
}

FBattleAttributeData& UBattleDamageComponent::GetMutableAttributeData()
{
    return Attributes;
}

FBattleAttributeData UBattleDamageComponent::GetAttributeDataThreadSafe() const
{
    FScopeLock Lock(&AttributeLock);
    return Attributes;
}

void UBattleDamageComponent::ApplyDamageResult(const FDamageResult& Result)
{
    FScopeLock Lock(&AttributeLock);
    Attributes.CurrentHealth = Result.HealthRemaining;
}

void UBattleDamageComponent::SetHealth(float NewHealth)
{
    FScopeLock Lock(&AttributeLock);
    Attributes.CurrentHealth = FMath::Clamp(NewHealth, 0.0f, Attributes.MaxHealth);
}

void UBattleDamageComponent::SetMaxHealth(float NewMaxHealth)
{
    FScopeLock Lock(&AttributeLock);
    Attributes.MaxHealth = FMath::Max(NewMaxHealth, 1.0f);
    Attributes.CurrentHealth = FMath::Min(Attributes.CurrentHealth, Attributes.MaxHealth);
}

void UBattleDamageComponent::Revive(float HealthPercent)
{
    FScopeLock Lock(&AttributeLock);
    Attributes.CurrentHealth = Attributes.MaxHealth * FMath::Clamp(HealthPercent, 0.0f, 1.0f);
    OnRevived(HealthPercent);
}

void UBattleDamageComponent::OnDamageReceived(const FDamageInfo& Info, const FDamageResult& Result)
{
    OnDamageReceivedDelegate.Broadcast(Info, Result);
}

void UBattleDamageComponent::OnDeath(const FDamageInfo& KillingBlow)
{
    OnDeathDelegate.Broadcast(KillingBlow);
}

void UBattleDamageComponent::OnHealReceived(float Amount, AActor* Healer)
{
    OnHealReceivedDelegate.Broadcast(Amount, Healer);
}

void UBattleDamageComponent::OnRevived(float HealthPercent)
{
    OnRevivedDelegate.Broadcast(HealthPercent);
}

UBattleDamageManager* UBattleDamageComponent::GetManager() const
{
    if (UWorld* World = GetWorld())
    {
        return World->GetSubsystem<UBattleDamageManager>();
    }
    return nullptr;
}

