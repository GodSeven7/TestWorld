#include "CombatManager.h"
#include "CombatComponent.h"
#include "CombatSkill.h"
#include "CombatWeapon.h"
#include "CombatSystemPlugin.h"
#include "CombatDebug.h"
#include "ICombatObject.h"
#include "DamageService.h"
#include "GameFramework/Actor.h"
#include "Async/ParallelFor.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

void UCombatManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CurrentTime = 0.0;
    RegisteredComponents.Empty();

    UE_LOG(LogCombatSystem, Log, TEXT("CombatManager initialized"));
}

void UCombatManager::Deinitialize()
{
    RegisteredComponents.Empty();

    Super::Deinitialize();
}

bool UCombatManager::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

const FCombatSkillConfig* UCombatManager::FindSkillConfig(FName SkillID) const
{
    if (!SkillTable || SkillID.IsNone())
        return nullptr;

    if (FCombatSkillConfig* Config = SkillTable->FindRow<FCombatSkillConfig>(SkillID, TEXT("FindSkillConfig")))
        return Config;

    return nullptr;
}

const FWeaponStats* UCombatManager::FindWeaponStats(FName WeaponID) const
{
    if (!WeaponTable || WeaponID.IsNone())
        return nullptr;

    if (FWeaponStats* Stats = WeaponTable->FindRow<FWeaponStats>(WeaponID, TEXT("FindWeaponStats")))
        return Stats;

    return nullptr;
}

const FCharacterCombatConfig* UCombatManager::FindCharacterConfig(FName CharacterID) const
{
    if (!CharacterTable || CharacterID.IsNone())
        return nullptr;

    if (FCharacterCombatConfig* Config = CharacterTable->FindRow<FCharacterCombatConfig>(CharacterID, TEXT("FindCharacterConfig")))
        return Config;

    return nullptr;
}

void UCombatManager::RegisterCombatComponent(UCombatComponent* Comp)
{
    if (!Comp || RegisteredComponents.Contains(Comp))
        return;

    RegisteredComponents.Add(Comp);

    if (UCombatWeapon* Weapon = Comp->GetWeapon())
    {
        int32 WeaponSkillIndex = Comp->FindSkillIndex(Weapon->GetSkillID());
        if (WeaponSkillIndex != INDEX_NONE)
        {
            float InitialDelay = FMath::FRand() * Weapon->GetTotalCycleTime();
            Comp->SetSkillCooldown(WeaponSkillIndex, InitialDelay);
        }
    }
}

void UCombatManager::UnregisterCombatComponent(UCombatComponent* Comp)
{
    if (!Comp)
        return;

    RegisteredComponents.Remove(Comp);
}

void UCombatManager::ProcessCombat(float DeltaTime)
{
    SCOPED_NAMED_EVENT(Combat_ProcessCombat, FColor::Red);

    CurrentTime += DeltaTime;

    ProcessSkillExecutions(DeltaTime);
    ExecuteActiveSkills(DeltaTime);

    ProcessPendingKills();

    if (CombatDebug::IsDebugEnabled())
    {
        for (UCombatComponent* Comp : RegisteredComponents)
        {
            if (Comp)
            {
                CombatDebug::DrawDebug(GetWorld(), Comp);
            }
        }
    }

    DispatchEvents();
}

void UCombatManager::ProcessSkillExecutions(float DeltaTime)
{
    SCOPED_NAMED_EVENT(Combat_ProcessSkillExecutions, FColor::Green);

    for (UCombatComponent* Comp : RegisteredComponents)
    {
        if (Comp)
        {
            Comp->ProcessSkillQueue(DeltaTime);
        }
    }
}

