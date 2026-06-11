#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CombatTypes.h"
#include "SkillTypes.h"
#include "SpatialQueryService.h"
#include "DamageService.h"
#include "ProjectileService.h"
#include "CombatQueryService.h"
#include "CrowdSurroundService.h"
#include "GridObjectInfo.h"
#include "ICombatObject.h"
#include "CombatManager.generated.h"

class UCombatComponent;
class UCombatWeapon;
class UCombatSkill;
class AActor;

struct FAttackContext
{
    UCombatComponent* AttackerComp = nullptr;
    AActor* AttackerActor = nullptr;
    UCombatWeapon* Weapon = nullptr;
    const FCombatSkillConfig* SkillConfig = nullptr;
};

struct FDeferredSkillExecution
{
    UCombatComponent* Comp = nullptr;
    UCombatSkill* Skill = nullptr;
    const FCombatSkillConfig* SkillConfig = nullptr;
    FVector OwnerLocation = FVector::ZeroVector;
    FVector OwnerDirection = FVector::ForwardVector;
};

UCLASS()
class COMBATSYSTEMPLUGIN_API UCombatManager : public UWorldSubsystem, public ICombatQueryService
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    void RegisterCombatComponent(UCombatComponent* Comp);
    void UnregisterCombatComponent(UCombatComponent* Comp);

    void SetSpatialQueryService(ISpatialQueryService* InService) { SpatialQuery = InService; }
    ISpatialQueryService* GetSpatialQueryService() const { return SpatialQuery; }

    void SetDamageService(IDamageService* InService) { DamageService = InService; }
    IDamageService* GetDamageService() const { return DamageService; }

    void SetProjectileService(IProjectileService* InService) { ProjectileService = InService; }
    IProjectileService* GetProjectileService() const { return ProjectileService; }

    void SetCrowdSurroundService(ICrowdSurroundService* InService) { CrowdSurroundService = InService; }
    ICrowdSurroundService* GetCrowdSurroundService() const { return CrowdSurroundService; }

    // ── 配置表 ──
    void SetSkillTable(UDataTable* Table) { SkillTable = Table; }
    void SetWeaponTable(UDataTable* Table) { WeaponTable = Table; }
    void SetCharacterTable(UDataTable* Table) { CharacterTable = Table; }

    const FCombatSkillConfig* FindSkillConfig(FName SkillID) const;
    const FWeaponStats* FindWeaponStats(FName WeaponID) const;
    const FCharacterCombatConfig* FindCharacterConfig(FName CharacterID) const;

    virtual void RequestAttackForActor(AActor* Actor, AActor* Target) override;

    void ProcessCombat(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ProcessPendingKills();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    TArray<AActor*> QueryActorsInRadius(FVector Center, float Radius,
        UObject* ExcludeParent = nullptr, int32 ExcludeFactionMask = 0) const;

    TArray<AActor*> QueryActorsInShape(FVector Center, FVector Direction,
        const FCombatSkillConfig* Config, UObject* ExcludeParent = nullptr, int32 ExcludeFactionMask = 0) const;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    int32 GetRegisteredComponentCount() const { return RegisteredComponents.Num(); }

    void ApplyDamageCalculation(AActor* Target, float RawDamage,
        EDamageType DamageType, AActor* Instigator,
        const FVector& ImpactLocation,
        float& OutFinalDamage, bool& OutIsCritical);

protected:
    UPROPERTY()
    TArray<UCombatComponent*> RegisteredComponents;

    UPROPERTY()
    UDataTable* SkillTable = nullptr;

    UPROPERTY()
    UDataTable* WeaponTable = nullptr;

    UPROPERTY()
    UDataTable* CharacterTable = nullptr;

    double CurrentTime = 0.0;

    ISpatialQueryService* SpatialQuery = nullptr;
    IDamageService* DamageService = nullptr;
    IProjectileService* ProjectileService = nullptr;
    ICrowdSurroundService* CrowdSurroundService = nullptr;

    void ProcessSkillExecutions(float DeltaTime);
    void ExecuteActiveSkills(float DeltaTime);
    void ApplySkillResult(FDeferredSkillExecution& InExecution);

    void DispatchEvents();

    void ExecuteAttackOnTargets(const FAttackContext& Context,
        TArray<AActor*>& InOutCandidates, int32 MaxTargets);

    static FVector GetActorCombatPosition(AActor* Actor);

    TArray<AActor*> QueryActorsInCone(FVector Center, FVector Direction,
        float Length, float AngleDegrees, UObject* ExcludeParent = nullptr, int32 ExcludeFactionMask = 0) const;
    TArray<AActor*> QueryActorsInRectangle(FVector Center, FVector Direction,
        float Length, float Width, UObject* ExcludeParent = nullptr, int32 ExcludeFactionMask = 0) const;

    static constexpr int32 PARALLEL_THRESHOLD = 32;
};
