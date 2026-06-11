#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameCameraShakeService.h"
#include "CameraShakeManager.generated.h"

USTRUCT()
struct FCameraShakeInstance
{
    GENERATED_BODY()

    float RemainingTime = 0.f;

    float Scale = 1.f;

    float ElapsedTime = 0.f;
};

UCLASS()
class CAMERAADVANCEPLUGIN_API UCameraShakeManager : public UTickableWorldSubsystem, public IGameCameraShakeService
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
    virtual TStatId GetStatId() const override;

    virtual void RequestShake(float Duration, float Scale = 1.0f) override;
    virtual void StopAllShakes() override;
    virtual FGameCameraShakeOffset GetCurrentOffset() const override;

private:
    TArray<FCameraShakeInstance> ActiveShakes;

    FGameCameraShakeOffset CurrentOffset;

    static constexpr float LocationAmplitude = 50.0f;
    static constexpr float RotationAmplitude = 0.5f;
    static constexpr float Frequency = 25.0f;
    static constexpr float DecayRate = 8.0f;
};
