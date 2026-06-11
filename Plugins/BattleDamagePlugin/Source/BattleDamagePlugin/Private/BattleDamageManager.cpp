#include "BattleDamageManager.h"
#include "BattleDamageComponent.h"
#include "BattleDamageInterface.h"
#include "DamageNumberManager.h"
#include "Kismet/GameplayStatics.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

void UBattleDamageManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UBattleDamageManager::Deinitialize()
{
    Super::Deinitialize();
}

bool UBattleDamageManager::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UBattleDamageManager::QueueDamage(AActor* Target, const FDamageInfo& Info)
{
    FScopeLock Lock(&PendingDamagesLock);
    FPendingDamage Pending;
    Pending.Target = Target;
    Pending.Info = Info;
    PendingDamages.Add(Pending);
}

void UBattleDamageManager::QueueHeal(AActor* Target, float Amount, AActor* Healer)
{
    FScopeLock Lock(&PendingDamagesLock);
    FPendingDamage Pending;
    Pending.Target = Target;
    Pending.Info.BaseDamage = Amount;
    Pending.Info.DamageType = EDamageType::Heal;
    Pending.Info.Instigator = Healer;
    Pending.Info.Target = Target;
    PendingDamages.Add(Pending);
}

void UBattleDamageManager::ProcessPendingDamages()
{
    SCOPED_NAMED_EVENT(BattleDamage_ProcessPendingDamages, FColor::Red);

    TArray<FPendingDamage> DamagesToProcess;

    {
        FScopeLock Lock(&PendingDamagesLock);
        DamagesToProcess = MoveTemp(PendingDamages);
    }

    for (const FPendingDamage& Pending : DamagesToProcess)
    {
        AActor* Target = Pending.Target.Get();
        if (!Target)
        {
            continue;
        }

        if (Pending.Info.DamageType == EDamageType::Heal)
        {
            ApplyHeal(Target, Pending.Info.BaseDamage, Pending.Info.Instigator);
        }
        else
        {
            ApplyDamage(Target, Pending.Info);
        }
    }
}

void UBattleDamageManager::RegisterCombatant(IBattleDamageInterface* Combatant)
{
    FScopeLock Lock(&CombatantsLock);
    if (Combatant && !RegisteredCombatants.Contains(Combatant))
    {
        RegisteredCombatants.Add(Combatant);
        if (AActor* Actor = Cast<AActor>(Combatant))
        {
            OnCombatantRegistered.Broadcast(Actor);
        }
    }
}

void UBattleDamageManager::UnregisterCombatant(IBattleDamageInterface* Combatant)
{
    FScopeLock Lock(&CombatantsLock);
    if (Combatant)
    {
        RegisteredCombatants.Remove(Combatant);
        if (AActor* Actor = Cast<AActor>(Combatant))
        {
            OnCombatantUnregistered.Broadcast(Actor);
        }
    }
}

FDamageResult UBattleDamageManager::ApplyDamage(AActor* Target, const FDamageInfo& Info)
{
    FDamageResult Result;
    if (!Target)
    {
        return Result;
    }

    if (Info.DamageType == EDamageType::Heal)
    {
        Result.FinalDamage = ApplyHeal(Target, Info.BaseDamage, Info.Instigator);
        return Result;
    }

    UBattleDamageComponent* BattleComp = Target->FindComponentByClass<UBattleDamageComponent>();
    IBattleDamageInterface* Combatant = Cast<IBattleDamageInterface>(BattleComp);
    if (!Combatant)
    {
        return Result;
    }

    return CalculateAndApplyDamage(Combatant, Info);
}

