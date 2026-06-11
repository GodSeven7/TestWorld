#include "CombatWeapon.h"
#include "ICombatObject.h"
#include "CombatComponent.h"
#include "CombatManager.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"

#if WITH_EDITOR
TArray<FString> UCombatWeapon::GetWeaponIDOptions() const
{
    TArray<FString> Options;
    if (WeaponTableRef)
    {
        for (const FName& RowName : WeaponTableRef->GetRowNames())
        {
            Options.Add(RowName.ToString());
        }
    }
    return Options;
}
#endif

UCombatWeapon::UCombatWeapon()
{
}

void UCombatWeapon::InitWeapon(UCombatManager* Manager)
{
    InitSkill(Manager);

    if (Manager)
    {
        CachedStats = Manager->FindWeaponStats(WeaponID);
    }
}

bool UCombatWeapon::CanExecute(AActor* Owner) const
{
    return Super::CanExecute(Owner);
}

bool UCombatWeapon::CheckPreCondition(AActor* Owner) const
{
    if (!Owner)
        return false;

    UCombatComponent* Comp = Owner->FindComponentByClass<UCombatComponent>();
    if (!Comp)
        return false;

    UCombatManager* Manager = Comp->GetManager();
    if (!Manager)
        return false;

    const FCombatSkillConfig* Config = GetSkillConfig();
    if (!Config)
        return false;

    TArray<AActor*> Candidates = Manager->QueryActorsInShape(
        Comp->GetCombatWorldPosition(),
        Comp->GetCombatForwardVector(),
        Config,
        Comp->GetCombatParent(),
        Comp->GetFaction());

    return Candidates.Num() > 0;
}

void UCombatWeapon::ExecuteInternal(AActor* Owner, float DeltaTime)
{
}

AActor* UCombatWeapon::SelectTarget(AActor* Owner, const TArray<AActor*>& Candidates)
{
    if (Candidates.Num() == 0)
        return nullptr;

    if (Candidates.Num() == 1)
        return Candidates[0];

    const FCombatSkillConfig* Config = GetSkillConfig();
    ETargetStrategy Strategy = Config ? Config->TargetStrategy : ETargetStrategy::Nearest;

    FVector OwnerLocation = GetOwnerPosition(Owner);

    switch (Strategy)
    {
    case ETargetStrategy::Nearest:
        {
            AActor* Nearest = nullptr;
            float MinDist = FLT_MAX;
            for (AActor* Candidate : Candidates)
            {
                float Dist = FVector::DistSquared2D(OwnerLocation, GetTargetPosition(Candidate));
                if (Dist < MinDist)
                {
                    MinDist = Dist;
                    Nearest = Candidate;
                }
            }
            return Nearest;
        }

    case ETargetStrategy::Random:
        return Candidates[FMath::RandRange(0, Candidates.Num() - 1)];

    default:
        return Candidates[0];
    }
}

float UCombatWeapon::ExecuteAttack(AActor* Owner, AActor* Target)
{
    if (!Owner || !Target)
        return 0.0f;

    return CalculateDamageWithCritical();
}

float UCombatWeapon::CalculateDamageWithCritical() const
{
    if (!CachedStats)
        return 0.0f;

    float Damage = CachedStats->Damage;
    if (FMath::FRand() < CachedStats->CritChance)
    {
        Damage *= CachedStats->CritMultiplier;
    }
    return Damage;
}

FVector UCombatWeapon::GetOwnerPosition(AActor* Owner) const
{
    if (!Owner)
        return FVector::ZeroVector;

    if (UCombatComponent* CombatComp = Owner->FindComponentByClass<UCombatComponent>())
    {
        return CombatComp->GetCombatWorldPosition();
    }
    return Owner->GetActorLocation();
}

FVector UCombatWeapon::GetTargetPosition(AActor* Target) const
{
    if (!Target)
        return FVector::ZeroVector;

    if (UCombatComponent* CombatComp = Target->FindComponentByClass<UCombatComponent>())
    {
        return CombatComp->GetCombatWorldPosition();
    }
    return Target->GetActorLocation();
}
