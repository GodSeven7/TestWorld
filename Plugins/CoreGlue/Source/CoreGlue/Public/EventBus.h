#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EventBus.generated.h"

UENUM(BlueprintType)
enum class EQuestResult : uint8
{
    InProgress,
    Victory,
    Defeat
};

USTRUCT(BlueprintType)
struct FCombatantDeathEvent
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<AActor> DeadActor;

    UPROPERTY()
    TWeakObjectPtr<AActor> Killer;

    UPROPERTY()
    float Time = 0.0f;

    UPROPERTY()
    FVector InstigatorLocation = FVector::ZeroVector;

    UPROPERTY()
    FVector ImpactLocation = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FDamageAppliedEvent
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<AActor> Target;

    UPROPERTY()
    float Damage = 0.0f;

    UPROPERTY()
    float RemainingHealth = 0.0f;

    UPROPERTY()
    FVector InstigatorLocation = FVector::ZeroVector;

    UPROPERTY()
    FVector ImpactLocation = FVector::ZeroVector;

    UPROPERTY()
    bool bKilled = false;
};

USTRUCT(BlueprintType)
struct FQuestResultEvent
{
    GENERATED_BODY()

    UPROPERTY()
    EQuestResult Result = EQuestResult::InProgress;
};

USTRUCT(BlueprintType)
struct FActorSpawnedEvent
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<AActor> SpawnedActor;
};

USTRUCT(BlueprintType)
struct FFactionChangedEvent
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<AActor> ChangedActor;

    UPROPERTY()
    int32 OldFaction = 0;

    UPROPERTY()
    int32 NewFaction = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatantDeath, const FCombatantDeathEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageApplied, const FDamageAppliedEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestResult, const FQuestResultEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorObjSpawned, const FActorSpawnedEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFactionChanged, const FFactionChangedEvent&, Event);

UCLASS(BlueprintType)
class COREGLUE_API UEventBus : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "EventBus")
    FOnCombatantDeath OnCombatantDeath;

    UPROPERTY(BlueprintAssignable, Category = "EventBus")
    FOnDamageApplied OnDamageApplied;

    UPROPERTY(BlueprintAssignable, Category = "EventBus")
    FOnQuestResult OnQuestResult;

    UPROPERTY(BlueprintAssignable, Category = "EventBus")
    FOnActorObjSpawned OnActorObjSpawned;

    UPROPERTY(BlueprintAssignable, Category = "EventBus")
    FOnFactionChanged OnFactionChanged;
};