float UBattleDamageManager::ApplyHeal(AActor* Target, float Amount, AActor* Healer)
{
    if (!Target || Amount <= 0.0f)
    {
        return 0.0f;
    }

    UBattleDamageComponent* BattleComp = Target->FindComponentByClass<UBattleDamageComponent>();
    IBattleDamageInterface* Combatant = Cast<IBattleDamageInterface>(BattleComp);
    if (!Combatant || !Combatant->CanBeHealed())
    {
        return 0.0f;
    }

    FBattleAttributeData& Attrs = Combatant->GetMutableAttributeData();
    float OldHealth = Attrs.CurrentHealth;
    Attrs.CurrentHealth = FMath::Min(Attrs.CurrentHealth + Amount, Attrs.MaxHealth);
    float HealedAmount = Attrs.CurrentHealth - OldHealth;

    Combatant->OnHealReceived(HealedAmount, Healer);

    return HealedAmount;
}

FDamageResult UBattleDamageManager::CalculateAndApplyDamage(IBattleDamageInterface* Target, const FDamageInfo& Info)
{
    FDamageResult Result;

    if (!Target || Target->IsInvulnerable())
    {
        return Result;
    }

    const FBattleAttributeData& Attrs = Target->GetAttributeData();

    float MitigatedDamage = CalculateDamageMitigation(Info.BaseDamage, Info.DamageType, Attrs);
    Result.DamageMitigated = Info.BaseDamage - MitigatedDamage;
    Result.FinalDamage = MitigatedDamage;

    if (Info.bCanCritical)
    {
        Result.bIsCritical = RollCritical(Attrs.CriticalChance);
        if (Result.bIsCritical)
        {
            Result.FinalDamage *= Attrs.CriticalMultiplier;
        }
    }

    FBattleAttributeData& MutableAttrs = Target->GetMutableAttributeData();
    float NewHealth = MutableAttrs.CurrentHealth - Result.FinalDamage;
    Result.Overkill = FMath::Max(0.0f, -NewHealth);
    Result.bKilled = NewHealth <= 0.0f;
    Result.HealthRemaining = FMath::Max(0.0f, NewHealth);

    MutableAttrs.CurrentHealth = Result.HealthRemaining;
    Target->ApplyDamageResult(Result);
    Target->OnDamageReceived(Info, Result);

    if (Result.bKilled)
    {
        Target->OnDeath(Info);
        if (AActor* Actor = Cast<AActor>(Target))
        {
            OnCombatantDeath.Broadcast(Actor, Info);
        }
    }

    return Result;
}

float UBattleDamageManager::CalculateDamageMitigation(float BaseDamage, EDamageType DamageType, const FBattleAttributeData& Attributes)
{
    switch (DamageType)
    {
    case EDamageType::Physical:
        return FMath::Max(0.0f, BaseDamage - Attributes.Defense);
    case EDamageType::Magical:
        return FMath::Max(0.0f, BaseDamage - Attributes.MagicResistance);
    case EDamageType::Real:
    default:
        return BaseDamage;
    }
}

bool UBattleDamageManager::RollCritical(float CriticalChance)
{
    return FMath::FRand() <= CriticalChance;
}

