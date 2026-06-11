#pragma once

#include "CoreMinimal.h"
#include "GlueBase.h"
#include "ProjectileGlue.generated.h"

class UProjectileManager;
class USparseGridManager;
class UBattleDamageManager;

UCLASS()
class TESTWORLD_API UProjectileGlue : public UGlueBase
{
    GENERATED_BODY()

public:
    virtual void Initialize(UWorld* World) override;
    virtual void Shutdown() override;
    virtual void Tick(float DeltaTime) override;

    void Bind(
        UProjectileManager* InProjectileManager,
        USparseGridManager* InGridManager,
        UBattleDamageManager* InDamageManager);

    UProjectileManager* GetProjectileManager() const { return ProjectileManager; }

private:
    UPROPERTY()
    TObjectPtr<UProjectileManager> ProjectileManager;

    UPROPERTY()
    TObjectPtr<USparseGridManager> GridManager;

    UPROPERTY()
    TObjectPtr<UBattleDamageManager> DamageManager;
};
