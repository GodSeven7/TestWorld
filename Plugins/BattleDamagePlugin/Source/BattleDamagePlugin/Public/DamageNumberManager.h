#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DamageNumberTypes.h"
#include "DamageNumberManager.generated.h"

class UNiagaraComponent;
class AActor;

UCLASS()
class BATTLEDAMAGEPLUGIN_API UDamageNumberManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    bool IsInitialized() const { return bIsInitialized; }
    bool IsEnabled() const { return bIsEnabled; }

    void EnqueueDamageNumber(const FVector& WorldLocation, float Damage, bool bIsCritical);
    void FlushToNiagara();
    void ClearAllNumbers();

private:
    void LoadActorFromConfig();
    bool SpawnActorFromBlueprint();

    UPROPERTY()
    TSubclassOf<AActor> DamageNumberActorClass;

    UPROPERTY()
    TObjectPtr<AActor> DamageNumberActor;

    UPROPERTY()
    TObjectPtr<UNiagaraComponent> NiagaraComponent;

    TArray<FVector4> PendingDamageNumbers;
    mutable FCriticalSection PendingLock;

    FDamageNumberSettings CurrentSettings;

    bool bIsInitialized = false;
    bool bIsEnabled = false;

    static constexpr int32 DEFAULT_MAX_COUNT = 256;
};
