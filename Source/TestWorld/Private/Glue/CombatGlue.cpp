#include "Glue/CombatGlue.h"
#include "CombatManager.h"
#include "CombatComponent.h"
#include "CombatWeapon.h"
#include "SparseGridManager.h"
#include "SparseGridComponent.h"
#include "BattleDamageManager.h"
#include "BattleDamageTypes.h"
#include "AuraManager.h"
#include "ActorUnifiedDataComponent.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

static bool bDisableCombat = false;
FAutoConsoleVariableRef CVarDisableCombat(
	TEXT("Disable.Combat"),
	bDisableCombat,
	TEXT(""));

void UCombatGlue::Initialize(UWorld* World)
{
}

void UCombatGlue::Shutdown()
{
    if (DamageManager)
    {
        DamageManager->OnCombatantDeath.RemoveDynamic(this, &UCombatGlue::OnCombatantDeath);
        DamageManager->OnCombatHitProcessed.RemoveDynamic(this, &UCombatGlue::OnCombatHitProcessed);
    }
}

void UCombatGlue::Bind(
    UCombatManager* InCombatManager,
    USparseGridManager* InGridManager,
    UBattleDamageManager* InDamageManager,
    UEventBus* InEventBus)
{
    CombatManager = InCombatManager;
    GridManager = InGridManager;
    DamageManager = InDamageManager;
    EventBus = InEventBus;

    if (GetWorld())
    {
        AuraManager = GetWorld()->GetSubsystem<UAuraManager>();
    }

    if (CombatManager && GridManager)
    {
        if (GridManager)
            CombatManager->SetSpatialQueryService(Cast<ISpatialQueryService>(GridManager));

        if (DamageManager)
            CombatManager->SetDamageService(Cast<IDamageService>(DamageManager));
    }

    if (DamageManager)
    {
        DamageManager->OnCombatantDeath.AddDynamic(this, &UCombatGlue::OnCombatantDeath);
        DamageManager->OnCombatHitProcessed.AddDynamic(this, &UCombatGlue::OnCombatHitProcessed);
    }
}

void UCombatGlue::Tick(float DeltaTime)
{
    if (bSuspended || !CombatManager || bDisableCombat)
        return;

    CombatManager->ProcessCombat(DeltaTime);

    if (AuraManager)
    {
        TArray<FAuraUpdate> Updates;
        AuraManager->TickAuras(DeltaTime, Updates);

        for (const FAuraUpdate& Update : Updates)
        {
            if (!Update.Target.IsValid())
                continue;

            UActorUnifiedDataComponent* UnifiedComp = FindUnifiedDataComponent(Update.Target.Get());
            if (UnifiedComp)
            {
                UnifiedComp->SetPosition(Update.NewPosition);
                UnifiedComp->ClearMovementVelocity();
            }

            if (Update.AuraType == EAuraType::Knockback ||
                Update.AuraType == EAuraType::Launch ||
                Update.AuraType == EAuraType::DeathLaunch)
            {
                ActivateExternalMotion(Update.Target.Get());
            }
        }
    }
}

void UCombatGlue::OnCombatantDeath(AActor* Combatant, const FDamageInfo& KillingBlow)
{
    if (EventBus && Combatant)
    {
        FCombatantDeathEvent Event;
        Event.DeadActor = Combatant;
        Event.Killer = KillingBlow.Instigator;
        Event.InstigatorLocation = KillingBlow.Instigator ? KillingBlow.Instigator->GetActorLocation() : FVector::ZeroVector;
        Event.ImpactLocation = KillingBlow.ImpactLocation;
        EventBus->OnCombatantDeath.Broadcast(Event);
    }
}