FProcessedHitResult UBattleDamageManager::ProcessCombatHit(AActor* Target, float RawDamage, EDamageType DamageType, AActor* Instigator, const FVector& ImpactLocation, bool bCanCritical)
{
    FProcessedHitResult Result;
    Result.Target = Target;
    Result.Instigator = Instigator;
    Result.RawDamage = RawDamage;

    if (!Target || RawDamage <= 0.0f)
    {
        return Result;
    }

    if (DamageType == EDamageType::Heal)
    {
        ApplyHeal(Target, RawDamage, Instigator);
        Result.FinalDamage = RawDamage;
        Result.ImpactLocation = ImpactLocation;
        return Result;
    }

    UBattleDamageComponent* BattleComp = Target->FindComponentByClass<UBattleDamageComponent>();
    IBattleDamageInterface* Combatant = Cast<IBattleDamageInterface>(BattleComp);
    if (!Combatant)
    {
        return Result;
    }

    FDamageInfo Info;
    Info.BaseDamage = RawDamage;
    Info.DamageType = DamageType;
    Info.Instigator = Instigator;
    Info.Target = Target;
    Info.ImpactLocation = ImpactLocation;
    Info.bCanCritical = bCanCritical;

    FBattleAttributeData Attrs = Combatant->GetAttributeDataThreadSafe();

    float MitigatedDamage = CalculateDamageMitigation(Info.BaseDamage, Info.DamageType, Attrs);

    FDamageResult DamageResult;
    DamageResult.DamageMitigated = Info.BaseDamage - MitigatedDamage;
    DamageResult.FinalDamage = MitigatedDamage;

    if (Info.bCanCritical)
    {
        DamageResult.bIsCritical = RollCritical(Attrs.CriticalChance);
        if (DamageResult.bIsCritical)
        {
            DamageResult.FinalDamage *= Attrs.CriticalMultiplier;
        }
    }

    float NewHealth = Attrs.CurrentHealth - DamageResult.FinalDamage;
    DamageResult.Overkill = FMath::Max(0.0f, -NewHealth);
    DamageResult.bKilled = NewHealth <= 0.0f;
    DamageResult.HealthRemaining = FMath::Max(0.0f, NewHealth);

    Combatant->ApplyDamageResult(DamageResult);

    {
        FScopeLock Lock(&PendingDamageNotificationsLock);
        FPendingDamageNotification Notification;
        Notification.Target = Target;
        Notification.Info = Info;
        Notification.Result = DamageResult;
        PendingDamageNotifications.Add(Notification);
    }

    Result.FinalDamage = DamageResult.FinalDamage;
    Result.DamageMitigated = DamageResult.DamageMitigated;
    Result.bIsCritical = DamageResult.bIsCritical;
    Result.bKilled = DamageResult.bKilled;
    Result.ImpactLocation = ImpactLocation;

    if (DamageResult.bKilled)
    {
		QueueKill(Target, Instigator, RawDamage, DamageResult.FinalDamage, DamageType, DamageResult.bIsCritical);
    }

    return Result;
}

void UBattleDamageManager::QueueKill(AActor* Target, AActor* Instigator, float RawDamage, float FinalDamage, EDamageType DamageType, bool bIsCritical)
{
    if (!Target)
        return;

    FKillInfo KillInfo;
    KillInfo.Target = Target;
    KillInfo.Instigator = Instigator;
    KillInfo.RawDamage = RawDamage;
    KillInfo.FinalDamage = FinalDamage;
    KillInfo.DamageType = DamageType;
    KillInfo.bIsCritical = bIsCritical;

    FScopeLock Lock(&PendingKillsLock);
    PendingKills.Add(KillInfo);
}

int32 UBattleDamageManager::ProcessPendingKillsOnGameThread()
{
    TArray<FKillInfo> LocalKills;

    {
        FScopeLock Lock(&PendingKillsLock);
        if (PendingKills.IsEmpty())
        {
            return 0;
        }
        LocalKills = MoveTemp(PendingKills);
    }

    for (const FKillInfo& Kill : LocalKills)
    {
        if (!Kill.Target.IsValid())
        {
            continue;
        }

        AActor* TargetActor = Kill.Target.Get();
        if (!TargetActor || TargetActor->IsPendingKillPending())
        {
            continue;
        }

        FDamageInfo KillingBlow;
        KillingBlow.BaseDamage = Kill.RawDamage;
        KillingBlow.DamageType = Kill.DamageType;
        KillingBlow.Instigator = Kill.Instigator.Get();
        KillingBlow.Target = TargetActor;
        KillingBlow.ImpactLocation = TargetActor->GetActorLocation();
        KillingBlow.bCanCritical = Kill.bIsCritical;

        UBattleDamageComponent* BattleComp = TargetActor->FindComponentByClass<UBattleDamageComponent>();
        IBattleDamageInterface* Combatant = Cast<IBattleDamageInterface>(BattleComp);
        if (Combatant)
        {
            Combatant->OnDeath(KillingBlow);
        }

        OnCombatantDeath.Broadcast(TargetActor, KillingBlow);
    }

    return LocalKills.Num();
}

