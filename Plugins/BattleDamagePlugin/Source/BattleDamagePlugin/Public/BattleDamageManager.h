#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BattleDamageTypes.h"
#include "DamageService.h"
#include "BattleDamageManager.generated.h"

class IBattleDamageInterface;
class UDamageNumberManager;

USTRUCT(BlueprintType)
struct FPendingDamage
{
    GENERATED_BODY()

    UPROPERTY()
    FDamageInfo Info;

    UPROPERTY()
    TWeakObjectPtr<AActor> Target;
};

USTRUCT(BlueprintType)
struct FProcessedHitResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    AActor* Target = nullptr;

    UPROPERTY(BlueprintReadOnly)
    AActor* Instigator = nullptr;

    UPROPERTY(BlueprintReadOnly)
    float RawDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float FinalDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float DamageMitigated = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bIsCritical = false;

    UPROPERTY(BlueprintReadOnly)
    bool bKilled = false;

    UPROPERTY(BlueprintReadOnly)
    FVector ImpactLocation = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FKillInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TWeakObjectPtr<AActor> Target;

    UPROPERTY(BlueprintReadOnly)
    TWeakObjectPtr<AActor> Instigator;

    UPROPERTY(BlueprintReadOnly)
    float RawDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float FinalDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    EDamageType DamageType = EDamageType::Physical;

    UPROPERTY(BlueprintReadOnly)
    bool bIsCritical = false;
};

USTRUCT(BlueprintType)
struct FPendingDamageNotification
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<AActor> Target;

    UPROPERTY()
    FDamageInfo Info;

    FDamageResult Result;
};

UCLASS()
class BATTLEDAMAGEPLUGIN_API UBattleDamageManager : public UWorldSubsystem, public IDamageService
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    UFUNCTION(BlueprintCallable, Category = "Battle Damage")
    void QueueDamage(AActor* Target, const FDamageInfo& Info);

    UFUNCTION(BlueprintCallable, Category = "Battle Damage")
    void QueueHeal(AActor* Target, float Amount, AActor* Healer);

    UFUNCTION(BlueprintCallable, Category = "Battle Damage")
    void QueueKill(AActor* Target, AActor* Instigator = nullptr, float RawDamage = 0.0f, float FinalDamage = 0.0f, EDamageType DamageType = EDamageType::Real, bool bIsCritical = false);

    UFUNCTION(BlueprintCallable, Category = "Battle Damage")
    FDamageResult ApplyDamage(AActor* Target, const FDamageInfo& Info);

    UFUNCTION(BlueprintCallable, Category = "Battle Damage")
    float ApplyHeal(AActor* Target, float Amount, AActor* Healer);

    UFUNCTION(BlueprintCallable, Category = "Battle Damage")
    void ProcessPendingDamages();

    UFUNCTION(BlueprintCallable, Category = "Battle Damage")
    FProcessedHitResult ProcessCombatHit(AActor* Target, float RawDamage, EDamageType DamageType, AActor* Instigator, const FVector& ImpactLocation, bool bCanCritical = true);

    UFUNCTION(BlueprintCallable, Category = "Battle Damage")
    int32 ProcessPendingKillsOnGameThread();

    UFUNCTION(BlueprintCallable, Category = "Battle Damage")
    int32 ProcessPendingDamageNotifications();

    UFUNCTION(BlueprintCallable, Category = "Battle Damage")
    int32 GetPendingKillCount() const { return PendingKills.Num(); }

    UFUNCTION(BlueprintCallable, Category = "Battle Damage")
    void SetDamageNumberEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Battle Damage")
    bool IsDamageNumberEnabled() const;

    UFUNCTION(BlueprintCallable, Category = "Battle Damage")
    UDamageNumberManager* GetDamageNumberManager() const;

    void RegisterCombatant(IBattleDamageInterface* Combatant);
    void UnregisterCombatant(IBattleDamageInterface* Combatant);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatantRegistered, AActor*, Combatant);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatantUnregistered, AActor*, Combatant);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatantDeath, AActor*, Combatant, const FDamageInfo&, KillingBlow);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatHitProcessed, AActor*, Target, const FProcessedHitResult&, Result);

    UPROPERTY(BlueprintAssignable, Category = "Battle Damage")
    FOnCombatantRegistered OnCombatantRegistered;

    UPROPERTY(BlueprintAssignable, Category = "Battle Damage")
    FOnCombatantUnregistered OnCombatantUnregistered;

    UPROPERTY(BlueprintAssignable, Category = "Battle Damage")
    FOnCombatantDeath OnCombatantDeath;

    UPROPERTY(BlueprintAssignable, Category = "Battle Damage")
    FOnCombatHitProcessed OnCombatHitProcessed;

private:
    TArray<IBattleDamageInterface*> RegisteredCombatants;
    mutable FCriticalSection CombatantsLock;

    TArray<FPendingDamage> PendingDamages;
    mutable FCriticalSection PendingDamagesLock;

    TArray<FKillInfo> PendingKills;
    mutable FCriticalSection PendingKillsLock;

    TArray<FPendingDamageNotification> PendingDamageNotifications;
    mutable FCriticalSection PendingDamageNotificationsLock;

    bool bDamageNumberEnabled = true;

    FDamageResult CalculateAndApplyDamage(IBattleDamageInterface* Target, const FDamageInfo& Info);
    float CalculateDamageMitigation(float BaseDamage, EDamageType DamageType, const FBattleAttributeData& Attributes);
    bool RollCritical(float CriticalChance);

public:
    virtual FProcessedDamageResult ApplyDamageHit(
        AActor* Target,
        float RawDamage,
        EDamageType DamageType,
        AActor* Instigator,
        const FVector& ImpactLocation,
        bool bCanCritical = true) override;

    virtual int32 FlushDamageNotifications() override;
    virtual int32 FlushPendingKills() override;
};