void UCombatManager::ExecuteActiveSkills(float DeltaTime)
{
    SCOPED_NAMED_EVENT(Combat_ExecuteActiveSkills, FColor::Magenta);

    TArray<FDeferredSkillExecution> DeferredExecutions;

    for (UCombatComponent* Comp : RegisteredComponents)
    {
        if (!Comp || Comp->GetCurrentSkillPhase() != ESkillPhase::Active)
            continue;

        UCombatSkill* Skill = Comp->GetActiveSkill();
        if (!Skill)
            continue;

        const FCombatSkillConfig* Config = Skill->GetSkillConfig();
        if (!Config || Config->SkillType != ESkillType::Attack)
            continue;

        if (Comp->HasActiveSwing())
            continue;

        FDeferredSkillExecution Execution;
        Execution.Comp = Comp;
        Execution.Skill = Skill;
        Execution.SkillConfig = Config;
        Execution.OwnerLocation = Comp->GetCombatWorldPosition();
        Execution.OwnerDirection = Comp->GetCombatForwardVector();
        DeferredExecutions.Add(Execution);
    }

    if (DeferredExecutions.Num() == 0)
        return;

    if (DeferredExecutions.Num() > PARALLEL_THRESHOLD)
    {
        ParallelFor(DeferredExecutions.Num(), [this, &DeferredExecutions](int32 Index)
        {
            ApplySkillResult(DeferredExecutions[Index]);
        });
    }
    else
    {
        for (FDeferredSkillExecution& Execution : DeferredExecutions)
        {
            ApplySkillResult(Execution);
        }
    }
}

void UCombatManager::ApplySkillResult(FDeferredSkillExecution& InExecution)
{
    SCOPED_NAMED_EVENT(Combat_ApplySkillResult, FColor::Cyan);

    if (!InExecution.Comp || !InExecution.Comp->IsCombatActive())
        return;

    const FCombatSkillConfig* Config = InExecution.SkillConfig;
    if (!Config)
        return;

    AActor* Owner = Cast<AActor>(InExecution.Comp->GetCombatParent());
    if (!Owner)
        return;

    TArray<AActor*> Candidates = QueryActorsInShape(
        InExecution.OwnerLocation,
        InExecution.OwnerDirection,
        Config,
        InExecution.Comp->GetCombatParent(),
        InExecution.Comp->GetFaction());

    if (Candidates.Num() == 0)
        return;

    InExecution.Comp->AddSwing(CurrentTime + Config->Duration);

    UCombatWeapon* Weapon = InExecution.Comp->GetWeapon();

    FAttackContext Context;
    Context.AttackerComp = InExecution.Comp;
    Context.AttackerActor = Owner;
    Context.Weapon = Weapon;
    Context.SkillConfig = Config;

    ExecuteAttackOnTargets(Context, Candidates, Config->MaxTargets);
}

void UCombatManager::RequestAttackForActor(AActor* Actor, AActor* Target)
{
    if (!Actor)
        return;

    UCombatComponent* CombatComp = Actor->FindComponentByClass<UCombatComponent>();
    if (CombatComp)
    {
        CombatComp->RequestAttack(Target);
    }
}

FVector UCombatManager::GetActorCombatPosition(AActor* Actor)
{
    if (!Actor)
        return FVector::ZeroVector;

    if (UCombatComponent* Comp = Actor->FindComponentByClass<UCombatComponent>())
    {
        if (ICombatObject* Obj = Cast<ICombatObject>(Comp))
            return Obj->GetCombatWorldPosition();
    }
    return Actor->GetActorLocation();
}

void UCombatManager::ApplyDamageCalculation(AActor* Target, float RawDamage,
    EDamageType DamageType, AActor* Instigator,
    const FVector& ImpactLocation,
    float& OutFinalDamage, bool& OutIsCritical)
{
    OutFinalDamage = RawDamage;
    OutIsCritical = false;

    if (DamageService)
    {
        FProcessedDamageResult Result = DamageService->ApplyDamageHit(
            Target, RawDamage, DamageType, Instigator, ImpactLocation, true);
        OutFinalDamage = Result.FinalDamage;
        OutIsCritical = Result.bIsCritical;
    }
}

