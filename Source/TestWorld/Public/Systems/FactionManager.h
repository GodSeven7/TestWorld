#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FactionManager.generated.h"

UENUM(BlueprintType)
enum class EFactionRelation : uint8
{
    Friendly = 0,
    Hostile = 1,
    Neutral = 2
};

UCLASS()
class TESTWORLD_API UFactionManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Faction")
    EFactionRelation GetRelation(int32 FactionA, int32 FactionB) const;

    UFUNCTION(BlueprintCallable, Category = "Faction")
    bool IsHostile(int32 FactionA, int32 FactionB) const;

    UFUNCTION(BlueprintCallable, Category = "Faction")
    bool IsFriendly(int32 FactionA, int32 FactionB) const;

    UFUNCTION(BlueprintCallable, Category = "Faction")
    void SetRelation(int32 FactionA, int32 FactionB, EFactionRelation Relation);

private:
    TMap<int32, TMap<int32, EFactionRelation>> RelationMap;
};
