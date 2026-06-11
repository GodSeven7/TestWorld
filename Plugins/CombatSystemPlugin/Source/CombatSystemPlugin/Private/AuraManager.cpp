#include "AuraManager.h"

void UAuraManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ActiveAuras.Empty();
    FreeIndices.Empty();
    NextAuraID = 1;
}

void UAuraManager::Deinitialize()
{
    ActiveAuras.Empty();
    FreeIndices.Empty();
    Super::Deinitialize();
}

bool UAuraManager::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UAuraManager::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAuraManager, STATGROUP_Tickables);
}

int32 UAuraManager::ApplyAura(AActor* Target, AActor* Caster, const FAuraEffectEntry& Entry,
    const FVector& StartPosition, const FVector& Direction)
{
    if (!Target || Entry.AuraType == EAuraType::None)
        return INDEX_NONE;

    if (Entry.AuraType == EAuraType::Knockback && Entry.KnockbackParams.bInterruptible)
    {
        for (FAuraInstance& Aura : ActiveAuras)
        {
            if (Aura.bActive && Aura.Target == Target && Aura.AuraType == EAuraType::Knockback)
                Aura.bActive = false;
        }
    }
    else if (Entry.AuraType == EAuraType::Launch && Entry.LaunchParams.bInterruptible)
    {
        for (FAuraInstance& Aura : ActiveAuras)
        {
            if (Aura.bActive && Aura.Target == Target && Aura.AuraType == EAuraType::Launch)
                Aura.bActive = false;
        }
    }
    else if (Entry.AuraType == EAuraType::DeathLaunch)
    {
        CancelAurasForActor(Target);
    }

    FAuraInstance NewAura;
    NewAura.AuraID = NextAuraID++;
    NewAura.Target = Target;
    NewAura.Caster = Caster;
    NewAura.StartPosition = StartPosition;
    NewAura.Direction = Direction;
    NewAura.ElapsedTime = 0.0f;
    NewAura.AuraType = Entry.AuraType;
    NewAura.bActive = true;
    NewAura.KnockbackParams = Entry.KnockbackParams;
    NewAura.LaunchParams = Entry.LaunchParams;
    NewAura.DeathLaunchParams = Entry.DeathLaunchParams;

    int32 Index;
    if (FreeIndices.Num() > 0)
    {
        Index = FreeIndices.Pop();
        ActiveAuras[Index] = MoveTemp(NewAura);
    }
    else
    {
        Index = ActiveAuras.Add(MoveTemp(NewAura));
    }

    return NewAura.AuraID;
}

void UAuraManager::RemoveAura(int32 AuraID)
{
    for (int32 i = 0; i < ActiveAuras.Num(); ++i)
    {
        if (ActiveAuras[i].bActive && ActiveAuras[i].AuraID == AuraID)
        {
            ActiveAuras[i].bActive = false;
            FreeIndices.Add(i);
            ActiveAuras[i] = FAuraInstance();
            return;
        }
    }
}

void UAuraManager::RemoveAurasByType(AActor* Target, EAuraType Type)
{
    if (!Target)
        return;

    for (int32 i = 0; i < ActiveAuras.Num(); ++i)
    {
        if (ActiveAuras[i].bActive && ActiveAuras[i].Target == Target && ActiveAuras[i].AuraType == Type)
        {
            ActiveAuras[i].bActive = false;
            FreeIndices.Add(i);
            ActiveAuras[i] = FAuraInstance();
        }
    }
}

bool UAuraManager::HasAuraType(AActor* Target, EAuraType Type) const
{
    if (!Target)
        return false;

    for (const FAuraInstance& Aura : ActiveAuras)
    {
        if (Aura.bActive && Aura.Target == Target && Aura.AuraType == Type)
            return true;
    }
    return false;
}

