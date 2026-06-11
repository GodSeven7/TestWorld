#pragma once

#include "CoreMinimal.h"
#include "GlueBase.h"
#include "EventBus.h"
#include "GameCameraShakeService.h"
#include "CameraAdvanceGlue.generated.h"

class UCameraShakeManager;

UCLASS()
class TESTWORLD_API UCameraAdvanceGlue : public UGlueBase
{
    GENERATED_BODY()

public:
    virtual void Initialize(UWorld* World) override;
    virtual void Shutdown() override;
    virtual void Tick(float DeltaTime) override;

    void Bind(
        UCameraShakeManager* InShakeManager,
        UEventBus* InEventBus);

    UCameraShakeManager* GetCameraShakeManager() const { return ShakeManager; }

private:
    UPROPERTY()
    TObjectPtr<UCameraShakeManager> ShakeManager;

    UPROPERTY()
    TObjectPtr<UEventBus> EventBus;

    UFUNCTION()
    void OnDamageApplied(const FDamageAppliedEvent& Event);

    UFUNCTION()
    void OnCombatantDeath(const FCombatantDeathEvent& Event);

    void ApplyShakeOffsetToCamera();
};
