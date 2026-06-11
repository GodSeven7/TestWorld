#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameCameraShakeService.generated.h"

USTRUCT(BlueprintType)
struct FGameCameraShakeOffset
{
    GENERATED_BODY()

    UPROPERTY()
    FVector LocationOffset = FVector::ZeroVector;

    UPROPERTY()
    FRotator RotationOffset = FRotator::ZeroRotator;
};

UINTERFACE(MinimalAPI)
class UGameCameraShakeService : public UInterface
{
    GENERATED_BODY()
};

class IGameCameraShakeService
{
    GENERATED_BODY()

public:
    virtual void RequestShake(float Duration, float Scale = 1.0f) = 0;
    virtual void StopAllShakes() = 0;
    virtual FGameCameraShakeOffset GetCurrentOffset() const = 0;
};