void UAuraManager::TickAuras(float DeltaTime, TArray<FAuraUpdate>& OutUpdates)
{
    for (int32 i = ActiveAuras.Num() - 1; i >= 0; --i)
    {
        FAuraInstance& Aura = ActiveAuras[i];
        if (!Aura.bActive)
        {
            FreeIndices.Add(i);
            ActiveAuras[i] = FAuraInstance();
            continue;
        }

        if (!Aura.Target.IsValid())
        {
            Aura.bActive = false;
            FreeIndices.Add(i);
            ActiveAuras[i] = FAuraInstance();
            continue;
        }

        Aura.ElapsedTime += DeltaTime;

        FAuraUpdate Update;
        Update.Target = Aura.Target;
        Update.AuraType = Aura.AuraType;

        switch (Aura.AuraType)
        {
        case EAuraType::Knockback:
        {
            float T = FMath::Clamp(Aura.ElapsedTime / Aura.KnockbackParams.Duration, 0.0f, 1.0f);
            float Alpha = ApplyCurve(T, Aura.KnockbackParams.CurveType);
            FVector Displacement = Aura.Direction * Aura.KnockbackParams.Distance * Alpha;
            Update.NewPosition = Aura.StartPosition + Displacement;
            Update.NewPosition.Z = Aura.StartPosition.Z;
            if (T >= 1.0f)
            {
                Update.bComplete = true;
                Aura.bActive = false;
            }
            break;
        }
        case EAuraType::Launch:
        {
            const FLaunchParams& P = Aura.LaunchParams;
            float T = FMath::Clamp(Aura.ElapsedTime / P.Duration, 0.0f, 1.0f);
            float AlphaH = ApplyCurve(T, P.CurveType);
            FVector HDist = Aura.Direction * P.HorizontalDistance * AlphaH;
            Update.NewPosition = Aura.StartPosition + HDist;

            float HeightAlpha;
            if (T < P.RiseTimeRatio)
                HeightAlpha = T / P.RiseTimeRatio;
            else
                HeightAlpha = 1.0f - (T - P.RiseTimeRatio) / (1.0f - P.RiseTimeRatio);
            Update.NewPosition.Z = Aura.StartPosition.Z + P.LaunchHeight * HeightAlpha;

            if (T >= 1.0f)
            {
                Update.bComplete = true;
                Aura.bActive = false;
            }
            break;
        }
        case EAuraType::DeathLaunch:
        {
            const FDeathLaunchParams& P = Aura.DeathLaunchParams;
            float T = FMath::Clamp(Aura.ElapsedTime / P.Duration, 0.0f, 1.0f);
            float AlphaH = ApplyCurve(T, P.CurveType);
            FVector HDist = Aura.Direction * P.HorizontalDistance * AlphaH;
            Update.NewPosition = Aura.StartPosition + HDist;

            float HeightAlpha;
            if (T < P.RiseTimeRatio)
                HeightAlpha = T / P.RiseTimeRatio;
            else
                HeightAlpha = 1.0f - (T - P.RiseTimeRatio) / (1.0f - P.RiseTimeRatio);
            Update.NewPosition.Z = Aura.StartPosition.Z + P.LaunchHeight * HeightAlpha;

            if (T >= 1.0f)
            {
                Update.bComplete = true;
                Aura.bActive = false;
            }
            break;
        }
        default:
            break;
        }

        if (!Aura.bActive)
        {
            FreeIndices.Add(i);
            ActiveAuras[i] = FAuraInstance();
        }

        OutUpdates.Add(Update);
    }
}

void UAuraManager::CancelAurasForActor(AActor* Target)
{
    if (!Target)
        return;

    for (int32 i = 0; i < ActiveAuras.Num(); ++i)
    {
        if (ActiveAuras[i].bActive && ActiveAuras[i].Target == Target)
        {
            ActiveAuras[i].bActive = false;
            FreeIndices.Add(i);
            ActiveAuras[i] = FAuraInstance();
        }
    }
}

void UAuraManager::CancelAllAuras()
{
    for (int32 i = 0; i < ActiveAuras.Num(); ++i)
    {
        if (ActiveAuras[i].bActive)
        {
            ActiveAuras[i].bActive = false;
            FreeIndices.Add(i);
            ActiveAuras[i] = FAuraInstance();
        }
    }
}

float UAuraManager::ApplyCurve(float T, EAuraCurve CurveType) const
{
    switch (CurveType)
    {
    case EAuraCurve::EaseOut:
        return 1.0f - FMath::Square(1.0f - T);
    case EAuraCurve::EaseIn:
        return FMath::Square(T);
    case EAuraCurve::Linear:
    default:
        return T;
    }
}