void UCombatManager::ExecuteAttackOnTargets(const FAttackContext& Context,
    TArray<AActor*>& InOutCandidates, int32 MaxTargets)
{
    if (!Context.AttackerComp || !Context.AttackerActor)
        return;

    UCombatWeapon* Weapon = Context.Weapon;
    const FCombatSkillConfig* Config = Context.SkillConfig;

    int32 TargetsAttacked = 0;
    while (TargetsAttacked < MaxTargets && InOutCandidates.Num() > 0)
    {
        AActor* Target = nullptr;
        if (Weapon)
        {
            Target = Weapon->SelectTarget(Context.AttackerActor, InOutCandidates);
        }
        else if (InOutCandidates.Num() > 0)
        {
            Target = InOutCandidates[0];
        }

        if (!Target)
            break;

        if (Context.AttackerComp->IsInSwing(Target))
        {
            InOutCandidates.Remove(Target);
            continue;
        }

        InOutCandidates.Remove(Target);

        float RawDamage = 0.0f;
        if (Weapon)
        {
            RawDamage = Weapon->ExecuteAttack(Context.AttackerActor, Target);
            Context.AttackerComp->AddSwingHit(Target);
        }

        EDamageType DamageType = EDamageType::Physical;
        if (Weapon && Weapon->GetWeaponStats())
        {
            DamageType = Weapon->GetWeaponStats()->DamageType;
        }

        FCombatEventData Event;
        Event.Attacker = Context.AttackerActor;
        Event.Target = Target;
        Event.ImpactLocation = GetActorCombatPosition(Target);
        Event.Damage = RawDamage;
        Event.bIsCritical = false;
        Event.WeaponID = Weapon ? Weapon->GetWeaponID() : NAME_None;
        Event.DamageType = DamageType;

        ApplyDamageCalculation(Target, RawDamage, DamageType,
            Context.AttackerActor, Event.ImpactLocation, Event.Damage, Event.bIsCritical);

        Context.AttackerComp->AddPendingEvent(Event);
        TargetsAttacked++;
    }
}

TArray<AActor*> UCombatManager::QueryActorsInShape(
    FVector Center, FVector Direction, const FCombatSkillConfig* Config, UObject* ExcludeParent, int32 ExcludeFactionMask) const
{
    if (!Config)
        return TArray<AActor*>();

    switch (Config->AttackShape)
    {
    case EAttackShape::Circle:
        return QueryActorsInRadius(Center, Config->Range, ExcludeParent, ExcludeFactionMask);

    case EAttackShape::Cone:
        return QueryActorsInCone(Center, Direction, Config->Range,
            Config->ConeAngle, ExcludeParent, ExcludeFactionMask);

    case EAttackShape::Rectangle:
        return QueryActorsInRectangle(Center, Direction,
            Config->Range, Config->RectangleWidth, ExcludeParent, ExcludeFactionMask);

    default:
        return QueryActorsInRadius(Center, Config->Range, ExcludeParent, ExcludeFactionMask);
    }
}

TArray<AActor*> UCombatManager::QueryActorsInCone(
    FVector Center, FVector Direction, float Length, float AngleDegrees,
    UObject* ExcludeParent, int32 ExcludeFactionMask) const
{
    TArray<AActor*> Result;
    if (!SpatialQuery)
        return Result;

    float AngleRad = FMath::DegreesToRadians(AngleDegrees * 0.5f);
    float LengthSq = Length * Length;
    FVector DirectionNormalized = Direction.GetSafeNormal2D();

    TArray<FSpatialQueryHandle> Handles = SpatialQuery->QueryHandlesInSphereExcludingFactions(Center, Length, ExcludeFactionMask);

    for (const FSpatialQueryHandle& Handle : Handles)
    {
        IGridObjectInfo* GridObj = SpatialQuery->GetObjectFromHandle(Handle);
        if (!GridObj)
            continue;

        FVector Position = GridObj->GetGridPosition();
        FVector ToTarget = Position - Center;
        float DistSq = ToTarget.SizeSquared2D();

        if (DistSq > LengthSq)
            continue;

        float Angle = FMath::Acos(FVector::DotProduct(DirectionNormalized, ToTarget.GetSafeNormal2D()));
        if (Angle <= AngleRad)
        {
            if (AActor* Actor = GridObj->GetOwnerActor())
            {
                if (UCombatComponent* Comp = Actor->GetComponentByClass<UCombatComponent>())
                {
                    if (Comp->GetCombatParent() != ExcludeParent)
                        Result.Add(Actor);
                }
            }
        }
    }

    return Result;
}

