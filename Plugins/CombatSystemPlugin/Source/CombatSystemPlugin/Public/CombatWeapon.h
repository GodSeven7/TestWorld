#pragma once

#include "CoreMinimal.h"
#include "CombatSkill.h"
#include "CombatTypes.h"
#include "CombatWeapon.generated.h"

class UDataTable;

UCLASS(Blueprintable, Abstract)
class COMBATSYSTEMPLUGIN_API UCombatWeapon : public UCombatSkill
{
    GENERATED_BODY()

public:
    UCombatWeapon();

    // 武器ID，同时作为 WeaponTable 的 Row Name
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat"
#if WITH_EDITORONLY_DATA
        , meta = (GetOptions = "GetWeaponIDOptions")
#endif
    )
    FName WeaponID;

#if WITH_EDITORONLY_DATA
    // 编辑器下拉用的表引用（仅编辑器使用，运行时由 CombatManager 的表驱动）
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Editor")
    UDataTable* WeaponTableRef;

    UFUNCTION()
    TArray<FString> GetWeaponIDOptions() const;
#endif

    // 从武器表查找并缓存配置，由 CombatComponent::BeginPlay 调用
    void InitWeapon(UCombatManager* Manager);

    const FWeaponStats* GetWeaponStats() const { return CachedStats; }
    FName GetWeaponID() const { return WeaponID; }

    virtual AActor* SelectTarget(AActor* Owner, const TArray<AActor*>& Candidates);
    virtual float ExecuteAttack(AActor* Owner, AActor* Target);

    virtual bool CanExecute(AActor* Owner) const override;
    virtual bool CheckPreCondition(AActor* Owner) const override;
    virtual void ExecuteInternal(AActor* Owner, float DeltaTime) override;

protected:
    float CalculateDamageWithCritical() const;
    FVector GetOwnerPosition(AActor* Owner) const;
    FVector GetTargetPosition(AActor* Target) const;

private:
    const FWeaponStats* CachedStats = nullptr;
};
