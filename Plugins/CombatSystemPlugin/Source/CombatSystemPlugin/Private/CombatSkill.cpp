#include "CombatSkill.h"
#include "CombatManager.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"

#if WITH_EDITOR
TArray<FString> UCombatSkill::GetSkillIDOptions() const
{
    TArray<FString> Options;
    if (SkillTableRef)
    {
        for (const FName& RowName : SkillTableRef->GetRowNames())
        {
            Options.Add(RowName.ToString());
        }
    }
    return Options;
}
#endif

void UCombatSkill::InitSkill(UCombatManager* Manager)
{
    if (Manager)
    {
        CachedConfig = Manager->FindSkillConfig(SkillID);
    }
}

bool UCombatSkill::CanExecute(AActor* Owner) const
{
    return Owner != nullptr;
}

bool UCombatSkill::CheckPreCondition(AActor* Owner) const
{
    return true;
}

void UCombatSkill::OnSkillStarted(AActor* Owner)
{
}

void UCombatSkill::ExecuteInternal(AActor* Owner, float DeltaTime)
{
}

void UCombatSkill::OnSkillCompleted(AActor* Owner)
{
}

float UCombatSkill::GetWindupTime() const
{
    return CachedConfig ? CachedConfig->WindupTime : 0.f;
}

float UCombatSkill::GetDuration() const
{
    return CachedConfig ? CachedConfig->Duration : 0.f;
}

float UCombatSkill::GetFollowThroughTime() const
{
    return CachedConfig ? CachedConfig->FollowThroughTime : 0.f;
}

float UCombatSkill::GetCooldown() const
{
    return CachedConfig ? CachedConfig->Cooldown : 0.f;
}

float UCombatSkill::GetTotalCycleTime() const
{
    return CachedConfig ? CachedConfig->GetTotalCycleTime() : 0.f;
}