int32 UBattleDamageManager::ProcessPendingDamageNotifications()
{
    TArray<FPendingDamageNotification> LocalNotifications;

    {
        FScopeLock Lock(&PendingDamageNotificationsLock);
        if (PendingDamageNotifications.IsEmpty())
        {
            return 0;
        }
        LocalNotifications = MoveTemp(PendingDamageNotifications);
    }

    UDamageNumberManager* DamageNumberMgr = nullptr;
    if (bDamageNumberEnabled)
    {
        DamageNumberMgr = GetDamageNumberManager();
    }

    for (const FPendingDamageNotification& Notification : LocalNotifications)
    {
        if (!Notification.Target.IsValid())
        {
            continue;
        }

        AActor* TargetActor = Notification.Target.Get();
        if (!TargetActor || TargetActor->IsPendingKillPending())
        {
            continue;
        }

        UBattleDamageComponent* BattleComp = TargetActor->FindComponentByClass<UBattleDamageComponent>();
        IBattleDamageInterface* Combatant = Cast<IBattleDamageInterface>(BattleComp);
        if (Combatant)
        {
            Combatant->OnDamageReceived(Notification.Info, Notification.Result);
        }

        if (DamageNumberMgr && Notification.Result.FinalDamage > 0.0f)
        {
            DamageNumberMgr->EnqueueDamageNumber(
                Notification.Info.ImpactLocation,
                Notification.Result.FinalDamage,
                Notification.Result.bIsCritical
            );
        }

        FProcessedHitResult Result;
        Result.Target = TargetActor;
        Result.Instigator = Notification.Info.Instigator;
        Result.RawDamage = Notification.Info.BaseDamage;
        Result.FinalDamage = Notification.Result.FinalDamage;
        Result.DamageMitigated = Notification.Result.DamageMitigated;
        Result.bIsCritical = Notification.Result.bIsCritical;
        Result.bKilled = Notification.Result.bKilled;
        Result.ImpactLocation = Notification.Info.ImpactLocation;

        OnCombatHitProcessed.Broadcast(TargetActor, Result);
    }

    if (DamageNumberMgr)
    {
        DamageNumberMgr->FlushToNiagara();
    }

    return LocalNotifications.Num();
}

void UBattleDamageManager::SetDamageNumberEnabled(bool bEnabled)
{
    bDamageNumberEnabled = bEnabled;
}

bool UBattleDamageManager::IsDamageNumberEnabled() const
{
    return bDamageNumberEnabled;
}

UDamageNumberManager* UBattleDamageManager::GetDamageNumberManager() const
{
    if (UWorld* World = GetWorld())
    {
        return World->GetSubsystem<UDamageNumberManager>();
    }
    return nullptr;
}

FProcessedDamageResult UBattleDamageManager::ApplyDamageHit(
    AActor* Target,
    float RawDamage,
    EDamageType DamageType,
    AActor* Instigator,
    const FVector& ImpactLocation,
    bool bCanCritical)
{
    FProcessedHitResult HitResult = ProcessCombatHit(Target, RawDamage, DamageType, Instigator, ImpactLocation, bCanCritical);

    FProcessedDamageResult Result;
    Result.Target = HitResult.Target;
    Result.Instigator = HitResult.Instigator;
    Result.RawDamage = HitResult.RawDamage;
    Result.FinalDamage = HitResult.FinalDamage;
    Result.DamageMitigated = HitResult.DamageMitigated;
    Result.bIsCritical = HitResult.bIsCritical;
    Result.bKilled = HitResult.bKilled;
    return Result;
}

int32 UBattleDamageManager::FlushDamageNotifications()
{
    return ProcessPendingDamageNotifications();
}

int32 UBattleDamageManager::FlushPendingKills()
{
    return ProcessPendingKillsOnGameThread();
}
