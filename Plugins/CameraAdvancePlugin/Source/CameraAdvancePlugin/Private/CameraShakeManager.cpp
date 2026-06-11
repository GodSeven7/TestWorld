#include "CameraShakeManager.h"

void UCameraShakeManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UCameraShakeManager::Deinitialize()
{
    ActiveShakes.Empty();
    CurrentOffset = FGameCameraShakeOffset();
    Super::Deinitialize();
}

bool UCameraShakeManager::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UCameraShakeManager::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UCameraShakeManager, STATGROUP_Tickables);
}

void UCameraShakeManager::RequestShake(float Duration, float Scale)
{
    if (Duration <= 0.f)
        return;

    FCameraShakeInstance Instance;
    Instance.RemainingTime = Duration;
    Instance.Scale = Scale;
    Instance.ElapsedTime = 0.f;
    ActiveShakes.Add(Instance);
}

void UCameraShakeManager::StopAllShakes()
{
    ActiveShakes.Empty();
    CurrentOffset = FGameCameraShakeOffset();
}

FGameCameraShakeOffset UCameraShakeManager::GetCurrentOffset() const
{
    return CurrentOffset;
}

void UCameraShakeManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    FVector TotalLocationOffset = FVector::ZeroVector;
    FRotator TotalRotationOffset = FRotator::ZeroRotator;

    for (int32 i = ActiveShakes.Num() - 1; i >= 0; --i)
    {
        FCameraShakeInstance& Instance = ActiveShakes[i];
        Instance.ElapsedTime += DeltaTime;
        Instance.RemainingTime -= DeltaTime;

        if (Instance.RemainingTime <= 0.f)
        {
            ActiveShakes.RemoveAtSwap(i);
            continue;
        }

        float Decay = FMath::Exp(-DecayRate * Instance.ElapsedTime);
        float SinValue = FMath::Sin(Frequency * Instance.ElapsedTime * 2.f * PI);

        float LocMag = LocationAmplitude * SinValue * Decay * Instance.Scale;
        float RotMag = RotationAmplitude * SinValue * Decay * Instance.Scale;

        TotalLocationOffset.X += LocMag;
        TotalLocationOffset.Y += LocMag * 0.7f;
        TotalRotationOffset.Pitch += RotMag;
        TotalRotationOffset.Yaw += RotMag * 0.5f;
    }

    CurrentOffset.LocationOffset = TotalLocationOffset;
    CurrentOffset.RotationOffset = TotalRotationOffset;
}