void UCombatGlue::OnCombatHitProcessed(AActor* Target, const FProcessedHitResult& Result)
{
    if (!Target)
        return;

    if (EventBus)
    {
        FDamageAppliedEvent Event;
        Event.Target = Target;
        Event.Damage = Result.FinalDamage;
        Event.InstigatorLocation = Result.Instigator ? Result.Instigator->GetActorLocation() : FVector::ZeroVector;
        Event.ImpactLocation = Result.ImpactLocation;
        Event.bKilled = Result.bKilled;
        EventBus->OnDamageApplied.Broadcast(Event);
    }

    if (!AuraManager || !Result.Instigator)
        return;

    UCombatComponent* AttackerComp = FindCombatComponent(Result.Instigator);
    if (!AttackerComp)
        return;

    UCombatWeapon* Weapon = AttackerComp->GetWeapon();
    if (!Weapon || !Weapon->GetSkillConfig() || Weapon->GetSkillConfig()->OnHitAuras.Num() == 0)
        return;

    UActorUnifiedDataComponent* TargetUnified = FindUnifiedDataComponent(Target);
    if (!TargetUnified)
        return;

    FVector TargetPos = TargetUnified->GetPosition();
    UActorUnifiedDataComponent* AttackerUnified = FindUnifiedDataComponent(Result.Instigator);
    FVector AttackerPos = AttackerUnified ? AttackerUnified->GetPosition() : Result.ImpactLocation;

    FVector Direction = TargetPos - AttackerPos;
    Direction.Z = 0.0f;
    if (Direction.IsNearlyZero())
        Direction = FVector::ForwardVector;
    Direction.Normalize();

    for (const FAuraEffectEntry& Entry : Weapon->GetSkillConfig()->OnHitAuras)
    {
        if (Entry.AuraType == EAuraType::None)
            continue;

        FAuraEffectEntry EffectiveEntry = Entry;

        if (Result.bKilled)
        {
            if (EffectiveEntry.AuraType == EAuraType::Knockback)
            {
                EffectiveEntry.AuraType = EAuraType::DeathLaunch;
                EffectiveEntry.DeathLaunchParams.HorizontalDistance = Entry.KnockbackParams.Distance;
                EffectiveEntry.DeathLaunchParams.LaunchHeight = 300.0f;
                EffectiveEntry.DeathLaunchParams.Duration = 0.8f;
                EffectiveEntry.DeathLaunchParams.RiseTimeRatio = 0.3f;
                EffectiveEntry.DeathLaunchParams.CurveType = EAuraCurve::EaseOut;
            }
            else if (EffectiveEntry.AuraType == EAuraType::Launch)
            {
                EffectiveEntry.AuraType = EAuraType::DeathLaunch;
                EffectiveEntry.DeathLaunchParams.HorizontalDistance = Entry.LaunchParams.HorizontalDistance;
                EffectiveEntry.DeathLaunchParams.LaunchHeight = Entry.LaunchParams.LaunchHeight;
                EffectiveEntry.DeathLaunchParams.Duration = Entry.LaunchParams.Duration;
                EffectiveEntry.DeathLaunchParams.RiseTimeRatio = Entry.LaunchParams.RiseTimeRatio;
                EffectiveEntry.DeathLaunchParams.CurveType = Entry.LaunchParams.CurveType;
            }
        }
        else
        {
            if (EffectiveEntry.AuraType == EAuraType::DeathLaunch)
            {
                EffectiveEntry.AuraType = EAuraType::Launch;
                EffectiveEntry.LaunchParams.HorizontalDistance = Entry.DeathLaunchParams.HorizontalDistance;
                EffectiveEntry.LaunchParams.LaunchHeight = Entry.DeathLaunchParams.LaunchHeight;
                EffectiveEntry.LaunchParams.Duration = Entry.DeathLaunchParams.Duration;
                EffectiveEntry.LaunchParams.RiseTimeRatio = Entry.DeathLaunchParams.RiseTimeRatio;
                EffectiveEntry.LaunchParams.CurveType = Entry.DeathLaunchParams.CurveType;
                EffectiveEntry.LaunchParams.bInterruptible = true;
            }
        }

        AuraManager->ApplyAura(Target, Result.Instigator, EffectiveEntry, TargetPos, Direction);
    }
}

UActorUnifiedDataComponent* UCombatGlue::FindUnifiedDataComponent(AActor* Actor) const
{
    if (!Actor)
        return nullptr;

    if (UActorUnifiedDataComponent* Comp = Actor->FindComponentByClass<UActorUnifiedDataComponent>())
        return Comp;

    return nullptr;
}

UCombatComponent* UCombatGlue::FindCombatComponent(AActor* Actor) const
{
    if (!Actor)
        return nullptr;

    return Actor->FindComponentByClass<UCombatComponent>();
}

void UCombatGlue::ActivateExternalMotion(AActor* Actor) const
{
    if (!Actor)
    {
        return;
    }

    if (USparseGridComponent* SparseGridComponent = Actor->FindComponentByClass<USparseGridComponent>())
    {
        // Removed ActivateExternalForce and ExternalForceDuration — push force no longer driven here
        if (GridManager && SparseGridComponent->GetGridHandle().IsValid())
        {
            GridManager->UpdateObjectPosition(SparseGridComponent->GetGridHandle(), SparseGridComponent->GetSparseGridPosition());
        }
    }
}

void UCombatGlue::TickDamageProcessing()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UCombatGlue_TickDamageProcessing);

    if (!DamageManager)
        return;

    DamageManager->ProcessPendingDamages();
}
