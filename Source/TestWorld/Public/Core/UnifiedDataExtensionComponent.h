#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnifiedDataExtension.h"
#include "ActorUnifiedDataComponent.h"
#include "BattleDamageTypes.h"
#include "GOAPTypes.h"
#include "CombatTypes.h"
#include "UnifiedDataExtensionComponent.generated.h"

UCLASS(ClassGroup = "UnifiedData", meta = (BlueprintSpawnableComponent))
class TESTWORLD_API UUnifiedDataExtensionComponent : public UActorComponent, public IUnifiedDataExtension
{
    GENERATED_BODY()

public:
    UUnifiedDataExtensionComponent();

    virtual void CreateExtensionSnapshot() override;
    virtual void ApplyExtensionToActor(float DeltaTime) override;

    // --- BattleAttributes ---
    FBattleAttributeData& GetMutableBattleAttributes() { return CurrentBattleAttributes; }
    const FBattleAttributeData& GetBattleAttributes() const { return CurrentBattleAttributes; }
    const FBattleAttributeData& GetSnapshotBattleAttributes() const { return SnapshotBattleAttributes; }

    // --- GOAP WorldState ---
    FGOAPWorldState& GetMutableWorldState() { return CurrentWorldState; }
    const FGOAPWorldState& GetWorldState() const { return CurrentWorldState; }

    // --- GOAP GoalState ---
    FGOAPWorldState& GetMutableGoalState() { return CurrentGoalState; }
    const FGOAPWorldState& GetGoalState() const { return CurrentGoalState; }

    // --- CombatData ---
    FCombatSnapshotData& GetMutableCombatData() { return CurrentCombatData; }
    const FCombatSnapshotData& GetCombatData() const { return CurrentCombatData; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY() FBattleAttributeData CurrentBattleAttributes;
    UPROPERTY() FBattleAttributeData SnapshotBattleAttributes;
    UPROPERTY() FGOAPWorldState CurrentWorldState;
    UPROPERTY() FGOAPWorldState SnapshotWorldState;
    UPROPERTY() FGOAPWorldState CurrentGoalState;
    UPROPERTY() FGOAPWorldState SnapshotGoalState;
    UPROPERTY() FCombatSnapshotData CurrentCombatData;
    UPROPERTY() FCombatSnapshotData SnapshotCombatData;
};
