#include "Glue/CharacterGlue.h"
#include "CharacterAnimManager.h"
#include "ActorUnifiedDataComponent.h"
#include "CharacterSkelMeshComponent.h"
#include "GameFramework/Actor.h"

static bool bDisableAnimation = false;
FAutoConsoleVariableRef CVarDisableAnimation(
	TEXT("Disable.Animation"),
	bDisableAnimation,
	TEXT(""));

void UCharacterGlue::Initialize(UWorld* World)
{
}

void UCharacterGlue::Shutdown()
{
}

void UCharacterGlue::Bind(
    UCharacterAnimManager* InCharacterAnimManager,
    UEventBus* InEventBus)
{
    CharacterAnimManager = InCharacterAnimManager;
    EventBus = InEventBus;
}

void UCharacterGlue::SetUnifiedDataComponents(const TArray<TObjectPtr<UActorUnifiedDataComponent>>* InComponents)
{
    UnifiedDataComponents = InComponents;
}

void UCharacterGlue::Tick(float DeltaTime)
{
    // 将UnifiedData中的bIsMoving传播到CharacterSkelMeshComponent，供动画系统使用
    if (UnifiedDataComponents)
    {
        for (const UActorUnifiedDataComponent* UnifiedComp : *UnifiedDataComponents)
        {
            if (!UnifiedComp)
                continue;

            AActor* Owner = UnifiedComp->GetOwner();
            if (!Owner)
                continue;

            if (UCharacterSkelMeshComponent* MeshComp = Owner->FindComponentByClass<UCharacterSkelMeshComponent>())
            {
                MeshComp->SetMoving(UnifiedComp->IsMoving());
            }
        }
    }

    if (!CharacterAnimManager || bDisableAnimation)
        return;

    CharacterAnimManager->SetGameplaySuspended(bSuspended);
    CharacterAnimManager->Tick(DeltaTime);
}
