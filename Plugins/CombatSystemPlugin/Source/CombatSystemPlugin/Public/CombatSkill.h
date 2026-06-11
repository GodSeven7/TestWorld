#pragma once

#include "CoreMinimal.h"
#include "SkillTypes.h"
#include "CombatSkill.generated.h"

class UCombatManager;
class UDataTable;

UCLASS(Blueprintable, Abstract)
class COMBATSYSTEMPLUGIN_API UCombatSkill : public UObject
{
    GENERATED_BODY()

public:
    // 技能ID，同时作为 SkillTable 的 Row Name
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat"
#if WITH_EDITORONLY_DATA
        , meta = (GetOptions = "GetSkillIDOptions")
#endif
    )
    FName SkillID;

#if WITH_EDITORONLY_DATA
    // 编辑器下拉用的表引用（仅编辑器使用，运行时由 CombatManager 的表驱动）
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Editor")
    UDataTable* SkillTableRef;

    UFUNCTION()
    TArray<FString> GetSkillIDOptions() const;
#endif

    // 从技能表查找并缓存配置，由 CombatComponent::BeginPlay 调用
    void InitSkill(UCombatManager* Manager);

    const FCombatSkillConfig* GetSkillConfig() const { return CachedConfig; }

    virtual bool CanExecute(AActor* Owner) const;
    virtual bool CheckPreCondition(AActor* Owner) const;
    virtual void OnSkillStarted(AActor* Owner);
    virtual void ExecuteInternal(AActor* Owner, float DeltaTime);
    virtual void OnSkillCompleted(AActor* Owner);

    virtual FName GetSkillID() const { return SkillID; }
    virtual float GetWindupTime() const;
    virtual float GetDuration() const;
    virtual float GetFollowThroughTime() const;
    virtual float GetCooldown() const;

    float GetTotalCycleTime() const;

private:
    const FCombatSkillConfig* CachedConfig = nullptr;
};
