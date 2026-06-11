#include "Core/UnifiedDataExtensionComponent.h"

UUnifiedDataExtensionComponent::UUnifiedDataExtensionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UUnifiedDataExtensionComponent::BeginPlay()
{
    Super::BeginPlay();

    if (UActorUnifiedDataComponent* UDC = GetOwner()->FindComponentByClass<UActorUnifiedDataComponent>())
    {
        UDC->RegisterExtension(this);
    }
}

void UUnifiedDataExtensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UActorUnifiedDataComponent* UDC = GetOwner()->FindComponentByClass<UActorUnifiedDataComponent>())
    {
        UDC->UnregisterExtension(this);
    }
    Super::EndPlay(EndPlayReason);
}

void UUnifiedDataExtensionComponent::CreateExtensionSnapshot()
{
    SnapshotBattleAttributes = CurrentBattleAttributes;
    SnapshotWorldState = CurrentWorldState;
    SnapshotGoalState = CurrentGoalState;
    SnapshotCombatData = CurrentCombatData;
}

void UUnifiedDataExtensionComponent::ApplyExtensionToActor(float DeltaTime)
{
    // 扩展数据当前不需要 Apply 回写，预留接口
}
