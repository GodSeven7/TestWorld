#pragma once

#include "CoreMinimal.h"
#include "GlueBase.h"
#include "EventBus.h"
#include "CharacterGlue.generated.h"

class UCharacterAnimManager;
class UActorUnifiedDataComponent;

UCLASS()
class TESTWORLD_API UCharacterGlue : public UGlueBase
{
    GENERATED_BODY()

public:
    virtual void Initialize(UWorld* World) override;
    virtual void Shutdown() override;
    virtual void Tick(float DeltaTime) override;

    void Bind(
        UCharacterAnimManager* InCharacterAnimManager,
        UEventBus* InEventBus);

    void SetUnifiedDataComponents(const TArray<TObjectPtr<UActorUnifiedDataComponent>>* InComponents);

    UCharacterAnimManager* GetCharacterAnimManager() const { return CharacterAnimManager; }

private:
    UPROPERTY()
    TObjectPtr<UCharacterAnimManager> CharacterAnimManager;

    UPROPERTY()
    TObjectPtr<UEventBus> EventBus;

    const TArray<TObjectPtr<UActorUnifiedDataComponent>>* UnifiedDataComponents = nullptr;
};
