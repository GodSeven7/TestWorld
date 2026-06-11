#pragma once

#include "CoreMinimal.h"
#include "GlueBase.h"
#include "GOAPGlue.generated.h"

class UGOAPManager;
class USparseGridManager;
class UCombatManager;
class UCrowdSurroundManager;

UCLASS()
class TESTWORLD_API UGOAPGlue : public UGlueBase
{
    GENERATED_BODY()

public:
    virtual void Initialize(UWorld* World) override;
    virtual void Shutdown() override;
    virtual void Tick(float DeltaTime) override;

    void Bind(
        UGOAPManager* InGOAPManager,
        USparseGridManager* InGridManager,
        UCombatManager* InCombatManager,
        UCrowdSurroundManager* InCrowdSurroundManager);

    UGOAPManager* GetGOAPManager() const { return GOAPManager; }

private:
    UPROPERTY()
    TObjectPtr<UGOAPManager> GOAPManager;

    UPROPERTY()
    TObjectPtr<USparseGridManager> GridManager;

    UPROPERTY()
    TObjectPtr<UCombatManager> CombatManager;

    UPROPERTY()
    TObjectPtr<UCrowdSurroundManager> CrowdSurroundManager;
};