TArray<AActor*> UCombatManager::QueryActorsInRectangle(
    FVector Center, FVector Direction, float Length, float Width,
    UObject* ExcludeParent, int32 ExcludeFactionMask) const
{
    TArray<AActor*> Result;
    if (!SpatialQuery)
        return Result;

    float HalfWidth = Width * 0.5f;
    float LengthSq = Length * Length;
    FVector DirectionNormalized = Direction.GetSafeNormal2D();
    FVector RightVector = FVector::CrossProduct(FVector::UpVector, DirectionNormalized).GetSafeNormal2D();

    TArray<FSpatialQueryHandle> Handles = SpatialQuery->QueryHandlesInSphereExcludingFactions(Center, Length, ExcludeFactionMask);

    for (const FSpatialQueryHandle& Handle : Handles)
    {
        IGridObjectInfo* GridObj = SpatialQuery->GetObjectFromHandle(Handle);
        if (!GridObj)
            continue;

        FVector Position = GridObj->GetGridPosition();
        FVector ToTarget = Position - Center;

        float ForwardDist = FVector::DotProduct(ToTarget, DirectionNormalized);
        float RightDist = FVector::DotProduct(ToTarget, RightVector);

        if (ForwardDist >= 0 && ForwardDist <= Length && FMath::Abs(RightDist) <= HalfWidth)
        {
            if (AActor* Actor = GridObj->GetOwnerActor())
            {
                if (UCombatComponent* Comp = Actor->GetComponentByClass<UCombatComponent>())
                {
                    if (Comp->GetCombatParent() != ExcludeParent)
                        Result.Add(Actor);
                }
            }
        }
    }

    return Result;
}

void UCombatManager::DispatchEvents()
{
    SCOPED_NAMED_EVENT(Combat_DispatchEvents, FColor::Purple);

    for (UCombatComponent* Comp : RegisteredComponents)
    {
        if (Comp)
        {
            Comp->FlushPendingEvents();
        }
    }
}

void UCombatManager::ProcessPendingKills()
{
    if (DamageService)
    {
        int32 NotificationCount = DamageService->FlushDamageNotifications();
        int32 KillCount = DamageService->FlushPendingKills();
        if (KillCount > 0)
        {
            UE_LOG(LogCombatSystem, Log, TEXT("Processed %d kills"), KillCount);
        }
    }
}

TArray<AActor*> UCombatManager::QueryActorsInRadius(
    FVector Center,
    float Radius,
    UObject* ExcludeParent,
    int32 ExcludeFactionMask) const
{
    SCOPED_NAMED_EVENT(Combat_QueryActorsInRadius, FColor::White);

    TArray<AActor*> Result;

    if (!SpatialQuery)
    {
        return Result;
    }

    float RadiusSq = Radius * Radius;

    TArray<FSpatialQueryHandle> Handles = SpatialQuery->QueryHandlesInSphereExcludingFactions(Center, Radius, ExcludeFactionMask);

    for (const FSpatialQueryHandle& Handle : Handles)
    {
        IGridObjectInfo* GridObj = SpatialQuery->GetObjectFromHandle(Handle);
        if (!GridObj)
            continue;

        FVector Position = GridObj->GetGridPosition();
        if (FVector::DistSquared2D(Center, Position) > RadiusSq)
            continue;

        AActor* Actor = GridObj->GetOwnerActor();
        if (!Actor)
            continue;

        UCombatComponent* CombatComp = Actor->GetComponentByClass<UCombatComponent>();
        if (CombatComp && CombatComp->GetCombatParent() == ExcludeParent)
            continue;

        Result.Add(Actor);
    }

    return Result;
}
