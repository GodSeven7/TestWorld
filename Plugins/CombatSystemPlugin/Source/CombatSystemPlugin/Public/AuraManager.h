#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AuraTypes.h"
#include "AuraManager.generated.h"

UCLASS()
class COMBATSYSTEMPLUGIN_API UAuraManager : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual TStatId GetStatId() const override;

    int32 ApplyAura(AActor* Target, AActor* Caster, const FAuraEffectEntry& Entry,
        const FVector& StartPosition, const FVector& Direction);

    void RemoveAura(int32 AuraID);
    void RemoveAurasByType(AActor* Target, EAuraType Type);
    bool HasAuraType(AActor* Target, EAuraType Type) const;
    void TickAuras(float DeltaTime, TArray<FAuraUpdate>& OutUpdates);
    void CancelAurasForActor(AActor* Target);
    void CancelAllAuras();

private:
    UPROPERTY()
    TArray<FAuraInstance> ActiveAuras;

    TArray<int32> FreeIndices;
    int32 NextAuraID = 1;

    float ApplyCurve(float T, EAuraCurve CurveType) const;

protected:
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
};
