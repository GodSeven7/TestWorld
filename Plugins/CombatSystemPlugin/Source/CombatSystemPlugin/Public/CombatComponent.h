#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatTypes.h"
#include "SkillTypes.h"
#include "ICombatObject.h"
#include "CombatComponent.generated.h"

class UCombatSkill;
class UCombatWeapon;
class UCombatManager;
class AActor;
class UDataTable;

USTRUCT()
struct FActiveSwing
{
    GENERATED_BODY()

    double EndTime = 0.0;
    TSet<TWeakObjectPtr<AActor>> HitActors;
};

UENUM()
enum class EAttackPhase : uint8
{
    Idle,
    Windup,
    Active,
    FollowThrough
};

UCLASS(ClassGroup = "Combat", meta = (BlueprintSpawnableComponent))
class COMBATSYSTEMPLUGIN_API UCombatComponent : public UActorComponent, public ICombatObject
{
    GENERATED_BODY()

public:
    UCombatComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    int32 Faction = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    bool bAutoRegister = true;

#if WITH_EDITORONLY_DATA
    // ── 编辑器下拉用的表引用（仅编辑器使用）──
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Editor")
    UDataTable* CharacterTableRef;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Editor")
    UDataTable* WeaponTableRef;
#endif

    // ── 配置表ID ──
    // 方案B入口：角色配置ID，从 CharacterTable 加载技能列表
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Config"
#if WITH_EDITORONLY_DATA
        , meta = (GetOptions = "GetCharacterIDOptions")
#endif
    )
    FName CharacterID;

    // 方案A入口：武器ID，从 WeaponTable 加载武器属性和默认技能
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Config"
#if WITH_EDITOR
        , meta = (GetOptions = "GetWeaponIDOptionsFromComp")
#endif
    )
    FName WeaponID;

#if WITH_EDITOR
    UFUNCTION()
    TArray<FString> GetCharacterIDOptions() const;

    UFUNCTION()
    TArray<FString> GetWeaponIDOptionsFromComp() const;
#endif

    // ── 运行时状态 ──
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    UCombatWeapon* Weapon = nullptr;

    UPROPERTY(BlueprintAssignable, Category = "Combat")
    FOnAttackDelegate OnAttack;

    UPROPERTY(BlueprintAssignable, Category = "Combat")
    FOnHitDelegate OnHit;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    UCombatWeapon* GetWeapon() const { return Weapon; }

    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool HasWeapon() const { return Weapon != nullptr; }

    UFUNCTION(BlueprintCallable, Category = "Combat")
    int32 GetFaction() const { return Faction; }

    UFUNCTION(BlueprintCallable, Category = "Combat")
    float GetWeaponRange() const;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool IsCombatActive() const;

    void AddPendingEvent(const FCombatEventData& Event);
    void FlushPendingEvents();

    bool IsInSwing(AActor* Target) const;
    bool HasActiveSwing() const;
    void AddSwing(double EndTime);
    void AddSwingHit(AActor* Target);
    void CleanupExpiredSwings(double CurrentTime);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    EAttackPhase GetAttackPhase() const { return CurrentPhase; }

    UFUNCTION(BlueprintCallable, Category = "Combat")
    virtual void SetAttackPhase(EAttackPhase Phase);

    virtual void OnCombatAttackExecuted(float Damage, bool bIsCritical);
    virtual void OnCombatHitReceived(float Damage);

    UCombatManager* GetManager() const;

    virtual FVector GetCombatWorldPosition() const override;
    virtual FVector GetCombatForwardVector() const override;
    virtual int32 GetCombatFaction() const override;
    virtual UObject* GetCombatParent() const override;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void RequestAttack(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool CanAttack() const;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool IsAttacking() const;

    // ── 技能系统 ──

    UPROPERTY(BlueprintReadOnly, Category = "Combat|Skill")
    TArray<UCombatSkill*> Skills;

    UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
    int32 AddSkill(UCombatSkill* Skill);

    UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
    void RemoveSkill(int32 SkillIndex);

    UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
    bool RequestSkill(int32 SkillIndex);

    UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
    bool CanCastSkill(int32 SkillIndex) const;

    UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
    bool IsSkillOnCooldown(int32 SkillIndex) const;

    UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
    float GetSkillCooldownRemaining(int32 SkillIndex) const;

    UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
    UCombatSkill* GetActiveSkill() const;

    UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
    int32 FindSkillIndex(FName InSkillID) const;

    UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
    ESkillPhase GetCurrentSkillPhase() const { return CurrentSkillPhase; }

    void InitSkillQueue();
    void ProcessSkillQueue(float DeltaTime);
    void UpdateSkillAvailableTime(int32 InSkillIndex, double NewAvailableTime);

    UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
    void AbortActiveSkill();

    UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
    void SetSkillCooldown(int32 SkillIndex, float Cooldown);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    TArray<FCombatEventData> PendingEvents;
    mutable FCriticalSection EventLock;

    TArray<FActiveSwing> ActiveSwings;
    mutable FCriticalSection SwingLock;

    EAttackPhase CurrentPhase = EAttackPhase::Idle;

    // ── 技能队列内部状态 ──
    TArray<FScheduledSkill> SkillQueue;
    int32 ActiveSkillIndex = INDEX_NONE;
    ESkillPhase CurrentSkillPhase = ESkillPhase::Idle;
    float SkillPhaseTimer = 0.0f;
    bool bSkillQueueDirty = false;

    void StartSkill(int32 SkillIndex);

    // 从配置表加载技能
    void LoadSkillsFromTables();
    void InitAllSkills(UCombatManager* Manager);
};
